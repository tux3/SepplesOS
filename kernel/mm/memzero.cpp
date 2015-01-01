#include <mm/memzero.h>

extern "C" void memzero(u8* dst, u64 count)
{
    while (count--)
        *dst++ = 0;
}
