#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <std/types.h>

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

void setBreakpoint(u8 regNum, u32 addr, u8 len, u8 mode); ///< Sets the BP and enables it
void enableBreakpoint(u8 regNum); ///< Enables the breakpoint number regNum
void disableBreakpoint(u8 regNum); ///< Disables the breakpoint number regNum
void clearBreakpointDR6(); ///< Clears the DR6 status register

#endif // _DEBUG_H_
