#include <fs/node.h>
#include <fs/filesystem.h>
#include <process.h>
#include <error.h>
#include <lib/string.h>
#include <lib/llist.h>

namespace IO
{
namespace FS
{
    Node::Node(const char* name, const int type, Node* const parent)
    {
        if (strlen(name)+1 > FS_NODE_NAME_MAXLENGTH)
            fatalError("Node::Node(): Given name is longer (length:%d) than NODE_NAME_MAXLENGTH\n", strlen(name)+1);
        /// Initialise la Node
        strcpy(this->name, name);
        this->type = type;
        this->parent = parent;
        this->opens = 0;
        this->nodeId = gFS.getNodeIdCounter();
    }

    u64 Node::getSize() const
    {
        return 0;
    }

    Node* Node::getParent() const
    {
        return parent;
    }

    const char* Node::getName() const
    {
        return name;
    }

    u8 Node::getType() const
    {
        return type;
    }

    u64 Node::getNodeId() const
    {
        return nodeId;
    }

    void Node::open()
    {
        Process::OpenFile *desc = new Process::OpenFile;
        desc->file=this;
        desc->ptr=0;
        gTaskmgr.getCurrent()->openFiles << desc;
        opens++;
    }

    void Node::close()
    {
        // Get the process's file cursor (file must be open)
        bool found=false;
        Process *curProc = gTaskmgr.getCurrent();
        for (u32 i=0; i<curProc->openFiles.size(); i++)
        {
            if (curProc->openFiles[i]->file == this)
            {
                curProc->openFiles.removeAt(i);
                found=true;
                break;
            }
        }
        if (!found)
        {
            error("File is already closed.\n");
            return;
        }

        if (opens)
            opens--;
    }

    llist<Node*> Node::getChilds() const
    {
        llist<Node*> copy(childs); // Constructeur de copie généré par le compilo
        return copy; // Renvoie une copie de la liste mais avec les pointeurs sur les vrais enfants (c'est voulu)
    }

    bool Node::seek(const u64 pos) // Moves the (RW) file cursor/ptr to the given position. Equivalent of both seekp and seekg at the same time.
    {
        if (type != NODE_FILE)
        {
            error("Can't set node cursor pos, it isn't a file.\n");
            return false;
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
            error("Can't set node cursor pos, descriptor not opened.\n");
            return false;
        }
        else
        {
            *offset = pos;
            return true;
        }
    }

    u64 Node::read(char* buf, const u64 len) const
    {
        if(buf&&len){}
        return 0;
    }

    u64 Node::write(const char* buf, const u64 len)
    {
        if(buf&&len){}
        return 0;
    }
} // FS
} // IO
