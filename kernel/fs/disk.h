#ifndef DISK_H_INCLUDED
#define DISK_H_INCLUDED

#include <std/types.h>

namespace IO
{
    namespace FS
    {
        int blRead(int drive, int numblock, int count, char *buf);
        int blWrite(int drive, int numblock, int count, char *buf);

        // Cast to u64 before performing operations on offset !
        // For example diskRead(a,b*512,c,d) may overflow, diskRead(a,(u64)b*512,c,d) will not
        int diskRead(int drive, u64 offset, char *buf, u64 count);
        int diskReadBl(int drive, int blOffset, int byteOffset, char *buf, int blCount, int byteCount);
    }
}

#endif // DISK_H_INCLUDED
