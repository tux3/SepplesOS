#include <mm/memcpy.h>

void* memcpy(void* _dest, const void* _src, size_t count)
{
    u8* dest = (u8*)_dest;
    const u8* src = (u8*)_src;

    u8* odest = dest;

	while (count--)
        *dest++ = *src++;

    return odest;
}
