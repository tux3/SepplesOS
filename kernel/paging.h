#ifndef PAGING_H_INCLUDED
#define PAGING_H_INCLUDED

#include <std/types.h>
#include <lib/llist.h>

#define	PAGESIZE 	    0x1000
#define	PAGESIZE_BIG    0x400000
#define	RAM_MAXPAGE	    0x100000
#define KMALLOC_MINSIZE	16

#define	VADDR_PAGE_DIR_OFFSET(addr)	((addr) & 0xFFC00000) >> 22
#define	VADDR_PAGE_TABLE_OFFSET(addr)	((addr) & 0x003FF000) >> 12
#define	VADDR_PAGE_OFFSET(addr)	(addr) & 0x00000FFF
#define PAGE(addr)		(addr) >> 12

#define	PAGING_FLAG 	0x80000000	// CR0 - bit 31
#define PSE_FLAG	    0x00000010	// CR4 - bit 4

#define PAGE_PRESENT	0x00000001	// page directory / table
#define PAGE_WRITE	    0x00000002
#define PAGE_USER		0x00000004
#define PAGE_4MB		0x00000080

#ifndef __PAGING_STRUCT__
#define __PAGING_STRUCT__

// Memory structs
struct page
{
	char *vAddr;
	char *pAddr;
};

struct pageDirectoryEntry
{
	struct page *base;
	llist<struct page*> pageTable;
};

typedef struct pageDirectoryEntry* pageDirectory;

struct vmArea
{
	char *vmStart;
	char *vmEnd;	// exclude
};

struct kmallocHeader
{
	unsigned long size:31;	// taille totale de l'enregistrement
	unsigned long used:1;
} __attribute__ ((packed));

#endif

// Manages the memory and paging
class Paging
{
public:
    Paging();
    void init(u32 highMem); ///< Activates paging

    char *getPageFrame(); ///< Gets a free page from the bitmap (and mark it as used)
    //struct page* getPageFromHeap(); ///< Gets a free page from the bitmap and associate it to a virtual page in the heap
    //int releasePageFromHeap(char *vAddr); ///< Free a page from the bitmap and the heap

    void setPageFrameUsed(u32 page); ///< Marks a page as used in the bitmap
    void releasePageFrame(u32 pAddr); ///< Marks a page as free in the bitmap

    pageDirectory pageDirCreate(); ///< Creates a page directory (usually for a process)
    int pageDirDestroy(pageDirectory pageDir); ///< Destroys a page directory

    int pageDir0AddPage(char *vAddr, char *pAddr, int flags); ///< Add an entry in the kernel's page dir
    int pageDirAddPage(char *vAddr, char *pAddr, int flags, pageDirectory pageDir); ///< Add an entry in the current page dir
    int pageDirRemovePage(char* vAddr); ///< Remove an entry in the current page dir

    char* getPAddr(char* vAddr); ///< Returns the physicall address associated to a virtual address
    u32* getPageDir0();
    u32 getKMallocUsed(); ///< Return the amount of memory allocated on the heap with kmalloc
    void printStats(); ///< Displays some infos about memory usage. Will call checkAllocChunks() first.
    bool isMallocReady(); ///< Returns mallocReady.

    void checkAllocChunks(); ///< Checks all the memory chunks in the heap for inconsistencies.
    char* kmalloc(unsigned long size); ///< Alloc heap memory for the kernel and returns a pointer to it
    void kfree(void* vAddr); ///< Frees a block allocated with kmalloc
    struct kmallocHeader* ksbrk(unsigned npages); ///< Alloc a n-pages-sized chunk on the heap for kmalloc

private:
    //llist<struct vmArea*> kernFreeVm; ///< List of the kernel's free pages
    u32* pageDir0;	///< kernel page directory. 1024 elements.
    u8* memBitmap; ///< Physical page alloc bitmap. Size of RAM_MAXPAGE/8 bytes. (= 1MiB large currently)
    u32 kmallocUsed;
    char* curKernHeap; ///< Points to the end of the heap currently used by malloc
    bool mallocReady; ///< False when we can NOT use kmalloc/new at the moment.
};

char* memcpy(char* dst, const char* src, size_t n);

// The paging must be activated before those can work.
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void *p);
void operator delete[](void *p);

/// Singleton
extern Paging gPaging;

#endif // PAGING_H_INCLUDED
