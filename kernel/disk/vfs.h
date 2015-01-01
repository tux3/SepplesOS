#ifndef VFS_H
#define VFS_H

#include <lib/sizedArray.h>
#include <disk/partTable.h>

class Node
{
public:
    Node(const char* Name, Node* Parent, u8 Type=0);
    virtual ~Node(); ///< Does not delete its children nodes
    virtual char* getName() const; ///< Returns a C string representing the name of the node
    virtual u64 getSize() const; ///< Returns the size of the eventual data stored in the node
    virtual u8 getType() const; ///< Returns a filesystem-defined type of node; Usually file/folder/...
    virtual Node* getParent() const; ///< Returns the logical parent of this node
    virtual u64 read(u64 size, u8* dest); ///< Returns how much was successfully read
    virtual bool seekg(u64 pos); ///< Seeks to the given position of the input stream. Returns false on error.
    virtual SizedArray<Node*> lookup(); ///< Returns an array of child nodes
    virtual Node* createChild(char* name, Node* Parent, u8 type=0); ///< Creates a child node. Returns a nullptr on error
    virtual bool addChild(Node* other); ///< Returns false on error

protected:
    SizedArray<Node*> childs;
    Node* parent;
    char* name;
    u8 type;
};

struct Filesystem
{
    Filesystem(const char* Name=nullptr){name = Name;}
    virtual Node* mount(PartTable::Partition, Node* parent, char* destname); ///< Tries to mount the filesystem, returns a nullptr on error
    const char* name;
};

class VFS
{
public:
    VFS() = delete;
    static void init();
    static Node* getRoot();
    static void registerFilesystem(Filesystem* fs);
    static Node* mount(PartTable::Partition, Node* parent=&root, char* destname=nullptr); ///< Tries to mount the filesystem, returns a nullptr on error
    static Node* resolvePath(const char* path); ///< Returns a nullptr on error

private:
    static Node root;
    static SizedArray<Filesystem*> filesystems;
};

#endif // VFS_H
