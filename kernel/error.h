#ifndef _ERROR_H_
#define _ERROR_H_

#include <mm/malloc.h>
#include <vga/vgatext.h>
#include <power.h>

extern "C" void error(const char* msg);		// Print error
extern "C" void fatalError(const char* msg);	// Print error and halt

template <class T, class... Args> void error(const char* format, const T& value, const Args&... args)
{
    VGAText::setCurStyle(VGAText::CUR_RED); // Errors are red
    if (!Malloc::isMallocReady() && VGAText::isLogEnabled())
    {
        VGAText::enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        VGAText::print("error: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    VGAText::printf(format,value,args...);
    VGAText::setCurStyle();
}

template <class T, class... Args> void fatalError(const char* format, const T& value, const Args&... args)
{
    VGAText::setCurStyle(VGAText::CUR_BLACK, false, VGAText::CUR_RED); // Critical errors are black on red
    if (!Malloc::isMallocReady() && VGAText::isLogEnabled())
    {
        VGAText::enableLog(false); // The log would use new/kmalloc, and we need to display the error anyway
        VGAText::print("fatalError: malloc isn't ready. VGAText log forcibly disabled.\n");
    }
    VGAText::printf(format,value,args...);
    VGAText::setCurStyle();
    halt();
}

//extern "C" void __cxa_pure_virtual(); // If we managed to call a pure virtual function (congrats), we land here

#endif // _ERROR_H_
