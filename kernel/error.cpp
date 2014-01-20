#include <error.h>
#include <paging.h>
#include <screen.h>
#include <power.h>

using namespace IO;

extern "C" void error(const char* msg)
{
    gTerm.setCurStyle(VGAText::CUR_RED); // Error style
    if (!gPaging.isMallocReady() && gTerm.isLogEnabled())
    {
        gTerm.enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        gTerm.print("error: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    gTerm.print(msg);
    gTerm.setCurStyle(); // Remet style normal
}

extern "C" void fatalError(const char* msg)
{
    gTerm.setCurStyle(VGAText::CUR_BLACK, false, VGAText::CUR_RED); // Critical error style
    if (!gPaging.isMallocReady() && gTerm.isLogEnabled())
    {
        gTerm.enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        gTerm.print("fatalError: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    gTerm.print(msg);
    gTerm.setCurStyle(); // Remet style normal
    halt();
}

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
    fatalError("Call to pure virtual function. System hatled.\n");
}
