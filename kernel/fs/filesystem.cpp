#include <fs/filesystem.h>
#include <fs/disk.h>
#include <fs/ext2.h>
#include <fs/nodeVFS.h>
#include <lib/string.h>
#include <lib/humanReadable.h>
#include <paging.h>
#include <screen.h>
#include <error.h>
#include <lib/llist.h>

namespace IO
{
namespace FS
{
    const char* partition::getTypeName()
    {
        switch (fsId)
        {
        case FSTYPE_FREE:
            return "free";
        case FSTYPE_EBR:
            return "EBR";
        case FSTYPE_NTFS:
            return "ntfs";
        case FSTYPE_FAT32_CHS:
            return "FAT32-CHS";
        case FSTYPE_FAT32_LBA:
            return "FAT32-LBA";
        case FSTYPE_SWAP:
            return "linux swap";
        case FSTYPE_LINUX_NATIVE:
            return "linux (unknown)";
        case FSTYPE_EXT2:
            return "ext2";
        case FSTYPE_EXT3:
            return "ext3";
        case FSTYPE_EXT4:
            return "ext4";
        default:
            return "unknown";
        }
    }

    FilesystemManager::FilesystemManager()
    {
        nodeIdCounter=FS_ROOT_NODE_ID;
        fRoot = new NodeVFS((char*)"/", NODE_DRIVE, fRoot);
    }

    void FilesystemManager::init()
    {
        initVfs(); // Creates some basic VFS nodes (/dev/, ...)
        readPartitionTable(0); // Try to read the partition table and create partList
    }

    llist<struct partition> FilesystemManager::getPartList()
    {
        return partList;
    }

    // Try to read the partition table. Will fill partList
    llist<struct partition> FilesystemManager::readPartitionTable(const int driveId)
    {
        partList.empty();

        // Try to read the MBR
        u16 magic;
        diskRead(driveId, (0x1FE), (char *) &magic, 2);
        if (magic == 0xAA55)
        {
            for (int i=0; i<4; i++)
            {
                // Add the partition
                struct partition part;
                diskRead(driveId, (0x01BE + (16 * i)), (char *) &part, 16);
                part.diskId = driveId;
                part.id=i+1;
                part.fsId = detectFsType(part);
                //if (part.fsId != FSTYPE_UNKNOWN && part.fsId != FSTYPE_FREE && part.size != 0) // Add only if it's valid
                partList << part;
            }
        }
        else // Try to read the whole disk as a ext2/ext3 partition
        {
			struct partition part;
			part.bootable = 0;
			part.sLba=0;
			part.fsId=FSTYPE_EXT2;
			part.diskId=driveId;
			part.id=1;
			part.size=0;
			struct ext2Disk* hd = (struct ext2Disk *) gPaging.kmalloc(sizeof(struct ext2Disk));
            hd->device = 0;
            hd->part = part;
            struct ext2SuperBlock* sb = ext2ReadSb(hd, 0);
			if (sb->sMagic == 0xef53)
                partList << part;
            delete sb;
            delete hd;
        }

        return partList;
    }

    // Mount the given partition
    bool FilesystemManager::mount(const int partId)
    {
        // VFS ?
        if (partId == 0)
            return initVfs();

        for (u8 i=0; i<partList.size(); i++)
            if (partList[i].id == partId)
                return mount(partList[i]); // Call the other mount
        return false; // partId not found
    }

    bool FilesystemManager::mount(const struct partition& part) /// Mount the given partition
    {
        if (part.fsId == FSTYPE_EXT2 || part.fsId == FSTYPE_EXT3 || part.fsId == FSTYPE_EXT4)
        {
            // Find the first free number in /dev and create the corresponding node (e.g : /dev/1)
            u32 i=0;
            char buf1[]="/dev/";
            char buf2[16];

            do
            {
                strcpy(buf2,buf1);
                itoa(i,buf2+5,10); // Put the number after /dev/
                i++;
            } while (pathToNode(buf2));
            i--;

            // Read the ext2 partition
            struct ext2Disk* hd = ext2GetDiskInfo(part.diskId, part);
            if (!hd) // ext2GetDiskInfo() returns 0 on errors
            {
                error("mount: ext2GetDiskInfo failed\n");
                return false;
            }

            // Read the inode at the root of the partition
            struct ext2Inode* inode = ext2ReadInode(hd, EXT2_ROOT_INUM);

            // Create the drive node in /dev/
            itoa(i,buf2,10); // Put the number in buf
            ((NodeEXT2*)pathToNode("/dev/"))->createChild(buf2, NODE_DRIVE, hd, EXT2_ROOT_INUM, inode);

            return true;
        }
        else
        {
            error("mount: Unsupported filesystem\n");
            return false;
        }
    }

    // detect_fs_type() : Detect the type of the partition's filesystem (ext2, fat32, ...)
    int FilesystemManager::detectFsType(const struct partition& part)
    {
        if (part.fsId == 0) // Unused MBR entry (No partition)
            return FSTYPE_FREE;
        else if (part.fsId == 0x05 || part.fsId == 0x0f) // EBR (Extended Boot Record)
            return FSTYPE_EBR;
        else if (part.fsId == 0x07) // NTFS
            return FSTYPE_NTFS;
        else if (part.fsId == 0x0b) // FAT32 CHS
            return FSTYPE_FAT32_CHS;
        else if (part.fsId == 0x0c) // FAT32 LBA
            return FSTYPE_FAT32_LBA;
        else if (part.fsId == 0x82) // Linux SWAP
            return FSTYPE_FAT32_LBA;
        else if (part.fsId == 0x83) // Linux native (ext*)
        {
            int type = FSTYPE_LINUX_NATIVE;
            struct ext2SuperBlock *sb = new ext2SuperBlock;
            diskRead(part.diskId, (u64)part.sLba*512 + 1024, (char *) sb, sizeof(struct ext2SuperBlock));
            if (sb->sMagic == 0xef53)       type = FSTYPE_EXT2;
            else                            return FSTYPE_LINUX_NATIVE;

            if (sb->sFeatureCompat & 0x4)   type = FSTYPE_EXT3;

            delete sb;
            return type;
        }

        // Unknown filesystem (couldn't detect)
        return FSTYPE_UNKNOWN;
    }

    /// Prepare the VFS
    bool FilesystemManager::initVfs()
    {
        /// This function must be called only once !
        if (fRoot->getChilds().size() != 0)
        {
            error("FilesystemManager::initVfs(): VFS already initialized.\n");
            return false;
        }

        /// Set the root as the current folder of the current task
        gTaskmgr.getCurrent()->pwd = fRoot;

        /// Add some subfolders to the VFS
        fRoot->createChild((char*)"dev",NODE_DIRECTORY);
        fRoot->createChild((char*)"mod",NODE_DIRECTORY);

        return true;
    }

    // Returns the node associated to path or nullptr on errors
    // path is absolute if starting with /, else it's relative to the current process's working dir
    Node* FilesystemManager::pathToNode(const char* path)
    {
        Node* current;

        // Absolute or relative
        if (path[0] == '/')
            current = fRoot;
        else
            current = gTaskmgr.getCurrent()->pwd;

        const char* beg_p = path;
        while (*beg_p == '/')
            beg_p++;
        const char* end_p = beg_p + 1;

        while (*beg_p != 0)
        {
            if (current->getType() != NODE_DIRECTORY && current->getType() != NODE_DRIVE)
                return nullptr;

            // Find the name of the subdir
            while (*end_p != 0 && *end_p != '/')
                end_p++;
            char* name = new char[end_p - beg_p + 1];
            memcpy(name, beg_p, end_p - beg_p);
            name[end_p - beg_p] = 0;

            if (strcmp("..", name) == 0) 		// ".."
                current = current->getParent();
            else if (strcmp(".", name) == 0);	// "."
            else
            {
                bool found=false;
                for (u32 i=0; i<current->getChilds().size() && !found; i++)
                    if (strcmp(current->getChilds()[i]->getName(),name) == 0)
                    {
                        found=true;
                        current = current->getChilds()[i];
                    }
                if (!found)
                    return nullptr;
            }

            beg_p = end_p;
            while (*beg_p == '/')
                beg_p++;
            end_p = beg_p + 1;

            delete name;
        }
        return current;
    }

    const Node* FilesystemManager::getRootNode()
    {
        return fRoot;
    }

    u64 FilesystemManager::getNodeIdCounter(bool increment)
    {
        if (increment)
            return nodeIdCounter++;
        else
            return nodeIdCounter;
    }
}; // FS
}; // IO
