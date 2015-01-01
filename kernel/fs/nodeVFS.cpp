#include <fs/nodeVFS.h>
#include <process.h>
#include <error.h>

namespace IO
{
namespace FS
{
    /// NodeVFS
    NodeVFS::NodeVFS(const char* name, const int type, Node* const parent) :
    Node::Node(name, type, parent)
    {
        /// Creer la node en mémoire
        if (type == NODE_FILE) // Un fichier, donc données brute
        {
            data=0;
            dataSize=0;
        }
        else
            data=0;
    }

    bool NodeVFS::createChild(const Node* child)
    {
        NodeVFS* childCopy = new NodeVFS(child->getName(), child->getType(), this);
        childs << childCopy;
        return true; // Tant qu'on a de la RAM libre, ca ne peut que reussir (si on en a plus globalPaging fera fatalError)
    }

    bool NodeVFS::createChild(const char* name, const int type)
    {
        NodeVFS* child = new NodeVFS(name, type, this);
        childs << child;
        return true; // Tant qu'on a de la RAM libre, ca ne peut que reussir (si on en a plus globalPaging fera fatalError)
    }

    bool NodeVFS::remove()
    {
        if (childs.size()) // On ne peut pas se supprimmer si on a des enfants
        {
            error("NodeVFS::remove() Trying to remove a node with childs (childs:%d)", childs.size());
            return false;
        }
        else if (opens)
        {
            error("NodeVFS::remove() Trying to remove a currently used node (opens:%d)", opens);
            return false;
        }

        for(u32 i=0; i<parent->childs.size(); i++)
            if (parent->childs[i]->name == this->name)
                parent->childs.removeAt(i);
        return true;
    }

    u64 NodeVFS::read(char* buf, const u64 len) const
    {
        if (type != NODE_FILE)
        {
            error("Can't read node, it isn't a file.\n");
            return 0;
        }

        // Get the process's file cursor (file must be open)
        u64* offset;
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
            error("Can't read node, descriptor not opened.\n");
            return 0;
        }

        // Read what we can
        if (*offset > dataSize)
            return 0;
        else if (len + *offset > dataSize)
        {
            memcpy(buf, data+*offset, (u64)dataSize-*offset);
            *offset = dataSize;
            return dataSize-*offset;
        }
        else
        {
            memcpy(buf, data+*offset, len);
            *offset += len;
            return len;
        }
    }

    u64 NodeVFS::write(const char* buf, const u64 len)
    {

        if (type != NODE_FILE)
        {
            error("Can't write node, it isn't a file.\n");
            return 0;
        }

        // Get the process's file cursor (file must be open)
        u64* offset;
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
            error("Can't write node, descriptor not opened.\n");
            return 0;
        }

        // Write what we can
        if (*offset > dataSize)
        {
            error("Can't write node, offset > descriptor size\n");
            return 0;
        }
        else if (len + *offset > dataSize)
        {
            char *newData = (char*) gPaging.kmalloc(sizeof(char) * (len+*offset));
            memcpy(newData, data, dataSize);
            memcpy(newData+*offset, buf, (u64)len);
            gPaging.kfree(data);
            data = newData;
            dataSize=len+*offset;
            *offset = dataSize;
            return len;
        }
        else
        {
            memcpy(data+*offset, buf, len);
            *offset += len;
            return len;
        }
    }

    u64 NodeVFS::getSize() const
    {
        if (type==NODE_FILE)
            return dataSize;
        else
            return 0;
    }
} // FS
} // IO
