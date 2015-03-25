#ifndef PAGING_H
#define PAGING_H

#include <std/types.h>
#include <mm/memmap.h>

#define	PAGESIZE 	    0x1000
#define	PAGESIZE_BIG    0x400000

#define	VADDR_PAGE_DIR_OFFSET(addr)	((addr) & 0xFFC00000) >> 22
#define	VADDR_PAGE_TABLE_OFFSET(addr)	((addr) & 0x003FF000) >> 12
#define	VADDR_PAGE_OFFSET(addr)	(addr) & 0x00000FFF
#define PAGE(addr)		(addr) >> 12

#define PAGE_ATTR_PRESENT	0x00000001
#define PAGE_ATTR_WRITE	    0x00000002
#define PAGE_ATTR_USER		0x00000004
#define PAGE_ATTR_SIZE		0x00000080

struct Page
{
    u8 *vAddr;
    u8 *pAddr;
};

class Paging
{
public:
    Paging()=delete;
    static void init(long availableMem);
    static unsigned getMaxPhysPages();
    static void markPhysPageFree(unsigned page);
    static void markPhysPageFree(void* paddr);
    static void markPhysPageUsed(unsigned page);
    static void markPhysPageUsed(void* paddr);

    /// Returns the vaddr of the first available unmapped page, or nullptr on error
    /// May setup new paging structures if needed
    static u8* getUnmappedVAddr();

    /// Returns the paddr mapped to the given vaddr, or a nullptr on error
    static u8* getPAddr(u8* vaddr);
    /// Returns the paddr of the first free physical page, or a nullptr on error
    /// Will not mark the physical page as used
    static u8* getFreePhysPage();
    /// Maps this virtual page to the given paddr and return its vaddr
    /// If vaddr is a nullptr, the first free vaddr will be used
    /// If paddr is a nullptr, the first free paddr will be used
    /// The page and any missing paging structure will be mapped with the given flags set,
    /// and the flags PAGE_ATTR_PRESENT and PAGE_ATTR_WRITE are always set
    /// Returns a nullptr if the page is already mapped or if a paging structure was not present.
    static u8* mapPage(u8* vaddr=nullptr, u8* paddr=nullptr, int flags=0, u8* PML4=(u8*)PML4_ADDR);
    /// Unmaps a page, frees the associated physical page
    static void unmapPage(u8* vaddr);
    /// Creates a new page dir for a task, returns its paddr
    static u8* createPageDir();
    /// Destroys a page dir created with createPageDir
    static void destroyPageDir(u8* pml4);
    /// Maps a page with the given flags in the given page dir
    static void pageDirAddPage(u8* vaddr, u32 flags, u8* PML4);

    static void printStats(); ///< Outputs some info on paging stats

protected:
    static void tmpmap(void* pdst); ///< Maps the temporary map page to this page-aligned physical address.
    static void phyread(void* vdst, void* psrc, size_t size); ///< Reads data from a physical address. Fairly slow.
    static void phywrite(void* pdst, void* vsrc, size_t size); ///< Writes data to a physical address. Fairly slow.
    static void phyzero(void* pdst, size_t size); ///< Zeroes out a region of physical memory. Fairly slow.

private:
    static u8* physPageBitmap;
    static unsigned maxPhysPages;
};

#endif // PAGING_H
