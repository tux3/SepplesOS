#ifndef _ERROR_H_
#define _ERROR_H_

#include <power.h>
#include <screen.h>

#define BOCHSBREAK asm("xchg %bx, %bx");

void error(const char* msg);		// Print error
void fatalError(const char* msg);	// Print error and halt

template <class T, class... Args> static void error(const char* format, const T& value, const Args&... args);
template <class T, class... Args> static void fatalError(const char* format, const T& value, const Args&... args);

template <class T, class... Args> void error(const char* format, const T& value, const Args&... args)
{
    gTerm.setCurStyle(IO::VGAText::CUR_RED); // Errors are red
    gTerm.printf(format,value,args...);
    gTerm.setCurStyle();
}

template <class T, class... Args> void fatalError(const char* format, const T& value, const Args&... args)
{
    gTerm.setCurStyle(IO::VGAText::CUR_BLACK, false, IO::VGAText::CUR_RED); // Critical errors are black on red
    gTerm.printf(format,value,args...);
    gTerm.setCurStyle();
    halt();
}

//extern "C" void __cxa_pure_virtual(); // If we managed to call a pure virtual function (congrats), we land here

#endif // _ERROR_H_
