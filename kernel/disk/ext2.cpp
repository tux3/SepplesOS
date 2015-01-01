#include <disk/ext2.h>
#include <disk/vfs.h>
#include <disk/disk.h>
#include <mm/malloc.h>
#include <vga/vgatext.h>
#include <error.h>
#include <debug.h>

using namespace ATA;

namespace ext2
{

static FilesystemEXT2 ext2fs(nullptr);

void initfs()
{
    static_init(&ext2fs, "ext2");
    VFS::registerFilesystem(&ext2fs);
}

FilesystemEXT2::FilesystemEXT2(const char *name)
    : Filesystem(name)
{
}

NodeEXT2::NodeEXT2(const char *Name, Node* Parent, const Ext2Disk *Hd, u32 Ext2Inum, const struct ext2Inode* Ext2Inode, u8 Type)
    : Node::Node(Name, Parent, Type)
{
    hd = new Ext2Disk(*Hd);
    inum = Ext2Inum;
    inode = new struct ext2Inode(*Ext2Inode);
}

NodeEXT2::~NodeEXT2()
{
    delete hd;
    delete inode;
}

Node* FilesystemEXT2::mount(PartTable::Partition part, Node* parent, char* destname)
{
    Ext2Disk* hd = ext2GetDiskInfo(part.diskId, part);
    if (!hd)
    {
        logE9("FilesystemEXT2::mount: ext2GetDiskInfo failed\n");
        return nullptr;
    }

    // Read the inode at the root of the partition
    struct ext2Inode* inode = ext2ReadInode(hd, EXT2_ROOT_INUM);

    NodeEXT2* newnode = new NodeEXT2(destname, parent, hd, EXT2_ROOT_INUM, inode, NODE_ROOT);
    delete hd;
    delete inode;
    return newnode;
}

bool NodeEXT2::addChild(Node*)
{
    return false;
}

// Initialise la structure decrivant le disque logique.
// Offset correspond au debut de la partition.
struct Ext2Disk *ext2GetDiskInfo(const int device, const PartTable::Partition &part)
{
    int i, j;
    Ext2Disk* hd = new Ext2Disk;
    hd->device = device;
    hd->part = part;
    hd->sb = ext2ReadSb(hd, (u64)part.startLba * 512);
    hd->blocksize = 1024 << hd->sb->sLogBlockSize;

    if (hd->sb->sMagic != 0xef53)
    {
        logE9("ext2GetDiskInfo: Partition not detected as ext2/ext3, signature is : %x\n", hd->sb->sMagic);
        return nullptr;
    }

    if (hd->sb->sBlocksPerGroup == 0 || hd->sb->sInodesPerGroup == 0) // Eviter la DIV0 Fault
    {
        logE9("ext2GetDiskInfo : Partiton error : 0 blocks or inodes per group\n");
        return nullptr;
    }

    i = (hd->sb->sBlocksCount / hd->sb->sBlocksPerGroup) +
        ((hd->sb->sBlocksCount % hd->sb->sBlocksPerGroup) ? 1 : 0);
    j = (hd->sb->sInodesCount / hd->sb->sInodesPerGroup) +
        ((hd->sb->sInodesCount % hd->sb->sInodesPerGroup) ? 1 : 0);
    hd->groups = (i > j) ? i : j;

    hd->gd = ext2ReadGd(hd, (u64)part.startLba * 512);

    // DEBUG: affiche des infos sur ce qui a été lu sur la patition

    if (hd->sb->sFeatureIncompat & 0x1)
    {
        logE9("ext2GetDiskInfo: Compression required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x4)
    {
        logE9("ext2GetDiskInfo: Journal replay recovery required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x8)
    {
        logE9("ext2GetDiskInfo: Journal device required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x10)
    {
        logE9("ext2GetDiskInfo: Meta block groups required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x40)
    {
        logE9("ext2GetDiskInfo: Extents required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x80)
    {
        logE9("ext2GetDiskInfo: 64bit FS size required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x100)
    {
        logE9("ext2GetDiskInfo: Multiple mount protection required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x200)
    {
        logE9("ext2GetDiskInfo: Flexible block groups required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x400)
    {
        logE9("ext2GetDiskInfo: Extended attributes required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x1000)
    {
        logE9("ext2GetDiskInfo: Data in directory entry required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x2000)
    {
        logE9("ext2GetDiskInfo: Meta checksum required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x4000)
    {
        logE9("ext2GetDiskInfo: Large directory (>2GB) required, but not supported\n");
        return nullptr;
    }
    else if (hd->sb->sFeatureIncompat & 0x8000)
    {
        logE9("ext2GetDiskInfo: Data in inode required, but not supported\n");
        return nullptr;
    }

    logE9("Total number of inodes : %d\n", hd->sb->sInodesPerGroup);
    logE9("Total number of blocks : %d\n", hd->sb->sBlocksPerGroup);
    logE9("Total number of inodes per group : %d\n", hd->sb->sInodesPerGroup);
    logE9("Total number of blocks per group : %d\n", hd->sb->sBlocksPerGroup);
    logE9("First block : %d\n", hd->sb->sFirstDataBlock);
    logE9("First ext2Inode : %d\n", hd->sb->sFirstIno);
    logE9("Block size : %d\n", 1024 << hd->sb->sLogBlockSize);
    logE9("Inode size : %d\n", hd->sb->sInodeSize);
    logE9("Minor revision : %d\n", hd->sb->sMinorRevLevel);
    logE9("Major revision : %d\n", hd->sb->sRevLevel);
    logE9("Ext2 signature (must be 0xef53) : 0x%x\n", hd->sb->sMagic);
    return hd;
}

ext2SuperBlock *ext2ReadSb(Ext2Disk *hd, u64 sPart)
{
    ext2SuperBlock* sb = new struct ext2SuperBlock;
    diskRead(hd->device, (u64)sPart + 1024, sizeof(struct ext2SuperBlock), sb);
    return sb;
}

ext2GroupDesc *ext2ReadGd(Ext2Disk *hd, u64 sPart)
{
    u64 offset = (hd->blocksize == 1024) ? 2048 : hd->blocksize; // locate block
    ext2GroupDesc *gd = new ext2GroupDesc[hd->groups];

    diskRead(hd->device, sPart+offset, hd->groups*sizeof(ext2GroupDesc), gd);
    return gd;
}

// Retourne la structure d'ext2Inode a partir de son numero
struct ext2Inode *ext2ReadInode(const struct Ext2Disk *hd, const int iNum)
{
    i64 grNum, index, offset;
    struct ext2Inode* ext2Inode = new struct ext2Inode;

    if (hd->sb->sInodesPerGroup == 0) // Eviter la DIV0 Fault
    {
        error("ext2ReadInode : Partiton error : 0 inodes per group\n");
        return 0;
    }

    // groupe qui contient l'ext2Inode
    grNum = (iNum - 1) / hd->sb->sInodesPerGroup;

    // index de l'ext2Inode dans le groupe
    index = (iNum - 1) % hd->sb->sInodesPerGroup;

    // offset de l'ext2Inode sur le disk depuis le debut de la partition
    offset = hd->gd[grNum].bgInodeTable * hd->blocksize + index * hd->sb->sInodeSize;

    // Read
    u64 readPos = ((u64)hd->part.startLba * 512) + offset;
    diskRead(hd->device, readPos, sizeof(struct ext2Inode), ext2Inode);
    return ext2Inode;
}

u8* ext2ReadFile(const Ext2Disk *hd, const struct ext2Inode *ext2Inode)
{
    u8* buf = new u8[hd->blocksize];
    int* p = (int*) new u8[hd->blocksize];
    int* pp = (int*) new u8[hd->blocksize];
    int* ppp = (int*) new u8[hd->blocksize];

    u64 size = ext2Inode->iSize; // Taille totale du fichier

    u8* mmapHead = new u8[size];
    u8* mmapBase = mmapHead;

    u64 baseOff = (u64)hd->part.startLba * 512;

    // direct block number
    for (u64 i = 0; i < 12 && ext2Inode->iBlock[i]; i++)
    {
        u64 readSize = baseOff + ext2Inode->iBlock[i] * hd->blocksize;
        diskRead(hd->device, readSize, hd->blocksize, buf);
        u64 n = ((size > hd->blocksize) ? hd->blocksize : size);
        memcpy(mmapHead, buf, n);
        mmapHead += n;
        size -= n;
    }

    // indirect block number
    if (ext2Inode->iBlock[12])
    {
        diskRead(hd->device, baseOff + ext2Inode->iBlock[12] * (u64)hd->blocksize, hd->blocksize, p);
        for (u64 i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            diskRead(hd->device, baseOff + p[i] * hd->blocksize, hd->blocksize, buf);

            u64 n = ((size > hd->blocksize) ? hd->blocksize : size);
            memcpy(mmapHead, buf, n);
            mmapHead += n;
            size -= n;
        }
    }

    // bi-indirect block number
    if (ext2Inode->iBlock[13])
    {
        diskRead(hd->device, baseOff + ext2Inode->iBlock[13]*hd->blocksize, hd->blocksize, p);

        for (u64 i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            diskRead(hd->device, baseOff + p[i]*hd->blocksize, hd->blocksize, pp);

            for (u64 j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                diskRead(hd->device, baseOff + pp[j]*hd->blocksize, hd->blocksize, buf);

                u64 n = ((size > hd->blocksize) ? hd-> blocksize : size);
                memcpy(mmapHead, buf, n);
                mmapHead += n;
                size -= n;
            }
        }
    }

    // tri-indirect block number
    if (ext2Inode->iBlock[14]) {
        diskRead(hd->device, ((u64)hd->part.startLba * 512) + ext2Inode->iBlock[14] * hd->blocksize, hd->blocksize, p);

        for (u64 i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            diskRead(hd->device, ((u64)hd->part.startLba * 512) + p[i] * hd->blocksize, hd->blocksize, pp);

            for (u64 j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                diskRead(hd->device, ((u64)hd->part.startLba * 512) + pp[j] * hd->blocksize, hd->blocksize, ppp);

                for (u64 k = 0; k < hd->blocksize / 4 && ppp[k]; k++) {
                    diskRead(hd->device, ((u64)hd->part.startLba * 512) + ppp[k] * hd->blocksize, hd->blocksize, buf);

                    u64 n = ((size > hd->blocksize) ? hd->blocksize : size);
                    memcpy(mmapHead, buf, n);
                    mmapHead += n;
                    size -= n;
                }
            }
        }
    }

    delete buf;
    delete p;
    delete pp;
    delete ppp;

    return mmapBase;
}

// is_directory(): renvoit si le fichier en argument est un repertoire
bool isDirectory(const struct ext2Inode* ext2Inode)
{
//				if (!node->ext2Inode)
//					node->ext2Inode = ext2_read_inode(fp->disk, fp->inum);

    return (ext2Inode->iMode & (EXT2_S_IFDIR)) ? true : false;
}


/// Met a jour la liste childs en lisant depuis la partition
SizedArray<Node*> NodeEXT2::lookup()
{
    /// TODO: Ask malloc to check if our blocks (inode, hd, ...) are still valid

    for (u64 i=0; i<childs.size; i++)
        delete childs[i];
    childs.empty();

    if (!isDirectory(inode))
    {
        error("NodeEXT2::lookup() : %s is not a directory !\n", name);
        return childs;
    }

//				/// Met a jour l'ext2Inode depuis le disque
//				ext2Inode = ext2ReadInode(hd, inum);

    /// Lis le contenu du repertoire (donc la liste des fichiers contenus)
    u8* mmap = ext2ReadFile(hd, inode);
    if (!mmap)
        fatalError("ext2ReadFile failed\n");

    /// Lecture de chaque entrée du repertoire et création de la node correspondante
    char *filename;
    u32 dsize = inode->iSize;			// taille du repertoire
    struct ext2DirectoryEntry* dentry = (struct ext2DirectoryEntry*) mmap;

    while (dentry->ext2Inode && dsize)
    {
        filename =  new char[dentry->nameLen + 1];
        memcpy(filename, &dentry->name, dentry->nameLen);
        filename[dentry->nameLen] = 0;

        //VGAText::printf("NodeEXT2::lookup recLen 0x%x\n", dentry->recLen);
        //VGAText::printf("NodeEXT2::lookup: reading inode %d: %s\n", dentry->ext2Inode, filename);

        /// Crée chaque node et les ajoutes a childs
        if (strcmp(".", filename) && strcmp("..", filename))
        {
            struct ext2Inode* childInode = ext2ReadInode(hd, dentry->ext2Inode);
            int childType;
            if (isDirectory(childInode))
                childType = NODE_DIRECTORY;
            else
                childType = NODE_FILE;

            /// Crée la node enfant en specifiant bien de ne pas l'écrire sur le disque, puisqu'on est justement en train de lire la structure depuis le disque
            if (!createChild(filename, this, hd, dentry->ext2Inode, childInode, childType))
                fatalError("NodeEXT2::lookup: Unable to create child node %d\n", filename);

            delete childInode;
        }

        delete filename;

        // Lecture de l'entree suivante
        dsize -= dentry->recLen;
        dentry = (struct ext2DirectoryEntry *) ((char *) dentry + dentry->recLen);
    }

    delete mmap;
    return childs;
}

Node* NodeEXT2::createChild(const char* Name, Node* Parent, const Ext2Disk* Hd, u32 Ext2Inum, const struct ext2Inode* Ext2Inode, u8 Type)
{
    NodeEXT2* child = new NodeEXT2(Name, Parent, Hd, Ext2Inum, Ext2Inode, Type);
    childs.append(child);
    return child;
}

u64 NodeEXT2::getSize() const
{
    if (type==NODE_FILE)
        return inode->iSize;
    else
        return 0;
}

u64 NodeEXT2::read(u64 len, u8* buf)
{
    if (type != NODE_FILE)
    {
        error("NodeEXT2::read: Can't read node, it isn't a file.\n");
        return 0;
    }

    // Get the process's file cursor (file must be open)
    u64 offset=0;
    /*
    u64* offset=nullptr;
    bool found=false;
    Process *curProc = gTaskmgr.getCurrent();
    for (u32 i=0; i<curProc->openFiles.size(); i++)
    {
        if (curProc->openFiles[i]->file == this)
        {
            offset = &curProc->openFiles[i]->ptr;
            found=true;
        }
    }
    if (!found)
    {
        error("NodeEXT2::read: Can't read node, descriptor not opened.\n");
        return 0;
    }
    */

    // Read what we can
    const u8* const data = ext2ReadFile(hd, inode);
    const u64 dataSize = inode->iSize;
    if (offset > dataSize)
        return 0;
    else if (len + offset > dataSize)
    {
        memcpy(buf, data+offset, (u64)dataSize-offset);
        delete[] data;
        offset = dataSize;
        return dataSize-offset;
    }
    else
    {
        memcpy(buf, data+offset, len);
        delete[] data;
        offset += len;
        return len;
    }
}

}
