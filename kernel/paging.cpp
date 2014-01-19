#include <paging.h>
#include <error.h>
#include <memmap.h>
#include <lib/humanReadable.h>

// Static members
bool Paging::pagingEnabled = false;

Paging::Paging()
{
    pageDir0 = (u32*) KERN_PAGE_DIR;	// kernel page directory
    memBitmap = (u8*) KERN_PAGE_BMP;         // page bitmap

    kmallocUsed = 0;

    return;
}

void Paging::init(u32 highMem)
{
    unsigned int page, pageLimit;
    struct vmArea *p;

    // Number of the last page
    pageLimit = (highMem * 1024) / PAGESIZE;

    // Mark all phisycal pages that fit in the available RAM as free
    for (page = 0; page < pageLimit / 8; page++)
        memBitmap[page] = 0;

    // Mark all pages that don't fit in the RAM as used
    for (page = pageLimit / 8; page < RAM_MAXPAGE / 8; page++)
        memBitmap[page] = 0xFF;

    // Mark physical pages reserved for the kernel as used
    for (page = 0; page < PAGE((u32) KERN_IDENTITY_END); page++)
        setPageFrameUsed(page);

    // Init the kernel's page directory
    pageDir0[0] = ((u32) KERN_IDENTITY_START | (PAGE_PRESENT | PAGE_WRITE | PAGE_4MB));
    for (u16 i = 1; i < 1023; i++)
        pageDir0[i] = ((u32)KERN_PAGE_TABLES + 0x1000 * (i-1)) | (PAGE_PRESENT | PAGE_WRITE); // Each page table is 0x1000 B long
    pageDir0[1023] = ((u32) pageDir0 | (PAGE_PRESENT | PAGE_WRITE)); // Page table mirroring trick

    // Init the kernel's first page table (partially) to complete the identity mapping
    for (u16 i = 0; i < 639; i++) // 639*PAGESIZE + PAGESIZE_BIG = 0x67F000 = KERN_IDENTITY_END
        *((u32*)KERN_PAGE_TABLES+4*i) = ((PAGESIZE_BIG + PAGESIZE*i) | (PAGE_PRESENT | PAGE_WRITE));

    // Enable pagination. Will enable malloc, free, new and delete
    asm("	mov %0, %%eax \n \
        mov %%eax, %%cr3 \n \
        mov %%cr4, %%eax \n \
        or %2, %%eax \n \
        mov %%eax, %%cr4 \n \
        mov %%cr0, %%eax \n \
        or %1, %%eax \n \
        mov %%eax, %%cr0"::"m"(pageDir0), "i"(PAGING_FLAG), "i"(PSE_FLAG));

    // Init the kernel heap used by kmalloc
    curKernHeap = (char *) KERN_HEAP;
    ksbrk(1); // Alloc one page for the kernel in the heap

    pagingEnabled=true;

    /*
    // Init the list of free virutal addresses
    p = (struct vmArea*) kmalloc(sizeof(struct vmArea));
    p->vmStart = (char*) KERN_PAGE_HEAP;
    p->vmEnd = (char*) KERN_PAGE_HEAP_LIM;
    kernFreeVm.prepend(p);
    */
}

char* Paging::getPageFrame()
{
    int page = -1;

    for (int byte = 0; byte < RAM_MAXPAGE / 8; byte++)
    {
        if (memBitmap[byte] == 0xFF)
            continue;
        for (int bit = 0; bit < 8; bit++)
            if (!(memBitmap[byte] & (1 << bit)))
            {
                page = 8 * byte + bit;
                setPageFrameUsed(page);
                return (char *) (page * PAGESIZE);
            }
    }

    error("Paging::getPageFrame: No free page frame found.\n");
    return nullptr;
}

/*
struct page* Paging::getPageFromHeap()
{
    struct page *page;
    struct vmArea *area;
    char *vAddr, *pAddr;

    // Get a free physical page
    pAddr = getPageFrame();
    if ((int)pAddr <= 0)
        fatalError("PANIC: getPageFromHeap(): no page frame available. System halted !\n");

    // Check if there's a free virtual page
    //if (list_empty(&kernFreeVm))
    if (kernFreeVm.isEmpty())
        fatalError("PANIC: getPageFromHeap(): not memory left in page heap. System halted !\n");

    // Take the first free virtual page
    //area = list_first_entry(&kernFreeVm, struct vm_area, list);
    area = kernFreeVm[0]; // We just checked that the list isn't empty, so [] is safe
    vAddr = area->vmStart;

    // Update the list of free virtual pages
    area->vmStart += PAGESIZE;
    if (area->vmStart == area->vmEnd)
    {
        //list_del(&area->list);
        kernFreeVm.removeFirst(); // We took area = kernFreeVm[0]
        kfree(area);
    }

    // Update the kernel's address space
    pageDir0AddPage(vAddr, pAddr, 0);

    // Return the page
    page = (struct page*) kmalloc(sizeof(struct page));
    page->vAddr = vAddr;
    page->pAddr = pAddr;

    return page;
}

int Paging::releasePageFromHeap(char *vAddr)
{
    struct vmArea *nextArea, *prevArea, *newArea;
    char *pAddr;

    // Find the page associated with vAddr and free it
    pAddr = getPAddr(vAddr);
    if (pAddr)
        releasePageFrame((u32)pAddr);
    else
    {
        error("releasePageFromHeap(): no page frame associated with vAddr %x\n", vAddr);
        return 1;
    }

    // Update the page dir
    pageDirRemovePage(vAddr);

    // Update the list of free virtual memory
    unsigned int i; // Index of nextArea
    // Find the page just after our address and insert the page we free'd just before
    for(i=0; i<kernFreeVm.size(); i++)
    {
        nextArea=kernFreeVm[i];
        if (nextArea->vmStart > vAddr)
        {
            if (i == 0)	// If there's nothing before, prev_area becomes the last, else it's the one just before next_area
                prevArea = kernFreeVm[kernFreeVm.size()-1];
            else
                prevArea = kernFreeVm[i-1];
            break;
        }
    }

    //prev_area = list_entry(nextArea->list.prev, struct vm_area, list);

    // Either expand the previous page, the next page, or insert ours in between
    if (prevArea->vmEnd == vAddr)
    {
        prevArea->vmEnd += PAGESIZE;
        if (prevArea->vmEnd == nextArea->vmStart) // If the previous and next are touching, fuse them together
        {
            prevArea->vmEnd = nextArea->vmEnd;
            //list_del(&nextArea->list);
            kernFreeVm.removeAt(i);
            kfree(nextArea);
        }
    }
    else if (nextArea->vmStart == vAddr + PAGESIZE)
        nextArea->vmStart = vAddr;
    else if (nextArea->vmStart > vAddr + PAGESIZE)
    {
        newArea = (struct vmArea*) kmalloc(sizeof(struct vmArea));
        newArea->vmStart = vAddr;
        newArea->vmEnd = vAddr + PAGESIZE;
        //list_add(&newArea->list, &prev_area->list);
        kernFreeVm.insert(i, newArea); // Insert at the pos of next_area (thus becomes the one just before)
    }
    else
        fatalError("releasePageFromHeap(): Free vm list is corruped. System halted.\n");

    return 0;
}
*/

void Paging::setPageFrameUsed(u32 page)
{
    memBitmap[((u32) page)/8] |= (1 << (((u32) page)%8));;
}

void Paging::releasePageFrame(u32 pAddr)
{
    memBitmap[((u32) pAddr/PAGESIZE)/8] &= ~(1 << (((u32) pAddr/PAGESIZE)%8));
}

u32* Paging::getPageDir0()
{
    return pageDir0;
}

u32 Paging::getKMallocUsed()
{
    return kmallocUsed;
}

/// Update the kernel's address space
/// This space is shared between all the page directories !
int Paging::pageDir0AddPage(char *vAddr, char *pAddr, int flags)
{
    u32 *pageDirE;
    u32 *pageTableE;

    if (vAddr >= (char *) USER_OFFSET)
    {
        error("pageDir0AddPage(): 0x%x is not in kernel space !\n", vAddr);
        return 0;
    }

    // Check that the page really is here
    // 0xFFFFF000 is vAddr of last entry in pageDir, wich is mapped to pAddr of pageDir (mirroring trick)
    // The 10 first bits of the vAddr are the index of the entry into the page dir.
    pageDirE = (u32 *) (0xFFFFF000 | (((u32) vAddr & 0xFFC00000) >> 20));
    if ((*pageDirE & PAGE_PRESENT) == 0)
        fatalError("pageDir0AddPage(): kernel page table not found for vAddr 0x%x. System halted !\n", vAddr);

    // Edit the entry in the page table
    // The first 2*10 bits are indexes into the dir/table. Each entry is 4B large.
    pageTableE = (u32 *) (0xFFC00000 | (((u32) vAddr & 0xFFFFF000) >> 10));
    *pageTableE = ((u32) pAddr) | (PAGE_PRESENT | PAGE_WRITE | flags);
    return 0;
}

int Paging::pageDirRemovePage(char *vAddr)
{
    u32 *pageTablee;

    if (getPAddr(vAddr))
    {
        pageTablee = (u32 *) (0xFFC00000 | (((u32) vAddr & 0xFFFFF000) >> 10));
        *pageTablee = (*pageTablee & (~PAGE_PRESENT));
        asm("invlpg %0"::"m"(vAddr));
    }

    return 0;
}

char* Paging::getPAddr(char *vAddr)
{
    u32 *pageDire;		// Virtual address of the entry in the page directory
    u32 *pageTablee;		// Virtual address of the entry in the page tables

    // 0xFFFFF000 is vAddr of last entry in pageDir, wich is mapped to pAddr of pageDir (mirroring trick)
    pageDire = (u32 *) (0xFFFFF000 | (((u32) vAddr & 0xFFC00000) >> 20));
    if ((*pageDire & PAGE_PRESENT)) {
        pageTablee = (u32 *) (0xFFC00000 | (((u32) vAddr & 0xFFFFF000) >> 10));
        if ((*pageTablee & PAGE_PRESENT))
            return (char *) ((*pageTablee & 0xFFFFF000) + (VADDR_PAGE_OFFSET((u32) vAddr)));
    }

    return 0;
}

void Paging::printStats()
{
    checkAllocChunks(); // We wouldn't want to display wrong info, better check that everything is clean first.

    // Count how many free physical pages we have
    unsigned freePMem = 0;
    for (int byte = 0; byte < RAM_MAXPAGE / 8; byte++)
    {
        if (memBitmap[byte] == 0xFF)
            continue;
        else if (memBitmap[byte] == 0)
            freePMem += 8*PAGESIZE;
        else
            for (int bit = 0; bit < 8; bit++)
                if (!(memBitmap[byte] & (1 << bit)))
                    freePMem += PAGESIZE;
    }

    // Compute free memory
    unsigned freeMem = (KERN_HEAP_LIM - KERN_HEAP) - kmallocUsed;
    if (freePMem < freeMem)
        freeMem = freePMem;

    char* buf = new char[64];
    gTerm.printf("Heap : %s used, %s free\n", toIEC(buf,32,kmallocUsed), toIEC(buf+32,32,freeMem));

    u64 esp;
    asm("mov %%esp, %0":"=m"(esp):);
    gTerm.printf("Stack : %s used, %s free\n", toIEC(buf,32,KERN_STACK-esp), toIEC(buf+32,32,esp-KERN_STACK_LIM));
    delete buf;
}

void Paging::checkAllocChunks()
{
    u32 totalSize=0;

    struct kmallocHeader *chunk, *oldChunk;

    chunk = oldChunk = (struct kmallocHeader *) KERN_HEAP;
    for (;;) // Check all the chunks, break at the end
    {
        // If we're at (or after) the end
        if (chunk == (struct kmallocHeader *) curKernHeap)
            break;
        else if (chunk > (struct kmallocHeader *) curKernHeap)
            fatalError("checkAllocChunks: chunk on 0x%x while end of malloc heap is on 0x%x\n",chunk, curKernHeap);

        // If a chunk has a null size, try to repair it
        if (chunk->size == 0)
        {
            error("checkAllocChunks: Corrupted chunk on 0x%x with null size (heap 0x%x) !\n", chunk, curKernHeap);
            if (chunk == (struct kmallocHeader *) KERN_HEAP)
                fatalError("The first chunk is corrupted. Can't fix.\n");
            else
            {
                oldChunk->size += sizeof(struct kmallocHeader); // The previous block now overwites this one
                error("Chunk fixed.\n");
            }
        }

        // Go to the next chunk
        if (chunk->used)
            totalSize += chunk->size;
        oldChunk = chunk;
        chunk = (struct kmallocHeader *)((int)chunk + chunk->size);
    }

    if (totalSize != kmallocUsed)
        error("checkAllocChunks: Total size is %d but kmallocUsed is %d.\n", totalSize, kmallocUsed);
}

struct kmallocHeader* Paging::ksbrk(unsigned npages)
{
    if (npages == 0)
    {
        error("ksbrk: Called with npages == 0\n");
        return (struct kmallocHeader *) -1;
    }

    struct kmallocHeader *chunk;
    char *pAddr;

    // If we don't have enough room left in the heap, we're in trouble.
    if ((curKernHeap + (npages * PAGESIZE)) > (char *) KERN_HEAP_LIM)
    {
        error("Paging::ksbrk: No virtual memory left for kernel heap\n");
        return (struct kmallocHeader *) -1;
    }

    chunk = (struct kmallocHeader *) curKernHeap;

    // Alloc a free page
    for (int i = 0; i < npages; i++)
    {
        pAddr = getPageFrame();
        if ((int)pAddr <= 0)
        {
            error("Paging::ksbrk: No free page frame available !\n");
            return (struct kmallocHeader *) -1;
        }

        // Add to the page dir
        pageDir0AddPage(curKernHeap, pAddr, 0);

        curKernHeap += PAGESIZE;
    }

    // Mark for kmalloc
    chunk->size = PAGESIZE * npages;
    chunk->used = 0;

    return chunk;
}

char* Paging::kmalloc(unsigned long size)
{
    // If someone tries to allocate 0B, warn him that he might be in trouble and return the nullptr
    if (!size)
    {
        error("Paging::kmalloc called with a size of 0 !\n");
        return nullptr;
    }

    unsigned long realsize;	// Total size of the record
    struct kmallocHeader *chunk, *other;

    if ((realsize = sizeof(struct kmallocHeader) + size) < KMALLOC_MINSIZE)
        realsize = KMALLOC_MINSIZE;

    // We search a free block of size 'size' bytes in the heap
    chunk = (struct kmallocHeader *) KERN_HEAP;
    while (chunk->used || chunk->size < realsize)
    {
        if (chunk->size == 0)
        {
            error("kmalloc() : Corrupted chunk on 0x%x with null size (heap end:0x%x) !\n", chunk, curKernHeap);
            if (chunk == (struct kmallocHeader *) KERN_HEAP)
                fatalError("The first chunk is corrupted. Can't fix.\n");
            else
            {
                other->size += sizeof(struct kmallocHeader); // The previous block now overwites this one
                error("Chunk fixed.\n");
            }
        }

        other = chunk;
        chunk = (struct kmallocHeader *) ((char *) chunk + chunk->size); // Go to the next chunk

        if (chunk == (struct kmallocHeader *) curKernHeap)
        {
            if ((int)ksbrk((realsize / PAGESIZE) + 1) < 0)
                fatalError("kmalloc() : no memory left for kernel !\n");
        }
        else if (chunk > (struct kmallocHeader *) curKernHeap)
            fatalError("kmalloc() : chunk on 0x%x while end of malloc heap is on 0x%x !\n",chunk, curKernHeap);
    }

     // We found a block with a size >= 'size'. We make sure that each block has a minimal size
    if (chunk->size - realsize < KMALLOC_MINSIZE)
        chunk->used = 1;
    else
    {
        other = (struct kmallocHeader *) ((char *) chunk + realsize);
        other->size = chunk->size - realsize;
        other->used = 0;

        if (!other->size)
            fatalError("kmalloc() : chunk size of 'other' set to 0 !\n");

        chunk->size = realsize;
        chunk->used = 1;

        if (!chunk->size)
            fatalError("kmalloc() : chunk size of 'chunk' set to 0 !\n");
    }

    kmallocUsed += realsize;

    // Return a pointer to the data
    return (char *) chunk + sizeof(struct kmallocHeader);
}

void Paging::kfree(void *vAddr)
{
    if (!Paging::pagingEnabled)
        fatalError("kfree called with paging disabled");

    struct kmallocHeader *chunk, *other;

    // Free the allocated block
    chunk = (struct kmallocHeader *) ((char*)vAddr - sizeof(struct kmallocHeader));
    chunk->used = 0;

    kmallocUsed -= chunk->size;

    // Merge the free'd block with the next one, if he's free too
    while ((other = (struct kmallocHeader *) ((char *) chunk + chunk->size))
           && other < (struct kmallocHeader *) curKernHeap
           && other->used == 0)
        chunk->size += other->size;
        if (!chunk->size)
            fatalError("kfree() : chunk size set to 0 !\n");
}

char* memcpy(char* dst, const char* src, size_t n)
{
	char *d = dst;
	const char *s = src;
	while (n--)
		*d++ = *s++;
	return dst;
}

void* operator new(size_t size)
{
    if (!Paging::pagingEnabled)
        fatalError("new called with paging disabled");
	return gPaging.kmalloc(size);;
}

void* operator new[](size_t size)
{
    if (!Paging::pagingEnabled)
        fatalError("new[] called with paging disabled");
	return gPaging.kmalloc(size);
}

void operator delete(void *p)
{
    if (!Paging::pagingEnabled)
        fatalError("delete called with paging disabled");
	gPaging.kfree(p);
}

void operator delete[](void *p)
{
    if (!Paging::pagingEnabled)
        fatalError("delete[] called with paging disabled");
	gPaging.kfree(p);
}
