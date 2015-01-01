#ifndef _ERROR_H_
#define _ERROR_H_

#include <paging.h>
#include <screen.h>
#include <power.h>

#define BOCHSBREAK asm("xchg %bx, %bx");

extern "C" void error(const char* msg);		// Print error
extern "C" void fatalError(const char* msg);	// Print error and halt

template <class T, class... Args> void error(const char* format, const T& value, const Args&... args)
{
    gTerm.setCurStyle(IO::VGAText::CUR_RED); // Errors are red
    if (!gPaging.isMallocReady() && gTerm.isLogEnabled())
    {
        gTerm.enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        gTerm.print("error: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    gTerm.printf(format,value,args...);
    gTerm.setCurStyle();
}

template <class T, class... Args> void fatalError(const char* format, const T& value, const Args&... args)
{
    gTerm.setCurStyle(IO::VGAText::CUR_BLACK, false, IO::VGAText::CUR_RED); // Critical errors are black on red
    if (!gPaging.isMallocReady() && gTerm.isLogEnabled())
    {
        gTerm.enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        gTerm.print("fatalError: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    gTerm.printf(format,value,args...);
    gTerm.setCurStyle();
    halt();
}

//extern "C" void __cxa_pure_virtual(); // If we managed to call a pure virtual function (congrats), we land here

#endif // _ERROR_H_
