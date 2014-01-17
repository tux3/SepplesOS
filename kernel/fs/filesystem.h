#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <std/types.h>
#include <lib/llist.h>

#define FS_ROOT_NODE_ID 0
#define FS_NODE_NAME_MAXLENGTH 256

namespace IO
{
    namespace FS
    {
        // filesystem types enum
        enum {
                FSTYPE_UNKNOWN,
                FSTYPE_FREE 		= 0x0,
                FSTYPE_EBR			= 0x5,		/// Extended Boot Record
                FSTYPE_NTFS			= 0x07,
                FSTYPE_FAT32_CHS	= 0x0b,
                FSTYPE_FAT32_LBA	= 0x0c,
                FSTYPE_SWAP			= 0x82,
                FSTYPE_EXT2			= 0x83,
                FSTYPE_EXT3			= 0x83
            };

        struct partition
        {
            u8 bootable;		// 0 = no, 0x80 = bootable
            u8 sHead;			// Starting head
            u16 sSector:6;		// Starting sector
            u16 sCyl:10;		// Starting cylinder
            u8 fsId;			// Partition Filesystem ID (type)
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

        /// Type of node (file, folder, ..)
        enum nodeType
        {
            NODE_FILE = 0,
            NODE_DIRECTORY = 1,
            NODE_DRIVE = 2
        };

        /// An abstract node of the VFS, all nodes inherit from this
        class Node
        {
            /// All of the derived classes must be Node's friend to be able to modify llist<Node*> childs
            friend class NodeVFS;
            friend class NodeEXT2;

            public:
                virtual u64 read(char* buf, const u64 len) const; // If len > file size, we read what we can
                virtual u64 write(const char* buf, const u64 len); // Expand the file if it's too small
                virtual bool seek(const u64 pos); // Moves the (RW) file cursor/ptr to the given position. Equivalent of both seekp and seekg at the same time.
                virtual bool createChild(const Node* child) = 0; // Creates the node Child on the VFS now
                virtual bool remove() = 0; // Deletes the node Child from the VFS now
                virtual llist<Node*> getChilds() const; // Returns a list of childs
                virtual u64 getSize() const; // Get size of data in node. Return 0 if not a file.
                Node* getParent() const;
                const char* getName() const;
                u8 getType() const;
                u64 getNodeId() const;
                void open(); // Open the file for the current process
                void close(); // Close the file for the current process

            protected:
                Node(const char* name, const int type, Node* const parent); // Abstract class
                char name[FS_NODE_NAME_MAXLENGTH]; // Name of the node
                u8 type; // nodeType
                Node* parent; // Parent
                llist<Node*> childs; // Childs
                u16 opens; // number of process that have opened the file
                u64 nodeId; // Unique ID of the node
        };

        /// A node of the VFS
        class NodeVFS : public Node
        {
            public:
                NodeVFS(const char* name, const int type, Node* const parent);
                u64 read(char* buf, const u64 len) const;
                u64 write(const char* buf, const u64 len);
                bool createChild(const char* name, const int type);
                bool createChild(const Node* child);
                bool remove();
                u64 getSize() const; // Get size of data in node. Return 0 if not a file.
            private:
                char* data; // Content of the node (if it's a file)
                u64 dataSize; // Size of the content
        };

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
    };
};

extern IO::FS::FilesystemManager gFS;

#endif // FILE_H_INCLUDED
