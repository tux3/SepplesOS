#ifndef DISK_H_INCLUDED
#define DISK_H_INCLUDED

#include <std/types.h>

namespace IO
{
    namespace FS
    {
        int blRead(int drive, int numblock, int count, char *buf);	// drive, block, count, buf
        int blWrite(int drive, int numblock, int count, char *buf); // drive, block, count, buf

        int diskRead(int drive, int offset, char *buf, int count);	// drive, offset, buf, count
        int diskReadBl(int drive, int blOffset, int byteOffset, char *buf, int blCount, int byteCount);
    }
}

#endif // DISK_H_INCLUDED
