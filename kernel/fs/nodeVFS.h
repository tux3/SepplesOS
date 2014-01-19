#ifndef NODEVFS_H_INCLUDED
#define NODEVFS_H_INCLUDED

#include <fs/node.h>

namespace IO
{
namespace FS
{
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
} // FS
} // IO

#endif // NODEVFS_H_INCLUDED
