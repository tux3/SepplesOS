#ifndef PARTTABLE_H
#define PARTTABLE_H

#include <std/types.h>
#include <lib/sizedArray.h>

namespace PartTable
{
    enum struct PartType
    {
        LINUX,
        FAT32,
        EMPTY,
        UNKNOWN,
    };

    struct Partition
    {
        u64 startLba;
        u64 sizeLba;
        u8 diskId;
        PartType type;
    };

    typedef SizedArray<Partition> PartTable;

    void mountAll(unsigned diskId);

    PartTable readTable(unsigned diskId);

    // MBR
    bool isMBR(unsigned diskId);
    PartTable readMBR(unsigned diskId);
}

#endif // PARTTABLE_H
