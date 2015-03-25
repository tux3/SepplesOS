#include <mm/paging.h>
#include <mm/memzero.h>
#include <mm/memmap.h>
#include <mm/memcpy.h>
#include <vga/vgatext.h>
#include <error.h>
#include <debug.h>
#include <lib/humanReadable.h>
#include <lib/algorithm.h>

u8* Paging::physPageBitmap;
unsigned Paging::maxPhysPages;

void Paging::init(long availableMem)
{
    physPageBitmap = (u8*)FREE_PAGES_START;
    maxPhysPages = availableMem/PAGESIZE + (bool)(availableMem%PAGESIZE);

    // Map pages for our phys page bitmap, zero it, then mark it in itself
    unsigned physPageBitmapSize = maxPhysPages/8 + (bool)(maxPhysPages%8);
    unsigned physPageBitmapPagesize = physPageBitmapSize/PAGESIZE + (bool)(physPageBitmapSize%PAGESIZE);
    u64 pagetable0_addr = PT0_ADDR;
    for (unsigned i=0; i<physPageBitmapPagesize; ++i)
    {
        u64* entry = (u64*) (pagetable0_addr + ((u64)physPageBitmap/PAGESIZE + i)*8);
        *entry = ((u64)physPageBitmap + i*PAGESIZE) | 0b11;
    }
    memzero(physPageBitmap, physPageBitmapSize);
    for (unsigned i=0; i<physPageBitmapPagesize; ++i)
        markPhysPageUsed((u64)physPageBitmap/PAGESIZE + i);

    // Mark all pages until the physical page bitmap as used
    for (unsigned i=0; i<(u64)physPageBitmap/PAGESIZE; ++i)
        markPhysPageUsed(i);
}

void Paging::markPhysPageFree(unsigned page)
{
    if (page >= maxPhysPages)
    {
        error("Paging::markPhysPageFree: physical page 0x%x does not exist!\n",page);
        return;
    }

    physPageBitmap[page/8] &= ~(1 << (page%8));
}

void Paging::markPhysPageFree(void* paddr)
{
    if ((u64)paddr/PAGESIZE >= maxPhysPages)
    {
        error("Paging::markPhysPageFree: page at paddr 0x%x does not exist!\n",paddr);
        return;
    }

    physPageBitmap[(u64)paddr/PAGESIZE/8] &= ~(1 << (((u64)paddr/PAGESIZE)%8));
}

void Paging::markPhysPageUsed(unsigned page)
{
    if (page >= maxPhysPages)
    {
        error("Paging::markPhysPageUsed: physical page number 0x%x does not exist!\n",page);
        return;
    }

    physPageBitmap[page/8] |= (1 << ( page%8));
}

void Paging::markPhysPageUsed(void* paddr)
{
    if ((u64)paddr/PAGESIZE >= maxPhysPages)
    {
        error("Paging::markPhysPageUsed: paddr 0x%x does not exist!\n",paddr);
        return;
    }

    physPageBitmap[(u64)paddr/PAGESIZE/8] |= (1 << (((u64)paddr/PAGESIZE)%8));
}

unsigned Paging::getMaxPhysPages()
{
    return maxPhysPages;
}

u8* Paging::getFreePhysPage()
{
    long page = -1;
    for (unsigned byte = 0; byte < maxPhysPages / 8; byte++)
    {
        if (physPageBitmap[byte] == 0xFF)
            continue;
        for (int bit = 0; bit < 8; bit++)
            if (!(physPageBitmap[byte] & (1 << bit)))
            {
                page = 8 * byte + bit;
                return (u8 *) (page * PAGESIZE);
            }
    }

    error("Paging::getPageFrame: No free page frame found.\n");
    return nullptr;
}

u8* Paging::getPAddr(u8* vaddr)
{
    u64 entry;
    phyread(&entry, (u64*)((PML4_ADDR) + ((u64)vaddr>>39 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        return nullptr;
    phyread(&entry, (u64*)((entry&~(0x7FFL)) + ((u64)vaddr>>30 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        return nullptr;
    phyread(&entry, (u64*)((entry&~(0x7FFL)) + ((u64)vaddr>>21 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        return nullptr;
    phyread(&entry, (u64*)((entry&~(0x7FFL)) + ((u64)vaddr>>12 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        return nullptr;

    return (u8*)(entry&~(0x7FFL));
}

u8* Paging::mapPage(u8* vaddr, u8* paddr, int flags, u8 *PML4)
{
    /// TODO: Implement creating new PDPTs on the fly

    if (!vaddr)
        vaddr = getUnmappedVAddr();
    if (!vaddr)
        return nullptr;

    if (flags&~0xFFF)
        error("Paging::mapPage: flags&~0xFFF should be zero, but flags is %p\n",flags);
    flags &= 0xFFF;

    u64 PML4entry;
    phyread(&PML4entry, (u64*)(((u64)PML4) + ((u64)vaddr>>39 & 0x1FF)*8), 8);
    if (!(PML4entry & PAGE_ATTR_PRESENT))
    {
        fatalError("mapPage: Trying to map throught a null PML4 entry for %p\n",vaddr);
        return nullptr;
    }
    u64 PDPTentry;
    phyread(&PDPTentry, (u64*)((PML4entry&~(0xFFFUL)) + ((u64)vaddr>>30 & 0x1FF)*8), 8);
    if (!(PDPTentry & PAGE_ATTR_PRESENT))
    {
        //logE9("mapPage: The required PDPT entry is null for %p:%p, building a new PD!\n",PML4,vaddr);
        // Get a physical page to store our new PD in
        u8* pdpaddr = getFreePhysPage();
        if (!pdpaddr)
            fatalError("Paging::mapPage: Couldn't get a physical page!\n");
        pdpaddr = (u8*)((u64)pdpaddr&~(0xFFFUL));
        markPhysPageUsed((u64)pdpaddr/PAGESIZE);

        // Create our new page dir in the newly mapped page
        phyzero(pdpaddr, PAGESIZE);
        PDPTentry = ((u64)pdpaddr&~(0xFFFUL)) | (PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | flags);
        phywrite((u64*)((PML4entry&~(0xFFFUL)) + ((u64)vaddr>>30 & 0x1FF)*8), &PDPTentry, 8);
    }
    u64 PDentry;
    phyread(&PDentry, (u64*)((PDPTentry&~(0xFFFUL)) + ((u64)vaddr>>21 & 0x1FF)*8), 8);
    if (!(PDentry & PAGE_ATTR_PRESENT))
    {
        //logE9("mapPage: The required PD entry is null for %p:%p, building a new PT!\n",PML4,vaddr);
        // Get a physical page to store our new PT in
        u8* ptpaddr = getFreePhysPage();
        if (!ptpaddr)
            fatalError("Paging::mapPage: Couldn't get a physical page!\n");
        ptpaddr = (u8*)((u64)ptpaddr&~(0xFFFUL));
        markPhysPageUsed((u64)ptpaddr/PAGESIZE);

        // Create our new page table in the newly mapped page
        phyzero(ptpaddr, PAGESIZE);
        PDentry = ((u64)ptpaddr&~(0xFFFUL)) | (PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | flags);
        phywrite((u64*)((PDPTentry&~(0xFFFUL)) + ((u64)vaddr>>21 & 0x1FF)*8), &PDentry, 8);
    }
    u64 PTentry;
    phyread(&PTentry, (u64*)((PDentry&~(0xFFFUL)) + ((u64)vaddr>>12 & 0x1FF)*8), 8);
    if (PTentry & PAGE_ATTR_PRESENT)
    {
        logE9("mapPage: Page %p:%p is already mapped to %p!\n",PML4,vaddr,PTentry&~(0xFFFUL));
        logE9("mapPage: PML4e:%p, PDPTe:%p, PDe:%p, PTe:%p\n",PML4entry,PDPTentry,PDentry,PTentry);
        return nullptr;
    }

    if (!paddr)
        paddr = getFreePhysPage();
    if (!paddr)
    {
        error("mapPage: Couldn't get a free physical page for %p:%p\n",PML4,vaddr);
        return nullptr;
    }
    paddr = (u8*)((u64)paddr&~(0xFFFUL));

    PTentry = (u64)paddr | (PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | flags);
    phywrite((u64*)((PDentry&~(0xFFFUL)) + ((u64)vaddr>>12 & 0x1FF)*8), &PTentry, 8);
    markPhysPageUsed((u64)paddr/PAGESIZE);
    //logE9("Mapped page %p:%p -> %p\n",PML4,vaddr,paddr);
    //logE9("mapPage: PML4e:%p, PDPTe:%p, PDe:%p, PTe:%p\n",PML4entry,PDPTentry,PDentry,PTentry);
    return vaddr;
}

void Paging::unmapPage(u8* vaddr)
{
    u8* paddr;
    u64 entry;
    u64 PDentry;
    phyread(&entry, (u64*)((PML4_ADDR) + ((u64)vaddr>>39 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        goto fail;
    phyread(&entry, (u64*)((entry&~(0xFFFUL)) + ((u64)vaddr>>30 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        goto fail;
    phyread(&PDentry, (u64*)((entry&~(0xFFFUL)) + ((u64)vaddr>>21 & 0x1FF)*8), 8);
    if (!(PDentry & PAGE_ATTR_PRESENT))
        goto fail;
    phyread(&entry, (u64*)((PDentry&~(0xFFFUL)) + ((u64)vaddr>>12 & 0x1FF)*8), 8);
    if (!(entry & PAGE_ATTR_PRESENT))
        goto fail;

    paddr = (u8*)(entry&~(0xFFFUL));
    markPhysPageFree(paddr);
    phyzero((u64*)((PDentry&~(0xFFFUL)) + ((u64)vaddr>>12 & 0x1FF)*8), 8);
    asm("invlpg (%0)"::"r"(vaddr));
    //logE9("Unmapped page %p -> %p\n",vaddr,paddr);
    return;

fail:
    error("Paging::unmapPage: %p is not mapped!\n", vaddr);
    return;
}

u8* Paging::getUnmappedVAddr()
{
    for (u64 PML4i=0; PML4i<512; ++PML4i)
    {
        /// TODO: If PDPT !present, make one and continue
        u64 PML4e;
        phyread(&PML4e, (u64*)(PML4_ADDR+PML4i*8), 8);
        if (!(PML4e & PAGE_ATTR_PRESENT))
            fatalError("getUnmappedVAddr: The first page dir pointer table's full, but building a new one is not implemented!\n");

        for (u64 PDPTi=0; PDPTi<512; ++PDPTi)
        {
            /// TODO: If PD !present, make one and continue
            u64 PDPTe;
            phyread(&PDPTe, (u64*)((PML4e&~(0xFFFUL))+PDPTi*8), 8);
            if (!(PDPTe & PAGE_ATTR_PRESENT))
                fatalError("getUnmappedVAddr: The first page dir's full, but building a new one is not implemented!\n");

            for (u64 PDi=0; PDi<512; ++PDi)
            {
                u64 PDe;
                phyread(&PDe, (u64*)((PDPTe&~(0xFFFUL))+PDi*8), 8);
                if (!(PDe & PAGE_ATTR_PRESENT))
                {
                    //logE9("getUnmappedVAddr: The previous page table's full, building a new one!\n");
                    // Get a physical page to store our new PT in
                    u8* paddr = getFreePhysPage();
                    if (!paddr)
                        fatalError("Paging::getUnmappedVAddr: Couldn't get a physical page!\n");
                    paddr = (u8*)((u64)paddr&~(0xFFFUL));
                    markPhysPageUsed((u64)paddr/PAGESIZE);

                    // Create our new page table in the newly mapped page
                    phyzero(paddr, PAGESIZE);
                    PDe = ((u64)paddr&~(0xFFFUL)) | 0b11;
                    phywrite((u64*)((PDPTe&~(0xFFFUL))+PDi*8), &PDe, 8);
                }

                // Skip the first 0x100000 bytes
                // If a page's not present, just build and return its vaddr
                u64 PTi = 0;
                if ((u64*)(PDe&~(0xFFFUL)) == (u64*)PT0_ADDR)
                    PTi += FREE_PAGES_START/PAGESIZE;
                for (; PTi<512; ++PTi)
                {
                    u64 PTe;
                    phyread(&PTe, (u64*)((PDe&~(0xFFFUL))+PTi*8), 8);
                    if (!(PTe & PAGE_ATTR_PRESENT))
                        return (u8*)((PML4i<<39) + (PDPTi<<30) + (PDi<<21) + (PTi<<12));
                }
            }
        }
    }

    // We've found nothing in the whole PML4 !
    error("Paging::getUnmappedVAddr: No unmapped vaddr left!\n");
    return nullptr;
}

u8* Paging::createPageDir()
{
    u8* PML4 = getFreePhysPage();
    if (!PML4)
        return nullptr;
    markPhysPageUsed(PML4);
    u8* PDPT0 = getFreePhysPage();
    if (!PDPT0)
    {
        markPhysPageFree(PML4);
        return nullptr;
    }
    markPhysPageUsed(PDPT0);

    tmpmap(PML4);
    memzero((u8*)TMPMAP_PAGE_VADDR, PAGESIZE);
    *(u64*)TMPMAP_PAGE_VADDR = (u64)PDPT0 | 0b111;

    tmpmap(PDPT0);
    memzero((u8*)TMPMAP_PAGE_VADDR, PAGESIZE);

    // If USER_OFFSET is 0x40000000UL, then we mirror exactly one page dir
    // All users share the kernel's page dir 0
    *(u64*)TMPMAP_PAGE_VADDR = (u64)PD0_ADDR | 0b111;

    return PML4;
}

void Paging::destroyPageDir(u8* pml4)
{
    // Free everything that isn't in the PD0
    for (u64 PML4i=0; PML4i<512; ++PML4i)
    {
        u64 PML4e;
        phyread(&PML4e, (u64*)(pml4+PML4i*8), 8);
        if (!(PML4e & PAGE_ATTR_PRESENT))
            continue;

        for (u64 PDPTi=0; PDPTi<512; ++PDPTi)
        {
            u64 PDPTe;
            phyread(&PDPTe, (u64*)((PML4e&~(0xFFFUL))+PDPTi*8), 8);
            if (!(PDPTe & PAGE_ATTR_PRESENT))
                continue;

            if ((PDPTe&~(0xFFFUL)) == PD0_ADDR)
                continue;

            for (u64 PDi=0; PDi<512; ++PDi)
            {
                u64 PDe;
                phyread(&PDe, (u64*)((PDPTe&~(0xFFFUL))+PDi*8), 8);
                if (!(PDe & PAGE_ATTR_PRESENT))
                    continue;

                for (u64 PTi = 0; PTi<512; ++PTi)
                {
                    u64 PTe;
                    phyread(&PTe, (u64*)((PDe&~(0xFFFUL))+PTi*8), 8);
                    if (PTe & PAGE_ATTR_PRESENT)
                        markPhysPageFree((u8*)(PTe&~0xFFFUL));
                }
                markPhysPageFree((u8*)(PDe&~(0xFFFUL)));
            }
            markPhysPageFree((u8*)(PDPTe&~(0xFFFUL)));
        }
        markPhysPageFree((u8*)(PML4e&~(0xFFFUL)));
    }
    markPhysPageFree((u8*)(pml4));

    return;
}

void Paging::printStats()
{
    // Count how many pages are mapped
    u64 nMappedPages=0;
    for (u64 PML4i=0; PML4i<512; ++PML4i)
    {
        u64 PML4e;
        phyread(&PML4e, (u64*)(PML4_ADDR+PML4i*8), 8);
        if (!(PML4e & PAGE_ATTR_PRESENT))
            goto done;

        for (u64 PDPTi=0; PDPTi<512; ++PDPTi)
        {
            u64 PDPTe;
            phyread(&PDPTe, (u64*)((PML4e&~(0xFFFUL))+PDPTi*8), 8);
            if (!(PDPTe & PAGE_ATTR_PRESENT))
                goto done;

            for (u64 PDi=0; PDi<512; ++PDi)
            {
                u64 PDe;
                phyread(&PDe, (u64*)((PDPTe&~(0xFFFUL))+PDi*8), 8);
                if (!(PDe & PAGE_ATTR_PRESENT))
                    goto done;

                for (u64 PTi=0; PTi<512; ++PTi)
                {
                    u64 PTe;
                    phyread(&PTe, (u64*)((PDe&~(0xFFFUL))+PTi*8), 8);
                    if (PTe & PAGE_ATTR_PRESENT)
                        nMappedPages++;
                }
            }
        }
    }

done:
    u8 buf[64];
    VGAText::printf("Paging: %p pages mapped (%s), %p pages free (%s)\n",nMappedPages,toIEC(buf,32,nMappedPages*PAGESIZE),
                    maxPhysPages-nMappedPages,toIEC(buf+32,32,(maxPhysPages-nMappedPages)*PAGESIZE));
}

void Paging::tmpmap(void* pdst)
{
    static u64* PTe = (u64*)(PT0_ADDR + TMPMAP_PAGE_VADDR/PAGESIZE*8);

    u64 page_pdst = (u64)pdst&~(0x1FF);
    if ((u64)pdst != page_pdst)
        fatalError("tmpmap: pdst must be page-aligned!\n");

    *PTe = page_pdst | 0b11;
    asm volatile("invlpg (%0)"::"r"(TMPMAP_PAGE_VADDR));
}

void Paging::phyread(void* _vdst, void* _psrc, size_t size)
{
    u8* vdst = (u8*)_vdst, *psrc = (u8*)_psrc;
    u8* tmppage = (u8*)TMPMAP_PAGE_VADDR;
    while(size)
    {
        u8* psrc_page = (u8*)((u64)psrc&~0xFFFUL);
        tmpmap(psrc_page);

        u64 offset = (u64)psrc - (u64)psrc_page;
        u64 realsize = MIN(size, PAGESIZE - offset);

        memcpy((char*)vdst, (char*)(tmppage+offset), realsize);
        psrc += realsize;
        vdst += realsize;
        size -= realsize;
    }
}

void Paging::phywrite(void* _pdst, void* _vsrc, size_t size)
{
    u8* pdst = (u8*)_pdst, *vsrc = (u8*)_vsrc;
    u8* tmppage = (u8*)TMPMAP_PAGE_VADDR;
    while(size)
    {
        u8* pdst_page = (u8*)((u64)pdst&~0xFFFUL);
        tmpmap(pdst_page);

        u64 offset = (u64)pdst - (u64)pdst_page;
        u64 realsize = MIN(size, PAGESIZE - offset);

        memcpy((char*)(tmppage+offset), (char*)vsrc, realsize);
        pdst += realsize;
        vsrc += realsize;
        size -= realsize;
    }
}

void Paging::phyzero(void* _pdst, size_t size)
{
    u8* pdst = (u8*)_pdst;
    u8* tmppage = (u8*)TMPMAP_PAGE_VADDR;
    while(size)
    {
        u8* pdst_page = (u8*)((u64)pdst&~0xFFFUL);
        tmpmap(pdst_page);

        u64 offset = (u64)pdst - (u64)pdst_page;
        u64 realsize = MIN(size, PAGESIZE - offset);

        memzero(tmppage+offset, realsize);
        pdst += realsize;
        size -= realsize;
    }
}
