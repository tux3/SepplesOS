#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <std/types.h>
#include <lib/string.h>

#define BOCHSBREAK asm volatile("xchg %bx, %bx");

/// Log messages on the host OS with the E9 hack
void logE9(char c);
void logE9(const char* msg);
template <class T, class... Args> void logE9(const char* format, const T& value, const Args&... args)
{
    while (*format)
    {
        if (*format == '%')
        {
            ++format;
            if (*format == 'd')
            {
                char buf[32];
                itoa((i64)value, buf, 10);
                logE9(buf);
            }
            else if(*format == 'x')
            {
                char buf[32];
                itoa((i64)value, buf, 16);
                logE9(buf);
            }
            else if(*format == 'p')
            {
                char buf[32];
                utoa((u64)value, buf, 16);
                logE9('0'); logE9('x');
                logE9(buf);
            }
            else if(*format == 'b')
            {
                char buf[32];
                itoa((i64)value, buf, 2);
                logE9(buf);
            }
            else if(*format == 'c')
                logE9((long)value);
            else if(*format == 's')
                logE9((const char*)(long)value);

            // Si format inconu, continuing happily, on ne peut pas afficher d'erreur à l'intérieur d'un printf !
            logE9(++format, args...); // Récursion sans revenir (car return après)
            return;
        }
        else
            logE9(*format); // Fin de récursion
        ++format;
    }
    // Erreur : trop d'arguments par rapport au format, mais on ne peut pas afficher d'erreur à l'intérieur d'un printf !
    return;
}

/**

regNum:
You can only have 4 hardware breakpoints at the same time, in registers DR0 - DR3

mode:
00 - Break on instructions only
01 - Break on data writes only
10 - Undefined
11 - Break on data reads or writes but not instruction fetches.

len :
00 - 1-byte length.
01 - 2-byte length.
10 - Undefined (or 8 byte length for some models).
11 - 4-byte length.

**/

enum BreakpointMode
{
    BP_INST=0,
    BP_DATAW=1,
    BP_UNDEFMODE=2,
    BP_DATARW=3,
};

enum BreakpointLength
{
    BP_BYTE=0,
    BP_WORD=1, ///< As in 2 bytes
    BP_UNDEFLEN=2,
    BP_DWORD=3, ///< As in 4 bytes
};

void setBreakpoint(u8 regNum, u64 addr, BreakpointLength len, BreakpointMode mode); ///< Sets the BP and enables it
void enableBreakpoint(u8 regNum); ///< Enables the breakpoint number regNum
void disableBreakpoint(u8 regNum); ///< Disables the breakpoint number regNum
void clearBreakpointDR6(); ///< Clears the DR6 status register

#endif // _DEBUG_H_
