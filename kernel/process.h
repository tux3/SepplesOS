#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

//#include <terminal.h>
#include <paging.h>
#include <fs/ext2.h>
#include <fs/filesystem.h>
#include <lib/llistForward.h>

#define KERNELMODE 	0
#define USERMODE 	1

#define MAXPID 		64

struct Process
{
        /// Process attributs must be saved/restored during task switching ...
        int pid;
        struct
        {
            u32 eax, ecx, edx, ebx;
            u32 esp, ebp, esi, edi;
            u32 eip, eflags;
            u32 cs:16, ss:16, ds:16, es:16, fs:16, gs:16;
            u32 cr3;
        } regs __attribute__ ((packed));

        struct
        {
            u32 esp0;
            u16 ss0;
        } kstack __attribute__ ((packed));

        ///
        /// WATCH OUT ! You just CAN'T modify something before this limit without also
        /// modifyng the assembly function do_switch() wich assure the task switching.
        ///

        // Opened files/dirs
        struct OpenFile
        {
            IO::FS::Node* file;	// descripteur de fichier
            u64 ptr;		// pointeur de lecture dans le fichier
            u32 dirPos; // Position in the directory for readdir/telldir/seekdir/rewinddir
        };

        // NOTE: redondancy between regs.cr3 and page_dir->base->pAddr
        struct pageDirectoryEntry* pageDir;
        llist<struct page*> pageList; ///< Pages used by the process (text, data, stack)
        char *bExec;
        char *eExec;
        char *bBss;
        char *eBss;
        char *bHeap;
        char *eHeap;			///< Points to the top of the "heap"
        IO::FS::Node* pwd; ///< Points to the current dir
        llist<struct OpenFile*> openFiles; ///< Linked list of open files

        Process *parent;		///< Parent process
        llist<Process*> childs; ///< Child processes
        llist<Process*> siblings; ///< Sibling processes

        // IO::Terminal *terminal;	// Terminal lié au process

        u32 signal; ///< Last signal received
        void* sigfn[32]; ///< Function pointers to handles each signals.

        int status;	///< exit status

        int state;	///< -1 zombie, 0 not runnable, 1 ready/running, 2 sleep

} __attribute__((packed));

class TaskManager
{
    public:
        TaskManager();
        void init();
        int exec();
        int loadTask(const char* path);
        int loadTask(const char* path, const int argc, const char **argv);
        //int loadTask(struct IO::Disk::Node *node, const int argc, const char **argv);
        void exitTask(int status); /// Exits and destroys a task
        void schedule();
        void switchToTask(int n, int mode);
        void doSwitch(Process *current); ///< Defined in do_switch.asm
        Process* getCurrent();
        int getNProc();

    private:
        Process* pList; ///< Array of process. Fixed size of MAXPID+1
        Process* current;
        int nProc;
};

/// Singleton
extern TaskManager gTaskmgr;

#endif // PROCESS_H_INCLUDED
