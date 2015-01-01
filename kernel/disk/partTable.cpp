#include <disk/partTable.h>
#include <disk/disk.h>
#include <disk/vfs.h>
#include <error.h>
#include <vga/vgatext.h>

using namespace ATA;

namespace PartTable
{

void mountAll(unsigned diskId)
{
    PartTable table = readTable(diskId);
    if (!table.size)
        error("PartTable::mountAll: Couldn't read the partition table of disk %d\n",diskId);

    for (u64 i=0; i<table.size; i++)
    {
        if (table.elems[i].type != PartType::EMPTY && table.elems[i].type != PartType::UNKNOWN)
        {
            //error("Mounting part %d start %p type %d\n",i,table.elems[i].startLba*512,table.elems[i].type);
            if (!VFS::mount(table.elems[i]))
            {
                //error("mountAll: Couldn't mount partition %d\n", i);
            }
        }
    }
}

PartTable readTable(unsigned diskId)
{
    if (isMBR(diskId))
        return readMBR(diskId);

    return PartTable();
}

// MBR

bool isMBR(unsigned diskId)
{
    u16 magic;
    diskRead(diskId, 0x1FE, sizeof(magic), &magic);

    return magic == 0xAA55;
}

PartTable readMBR(unsigned diskId)
{
    PartTable table;
    table.size = 4;
    table.elems = new Partition[4];

    u8 buf[16];
    for (int i=0; i<4; i++)
    {
        diskRead(diskId, (0x01BE + (16 * i)), 16, &buf);
        table.elems[i].diskId = diskId;
        table.elems[i].startLba = *(u32*)(buf+0x8);
        table.elems[i].sizeLba = *(u32*)(buf+0xC);

        if (buf[4] == 0)
            table.elems[i].type = PartType::EMPTY;
        else if (buf[4] == 0xB)
            table.elems[i].type = PartType::FAT32;
        else if (buf[4] == 0x83)
            table.elems[i].type = PartType::LINUX;
        else
        {
            table.elems[i].type = PartType::UNKNOWN;
            error("PartTable::readMBR: Unknown part type %p for part %d\n",buf[4],i);
        }
    }

    return table;
}

}
