#include <multiboot.h>
#include <vga/vgatext.h>
#include <lib/humanReadable.h>
#include <mm/paging.h>
#include <mm/malloc.h>
#include <mm/memzero.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/pic.h>
#include <arch/pinio.h>
#include <disk/disk.h>
#include <disk/partTable.h>
#include <disk/vfs.h>
#include <disk/ext2.h>
#include <proc/tss.h>
#include <proc/process.h>
#include <keyboard.h>
#include <error.h>
#include <debug.h>

using term = ::VGAText;

extern "C" int boot(int magic, multiboot_info* multibootHeader)
{
    term::clear();
    term::print("Sepples OS 2.0 booting\r\n");

    checkMultiboot(magic, multibootHeader);

    Paging::init(getAvailableMem(multibootHeader));
    reserveMbiMemmap(multibootHeader);

    IDT::init();
    TSS::init();
    GDT::init();
    PIC::init();
    PIC::setFrequency(25);
    Malloc::init();
    term::enableLog();

    ATA::init();
    VFS::init();
    ext2::initfs();
    PartTable::mountAll(0);

    TaskManager::init();

    i64 lastNProc = TaskManager::getNProc();
    Node* hello = VFS::resolvePath("/0/bin/hello.elf");
    if (hello)
    {
        term::print("Starting \"/0/bin/hello.elf\"\n");
        TaskManager::loadTask(hello);
    }
    else
        term::print("Failed to get path to hello.elf\n");

    Node* sh = VFS::resolvePath("/0/bin/sh.elf");
    if (sh)
    {
        term::print("Starting \"/0/bin/sh.elf\"\n");
        TaskManager::loadTask(sh);
    }
    else
        term::print("Failed to get path to sh.elf\n");

    sti;

    Keyboard::setLogInput(true);

    bool giveup=false;
    while (!giveup)
    {
        if (TaskManager::getNProc() != lastNProc || !lastNProc)
        {
            lastNProc = TaskManager::getNProc();
            if (!lastNProc)
            {
                term::setCurStyle(VGAText::CUR_YELLOW);
                term::print("All tasks are dead, starting a shell\n");
                term::setCurStyle();
                if (!TaskManager::loadTask("/0/bin/sh.elf"))
                {
                    error("Failed to load task, giving up\n");
                    giveup = true;
                }
            }
        }
    }

    term::printf("Kernel going to sleep...\n");
    for(;;)
    {
        asm("hlt");
    }
    return 0;
}
