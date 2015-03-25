#include <mm/malloc.h>
#include <mm/paging.h>
#include <mm/memcpy.h>
#include <std/types.h>
#include <lib/algorithm.h>
#include <lib/humanReadable.h>
#include <error.h>
#include <debug.h>

#if (CHECK_LEAK_NEW_DELETE)
#include <lib/sizedArray.h>
#endif

/**
 * How malloc werks :
 * Malloc manages chunks of memory, prepending a header before each of them
 * The header contains a 48bit chunk size, a 14bit magic value, and 2 bits of flags
 * If the magic value is incorrect, the malloc chunk list is corrupted and the system is halted
 *
 * There is a unique faux-contiguous linked list of chunks.
 * If a contigous region we would use is already mapped, we wrapped it in a special chunk and search further.
 * We acquire and release pages as needed, and try to keep all chunks in the same list/page.
 *
 * We always keep 8B of mapped space for a "not ours" chunk header at endOfChunks
 * So that if we have to wrap around an already mapped page, we always have a chunk header on hand
 */

namespace Malloc
{

struct ChunkHeader
{
    u64 size : 48; ///< Size of the header+chunk, aka distance to the next header
    u64 magic : 14; ///< Magic value, if invalid the chunk list is corrupted!
    u64 used : 1; ///< If true, this chunk is currently allocated and in use
    u64 notOurs : 1; ///< If true, this region wasn't mapped by malloc
};

static const unsigned MALLOC_MINSIZE = 0x20;
static const unsigned magic = 0x2B4F;
static unsigned long totalMemUsed = 0; ///< Raw amount of memory used by Malloc
static unsigned long totalMemAllocated = 0; ///< Memory actuallly allocated for users
static bool mallocReady = false; ///< Cheap non-atomic "mutex". To catch most bugs before they turns into a mess of corrupted memory.
static u8* firstChunk = nullptr;
static u8* endOfChunks = nullptr; ///< Points 1 byte after the last chunk's last byte

#if (CHECK_LEAK_NEW_DELETE)
struct AllocRecord
{
    u64 caller;
    u64 size;
    u8* addr;
};

static SizedArray<AllocRecord> allocRecords;
static bool noRecordSelf;
#endif

void init()
{
    static_assert(MALLOC_MINSIZE >= sizeof(ChunkHeader),"MALLOC_MINSIZE >= sizeof(ChunkHeader)");

    firstChunk = Paging::mapPage();
    totalMemUsed += PAGESIZE;
    endOfChunks = firstChunk + PAGESIZE - sizeof(ChunkHeader);
    ((ChunkHeader*)firstChunk)->size = PAGESIZE - sizeof(ChunkHeader);
    ((ChunkHeader*)firstChunk)->magic = magic;
    ((ChunkHeader*)firstChunk)->used = 0;
    ((ChunkHeader*)firstChunk)->notOurs = 0;
    mallocReady = true;

#if (CHECK_LEAK_NEW_DELETE)
    static_init(&allocRecords);
    noRecordSelf=false;
#endif
}

bool isMallocReady()
{
    return mallocReady;
}

u64 getMemUsed()
{
    return totalMemUsed;
}

u64 getMemAllocated()
{
    return totalMemAllocated;
}

u8* malloc(size_t size)
{
    if (!mallocReady)
    {
        error("malloc: Called while not ready\n");
        return nullptr;
    }
    mallocReady = false; // malloc must be carefull to not call itself recursively.

    if (!size)
    {
        error("malloc called with a size of 0 !\n");
        mallocReady = true;
        return nullptr;
    }

    u8* nextUnmappedVaddr = Paging::getUnmappedVAddr();

    ChunkHeader *chunk, *other=nullptr;
    unsigned long realsize = sizeof(ChunkHeader) + size;	// Total size of the record
    if (realsize < MALLOC_MINSIZE)
        realsize = MALLOC_MINSIZE;

    // We search a free block big enough
    //error("malloc: searching chunks for alloc of 0x%x bytes\n",size);
    chunk = (ChunkHeader *) firstChunk;
    while (chunk->used || chunk->size < realsize)
    {
        //error("malloc: chunk at 0x%x, size 0x%x\n", chunk, chunk->size);

#if (MALLOC_CHECK_CHUNKS_MAGIC)
        if (chunk->magic != magic)
        {
            error("malloc: Chunk with bad magic at 0x%x!\n", (u64)chunk);
            printChunks();
            halt();
        }
#endif

        if (!chunk->size)
            fatalError("malloc: Corrupted chunk at 0x%x with null size !\n", (u64)chunk);
        if (other && !other->used && !chunk->used && (u8*)chunk<endOfChunks)
        {
            other->size+=chunk->size;
            chunk = other;
            other = nullptr;
            continue;
        }

        if (chunk->notOurs && (u8*)chunk+sizeof(ChunkHeader) == nextUnmappedVaddr)
        {
            if (chunk->size != PAGESIZE+sizeof(ChunkHeader))
                fatalError("malloc: Reclaiming more than a page at a time not implemented, but the size is %p\n",chunk->size);

            if (Paging::mapPage(nextUnmappedVaddr))
            {
                chunk->used = false;
                chunk->notOurs = false;
            }

            nextUnmappedVaddr = Paging::getUnmappedVAddr();
        }

        other = chunk;
        chunk = (ChunkHeader*) ((char *) chunk + chunk->size); // Go to the next chunk

        if (chunk == (ChunkHeader*) endOfChunks) // Couldn't find a chunk, go fetch more pages
        {
            u8* oldEndOfChunks = endOfChunks;
            u64 needToAlloc = size+sizeof(ChunkHeader);
            u64 totalAlloced = 0;
            if (!other->used)
                needToAlloc -= MIN(needToAlloc, other->size);

            while (needToAlloc > 0)
            {
                //error("malloc: Still need to alloc 0x%x bytes -> 0x%x pages, next at 0x%x\n",needToAlloc, needToAlloc/PAGESIZE + (bool)(needToAlloc%PAGESIZE),(u64)endOfChunks);
                u8* nextUnmapped = Paging::mapPage(endOfChunks+sizeof(ChunkHeader));
                if (nextUnmapped != endOfChunks+sizeof(ChunkHeader))
                {
                    // Find how many contigous unmappable pages we need to wrap
                    nextUnmapped = Paging::getUnmappedVAddr();
                    u64 diff = nextUnmapped-endOfChunks-sizeof(ChunkHeader);
                    u64 nContigousUnmappable = diff/PAGESIZE + (bool)(diff%PAGESIZE);

                    // Restart allocating from here
                    ((ChunkHeader*)endOfChunks)->magic = magic;
                    ((ChunkHeader*)endOfChunks)->notOurs = 1;
                    ((ChunkHeader*)endOfChunks)->size = PAGESIZE*nContigousUnmappable+sizeof(ChunkHeader);
                    ((ChunkHeader*)endOfChunks)->used = 1;
                    other = (ChunkHeader*)endOfChunks;
                    oldEndOfChunks = endOfChunks+((ChunkHeader*)endOfChunks)->size;

                    nextUnmapped = Paging::mapPage(nextUnmapped);
                    endOfChunks += PAGESIZE*(nContigousUnmappable);
                    totalAlloced = -(sizeof(ChunkHeader));
                    needToAlloc = size+sizeof(ChunkHeader);
                }

                if (!other->used)
                    other->size += PAGESIZE;

                totalMemUsed += PAGESIZE;
                endOfChunks += PAGESIZE;
                totalAlloced += PAGESIZE;
                needToAlloc -= MIN(needToAlloc, (u64)PAGESIZE);
            }

            if (((u64)endOfChunks & 0xFFF) != 0xFF8)
                fatalError("endOfChunks doesn't leave room for 'not ours' chunk header: 0x%x\n",(u64)endOfChunks);

            if (!other->used)
            {
                if (other->size < realsize)
                    fatalError("malloc: The previous chunk is still too small after we're done growing it!\n");

                chunk = other;
                break;
            }
            else
            {
                other = (ChunkHeader*)oldEndOfChunks;
                other->magic = magic;
                other->size = totalAlloced;
                other->used = 0;
                other->notOurs = 0;
                chunk = other;
                break;
            }
        }
        else if (chunk > (ChunkHeader*) endOfChunks)
        {
            error("kmalloc() : chunk at 0x%x while end of malloc chunks is at 0x%x !\n",(u64)chunk, (u64)endOfChunks);
            printChunks();
            halt();
        }
    }
    //error("malloc: using chunk at 0x%x, size 0x%x (before resize)\n", chunk, chunk->size);

     // We found a block with a size >= 'size'. We make sure that each block has a minimal size
     // If we wouldn't have the room to separate this chunk in two chunks, just use the whole chunk
    chunk->used = 1;
    if (chunk->size - realsize >= MALLOC_MINSIZE)
    {
        other = (ChunkHeader*) ((u8*)chunk + realsize);
        other->size = chunk->size - realsize;
        other->magic = magic;
        other->used = 0;
        other->notOurs = 0;

        if (!other->size)
            fatalError("malloc() : chunk size of 'other' set to 0 !\n"); // This isn't supposed to be possible

        chunk->size = realsize;
        chunk->used = 1;

        if (!chunk->size)
            fatalError("malloc() : chunk size of 'chunk' set to 0 !\n"); // This isn't supposed to be possible
    }

    totalMemAllocated += chunk->size - sizeof(ChunkHeader);

    // Return a pointer to the data
    mallocReady = true;
    return (u8*) chunk + sizeof(ChunkHeader);
}

void free(void *vaddr)
{
    if (!mallocReady)
    {
        error("free: Called while not ready\n");
        return;
    }
    if (!vaddr)
        return;

    mallocReady = false;

    ChunkHeader *chunk, *other;

    // Free the allocated block
    chunk = (ChunkHeader*) ((char*)vaddr - sizeof(ChunkHeader));
    if (chunk->magic != magic)
    {
        error("free: Chunk with bad magic %p at %p!\n",chunk->magic,(u64)chunk);
        printChunks(true);
        halt();
    }
    if (!chunk->used)
    {
        error("free: Trying to free chunk at %p, but it's not used!\n",chunk);
        printChunks(true);
        halt();
    }
    chunk->used = 0;

    totalMemAllocated -= (chunk->size - sizeof(ChunkHeader));

    // Merge the free'd block with the next one, if he's free too
    while ((other = (ChunkHeader*) ((u8*) chunk + chunk->size))
           && other < (ChunkHeader*) endOfChunks
           && other->used == 0)
    {
#if (MALLOC_CHECK_CHUNKS_MAGIC)
        if (chunk->magic != magic)
        {
            error("free: Chunk with bad magic at 0x%x!\n", (u64)chunk);
            printChunks();
            halt();
        }
#endif

        chunk->size += other->size;
        if (!chunk->size)
            fatalError("kfree: chunk size set to 0 !\n");

        if (!other->size)
        {
            error("kfree: Corrupted chunk with null size !\n");
            printChunks();
            halt();
        }
    }

    // Free some pages at the end if possible
    if ((u8*)other == endOfChunks && chunk->size >= PAGESIZE)
    {
        u64 nPages = chunk->size / PAGESIZE;
        //error("We could free 0x%x pages, size is 0x%x\n",nPages,chunk->size);
        chunk->size -= PAGESIZE*nPages;
        endOfChunks -= PAGESIZE*nPages;
        totalMemUsed -= PAGESIZE*nPages;
        for (u64 i=0; i<nPages; ++i)
            Paging::unmapPage(endOfChunks + sizeof(ChunkHeader) + PAGESIZE*i);
    }

    mallocReady = true;
}

void printChunks(bool e9only)
{
#define log1(x)     if(e9only){logE9(x);}else{error(x);}
#define log2(x,y)     if(e9only){logE9(x,y);}else{error(x,y);}
#define log3(x,y,z) if(e9only){logE9(x,y,z);}else{error(x,y,z);}
#define log5(x,y,z,v,w) if(e9only){logE9(x,y,z,v,w);}else{error(x,y,z,v,w);}

    mallocReady = false;

#if (CHECK_LEAK_NEW_DELETE)
    log2("Got %d alloc records:\n",allocRecords.size);
    for (u64 i=0; i<allocRecords.size; ++i)
        log5("Record %d: chunk %p, size %p, alloc'd by %p\n",i,allocRecords[i].addr,allocRecords[i].size,allocRecords[i].caller);
#endif

    ChunkHeader *chunk;
    log3("printChunks: totalMemAllocated:0x%x, endOfChunks:0x%x\n", totalMemAllocated, (u64)endOfChunks);
    chunk = (ChunkHeader*) firstChunk;
    bool newline=false; // Used to format two chunks / line
    for (;;) // Check all the chunks, break at the end
    {
        // If we're at (or after) the end
        if (chunk == (ChunkHeader*) endOfChunks)
        {
            log1("printChunks: endOfChunks reached\n");
            mallocReady = true;
            return;
        }
        else if (chunk > (ChunkHeader*) endOfChunks)
        {
            log1("printChunks: chunk ends after endOfChunks\n");
            mallocReady = true;
            return;
        }

#if (MALLOC_CHECK_CHUNKS_MAGIC)
        if (chunk->magic != magic)
        {
            log3("printChunks: Chunk with bad magic 0x%x at 0x%x!\n", chunk->magic, (u64)chunk);
            halt();
        }
#endif

        log3("Chunk at 0x%x size 0x%x ", (u64)chunk, chunk->size);
        if (chunk->used) {
            log1("U");
        } else {
            log1("u");
        }
        if (chunk->notOurs) {
            log1("N");
        } else {
            log1("n");
        }
        if (newline) {
            log1("\n");
        } else {
            log1("\t\t");
        }
        newline = !newline;
        if (!chunk->size)
        {
            log1("printChuncks: Chunk with null size. Returning.\n");
            mallocReady = true;
            return;
        }
        chunk = (ChunkHeader*)((u64)chunk + chunk->size); // Next
    }
    mallocReady = true;
#undef log
}

u8* realloc(u8* vaddr, size_t newsize)
{
    if (!vaddr)
        return malloc(newsize);

    if (!mallocReady)
    {
        error("realloc: Called while not ready\n");
        return nullptr;
    }
    mallocReady = false;

    u64 hsize = sizeof(ChunkHeader);
    ChunkHeader* chunk = (ChunkHeader*)(vaddr-hsize);
    u64 realsize = newsize + hsize;
    if (realsize < MALLOC_MINSIZE)
        realsize = MALLOC_MINSIZE;
    if (chunk->magic != magic)
    {
        error("Realloc: Chunk with bad magic at %p, requested new size of %p\n",(u64)chunk,newsize);
        printChunks(true);
        halt();
    }
    else if (!chunk->used)
    {
        error("Realloc: Trying to realloc a free chunk at %p, requested new size of %p\n",(u64)chunk,newsize);
        printChunks(true);
        halt();
    }
    else if (chunk->notOurs)
    {
        error("Realloc: Trying to realloc a chunk that isn't ours at %p, requested new size of %p\n",(u64)chunk,newsize);
        printChunks(true);
        halt();
    }
    else if (chunk->size == realsize)
    {
        //error("realloc: Trying to realloc a chunk of the same size\n");
        mallocReady = true;
        return vaddr;
    }
    else if (chunk->size > realsize)
    {
        u64 sizediff = chunk->size - realsize;
        ChunkHeader* next = (ChunkHeader*) ((u64)chunk+chunk->size);
        if ((u8*)next < endOfChunks && !next->used)
        {
            totalMemAllocated -= sizediff;
            u64 newnextsize = next->size + sizediff; // memcpy might overwrite next->size
            ChunkHeader* newnext = (ChunkHeader*)((u8*)next - sizediff);
            memcpy(newnext, next, sizeof(ChunkHeader));
            newnext->size = newnextsize;
            chunk->size -= sizediff;
        }
        else if (sizediff >= MALLOC_MINSIZE)
        {
            totalMemAllocated -= sizediff;
            ChunkHeader* newnext = (ChunkHeader*)((u8*)chunk + realsize);
            newnext->size = sizediff;
            newnext->magic = magic;
            newnext->notOurs = false;
            newnext->used = false;
            chunk->size = realsize;
        }
        mallocReady = true;
        return vaddr;
    }

    // See if we can grow into the next chunk
    ChunkHeader* next = (ChunkHeader*) ((u64)chunk+chunk->size);
    if ((u8*)next < endOfChunks && !next->used && (chunk->size + next->size) >= realsize)
    {
        ChunkHeader* newnext = (ChunkHeader*) ((char *) chunk + realsize);
        i64 newnextsize = (chunk->size+next->size) - realsize;
        if (newnextsize > 0)
        {
            totalMemAllocated += realsize - chunk->size;

            newnext->size = newnextsize;
            newnext->magic = magic;
            newnext->used = 0;
            newnext->notOurs = 0;
            chunk->size = realsize;
        }
        else
        {
            totalMemAllocated += next->size;
            chunk->size += next->size;
        }
        mallocReady = true;
        return (u8*)chunk+hsize;
    }

    // See if we can grow into the previous chunk
    if ((u8*)chunk != firstChunk)
    {
        ChunkHeader* previous = (ChunkHeader*)firstChunk;
        while ((u64)previous+previous->size+hsize != (u64)vaddr)
        {
            previous =(ChunkHeader*)((u64)previous+previous->size);
        }
        if (!previous->used && chunk->size + previous->size >= realsize)
        {
            totalMemAllocated += realsize - chunk->size;
            i64 newnextsize = (previous->size+chunk->size) - realsize;
            memcpy((char*)previous+hsize, (char*)chunk+hsize, chunk->size-hsize);
            ChunkHeader* newnext = (ChunkHeader*) ((char *) previous + realsize);
            newnext->size = newnextsize;
            newnext->magic = magic;
            newnext->used = 0;
            newnext->notOurs = 0;
            previous->size = realsize;
            previous->used=1;
            mallocReady = true;
            return (u8*)previous+hsize;
        }
    }

    mallocReady = true;
    u8* newvaddr = malloc(newsize);
    memcpy((char*)newvaddr, (char*)vaddr, chunk->size-hsize);
    free(vaddr);
    return newvaddr;
}

void squeeze()
{
    ChunkHeader* other = nullptr;
    ChunkHeader* chunk = (ChunkHeader *) firstChunk;
    while (chunk < (ChunkHeader*) endOfChunks)
    {
#if (MALLOC_CHECK_CHUNKS_MAGIC)
        if (chunk->magic != magic)
        {
            error("Malloc::squeeze: Chunk with bad magic at 0x%x!\n", (u64)chunk);
            printChunks();
            halt();
        }
#endif

        if (chunk->size == 0)
        {
            error("squeeze: Corrupted chunk at 0x%x with null size !\n", (u64)chunk);
            if (chunk == (ChunkHeader *) firstChunk)
                fatalError("squeeze: The first chunk is corrupted. Can't fix.\n");
            else
            {
                other->size += sizeof(ChunkHeader); // The previous block now overwites this one
                error("squeeze: Chunk fixed.\n");
            }
        }

        if (other && !other->used && !chunk->used && (u8*)chunk<endOfChunks)
        {
            other->size+=chunk->size;
            chunk = other;
            other = nullptr;
            continue;
        }

        other = chunk;
        chunk = (ChunkHeader*) ((char *) chunk + chunk->size); // Go to the next chunk
    }

    // Free some pages at the end if possible
    if ((u8*)other == endOfChunks && chunk->size >= PAGESIZE)
    {
        u64 nPages = chunk->size / PAGESIZE;
        //error("We could free 0x%x pages, size is 0x%x\n",nPages,chunk->size);
        chunk->size -= PAGESIZE*nPages;
        endOfChunks -= PAGESIZE*nPages;
        totalMemUsed -= PAGESIZE*nPages;
        for (u64 i=0; i<nPages; ++i)
            Paging::unmapPage(endOfChunks + sizeof(ChunkHeader) + PAGESIZE*i);
    }
}

void printStats(bool _printChunks)
{
    u8 buf[64];
    if (mallocReady)
        VGAText::print("Malloc: Ready");
    else
        VGAText::print("Malloc: NOT ready");
    VGAText::printf(" with chunks from 0x%x to 0x%x\n",firstChunk,endOfChunks);
    VGAText::printf("Malloc: %s used, %s allocated\n", toIEC(buf,32,totalMemUsed),
                                toIEC(buf+32,32,totalMemAllocated));
    if (mallocReady && _printChunks)
        printChunks(true);
}

} // Malloc

void* operator new(size_t size)
{
#if (CHECK_LEAK_NEW_DELETE)
    if (!Malloc::noRecordSelf)
    {
        Malloc::noRecordSelf=true;
        u64 caller= (u64)__builtin_return_address(0);
        Malloc::AllocRecord record;
        record.caller = caller;
        record.size = size;
        record.addr = Malloc::malloc(size);
        Malloc::allocRecords.append(record);
        Malloc::noRecordSelf=false;
        return record.addr;
    }
    else
        return Malloc::malloc(size);
#else
    return Malloc::malloc(size);
#endif
}

void* operator new[](size_t size)
{
#if (CHECK_LEAK_NEW_DELETE)
    if (!Malloc::noRecordSelf)
    {
        Malloc::noRecordSelf=true;
        u64 caller= (u64)__builtin_return_address(0);
        Malloc::AllocRecord record;
        record.caller = caller;
        record.size = size;
        record.addr = Malloc::malloc(size);
        Malloc::allocRecords.append(record);
        Malloc::noRecordSelf=false;
        return record.addr;
    }
    else
        return Malloc::malloc(size);
#else
    return Malloc::malloc(size);
#endif
}

void operator delete(void *addr)
{
    Malloc::free(addr);

#if (CHECK_LEAK_NEW_DELETE)
    if (!Malloc::noRecordSelf)
    {
        Malloc::noRecordSelf=true;
        for (u64 i=0; i<Malloc::allocRecords.size; ++i)
        {
            if (Malloc::allocRecords[i].addr == addr)
            {
                Malloc::allocRecords.removeAt(i);
                break;
            }
        }
        Malloc::noRecordSelf = false;
    }
#endif
}

void operator delete[](void *addr)
{
    Malloc::free(addr);

#if (CHECK_LEAK_NEW_DELETE)
    if (!Malloc::noRecordSelf)
    {
        Malloc::noRecordSelf=true;
        for (u64 i=0; i<Malloc::allocRecords.size; ++i)
        {
            if (Malloc::allocRecords[i].addr == addr)
            {
                Malloc::allocRecords.removeAt(i);
                break;
            }
        }
        Malloc::noRecordSelf = false;
    }
#endif
}

void *operator new(size_t, void *p)     throw() { return p; }
void *operator new[](size_t, void *p)   throw() { return p; }
void  operator delete  (void *, void *) throw() { }
void  operator delete[](void *, void *) throw() { }
