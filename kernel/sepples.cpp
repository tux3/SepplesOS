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
    gPaging = Paging();
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
    gTerm.setCurStyle(IO::VGAText::CUR_BLUE,true); // printMbiMemmap will reset the style
    gTerm.printf("%d partitions detected\n", partList.size());
    for(u8 i=0; i<partList.size(); i++) /// Ne pas oublier le cast A L'INTERIEUR DU CALCUL, c'est "(uint64)s_lba * 512", sinon int overflow pendant la multiplication et toIEC recoit n'importe quoi
        gTerm.printf("Partition %d : start:%s, size:%s, type:%s\n", partList[i].id, toIEC(buf,64,(u64)partList[i].sLba*512), toIEC(buf,32,(u64)partList[i].size*512), partList[i].getTypeName());
    delete buf; buf=nullptr;
    gTerm.setCurStyle();

    gTerm.print("Kernel sleeping ...\n");
	while(1)
	{
        //asm("cli;")
		asm("hlt;");
	}
	return;
}
/// TODO: Have the different types of FS nodes in different .cpp files
/// TODO: Implement reading partitions tables other than the MBR.
