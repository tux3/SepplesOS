#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <std/types.h>

#define MALLOC_CHECK_CHUNKS_MAGIC 1

// If defined to 1, operator new will record the size, address, and origin
// of every allocation that goes through it, and delete will erase the corresponding record
// Records currently recorded can be seen with Malloc::printChunks(true),
// a large amount of records of the same size from the same location is likely
// to indicate a memory leak.
// Note that if free, realloc, or malloc are used directly,
// the records will not be updated and may become inacurate.
// TODO: Make realloc update the corresponding record, don't use malloc/free directly
#define CHECK_LEAK_NEW_DELETE 1

/// Malloc requires that Paging be initialized
namespace Malloc
{
    void init();

    u8* malloc(size_t size); ///< Reserves a chunk of memory of the given size for use by the kernel
    void free(void* vaddr); ///< Frees a previously allocated chunk of memory
    /// TODO: Implement realloc, to waste less space than manual malloc(bigger)+memcpy+free(old)
    u8* realloc(u8* vaddr, size_t newsize); ///< Returns a pointer to a chunk of size newsize with the same data
    void squeeze(); ///< Tries to free pages may unmap them or turn them into 'not ours' chunks

    bool isMallocReady(); ///< Malloc and free will never suceed if this returns false, howether, they will not cause UB.
    u64 getMemUsed(); ///< Returns the total number of bytes used by malloc, including headers and empty chunks
    u64 getMemAllocated(); ///< Returns the number of bytes currently allocated for users by malloc

    // Debug
    void checkChunks(); ///< Checks all the memory chunks for inconsistencies. Will try to repair them.
    void printChunks(bool e9only=false); ///< Print the list of malloc chunks. Will not try to repair them.
    void printStats(bool printChunks=false); ///< Outputs some stats on memory usage
}

// Paging must be initialized to use these functions.
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void *p);
void operator delete[](void *p);

void *operator new(size_t, void *p)     throw();
void *operator new[](size_t, void *p)   throw();
void  operator delete  (void *, void *) throw();
void  operator delete[](void *, void *) throw();

// Remember to use static_init new to manually contruct global static variables!
template <typename T, class... Args> void static_init(T* object, const Args&... args)
{
    *object = *(new (object) T(args...));
}

#endif // _MALLOC_H_
