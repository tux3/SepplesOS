#include <disk/vfs.h>
#include <lib/string.h>
#include <error.h>

SizedArray<Filesystem*> VFS::filesystems;
Node VFS::root("",0);

Node::Node(const char *Name, Node* Parent, u8 Type)
{
    if (!Name)
        fatalError("Node::Node: name must not be a nullptr\n");
    if (!Parent)
        fatalError("Node::Node: parent must not be a nullptr\n");

    parent = Parent;
    u64 len = strlen(Name);
    name = new char[len+1];
    strcpy(name, Name);
    type = Type;
    childs = SizedArray<Node*>();
}

Node::~Node()
{
    delete[] name;
    for (u64 i=0; i<childs.size; i++)
        delete childs[i];
}

char* Node::getName() const
{
    return name;
}

u64 Node::getSize() const
{
    return 0;
}

u8 Node::getType() const
{
    return 0;
}

Node* Node::getParent() const
{
    return parent;
}

u64 Node::read(u64, u8*)
{
    return 0;
}

bool Node::seekg(u64 pos)
{
    if (!pos)
        return true;
    else
        return false;
}

SizedArray<Node*> Node::lookup()
{
    return childs;
}

Node* Node::createChild(char* Name, Node* Parent, u8 Type)
{
    Node* newNode = new Node(Name, Parent, Type);
    childs.append(newNode);
    return newNode;
}

bool Node::addChild(Node* other)
{
    childs.append(other);
    return true;
}

Node* Filesystem::mount(PartTable::Partition, Node*, char*)
{
    return nullptr;
}

void VFS::init()
{
    static_init(&root, "", &root, 0);
    filesystems = SizedArray<Filesystem*>();
}

void VFS::registerFilesystem(Filesystem *fs)
{
    filesystems.append(fs);
}

Node* VFS::mount(PartTable::Partition part, Node *parent, char* destname)
{
    char buf[16];
    if (!destname)
    {
        SizedArray<Node*> mountpoints = root.lookup();
        destname = utoa(mountpoints.size, buf, 10);
    }

    Node* node;
    for (u64 i=0; i<filesystems.size; ++i)
    {
        node = filesystems.elems[i]->mount(part, parent, destname);
        if (node)
        {
            root.addChild(node);
            return node;
        }
    }
    return nullptr;
}

Node* VFS::getRoot()
{
    return &root;
}

// Returns the node associated to path or nullptr on errors
// path is absolute if starting with /, else it's relative to the current process's working dir
Node* VFS::resolvePath(const char* path)
{
    Node* current;

    if (!path[0])
        return nullptr;

    // Absolute or relative
    if (path[0] == '/')
        current = &root;
    else
        //current = gTaskmgr.getCurrent()->pwd;
        current = &root; ///< Until we get the working directory in Process

    const char* beg_p = path;
    while (*beg_p == '/')
        beg_p++;
    const char* end_p = beg_p + 1;

    while (*beg_p != 0)
    {
        //logE9("Looking up %s ...",current->getName());
        SizedArray<Node*> curChilds = current->lookup();
        //logE9("done\n");
        if (!curChilds.size)
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
            for (size_t i=0; i<curChilds.size && !found; i++)
            {
                if (strcmp(curChilds[i]->getName(),name) == 0)
                {
                    found=true;
                    current = curChilds[i];
                }
            }
            if (!found)
            {
                delete[] name;
                return nullptr;
            }
        }

        beg_p = end_p;
        while (*beg_p == '/')
            beg_p++;
        end_p = beg_p + 1;

        delete[] name;
    }
    return current;
}
