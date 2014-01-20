#include <error.h>
#include <gdt.h>
#include <keyboard.h>
#include <idt.h>
#include <multiboot.h>
#include <paging.h>
#include <pic.h>
#include <screen.h>
#include <process.h>
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

    char* buf = new char[64];
    gTerm.setCurStyle(IO::VGAText::CUR_BLUE,true);
    gTerm.printf("%d partitions detected\n", partList.size());
    for(u8 i=0; i<partList.size(); i++) /// Ne pas oublier le cast A L'INTERIEUR DU CALCUL, c'est "(uint64)s_lba * 512", sinon int overflow pendant la multiplication et toIEC recoit n'importe quoi
        gTerm.printf("Partition %d : start:%s, size:%s, type:%s\n", partList[i].id, toIEC(buf,64,(u64)partList[i].sLba*512), toIEC(buf,32,(u64)partList[i].size*512), partList[i].getTypeName());
    delete buf; buf=nullptr;
    gTerm.setCurStyle();

    /// TEST: Mount the first ext2/ext3 partition and read hello.txt
       // Remove partitions we can't read
    for (int i=0; i<partList.size();)
        if (partList[i].fsId != FSTYPE_EXT2)
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

/// TODO: Bug: Maybe for some diskRead we should do sLba*512. Check the arguments and the old code.
/// TODO: Implement reading partitions tables other than the MBR.

/// TODO: Fix that paging bug before anything else. Save a snapshot of the code and try to find the source of the problem
/// => Corrupted chunk on 0x685000 with null size (heap end:0x686000) !
/// => Fix the bug in the Sepples_corruptedchunk copy, not in here ! Copy the fix when it works

/// => disableLog should only delete if malloc is ready
