#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include <lib/llistForward.h>
#include <std/types.h>

#define FS_ROOT_NODE_ID 0
#define FS_NODE_NAME_MAXLENGTH 256

namespace IO
{
namespace FS
{
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
} // FS
} // IO

#endif // NODE_H_INCLUDED
