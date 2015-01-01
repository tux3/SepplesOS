#ifndef DISK_H_INCLUDED
#define DISK_H_INCLUDED

#include <std/types.h>

// Welcome to Hell, please enjoy your stay and the ATA madness
namespace ATA
{
void init();

bool readSector(int drive, int numblock, void* buf); ///< Reads one block (512B) from a disk
//int blWrite(int drive, int numblock, int count, char *buf);

int diskRead(int drive, u64 offset, u64 count,  void *dest);

}

#endif // DISK_H_INCLUDED
