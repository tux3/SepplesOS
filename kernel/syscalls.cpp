#include <syscalls.h>
#include <std/types.h>
#include <vga/vgatext.h>
#include <proc/process.h>
#include <keyboard.h>
#include <error.h>

/**
 *
 * Linux-compatible-ish syscalls (Base 0x0) :
 * 1. exit(status)
 *
 * Experimental syscalls (Base 0x10000)
 * 1. print(char* str)
 * 2. nget(char* destbuf, int bufsize)
 * 3. int run(char* str) : Starts a new child process, return its PID or 0 on error
 */

static bool isUserPtrValid(u64 ptr)
{
    if ((u64)ptr >= USER_OFFSET && (u64)ptr <= USER_STACK)
        return true;
    else
        return false;
}

extern "C" u64 doSyscalls(int sysnum, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;

    /// TODO: Implement and use copyFromUser/copyToUser
    /// Right now we'll use any arbitrary pointer userland gives us without any checks
    /// It still works since we're on the user's cr3, but it's a blatant sec hole...

    u64 ret=0;

    switch (sysnum)
    {
    case 1: // exit
    {
        sysExit(arg1);
        break;
    }
    case 0x10000+1: // print
    {
        if (isUserPtrValid(arg1))
            VGAText::print((const char*)arg1);
        break;
    }
    case 0x10000+2: // nget
    {
        if (isUserPtrValid(arg1) && isUserPtrValid(arg1+arg2))
            Keyboard::nget((char*)arg1, arg2);
        break;
    }
    case 0x10000+3: // run
    {
        if (isUserPtrValid(arg1))
            ret = TaskManager::loadTask((char*)arg1);
        else
            ret = 0;
        break;
    }
    default:
        error("Unknow syscall received : %d\n",sysnum);
    }

    return ret;
}

void sysExit(int status)
{
    TaskManager::exitTask(status); // Noreturn
}
