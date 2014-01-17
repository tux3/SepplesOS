#include <fs/filesystem.h>
#include <fs/disk.h>
#include <lib/string.h>
#include <lib/humanReadable.h>
#include <paging.h>
#include <process.h>
#include <screen.h>

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
                return "FAT32-chs";
            case FSTYPE_FAT32_LBA:
                return "FAT32-lba";
            case FSTYPE_SWAP:
                return "linux swap";
            case FSTYPE_EXT2:
                return "ext2/ext3";
            default:
                return "unknown";
            }
        }

        FilesystemManager::FilesystemManager()
        {
            nodeIdCounter=FS_ROOT_NODE_ID;
            fRoot = new NodeVFS((char*)"/", NODE_DRIVE, fRoot); // Initialise fRoot
        }

        void FilesystemManager::init()
        {
            initVfs(); // Initialise le VFS (creer les nodes de base)
            readPartitionTable(0); // Initialise partList en lisant la table des partition du premier disque dur
        }

        llist<struct partition> FilesystemManager::getPartList()
        {
            return partList;
        }

        /// Reads the IBM PC partition table from the MBR
        /// (TODO: for the moment only primary partitions, implement reading the EBR(Extended boot record) !)
        llist<struct partition> FilesystemManager::readPartitionTable(const int driveId)
        {
            partList.empty();
            int iEBR = -1; // Number of the first EBR found

            // Read the MBR
            for (int i=0; i<4; i++)
            {
                // Add the partition
                struct partition part;
                diskRead(driveId, (0x01BE + (16 * i)), (char *) &part, 16);
                part.id=i+1;
                if (part.fsId != 0 && part.size != 0) // Add only if it's valid
                    partList << part;

                // Search an extended partition
                if (detectFsType(part) == FSTYPE_EBR) // If we found one, remember it for later
                    iEBR=i;
            }

            /*
            // If we found an EBR, read it now
            if (iEBR >= 0)
            {
                gTerm.printf("Partition %d is extended\n", iEBR+1);
                int curId=5; // Logical partitions : 5-255
                unsigned int EBR = partList[iEBR]->sLba * 512; // Addr of the EBR
                u16 magic;
                do
                {
                    //globalTerm.printf("Trying to read partition %d\n", curId);
                    // Lis la premiere entree du EBR (partition logique)
                    part = new partition;
                    diskRead(driveId, (EBR + 0x01BE), (char *) part, 16);
                    if (part.fsId != 0 && part.size != 0) // Seulement si la partition est valide, on l'ajoute a la liste
                    {
                        if (EBR % 512 != 0) // Si EBR/512 n'est pas une divion exacte, y'as un serieux probleme vu que EBR ne peut venir que d'expressions de la forme (XXX->s_lba * 512)
                            fatalError("EBR address (0x%x) is not a multiple of 512 (0x200)", EBR);
                        part.sLba += EBR / 512; // Le s_lba qu'on a lu est l'offset par rapport a l'EBR courant, on converti en adresse absolue
                        part.id=curId;
                        partList << part;
                        curId++;
                    }
                    else
                        delete part;

                    // Check the EBR magic
                    diskRead(driveId, (EBR + 0x01FE), (char *)&magic, 2);

                    // Lis le seconde entree du EBR (pointeur sur l'EBR suivant)
                    part = new partition;
                    diskRead(driveId, (EBR + 0x01CE), (char *) part, 16);
                    EBR = part.sLba * 512 + partList[iEBR]->sLba * 512; // EBR suivant = Addr premier EBR + s_lba, si s_lba == 0, EBR = partList[iEBR-1]->s_lba et on sort de la boucle
                    //globalTerm.printf("EBR magic : 0x%x, next EBR offset (in blocks) : %d\n", magic, part.s_lba);
                    delete part;
                } while (magic == 0xAA55 && EBR != partList[iEBR]->sLba * 512); // Tant qu'on trouve des EBR correctes, on les lit

            }
            */

            return partList;
        }

        bool FilesystemManager::mount(const int partId) /// Mount the given partition
        {
            // VFS ?
            if (partId == 0)
                return initVfs();

            for (u8 i=0; i<partList.size(); i++)
                if (partList[i].id == partId)
                    return mount(partList[i]); /// Appelle l'autre mount
            return false; // Si on arrive ici c'est qu'on a pas trouvé partId
        }

        bool FilesystemManager::mount(const struct partition& part) /// Mount the given partition
        {
            if (part.fsId == FSTYPE_EXT2)
            {
                /// Cherche le premier numero libre dans /dev et cree la node correspondante (example : /dev/1)
                u32 i=0;
                char buf1[]="/dev/";
                char buf2[16];

                do
                {
                    strcpy(buf2,buf1);
                    itoa(i,buf2+5,10); // Met le numero juste apres /dev/
                    i++;
                } while (pathToNode(buf2));
                i--;

                /// Lis la partition ext2
                struct ext2Disk* hd = ext2GetDiskInfo(part.diskId, part);
                if (!hd) // ext2_get_disk_info() doit renvoyer 0 si il se foire, donc si illisible
                    return false;
//					{
//						kmsg(KMSG_ERROR,"Error reading disk infos, halting...\n");
//						halt();
//					}
//					f_root = init_root(hd);
//					current->pwd = f_root; // Passe la racine au thread kernel
//					kmsg(KMSG_SUCCESS,"Root partition mounted (ext2fs)\n");

                /// Lis l'inode de la racine de la partiton
                struct ext2Inode* inode = ext2ReadInode(hd, EXT2_ROOT_INUM);

                /// Creer la node drive dans dev
                itoa(i,buf2,10); // Met juste le numero dans buf
                ((NodeEXT2*)pathToNode("/dev/"))->createChild(buf2, NODE_DRIVE, hd, EXT2_ROOT_INUM, inode);

                return true;
            }
            else
                return false;
        }

        /*
         * detect_fs_type() : Detecte le type de systeme de fichier de la partition (ext2, fat32, ...)
         */
        int FilesystemManager::detectFsType(const struct partition& part)
        {
            /// Unused MBR entry (No partition)
            if (part.fsId == 0)
                return FSTYPE_FREE;

            /// EBR (Extended Boot Record)
            else if (part.fsId == 0x05 || part.fsId == 0x0f)
                return FSTYPE_EBR;

            /// NTFS
            else if (part.fsId == 0x07) // FAT LBA
                return FSTYPE_NTFS;

            /// FAT32 CHS
            else if (part.fsId == 0x0b)
                return FSTYPE_FAT32_CHS;

            /// FAT32 LBA
            else if (part.fsId == 0x0c)
                return FSTYPE_FAT32_LBA;

            /// Linux SWAP
            else if (part.fsId == 0x82)
                return FSTYPE_FAT32_LBA;

            /// Linux native (ext*)
            else if (part.fsId == 0x83)
            {
                /// Ext2/Ext3 (Ext3 = Ext2 + Journal, mais les 2 sont entierement compatibles dans les 2 sens, c'est vraiment le meme filesystem)
                struct ext2SuperBlock *sb;
                sb = (struct ext2SuperBlock *) gPaging.kmalloc(sizeof(struct ext2SuperBlock));
                diskReadBl(0, part.sLba + 2, 0, (char *) sb, (sizeof(struct ext2SuperBlock)/512),0); // Un bloc = 512 octets
                if (sb->sMagic == 0xef53) // Magic de l'ext2 et de l'ext3
                    return FSTYPE_EXT2;
                gPaging.kfree(sb);
            }

            /// Unknow
            else
            {
                //strcpy(fs_name, "unknow\0");
                return FSTYPE_UNKNOWN;
            }

            // Never reaches here - aka compiler warning bypass
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

        /// Renvoie la Node* associée au chemin (absolu si commence par / sinon relatif au pwd du processus courant) passé en paramètre
        Node* FilesystemManager::pathToNode(const char* path)
        {
            Node* current;

            /// Chemin absolu ou relatif
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
                    return 0;

                // Extraction du nom du sous-repertoire
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
                        return 0;
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
    };
};
