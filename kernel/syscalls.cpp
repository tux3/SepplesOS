#include <syscalls.h>
#include <std/types.h>
//#include <gdt.h>
#include <screen.h>
#include <io.h>
//#include <process.h>
//#include <lib/mem.h>
#include <memmap.h>
//#include <disk/filesystem.h>
//#include "console.h"
#include <lib/string.h>
//#include <signal.h>
//#include "rtc.h"

/*
 * Ce fichier gère l'interruption 0x30 (48), qui est l'interruption logicielle
 * utilisée pour les appels systèmes (syscalls). do_syscalls gère l'interruption 0x30.
 *
 * Certains appels systèmes peuvent être transférés dans une fonction
 * directement accessible par le noyau et préfixée sys_
 */

/**
Syscalls list  :

I.  Divers
    dump(*pa)
    time(*time_t)
    shutdown()
    restart()
    environ()

II. Filesystem
	exist(*path)
	stat(*path,*uBuf)
	fstat(fd,*uBuf)
	pwd(*uBuf)
	chdir(*path)
	open(*path/fd)
	close(fd)
	read(fd, *uBuf, size)
	write(fd, *uBuf, size)
	opendir(*path/fd)
	readdir(fd)
	closedir(fd)
	rewinddir(fd)
	telldir(fd)
	seekdir(fd)
	isatty(fd)
	link(*oldName,*newName)
	unlink(*name)
	lseek(fd,pos,dir)

III. Console
	getkey(key)
	putkey(key)
	scan(*uBuf)
	print(*uStr)

IV. Process
	exec(*path, **argv)
	exit(status)
	_exit(status)
	execve(*name,**argv,**env)
	wait(*status)
	times(*uBuf)
	kill(pid,sig)
	sigaction(sig,*fn)
	sigreturn()
	fork()
	getpid()

V. Memory
	sbrk(size)
**/

extern "C" void do_syscalls(int sys_num)
{
/*
    //globalTerm.print("Got a syscall !\n");

    uint32 *stack_ptr;


    // Stocke dans stack_ptr le pointeur vers les registres sauvegardes.
    // Les arguments sont transmis dans : ebx, ecx, edx, esi, edi
    // Le code de retour sera transmis dans %eax : stack_ptr[14]
    asm("mov %%ebp, %0": "=m"(stack_ptr):);

    switch (sys_num)
    {

    // I.Divers
    case 1: // dump(*pa)
    {
        uint32 *pa;
        asm("mov %%ebp, %0": "=m"(pa):);
        globalTerm.printf("eax: 0x%x ecx: 0x%x edx: 0x%x ebx: 0x%x\n", pa[12], pa[11], pa[10], pa[9]);
        globalTerm.printf("ds: 0x%x esi: 0x%x edi: 0x%x\n", pa[4], pa[6], pa[5]);
        globalTerm.printf("ss: 0x%x ebp: 0x%x esp: 0x%x\n", pa[17], pa[7], pa[16]);
        globalTerm.printf("cs: 0x%x eip: 0x%x\n", pa[14], pa[13]);
    }
    break;

    case 2: // time(*time_t)
    {
//    	time_t *time;
//    	cli;
//		asm("mov %%ebx, %0": "=m"(time):);
//        memcpy(time, &global_time, sizeof(time_t)); // global_time est défini dans rtc.h
//		sti;
    }
    break;

    // II.Fichiers
    case 20: // exist(*path)
    {
        char *path;
        IO::Disk::Node* node;

        cli;
        asm("mov %%ebx, %0": "=m"(path):);
        if (!(node = globalFilesystem.pathToNode(path)))
        {
            globalTerm.printf("exist:File doesn't exist : '%s'\n", path);
            stack_ptr[14] = 0;
        }
        else
        {
            globalTerm.printf("exist:File do exist : '%s'\n", path);
            stack_ptr[14] = 1;
        }
        sti;
    }
    break;
    case 21: // pwd(*uBuf)
    {
        char *uBuf;
        int sz;
        struct Dusk::IO::Disk::Node *fp;

        cli;
        asm("mov %%ebx, %0": "=m"(uBuf):);

        Dusk::Process* current = globalTaskManager.getCurrent();

        if (current->pwd->getNodeId() == FS_ROOT_NODE_ID)
        {
            uBuf[0] = '/';
            uBuf[1] = 0;
            return;
        }
        fp = current->pwd;
        sz = strlen(fp->getName()) + 1;
        while (fp->getParent()->getNodeId() != FS_ROOT_NODE_ID)
        {
            fp = fp->getParent();
            sz += (strlen(fp->getName()) + 1);
        }
        fp = current->pwd;
        uBuf[sz] = 0;
        while (sz > 0)
        {
            memcpy(uBuf + sz - strlen(fp->getName()), fp->getName(), strlen(fp->getName()));
            sz -= (strlen(fp->getName()) + 1);
            uBuf[sz] = '/';
            fp = fp->getParent();
        }
        // kmsgf(KMSG_INFO,"sys_pwd(): %s\n", current->pwd->name);
    }
    break;
    case 22: // chdir(*path)
    {
        char *path;
        Dusk::IO::Disk::Node* file;

        cli;
        asm("mov %%ebx, %0": "=m"(path):);

        if (!(file = globalFilesystem.pathToNode(path)))
        {
            errorHandler::error("Directory not found : %s\n", path);
            return;
        }

        if (file->getType() != Dusk::IO::Disk::NODE_DIRECTORY)
        {
            errorHandler::error("%s is not a directory\n", path);
            return;
        }

        globalTaskManager.getCurrent()->pwd = file;
        // kmsgf(KMSG_INFO,"sys_chdir(): chdir to %s\n", current->pwd->name);
    }
    break;
    case 23: // open(*path)
    {
//        char *path;
//
//        cli;
//		asm("mov %%ebx, %0": "=m"(path):);
//        stack_ptr[14] = sys_open(path); // Code de retour
    }
    break;
    case 24: // close(*fd)
    {
//        uint32 fd;
//        struct open_file *of;
//
//        cli;
//		asm("mov %%ebx, %0": "=m"(fd):);
//        //kmsgf(KMSG_INFO,"sys_close(): Process[%d] closing fd %d\n", current->pid, fd);
//        of = current->fd;
//        while (fd)
//        {
//            of = of->next;
//            if (of == 0)
//            {
//                errorHandler::error("sys_close(): Invalid file descriptor\n");
//                return;
//            }
//            fd--;
//        }
//
//        kfree(of->file->mmap);
//        of->file->mmap = 0;
//        of->file = 0;
//        of->ptr = 0;
    }
    break;
    case 25: // read(*fd, *u_buf, bufsize)
    {
//        char *u_buf;	// buffer d'entree utilisateur
//        uint32 fd;
//        uint32 bufsize;
//        struct open_file *of;
//
//        cli;
//        asm("	mov %%ebx, %0;	\
//            mov %%ecx, %1;	\
//            mov %%edx, %2": "=m"(fd), "=m"(u_buf), "=m"(bufsize):);

//        //kmsgf(KMSG_INFO"sys_read(): Reading %d bytes on fd %d\n", bufsize, fd);
//        of = current->fd;
//        while (fd)
//        {
//            of = of->next;
//            if (of == 0)
//            {
//                errorHandler::error("sys_read(): Invalid file descriptor\n");
//                stack_ptr[14] = -1; // Code de retour
//                return;
//            }
//            fd--;
//        }
//
//        if ((of->ptr + bufsize) > of->file->inode->i_size)
//            bufsize = of->file->inode->i_size - of->ptr;
//
//        memcpy(u_buf, (char *) (of->file->mmap + of->ptr), bufsize);
//        of->ptr += bufsize;
//
//        stack_ptr[14] = bufsize; // Code de retour
    }
    break;

    // III.Inputs

    case 30: // scan(*u_buf)
    {
        char *u_buf;	// buffer d'entree utilisateur
        asm("mov %%ebx, %0": "=m"(u_buf):);
        stack_ptr[14] = sys_scan(u_buf); // Code de retour
    }
    break;

    // IV.Outputs
    case 40: // print(*u_str)
    {
        char *uStr;
        asm("mov %%ebx, %0": "=m"(uStr):);

        cli; // Bloque l'ordonnanceur
        globalTerm.print(uStr);
        sti; // Relance l'ordonnanceur

    }
    break;

    // V.Execution
    case 50: // exec(*path, **argv)
    {
//        char *path;
//        char **argv;
//        asm("	mov %%ebx, %0	\n \
//            mov %%ecx, %1"
//            : "=m"(path), "=m"(argv) :);
//
//        stack_ptr[14] = sys_exec(path, argv); // Code de retour
    }
    break;
    case 51: // exit(status)
    {
        int status;

        cli; // Evite plusieurs interruptions à la fois, qui pourrait foutre en l'air le moment crucial

        asm("mov %%ebx, %0": "=m"(status):);
        sys_exit(status);
    }
    break;
    case 52: // wait(*status)
    {
        int *status;

        asm("mov %%ebx, %0": "=m"(status):);
        stack_ptr[14] = sys_wait(status); // Code de retour
    }
    break;
    case 53: // kill(pid,sig)
    {
//        int pid, sig;
//
//        cli;
//        asm("	mov %%ebx, %0	\n \
//            mov %%ecx, %1"
//            : "=m"(pid), "=m"(sig) :);
//

//        kmsgf(KMSG_INFO,"sys_kill(): sending signal %d to process %d\n", sig, pid);
//
//        if (p_list[pid].state > 0)
//        {
//            set_signal(&p_list[pid].signal, sig);
//            stack_ptr[14] = 0; // Code de retour
//        }
//        else
//            stack_ptr[14] = -1; // Code de retour
    }
    break;
    case 54: // sigaction(sig,*fn)
    {
//        char *fn;
//        int sig;
//
//        cli;
//        asm("	mov %%ebx, %0	\n \
//            mov %%ecx, %1"
//            : "=m"(sig), "=m"(fn) :);
//

//        //kmsgf(KMSG_INFO,"sys_sigaction(): signal %d trapped at %p by process %d\n", sig, fn, current->pid);
//
//        if (sig < 32)
//            current->sigfn[sig] = fn;
//
//        stack_ptr[14] = 0; // Code de retour
    }
    break;
    case 55: // sigreturn()
    {
//    	// WARNING : sys_sigreturn must NEVER be called by the userland,
//		//           it is used by the kernel to cleanup the stack after signals
//        cli;
//        sys_sigreturn();
    }
    break;
    case 56: // sbrk(size)
    {
//        int  size;
//		asm("mov %%ebx, %0": "=m"(size):);
//        stack_ptr[14] = (uint32) sys_sbrk(size); // Code de retour
    }
    break;



    // Unknow syscall
    default:
        globalTerm.printf("Unknow syscall received : %d\n",sys_num);
    }

    sti; // Relance les interruptions si besoin (elle ont peut-etre été stoppés plus haut)

*/
    return;
}




// 2. exit(status)
void sys_exit(int status)
{
/*    globalTaskManager.exitTask(status);
    globalTaskManager.switchToTask(0, KERNELMODE); */
}

//// 3. open(*path)
//int sys_open(char *path)
//{
//    uint32 fd;
//    struct file *fp;
//    struct open_file *of;
//
//    if (!(fp = path_to_file(path)))
//    {
//        // kmsgf(KMSG_ERROR,"sys_open(): can't open %s\n", path);
//        return -1;
//    }
//
//    // kmsgf(KMSG_ERROR,"sys_open(): process[%d] opening file %s\n", current->pid, fp->name);
//
//    fp->opened++;
//
//    if (!fp->inode)
//        fp->inode = ext2_read_inode(fp->disk, fp->inum);
//
//    /* Lecture du fichier */
//    fp->mmap = ext2_read_file(fp->disk, fp->inode);
//
//    /*
//     * Recherche d'un descripteur libre.
//     */
//    fd = 0;
//    if (current->fd == 0)
//    {
//        current->fd = (struct open_file *) kmalloc(sizeof(struct open_file));
//        current->fd->file = fp;
//        current->fd->ptr = 0;
//        current->fd->next = 0;
//    }
//    else
//    {
//        of = current->fd;
//        while (of->file && of->next)
//        {
//            of = of->next;
//            fd++;
//        }
//
//        if (of->file == 0)  	/* reutilisation d'un descripteur precedement ouvert */
//        {
//            of->file = fp;
//            of->ptr = 0;
//        }
//        else  		/* nouveau descripteur */
//        {
//            of->next = (struct open_file *) kmalloc(sizeof(struct open_file));
//            of->next->file = fp;
//            of->next->ptr = 0;
//            of->next->next = 0;
//            fd++;
//        }
//    }
//
//    return fd;
//}

// 6. scan(*u_buf)
int sys_scan(char *u_buf)
{/*
    Process *current = globalTaskManager.getCurrent();
    if (!current->terminal)
    {
        errorHandler::error("sys_scan(): process without term\n");
        return -1;
    }

    //globalTerm.print("sys_scan():Waiting term to be free ...\n");
    // Bloque si la console est deja utilisee
    //while (current_term->pread);

    //current->console->term->pread = current;
    //current->console->inlock = 1;
    char buf[SYS_SCAN_MAX];
    //globalTerm.print("sys_scan():Waiting buffer to be full ...\n");
    current->terminal->scan(buf);
    // Bloque jusqu'a ce que le buffer utilisateur soit rempli
    //while (current->console->inlock == 1);
    //globalTerm.print("sys_scan():Buffer full, returning ...\n");
    strcpy(u_buf, buf);

    return strlen(u_buf);
    */
}

//// 9. exec(*path, **argv)
//int sys_exec(char *path, char **argv)
//{
//    char **ap;
//    int argc, pid;
//    struct file *fp;
//
//    if (!(fp = path_to_file(path)))
//    {
//        kmsgf(KMSG_ERROR,"sys_exec(): %s: file not found\n", path);
//        return -1;
//    }
//
//    if (!fp->inode)
//        fp->inode = ext2_read_inode(fp->disk, fp->inum);
//
//    ap = argv;
//    argc = 0;
//    while (*ap++)
//        argc++;
//
//    cli;
//    pid = load_task(fp->disk, fp->inode, argc, argv);
//    sti;
//
//    /*
//    {
//    	struct process *proc;
//    	struct list_head *p;
//    	printk("proc[%d]: parent[%d]\n", current->pid, current->parent->pid); // Debug
//    	list_for_each(p, &current->child){
//    		proc = list_entry(p, struct process, sibling);
//    		printk("child[%d]\n", proc->pid); // Debug
//    	}
//    }
//    */
//
//    return pid;
//}

// 10. sbrk(size)
char* sys_sbrk(int size)
{
/*
    char *ret;
    ret = globalTaskManager.getCurrent()->eHeap;

    globalTaskManager.getCurrent()->eHeap += size;
    return ret;
    */
}

// 11. wait()
int sys_wait(int* status)
{
/*
    int pid;
    Process* children;
    Process* current = globalTaskManager.getCurrent();

    //globalTerm.printf("sys_wait(): [%d] wait for children death\n", current->pid);

    while (!isSignal(current->signal, SIGCHLD)) // Attend la mort d'un enfant (que c'est joyeux !)
        if (!current->childs.size()) // On a pas d'enfants mieux vaut pas les attendre
        {
            //errorHandler::error("sysWait called by a process without childs\n"); // Not a real error, should only be a warning/debug info
            return 0;
        }

    //globalTerm.printf("sys_wait(): [%d] has a dead children\n", current->pid);

    cli;

    // Recherche du fils mort
    bool found=false;
    for (uint32 i=0; i<current->childs.size(); i++)
    {
        children = current->childs[i];
        if (children->state == -1)
        {
            found=true;
            pid = children->pid;		// recupere le pid
            if (status)
                *status = children->status;	// recupere le status
            children->state = 0;		// libere le fils
            current->childs.removeAt(i); // enleve le fils de sa liste
            clearSignal(&current->signal, SIGCHLD);	// efface le signal SIGCHLD
            break;
        }
    }

    sti;
    if (found)
        return pid;
    else
    {
        errorHandler::error("sysWait:Can't find the dead children for process %d.\n", current->pid);
        return 0;
    }
    */
}

// sigreturn()
// WARNING : sys_sigreturn must NEVER be called by the userland,
//           it is used by the kernel to cleanup the stack after signals
void sys_sigreturn(void)
{
/*
    uint32 *esp;

    cli;

    asm("mov (%%ebp), %%eax; mov %%eax, %0": "=m"(esp):);
    esp += 17;

    Process* current = globalTaskManager.getCurrent();

    current->kstack.esp0 = esp[17];
    current->regs.ss = esp[16];
    current->regs.esp = esp[15];
    current->regs.eflags = esp[14];
    current->regs.cs = esp[13];
    current->regs.eip = esp[12];
    current->regs.eax = esp[11];
    current->regs.ecx = esp[10];
    current->regs.edx = esp[9];
    current->regs.ebx = esp[8];
    current->regs.ebp = esp[7];
    current->regs.esi = esp[6];
    current->regs.edi = esp[5];
    current->regs.ds = esp[4];
    current->regs.es = esp[3];
    current->regs.fs = esp[2];
    current->regs.gs = esp[1];

    globalTaskManager.switchToTask(0, KERNELMODE);
    */
}

int sys_nothing(int a)
{
/*
    //globalTaskManager.getCurrent();
    cli;
    for (int i=0;i<5000;i++)
    {
        if (!(i%100))
        {
            globalTerm.printf("i");
        }
    }
    globalTerm.printf("\n");

    sti;

    return a;
    */
}
