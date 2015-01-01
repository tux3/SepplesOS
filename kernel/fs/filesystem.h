#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <std/types.h>
#include <lib/llistForward.h>

namespace IO
{
    namespace FS
    {
        // filesystem types enum
        enum {
                FSTYPE_UNKNOWN,
                FSTYPE_FREE 		= 0x0,
                FSTYPE_EBR			= 0x5,		// Extended Boot Record
                FSTYPE_NTFS			= 0x07,
                FSTYPE_FAT32_CHS	= 0x0b,
                FSTYPE_FAT32_LBA	= 0x0c,
                FSTYPE_SWAP			= 0x82,
                FSTYPE_LINUX_NATIVE = 0x83,
                FSTYPE_EXT2,
                FSTYPE_EXT3,
                FSTYPE_EXT4
            };

        struct partition
        {
            u8 bootable;		// 0 = no, 0x80 = bootable
            u8 sHead;			// Starting head
            u16 sSector:6;		// Starting sector
            u16 sCyl:10;		// Starting cylinder
            u8 fsId;			// Filesystem ID (type), as reported by detectFsType
            u8 eHead;			// Ending Head
            u16 eSector:6;		// Ending Sector
            u16 eCyl:10;		// Ending Cylinder
            u32 sLba;			// Starting LBA value
            u32 size;			// Total Sectors in partition
            u8 diskId;			// Id of the disk the partition is from
            u8 id;				// Id of the partition
            const char* getTypeName();
        } __attribute__ ((packed));

        class Node;
        class NodeVFS;
        class NodeEXT2;

        class FilesystemManager
        {
            public:
                FilesystemManager();
                void init();
                llist<struct partition> readPartitionTable(const int driveId); ///< reads the IBM PC partition table from the MBR
                int detectFsType(const struct partition& part);
                bool mount(const int partId); ///< Mount the given partition
                bool mount(const struct partition& part); ///< Mount the given partition
                Node* pathToNode(const char* path); ///< Returns the Node corresponding to this path
                const Node* getRootNode(); ///< Returns fRoot
                u64 getNodeIdCounter(bool increment=true); ///< returns the current value of the ID counter. Increment if needed.h);
                llist<struct partition> getPartList();

            protected:
                bool initVfs(); /// Create the VFS in meory

            private:
                NodeVFS* fRoot; // Root Node "/"
                llist<struct partition> partList;
                u64 nodeIdCounter; // Gives the new nodes their ID, incremented each time a Node is created.
        };
    } // FS
} // IO

extern IO::FS::FilesystemManager gFS;

#endif // FILE_H_INCLUDED
