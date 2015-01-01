#include <error.h>
#include <gdt.h>
#include <keyboard.h>
#include <idt.h>
#include <multiboot.h>
#include <paging.h>
#include <pic.h>
#include <screen.h>
#include <process.h>
#include <debug.h>
#include <fs/filesystem.h>
#include <lib/humanReadable.h>
#include <lib/llist.h>

// Singletons
IO::VGAText gTerm;
IO::Keyboard gKbd;
Paging gPaging;
TaskManager gTaskmgr;
IO::FS::FilesystemManager gFS;

// Entry point of the kernel, called from boot.asm
extern "C" void boot(u32 mbmagic, struct multiboot_info *mbi)
{
    using namespace IO;
    using namespace IO::FS;

    // Prepare kernel and print some info
    gPaging = Paging();
    gTerm = VGAText();
    gKbd = Keyboard();
	gTerm.clear();
	gTerm.setCurStyle(VGAText::CUR_GREEN);
	gTerm.print("Sepples OS is booting ...\n");
    checkMbi(mbmagic, mbi); // Will reset gTerm's style. Will halt if the bootloader isn't multiboot-compliant

    // Set up and load descriptor tables
    IDT idtMgr;
    idtMgr.init();
    GDT gdtMgr;
    gdtMgr.init();
    gTerm.print("IDT and GDT loaded\n");

    // Set up the PIC (Programmable Interrupt Controller)
    PIC picMgr;
    picMgr.init();
    picMgr.setFrequency(50);
    gTerm.print("PIC set\n");

    // Enable paging and logging
    gPaging.init(mbi->high_mem);
    bool result = reserveMbiMemmap(mbi);
    gTerm.enableLog(true);
    if (result)
    {
        gTerm.setCurStyle(IO::VGAText::CUR_BLUE,true); // printMbiMemmap will reset the style
        gTerm.print("Memory map :\n");
        printMbiMemmap(mbi);
        gTerm.setCurStyle();
    }
    gTerm.print("Paging enabled\n");

    // Init taskmanager, enable interrupts
    gTaskmgr = TaskManager();
    gTaskmgr.init();
    sti;
    gTerm.showCursor(); // Interrupts are enabled, if we can write, we'll want to see the cursor
    gTerm.print("Task manager started, interrupts enabled\n");

    // Init VFS
    gFS = FilesystemManager();
    gFS.init();
    gTerm.print("VFS loaded\n");
    llist<struct partition> partList = gFS.getPartList();

    // List partitions
    char* buf = new char[64];
    gTerm.setCurStyle(IO::VGAText::CUR_BLUE,true);
    gTerm.printf("%d partitions detected\n", partList.size());
    for(u8 i=0; i<partList.size(); i++) /// Ne pas oublier le cast A L'INTERIEUR DU CALCUL, c'est "(uint64)s_lba * 512", sinon int overflow pendant la multiplication et toIEC recoit n'importe quoi
        gTerm.printf("Partition %d : start:%s, size:%s, type:%s\n", partList[i].id, toIEC(buf,64,(u64)partList[i].sLba*512), toIEC(buf,32,(u64)partList[i].size*512), partList[i].getTypeName());
    delete[] buf; buf=nullptr;
    gTerm.setCurStyle();

    /// TEST: Mount the first ext2/ext3 partition and read /etc/motd
    // Remove partitions we can't read
    for (unsigned i=0; i<partList.size();)
        if (partList[i].fsId != FSTYPE_EXT2 && partList[i].fsId != FSTYPE_EXT3 && partList[i].fsId != FSTYPE_EXT4)
            partList.removeAt(i);
        else
            i++;
    if (partList.size() && gFS.mount(partList[0]))
    {
        gTerm.setCurStyle(IO::VGAText::CUR_GREEN,true);
        gTerm.print("Partition mounted, reading the MOTD : \n");
        gTerm.setCurStyle(IO::VGAText::CUR_CYAN,true);
        NodeEXT2* node = (NodeEXT2*)gFS.pathToNode("/dev/0/etc/motd");
        if (node != nullptr)
        {
            u64 size = node->getSize();
            if (size)
            {
                char* buf = new char[size+1];
                buf[size]='\0';
                node->open();
                node->read(buf, size);
                node->close();
                gTerm.print(buf);
            }
        }
        gTerm.setCurStyle();
    }

    gTerm.print("Kernel sleeping ...\n");
	while(1)
	{
        //asm("cli;")
		asm("hlt;");
	}
	return;
}


/**
TODO: Implement multitasking
TODO: Implement reading partitions tables other than the MBR.
TODO: Have FilesystemManager::mount call NodeEXT2::mount instead of doing it himself
TODO: Replace the "Page Heap" with page-aligned kmalloc chunks of PAGESIZE bits.
        The get/releasePageFromHeap functions should make a page-aligned alloc with the kmalloc chunks.
        They just need to either insert a padding chunk and a page-aligned chunk
        or split a free block that crosses a page boundary and ends at or after the next page boundary
        Basically, if a block crosses two page-aligned boundaries, then we can use it

**/
