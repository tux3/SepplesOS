#ifndef EXT2_H_INCLUDED
#define EXT2_H_INCLUDED

#include <std/types.h>
#include <disk/vfs.h>
#include <disk/partTable.h>
#include <error.h>

#define EXT2_READONLY 1 	// Lecture seule
#define EXT2_ROOT_INUM 2 	// Numero d'inode de la racine

namespace ext2
{

enum NodeEXT2Type
{
    NODE_UNKNOWN,
    NODE_DIRECTORY=1,
    NODE_FILE,
    NODE_ROOT,
};

struct FilesystemEXT2 : public Filesystem
{
    FilesystemEXT2(const char* name);
    virtual Node* mount(PartTable::Partition part, Node* parent, char* destname) final; ///< Tries to mount the filesystem, returns a nullptr on error
};

void initfs();

struct ext2SuperBlock
{
    u32 sInodesCount;	/* Total number of inodes */
    u32 sBlocksCount;	/* Total number of blocks */
    u32 sRBlocksCount;	/* Total number of blocks reserved for the super user */
    u32 sFreeBlocksCount;	/* Total number of free blocks */
    u32 sFreeInodesCount;	/* Total number of free inodes */
    u32 sFirstDataBlock;	/* Id of the block containing the superblock structure */
    u32 sLogBlockSize;	/* Used to compute block size = 1024 << s_log_block_size */
    u32 sLogFragSize;	/* Used to compute fragment size */
    u32 sBlocksPerGroup;	/* Total number of blocks per group */
    u32 sFragsPerGroup;	/* Total number of fragments per group */
    u32 sInodesPerGroup;	/* Total number of inodes per group */
    u32 sMtime;		/* Last time the file system was mounted */
    u32 sWtime;		/* Last write access to the file system */
    u16 sMntCount;	/* How many `mount' since the last was full verification */
    u16 sMaxMntCount;	/* Max count between mount */
    u16 sMagic;		/* = 0xEF53 */
    u16 sState;		/* File system state */
    u16 sErrors;		/* Behaviour when detecting errors */
    u16 sMinorRevLevel;	/* Minor revision level */
    u32 sLastcheck;	/* Last check */
    u32 sCheckinterval;	/* Max. time between checks */
    u32 sCreatorOs;	/* = 5 */
    u32 sRevLevel;	/* = 1, Revision level */
    u16 sDefResuid;	/* Default uid for reserved blocks */
    u16 sDefResgid;	/* Default gid for reserved blocks */
    u32 sFirstIno;	/* First inode useable for standard files */
    u16 sInodeSize;	/* Inode size */
    u16 sBlockGroupNr;	/* Block group hosting this superblock structure */
    u32 sFeatureCompat; // Optional features (ext3 flag is here
    u32 sFeatureIncompat; // Required features to read.
    u32 sFeatureRoCompat; // Required features to write. If you don't support them, mount as readonly.
    u8 sUuid[16];		// Volume id
    char sVolumeName[16];	// Volume name
    char sLastMounted[64];	// Path where the file system was last mounted
    u32 sAlgoBitmap;	// For compression
    u8 sPadding[820];
} __attribute__ ((packed));

struct ext2GroupDesc
{
    u32 bgBlockBitmap;	/* Id of the first block of the "block bitmap" */
    u32 bgInodeBitmap;	/* Id of the first block of the "inode bitmap" */
    u32 bgInodeTable;	/* Id of the first block of the "inode table" */
    u16 bgFreeBlocksCount;	/* Total number of free blocks */
    u16 bgFreeInodesCount;	/* Total number of free inodes */
    u16 bgUsedDirsCount;	/* Number of inodes allocated to directories */
    u16 bgPad;		/* Padding the structure on a 32bit boundary */
    u32 bgReserved[3];	/* Future implementation */
} __attribute__ ((packed));

struct Ext2Disk
{
    Ext2Disk()
    {
        sb = nullptr;
        gd = nullptr;
    }

    Ext2Disk(const Ext2Disk& other)
    {
        device = other.device;
        part = other.part;
        sb = other.sb ? new ext2SuperBlock(*other.sb) : nullptr;
        blocksize = other.blocksize;
        groups = other.groups;
        if (other.gd)
        {
            gd = new ext2GroupDesc[other.groups];
            memcpy(gd,other.gd,other.groups * sizeof(ext2GroupDesc));
        }
    }

    Ext2Disk& operator=(const Ext2Disk& other)
    {
        device = other.device;
        part = other.part;
        sb = other.sb ? new ext2SuperBlock(*other.sb) : nullptr;
        blocksize = other.blocksize;
        groups = other.groups;
        if (other.gd)
        {
            gd = new ext2GroupDesc[other.groups];
            memcpy(gd,other.gd,other.groups * sizeof(ext2GroupDesc));
        }
        return *this;
    }

    ~Ext2Disk()
    {
        delete sb;
        delete gd;
    }

    int device;
    struct PartTable::Partition part;
    struct ext2SuperBlock *sb;
    u32 blocksize;
    u16 groups;		/* Total number of groups */
    struct ext2GroupDesc *gd;
};

struct ext2Inode
{
    u16 iMode;		/* File type + access rights */
    u16 iUid;
    u32 iSize;
    u32 iAtime;
    u32 iCtime;
    u32 iMtime;
    u32 iDtime;
    u16 iGid;
    u16 iLinksCount;
    u32 iBlocks;		/* 512 bytes blocks ! */
    u32 iFlags;
    u32 iOsd1;

    /*
     * [0] -> [11] : block number (32 bits per block)
     * [12]        : indirect block number
     * [13]        : bi-indirect block number
     * [14]        : tri-indirect block number
     */
    u32 iBlock[15];

    u32 iGeneration;
    u32 iFileAcl;
    u32 iDirAcl;
    u32 iFAddr;
    u8 iOsd2[12];
} __attribute__ ((packed));

struct ext2DirectoryEntry
{
    u32 ext2Inode;		/* inode number or 0 (unused) */
    u16 recLen;		/* offset to the next dir. entry */
    u8 nameLen;		/* name length */
    u8 fileType;
    char name;
} __attribute__ ((packed));


/* super_block: s_errors */
#define	EXT2_ERRORS_CONTINUE	1
#define	EXT2_ERRORS_RO		2
#define	EXT2_ERRORS_PANIC	3
#define	EXT2_ERRORS_DEFAULT	1

/* inode: i_mode */
#define	EXT2_S_IFMT	0xF000	/* format mask  */
#define	EXT2_S_IFSOCK	0xC000	/* socket */
#define	EXT2_S_IFLNK	0xA000	/* symbolic link */
#define	EXT2_S_IFREG	0x8000	/* regular file */
#define	EXT2_S_IFBLK	0x6000	/* block device */
#define	EXT2_S_IFDIR	0x4000	/* directory */
#define	EXT2_S_IFCHR	0x2000	/* character device */
#define	EXT2_S_IFIFO	0x1000	/* fifo */

#define	EXT2_S_ISUID	0x0800	/* SUID */
#define	EXT2_S_ISGID	0x0400	/* SGID */
#define	EXT2_S_ISVTX	0x0200	/* sticky bit */
#define	EXT2_S_IRWXU	0x01C0	/* user access rights mask */
#define	EXT2_S_IRUSR	0x0100	/* read */
#define	EXT2_S_IWUSR	0x0080	/* write */
#define	EXT2_S_IXUSR	0x0040	/* execute */
#define	EXT2_S_IRWXG	0x0038	/* group access rights mask */
#define	EXT2_S_IRGRP	0x0020	/* read */
#define	EXT2_S_IWGRP	0x0010	/* write */
#define	EXT2_S_IXGRP	0x0008	/* execute */
#define	EXT2_S_IRWXO	0x0007	/* others access rights mask */
#define	EXT2_S_IROTH	0x0004	/* read */
#define	EXT2_S_IWOTH	0x0002	/* write */
#define	EXT2_S_IXOTH	0x0001	/* execute */

#define EXT2_INUM_ROOT	2

class NodeEXT2 : public Node
{
public:
    NodeEXT2(const NodeEXT2& other)=delete;
    NodeEXT2(const char* Name, Node* Parent, const Ext2Disk *hd, u32 ext2Inum, const struct ext2Inode* ext2Inode, u8 Type=0);
    ~NodeEXT2(); ///< Does not delete its children nodes
    NodeEXT2& operator=(const NodeEXT2& other)=delete;
    //virtual char* getName(); ///< Returns a C string representing the name of the node
    virtual u64 getSize() const; ///< Returns the size of the eventual data stored in the node
    //virtual u8 getType(); ///< Returns a filesystem-defined type of node; Usually file/folder/...
    virtual u64 read(u64 size, u8* dest); ///< Returns how much was successfully read
    //virtual bool seekg(u64 pos); ///< Seeks to the given position of the input stream. Returns false on error.
    virtual SizedArray<Node*> lookup(); ///< Returns an array of child nodes
    virtual Node* createChild(const char* name, Node* Parent, const struct Ext2Disk* hd, u32 ext2Inum, const struct ext2Inode* ext2Inode, u8 Type=0); ///< Creates a child node. Returns a nullptr on error
    virtual bool addChild(Node*);
private:
    u16 ext2PartId;
    const struct Ext2Disk* hd;
    u32 inum;
    const struct ext2Inode* inode;
};

Ext2Disk *ext2GetDiskInfo(const int device, const struct PartTable::Partition&);
struct ext2SuperBlock* ext2ReadSb(Ext2Disk *, u64 sPart);
struct ext2GroupDesc* ext2ReadGd(Ext2Disk *, u64 sPart);

struct ext2Inode* ext2ReadInode(const Ext2Disk *, const int);
u8* ext2ReadFile(const Ext2Disk *, const struct ext2Inode *);
bool isDirectory(const struct ext2Inode* inode);

}

#endif // EXT2_H_INCLUDED
