#ifndef PROCESS_H
#define PROCESS_H

#include <lib/llistForward.h>
#include <disk/vfs.h>

#define KERNELMODE 	0
#define USERMODE 	1

#define MAXPID 		64

struct Page;

struct Process
{
    /// Process attributs must be saved/restored during task switching ...
    struct
    {
        u64 rax, rcx, rdx, rbx;
        u64 rsp, rbp, rsi, rdi;
        u64 r8, r9, r10, r11;
        u64 r12, r13, r14, r15;
        u64 rip, eflags;
        u64 cs:16, ss:16, ds:16, es:16, fs:16, gs:16;
        u64 cr3;
    } regs __attribute__ ((packed));

    struct
    {
        u64 rsp0;
        u16 ss0;
    } kstack __attribute__ ((packed));

    /// Anything in the structs BEFORE this point can't be changed without changing doSwitch.asm

    u64 pid;

    u8* pml4;
    llist<Page*> pageList;
    u8 *bExec;
    u8 *eExec;
    u8 *bBss;
    u8 *eBss;
    u8 *bHeap;
    u8 *eHeap;
    Process* parent;
    int state;	///< -1 zombie, 0 not runnable, 1 ready/running, 2 sleep
    int status; ///< Return status

    /// PID of a process whose death we're waiting for, if 0 we're not waiting
    /// The scheduler must not swith to the current process while this is != 0
    /// TODO: When killing a process, reset this to 0 for all processes waiting for the killed one
    u64 joiningWith;

    Process();
} __attribute__ ((packed));

namespace TaskManager
{
    void init();
    int exec();
    int loadTask(const char* path, const int argc=0, const char **argv=nullptr);
    int loadTask(Node *node, const int argc=0, const char **argv=nullptr);
    void exitTask(int status); /// Exits and destroys a task
    void schedule();
    void switchToTask(int n, int mode);
    void doSwitch(Process *current); ///< Defined in do_switch.asm
    Process* getCurrent();
    int getNProc();

    // The scheduler will not switch tasks when disabled, but interrupts will still fire normally.
    // Note that a task disabling the scheduler must be careful to re-enabled it itself.
    // If such a task is killed or freezes, it'd bring the whole system down. Be fucking careful.
    void disableScheduler();
    void enableScheduler();
}

extern "C" void doTaskSwitch(Process*,u16 kss,u64 krsp);

#endif // PROCESS_H
