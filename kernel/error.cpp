#include <error.h>
#include <mm/malloc.h>
#include <vga/vgatext.h>
#include <power.h>

extern "C" void error(const char* msg)
{
    VGAText::setCurStyle(VGAText::CUR_RED); // Error style
    if (!Malloc::isMallocReady() && VGAText::isLogEnabled())
    {
        VGAText::enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        VGAText::print("error: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    VGAText::print(msg);
    VGAText::setCurStyle(); // Remet style normal
}

extern "C" void fatalError(const char* msg)
{
    VGAText::setCurStyle(VGAText::CUR_BLACK, false, VGAText::CUR_RED); // Critical error style
    if (!Malloc::isMallocReady() && VGAText::isLogEnabled())
    {
        VGAText::enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        VGAText::print("fatalError: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    VGAText::print(msg);
    VGAText::setCurStyle(); // Remet style normal
    halt();
}

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
    fatalError("Call to pure virtual function. System hatled.\n");
}
