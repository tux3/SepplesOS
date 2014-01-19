#include <fs/disk.h>
#include <fs/ext2.h>
#include <fs/filesystem.h>
#include <screen.h>
#include <paging.h>

namespace IO
{
    namespace FS
    {
        // Initialise la structure decrivant le disque logique.
        // Offset correspond au debut de la partition.
        struct ext2Disk *ext2GetDiskInfo(const int device, const struct partition& part)
        {
            int i, j;
            struct ext2Disk *hd;

            hd = (struct ext2Disk *) gPaging.kmalloc(sizeof(struct ext2Disk));

            hd->device = device;
            hd->part = part;
            hd->sb = ext2ReadSb(hd, part.sLba * 512);
            hd->blocksize = 1024 << hd->sb->sLogBlockSize;

            if (hd->sb->sMagic != 0xef53)
            {
                error("Partition not detected as ext2, signature is : %x\n", hd->sb->sMagic);
                return 0;
            }

            if (hd->sb->sBlocksPerGroup == 0 || hd->sb->sInodesPerGroup == 0) // Eviter la DIV0 Fault
            {
                error("ext2GetDiskInfo : Partiton error : 0 blocks or inodes per group\n");
                return 0;
            }

            i = (hd->sb->sBlocksCount / hd->sb->sBlocksPerGroup) +
                ((hd->sb->sBlocksCount % hd->sb->sBlocksPerGroup) ? 1 : 0);
            j = (hd->sb->sInodesCount / hd->sb->sInodesPerGroup) +
                ((hd->sb->sInodesCount % hd->sb->sInodesPerGroup) ? 1 : 0);
            hd->groups = (i > j) ? i : j;

            hd->gd = ext2ReadGd(hd, part.sLba * 512);

            // DEBUG: affiche des infos sur ce qui a été lu sur la patition

            gTerm.setCurStyle(VGAText::CUR_BLUE, true);
            gTerm.printf("Total number of inodes : %d\n", hd->sb->sInodesPerGroup);
            gTerm.printf("Total number of blocks : %d\n", hd->sb->sBlocksPerGroup);
            gTerm.printf("Total number of inodes per group : %d\n", hd->sb->sInodesPerGroup);
            gTerm.printf("Total number of blocks per group : %d\n", hd->sb->sBlocksPerGroup);
            gTerm.printf("First block : %d\n", hd->sb->sFirstDataBlock);
            gTerm.printf("First ext2Inode : %d\n", hd->sb->sFirstIno);
            gTerm.printf("Block size : %d\n", 1024 << hd->sb->sLogBlockSize);
            gTerm.printf("Inode size : %d\n", hd->sb->sInodeSize);
            gTerm.printf("Minor revision : %d\n", hd->sb->sMinorRevLevel);
            gTerm.printf("Major revision : %d\n", hd->sb->sRevLevel);
            gTerm.printf("Ext2 signature (must be 0xef53) : 0x%x\n", hd->sb->sMagic);
            gTerm.setCurStyle();

            // FIN-DEBUG

            return hd;
        }

        struct ext2SuperBlock *ext2ReadSb(struct ext2Disk *hd, int sPart)
        {
            struct ext2SuperBlock *sb;

            sb = (struct ext2SuperBlock *) gPaging.kmalloc(sizeof(struct ext2SuperBlock));
            diskRead(hd->device, sPart + 1024, (char *) sb, sizeof(struct ext2SuperBlock));
            //diskReadBl(hd->device, sPart + 2, 0, (char *) sb, (sizeof(struct ext2SuperBlock)/512),0); // Un bloc = 512 octets

            return sb;
        }

        struct ext2GroupDesc *ext2ReadGd(struct ext2Disk *hd, int sPart)
        {
            //kmsg(KMSG_DEBUG,"ext2_read_gd():called\n");
            struct ext2GroupDesc *gd;
            int offset, gdSize;

            // locate block
            offset = (hd->blocksize == 1024) ? 2048 : hd->blocksize;

            // size occupied by the descriptors
            gdSize = hd->groups * sizeof(struct ext2GroupDesc);

            // Create the descriptors table
            gd = (struct ext2GroupDesc *) gPaging.kmalloc(gdSize);

            //kmsgf(KMSG_DEBUG,"ext2_read_gd():offset : %d\n",offset);
            //kmsgf(KMSG_DEBUG,"ext2_read_gd():gd_size : %d\n",gd_size);

            diskRead(hd->device, sPart + offset, (char *) gd, gdSize);
            //diskReadBl(hd->device, sPart, offset, (char *) gd, 0, gdSize); // Un bloc = 512 octets

            return gd;
        }

        /* Retourne la structure d'ext2Inode a partir de son numero */
        struct ext2Inode *ext2ReadInode(const struct ext2Disk *hd, const int iNum)
        {
            //kmsg(KMSG_DEBUG,"ext2_read_inode():called\n");
            int grNum, index, offset;
            struct ext2Inode *ext2Inode;

            ext2Inode = (struct ext2Inode *) gPaging.kmalloc(sizeof(struct ext2Inode));

            if (hd->sb->sInodesPerGroup == 0) // Eviter la DIV0 Fault
            {
                error("ext2ReadInode : Partiton error : 0 inodes per group\n");
                return 0;
            }

            /* groupe qui contient l'ext2Inode */
            grNum = (iNum - 1) / hd->sb->sInodesPerGroup;

            //kmsgf(KMSG_DEBUG,"ext2_read_inode():ext2Inode group : %d\n", gr_num);

            /* index de l'ext2Inode dans le groupe */
            index = (iNum - 1) % hd->sb->sInodesPerGroup;

            //kmsgf(KMSG_DEBUG,"ext2_read_inode():ext2Inode index in group : %d\n", index);

            /* offset de l'ext2Inode sur le disk depuis le debut de la partition */
            offset = hd->gd[grNum].bgInodeTable * hd->blocksize + index * hd->sb->sInodeSize;

            //kmsgf(KMSG_ERROR,"ext2_read_inode():ext2Inode offset:%d, s_lba:%d\n", offset, hd->part.s_lba);

            /* lecture */
            diskRead(hd->device, (hd->part.sLba * 512) + offset, (char *) ext2Inode, hd->sb->sInodeSize);
            //disk_read_bl(hd->device, hd->part.s_lba, offset, (char *) ext2Inode, 0, hd->sb->s_inode_size);

            return ext2Inode;
        }

        char *ext2ReadFile(const struct ext2Disk *hd, const struct ext2Inode *ext2Inode)
        {
            char *mmapBase, *mmapHead, *buf;

            int *p, *pp, *ppp;
            unsigned int i, j, k;
            unsigned int n, size;

            buf = (char *) gPaging.kmalloc(hd->blocksize);
            p = (int *) gPaging.kmalloc(hd->blocksize);
            pp = (int *) gPaging.kmalloc(hd->blocksize);
            ppp = (int *) gPaging.kmalloc(hd->blocksize);

            size = ext2Inode->iSize; // Taille totale du fichier

            mmapHead = mmapBase = gPaging.kmalloc(size);

            /* direct block number */
            for (i = 0; i < 12 && ext2Inode->iBlock[i]; i++) {
                diskRead(hd->device, (hd->part.sLba * 512) + ext2Inode->iBlock[i] * hd->blocksize, buf, hd->blocksize);
                n = ((size > hd->blocksize) ? hd->blocksize : size);
                memcpy(mmapHead, buf, n);
                mmapHead += n;
                size -= n;
            }

            /* indirect block number */
            if (ext2Inode->iBlock[12]) {
                diskRead(hd->device, (hd->part.sLba * 512) + ext2Inode->iBlock[12] * hd->blocksize, (char *) p, hd->blocksize);

                for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
                    diskRead(hd->device, (hd->part.sLba * 512) + p[i] * hd->blocksize, buf, hd->blocksize);

                    n = ((size > hd->blocksize) ? hd->blocksize : size);
                    memcpy(mmapHead, buf, n);
                    mmapHead += n;
                    size -= n;
                }
            }

            /* bi-indirect block number */
            if (ext2Inode->iBlock[13]) {
                diskRead(hd->device, (hd->part.sLba * 512) + ext2Inode->iBlock[13] * hd->blocksize, (char *) p, hd->blocksize);

                for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
                    diskRead(hd->device, (hd->part.sLba * 512) + p[i] * hd->blocksize, (char *) pp, hd->blocksize);

                    for (j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                        diskRead(hd->device, (hd->part.sLba * 512) + pp[j] * hd->blocksize, buf, hd->blocksize);

                        n = ((size > hd->blocksize) ? hd-> blocksize : size);
                        memcpy(mmapHead, buf, n);
                        mmapHead += n;
                        size -= n;
                    }
                }
            }

            /* tri-indirect block number */
            if (ext2Inode->iBlock[14]) {
                diskRead(hd->device, (hd->part.sLba * 512) + ext2Inode->iBlock[14] * hd->blocksize, (char *) p, hd->blocksize);

                for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
                    diskRead(hd->device, (hd->part.sLba * 512) + p[i] * hd->blocksize, (char *) pp, hd->blocksize);

                    for (j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                        diskRead(hd->device, (hd->part.sLba * 512) + pp[j] * hd->blocksize, (char *) ppp, hd->blocksize);

                        for (k = 0; k < hd->blocksize / 4 && ppp[k]; k++) {
                            diskRead(hd->device, (hd->part.sLba * 512) + ppp[k] * hd->blocksize, buf, hd->blocksize);

                            n = ((size > hd->blocksize) ? hd->blocksize : size);
                            memcpy(mmapHead, buf, n);
                            mmapHead += n;
                            size -= n;
                        }
                    }
                }
            }

            gPaging.kfree(buf);
            gPaging.kfree(p);
            gPaging.kfree(pp);
            gPaging.kfree(ppp);

            return mmapBase;
        }

        // is_directory(): renvoit si le fichier en argument est un repertoire
        bool isDirectory(const struct ext2Inode* ext2Inode)
        {
//				if (!node->ext2Inode)
//					node->ext2Inode = ext2_read_inode(fp->disk, fp->inum);

            return (ext2Inode->iMode & EXT2_S_IFDIR) ? true : false;
        }

        NodeEXT2::NodeEXT2(const char* name, const int type, Node* const parent, const struct ext2Disk* hd,
                            const u32 ext2Inum, const struct ext2Inode *ext2Inode, const bool createOnDisk)
        : Node::Node(name, type, parent)
        {
            /// Spécificités de l'ext2
            this->hd = hd;
            this->ext2Inum = ext2Inum;
            this->ext2Inode = ext2Inode;
            //this->partId =

            if (createOnDisk)
                if (type == NODE_FILE || type == NODE_DIRECTORY) /// On cherche a creer un nouveau fichier ou un dossier sur la partition ext2
                {
                    if (EXT2_READONLY)
                        fatalError("NodeEXT2::NodeEXT2() : Trying to create a file/directory while EXT2_READONLY is defined");

                    fatalError("NodeEXT2::NodeEXT2() : RW error, File and Directory creation not implemented");
                    /// TODO: Implementer la création de fichier/dossier sur la partition ext2
                }

            /// Fill the childs list (it's recursively by each child constructor !), will generate the whole child tree of this node
            if (EXT2_PRECACHE_ALL && (type == NODE_DRIVE || type == NODE_DIRECTORY))
                updateChildsCache();
        }

        const struct ext2Disk* NodeEXT2::getExt2Disk() const
        {
            return hd;
        }

        const struct ext2Inode* NodeEXT2::getExt2Inode() const
        {
            return ext2Inode;
        }

        llist<Node*> NodeEXT2::getChilds()
        {
            if (type != NODE_DIRECTORY && type != NODE_DRIVE)
                error("Node::getChilds() : Trying to get childs of a NODE_FILE\n");
            else
                /// Si on a pas deja l'arborescence en cache, il faut mettre a jour la liste des enfants
                if (!EXT2_PRECACHE_ALL)
                    if (!childs.size())
                        updateChildsCache(); // Remplis la liste childs en lisant depuis le disque

            llist<Node*> copy(childs); // Constructeur de copie généré par le compilo
            return copy; // Renvoie une copie de la liste mais avec les pointeurs sur les vrais enfants (c'est voulu)
        }

        /// Met a jour la liste childs en lisant depuis la partition
        void NodeEXT2::updateChildsCache()
        {
            //globalTerm.printf("NodeEXT2::updateChildsCache(): looking for %s childs\n", name);
            if (type != NODE_DIRECTORY && type != NODE_DRIVE)
            {
                error("Node::getChilds() : Trying to upadte childs list of %s, wich is a NODE_FILE\n", name);
                return;
            }

            if (!isDirectory(ext2Inode))
            {
                error("Node::getChilds() : %s is not a directory and not a drive !\nVFS Node type doesn't match EXT Inode type\n", name);
                return;
            }

//				/// Met a jour l'ext2Inode depuis le disque
//				ext2Inode = ext2_read_inode(hd, inum);

            /// Lis le contenu du repertoire (donc la liste des fichiers contenus)
            char* mmap = ext2ReadFile(hd, ext2Inode);

            /// Lecture de chaque entrée du repertoire et création de la node correspondante
            char *filename;
            u32 dsize = ext2Inode->iSize;			// taille du repertoire
            struct ext2DirectoryEntry* dentry = (struct ext2DirectoryEntry*) mmap;
            while (dentry->ext2Inode && dsize)
            {
                filename = (char *) gPaging.kmalloc(dentry->nameLen + 1);
                memcpy(filename, &dentry->name, dentry->nameLen);
                filename[dentry->nameLen] = 0;

                //globalTerm.printf("NodeEXT2::updateChildsCache(): reading ext2Inode %d: %s\n", dentry->ext2Inode, filename);

                /// Crée chaque node et les ajoutes a childs
                if (strcmp(".", filename) && strcmp("..", filename))
                {
                    struct ext2Inode* ext2Inode = ext2ReadInode(hd, dentry->ext2Inode);
                    int type;
                    if (isDirectory(ext2Inode))
                        type = NODE_DIRECTORY;
                    else
                        type = NODE_FILE;

                    /// Crée la node enfant en specifiant bien de ne pas l'écrire sur le disque, puisqu'on est justement en train de lire la structure depuis le disque
                    if (!createChild(filename, type, hd, dentry->ext2Inode, ext2Inode, false))
                        fatalError("NodeEXT2::updateChildsCache(): Unable to create child node %d\n", filename);
                }

                gPaging.kfree(filename);

                // Lecture de l'entree suivante
                dsize -= dentry->recLen;
                dentry = (struct ext2DirectoryEntry *) ((char *) dentry + dentry->recLen);
            }

            gPaging.kfree(mmap);
        }

        /// On va être obligés de créer une nouvelle ext2Inode sur le disque, donc cette fonction nécessite que EXT2_READONLY == 0
        bool NodeEXT2::createChild(const Node* node)
        {
            if (EXT2_READONLY)
                fatalError("NodeEXT2::createChild : Trying to create a new child while EXT2_READONLY is defined");

            if (this->type != NODE_DIRECTORY && this->type != NODE_DRIVE)
            {
                error("NodeEXT2::createChild : Node is not a directory and not a drive.\n");
                return false;
            }

            fatalError("NodeEXT2::createChild(const Node* node) : EXT2 File creation (RW) not implemented\n");

            if(node){}; // Do nothing with arg

            /// Todo: Creation fichier ext2
            /// To make a new child :
            /// Get parent directory ext2Inode (directory that will contain the newly created ext2Inode)
            /// Create a new ext2Inode by calling new_node(ldirp, lastc, mode, 0);
            /// If it fails release the ext2Inode, else pass the ext2Inode to the constructor

//				struct ext2_inode* ext2Inode = newNode(this->ext2Inode, node->getName(), 0, 0);
//
//				NodeEXT2* child = new NodeEXT2(node->getName(), node->getType(), this->partId, this, this->hd, ext2Inode, ext2Inode->inum)); // Copie la node en argument, donc sur cette partition/ce hd
//				childs << child;
            return true;
        }

        bool NodeEXT2::createChild(const char* name, const int type, const struct ext2Disk* hd, const u32 inum, const struct ext2Inode *ext2Inode,  const bool createOnDisk)
        {
            NodeEXT2* child = new NodeEXT2(name, type, this, hd, inum, ext2Inode, createOnDisk);
            childs << child;
            return true;
        }

        bool NodeEXT2::remove()
        {
            if (childs.size()) // On ne peut pas se supprimmer si on a des enfants
            {
                error("NodeEXT2::remove() Trying to remove a node with childs (childs:%d)", childs.size());
                return false;
            }
            else if (opens)
            {
                error("NodeEXT2::remove() Trying to remove a currently used node (opens:%d)", opens);
                return false;
            }

            if (type == NODE_FILE || type == NODE_DIRECTORY) /// On cherche a creer un nouveau fichier ou un dossier sur la partition ext2
            {
                if (EXT2_READONLY)
                    fatalError("NodeEXT2::NodeEXT2() : Trying to remove a file/directory while EXT2_READONLY is defined");

                fatalError("NodeEXT2::NodeEXT2() : RW error, File and Directory removal not implemented");
                /// TODO: Implementer la suppression de fichier/dossier sur la partition ext2
            }
            else // C'est le NODE_DRIVE qui libère *hd pour l'ext2
            {
                delete hd;
            }

            for(u32 i=0; i<parent->childs.size(); i++)
                if (parent->childs[i]->name == this->name)
                    parent->childs.removeAt(i);
            return true;
        }

        u64 NodeEXT2::getSize() const
        {
            if (type==NODE_FILE)
                return getExt2Inode()->iSize;
            else
                return 0;
        }

        u64 NodeEXT2::read(char* buf, const u64 len) const // Si len > taille du fichier, on lis ce que l'on peut
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
            const char* const data = ext2ReadFile(getExt2Disk(), getExt2Inode());
            const u64 dataSize = getExt2Inode()->iSize;
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

        u64 NodeEXT2::write(const char* buf, const u64 len) // Agrandis le fichier si il est trop petit.
        {
            if (EXT2_READONLY)
            {
                error("Can write ext2 node, ext2 driver was compiled with readonly.\n");
                return 0;
            }

            /// Do nothing with args and crash happily.
            if (buf&&len) {}
            fatalError("NodeEXT2::write not implemented.\n");

            /*
            if (type != NODE_FILE)
            {
                error("Can't write node, it isn't a file.\n");
                return 0;
            }

            // Get the process's file cursor (file must be open)
            u64* offset;
            bool found=false;
            Process *curProc = globalTaskManager.getCurrent();
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
                /// TODO: Resize file and write data
                dataSize=len+*offset;
                *offset = dataSize;
                return len;
            }
            else
            {
                /// TODO: Write data
                *offset += len;
                return len;
            }
            */
        }
    };
};
