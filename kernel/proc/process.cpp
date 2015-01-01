#include <proc/process.h>
#include <proc/tss.h>
#include <proc/elf.h>
#include <mm/memcpy.h>
#include <mm/memmap.h>
#include <mm/paging.h>
#include <arch/gdt.h>
#include <lib/llist.h>
#include <debug.h>
#include <error.h>

Process::Process()
{
    state = 0;
}

namespace TaskManager
{

static Process* pList; ///< Array of process. Fixed size of MAXPID+1
static Process* current;
static int nProc;
static bool schedulerEnabled;

void init()
{
    schedulerEnabled = true;

    nProc = 0;
    pList = new Process[MAXPID+1];

    // Init kernel thread
    current = &pList[0];
    current->pid = 0;
    current->state = 1;
    current->regs.cr3 = PML4_ADDR;
    //current->terminal = &gTerm;
    current->parent = current;

    // Init selectors
    int ss;
    asm("mov %%ss, %0":"=m"(ss));
    current->regs.ss = ss;
}

Process* getCurrent()
{
    return current;
}

int getNProc()
{
    return nProc;
}

void disableScheduler()
{
    schedulerEnabled = false;
}

void enableScheduler()
{
    schedulerEnabled = true;
}

int loadTask(const char* path, const int argc, const char **argv)
{
    Node* node = VFS::resolvePath(path);
    if (!node)
    {
        logE9("loadTask: File not found: %s\n",path);
        return 0;
    }
    else
        return loadTask(node, argc, argv);
}

int loadTask(Node *node, const int argc, const char **argv)
{
    u8* kstack;
    Process *previous;
    //Terminal *terminal;

    char **param=nullptr, **uparam=nullptr;
    u8* file;
    u64 stackp;
    u64 eEntry;

    int pid;
    int i;

    if (!node)
    {
        error("loadTask: Null node ptr. Probably a file not found.\n");
        return 0;
    }

    bool IF = gti; cli; // Screen operations should be atomic

    // Calcul du pid du nouveau processus. Assume qu'on n'atteindra jamais
    // la limite imposee par la taille de pList[]
    pid = 1;
    while (pList[pid].state != 0 && pid++ < MAXPID);
    if (pid >= MAXPID)
    {
        error("loadTask: No slot left for a new process, MAXPID=%d\n",MAXPID);
        if (IF) sti;
        return 0;
    }
    //error("Got PID %d for new task %s\n",pid,node->getName());

    nProc++;
    pList[pid].pid = pid;

    // On copie les arguments a passer au programme
    if (argc)
    {
        param = (char**) new char*[argc+1];
        for (i=0 ; i<argc ; i++)
        {
            param[i] = (char*) new char[strlen(argv[i]) + 1];
            strcpy(param[i], argv[i]);
        }
        param[i] = 0;
    }

    // Cree un repertoire de pages
    pList[pid].pml4 = Paging::createPageDir();

    // On change d'espace d'adressage pour passer sur celui du nouveau
    // processus.
    // Il faut faire pointer 'current' sur le nouveau processus afin que
    // les defauts de pages mettent correctement a jour les informations
    // de la struct process.
    //errorHandler::error("Starting task %d from task %d\n",pid,current->pid);
    previous = current;
    current = &pList[pid];
    asm("mov %0, %%rax; mov %%rax, %%cr3"::"m"(pList[pid].pml4));

    // Charge l'executable en memoire. En cas d'echec, on libere les
    // ressources precedement allouees.
    //node->open();
    file = new u8[node->getSize()];
    node->read(node->getSize(), file);
    //node->close();
    eEntry = (u64) Elf::loadElf(file, &pList[pid]); // Load elf in memory and return entry point in memory.
    delete[] file;
    if (eEntry == 0) // echec
    {
        for (i=0 ; i<argc ; i++)
            delete param[i];
        delete param;
        current = previous;
        asm("mov %0, %%rax ;mov %%rax, %%cr3"::"m" (current->regs.cr3));
        Paging::destroyPageDir(pList[pid].pml4);
        error("loadTask: Unable to load task in memory\n");
        if (IF) sti;
        return 0;
    }

    // Create a user stack. Page faults will allocate a stack as needed, but might as well
    // map the first pages properly
    stackp = USER_STACK - 16;
    Paging::mapPage((u8*)(stackp &~ 0xFFFUL), nullptr, PAGE_ATTR_USER, current->pml4);
    *(u64*)(stackp) = stackp+8;
    *(u64*)(stackp+8) = 0xCCCC30CD40C03193; // x86 shellcode: sysExit interrupt
    //*(u64*)(stackp+8) = 0xCC30CDC0FFC03193; // x86_64 shellcode: sysExit interrupt

    // Copie des parametres sur la pile utilisateur
    if (argc)
    {
        uparam = new char*[argc];
        for (i=0 ; i<argc ; i++)
        {
            stackp -= (strlen(param[i]) + 1);
            strcpy((char*) stackp, param[i]);
            uparam[i] = (char*) stackp;
        }

        stackp &= 0xFFFFFFF0;		// alignement

        // Creation des arguments de main() : argc, argv[]...
        stackp -= sizeof(char*);
        *((char**) stackp) = 0;

        for (i=argc-1 ; i>=0 ; i--) // argv[0] a argv[n]
        {
            stackp -= sizeof(char*);
            *((char**) stackp) = uparam[i];
        }

        stackp -= sizeof(char*);	// argv
        *((char**) stackp) = (char*) (stackp + 4);

        stackp -= sizeof(char*);	// argc
        *((int*) stackp) = argc;

        stackp -= sizeof(char*);

        for (i=0 ; i<argc ; i++)
            delete param[i];

        delete param;
        delete uparam;
    }

    // Cree la pile noyau
    kstack = Paging::mapPage(nullptr,nullptr,0,current->pml4);

    // On associe un terminal au processus
    //terminal = new VGAText(); // Creer un nouveau Terminal, de type VGAText

    // Initialise le reste des registres et des attributs
    pList[pid].regs.ss = GDT::getUserStackSelector();
    pList[pid].regs.rsp = stackp;
    pList[pid].regs.eflags = 0x0;
    pList[pid].regs.cs = GDT::getUserCode32Selector();
    pList[pid].regs.rip = eEntry;
    pList[pid].regs.ds = GDT::getUserDataSelector();
    pList[pid].regs.es = GDT::getUserDataSelector();
    pList[pid].regs.fs = GDT::getUserDataSelector();
    pList[pid].regs.gs = GDT::getUserDataSelector();

    pList[pid].regs.cr3 = (u64) pList[pid].pml4;

    pList[pid].kstack.ss0 = GDT::getDefaultSS();
    pList[pid].kstack.rsp0 = (u64) (kstack + PAGESIZE - 16);

    pList[pid].regs.rax = 0;
    pList[pid].regs.rcx = 0;
    pList[pid].regs.rdx = 0;
    pList[pid].regs.rbx = 0;

    pList[pid].regs.rbp = 0;
    pList[pid].regs.rsi = 0;
    pList[pid].regs.rdi = 0;

    // Attributs deja initialises par load_elf():
    // pList[pid].b_exec;
    // pList[pid].e_exec;
    // pList[pid].b_bss;
    // pList[pid].e_bss;
    pList[pid].bHeap = (u8*) ((u64) pList[pid].eBss & 0xFFFFF000) + PAGESIZE;
    pList[pid].eHeap = pList[pid].bHeap;

    //pList[pid].pwd = node->getParent(); // Working directory is containing folder.

    if (previous->state != 0)
        pList[pid].parent = previous;
    else
        pList[pid].parent = &pList[0];

    /*
    // Ajoute toi aux enfants de ton parent.
    if (previous->state != 0)
        previous->childs << &pList[pid];
    else
        pList[0].childs << &pList[pid];
    */

    //pList[pid].terminal = terminal;

    /*
    pList[pid].signal = 0;
    for(i=0 ; i<32 ; i++)
        pList[pid].sigfn[i] = (char*) SIG_DFL;
    */

    //pList[pid].status = 0;

    pList[pid].state = 1;

    current = previous;
    asm("mov %0, %%rax ;mov %%rax, %%cr3":: "m"(current->regs.cr3));

    if (IF) sti;
    return pid;
}

void exitTask(int status)
{
    u16 kss;
    u64 krsp;
    Page *page;
    //llist<struct Process::openFile*> fd;
    //Process *proc;
    //error("Exiting task %d\n",current->pid);

    if (!current->pid)
        fatalError("The kernel task was terminated, halting.\n");

    cli; // The point of noreturn!

    nProc--;
    current->state = -1;
    current->status = status;

    // Liberation des ressources memoire allouees au processus :
    // - les pages utilisees par le code executable
    // - la pile utilisateur
    // - la pile noyau
    // - le repertoire et les tables de pages

    // Libere la memoire occupee par le processus
    for (;current->pageList.size();)
    {
        page = current->pageList[0];
        Paging::unmapPage(page->vAddr);
        current->pageList.removeFirst();
        delete page;
    }

    // Libere la liste des fichiers ouverts
    /*
    fd = current->openFiles;
    while(fd.size())
    {
        fd[0]->file->close();
        globalPaging.kfree(fd[0]);
        fd.removeFirst();
    }

    // Met a jour la liste des processus du pere
    if (current->parent->state > 0)
        setSignal(&current->parent->signal, SIGCHLD);
    else
        errorHandler::error("sys_exit(): process %d without valid parent\n", current->pid);

    // Donne un nouveau pere aux enfants
    while (current->childs.size())
    {
        proc = current->childs[0];
        proc->parent = &pList[0];

        current->childs.removeFirst();
        pList[0].childs << proc;
    }

    // Libere la console
    globalPaging.kfree(current->terminal);
    current->terminal = 0;
    */

    // Libere la pile noyau
    kss = pList[0].regs.ss;
    krsp = pList[0].regs.rsp;

    current->state = 0;

    asm("mov %0, %%ss; mov %1, %%rsp; "::"m"(kss), "m"(krsp):"rsp");
    Paging::unmapPage((u8*)((u64) current->kstack.rsp0 & 0xFFFFF000));

    asm("mov %0, %%rax; mov %%rax, %%cr3"::"g"(PML4_ADDR):"rax");
    Paging::destroyPageDir(current->pml4);

    switchToTask(0, KERNELMODE); // We just unmapped our own stack, better be noreturn
}

void schedule()
{
    if (!schedulerEnabled)
        return;

    Process *p;
    u64 *stack_ptr;
    u64 i, newpid;

    // Stocke dans stack_ptr le pointeur vers les registres sauvegardes
    asm("mov %%rbp, %0": "=m"(stack_ptr):);

    // Si il n'y a aucun processus, on retourne directement
    if (!nProc) {
        //globalTerm.print(".");
        return;
    }

    // Si il y a un seul processus, on le laisse tourner
    else if (nProc == 1 && current->pid != 0) {
        //globalTerm.print(",");
        return;
    }

    // Si il y a au moins deux processus, on commute vers le suivant
    else
    {
        //VGAText::print(";");
        // Sauver les registres du processus courant
        current->regs.eflags = stack_ptr[17];
        current->regs.cs = stack_ptr[16];
        current->regs.rip = stack_ptr[15];
        current->regs.rax = stack_ptr[14];
        current->regs.rcx = stack_ptr[13];
        current->regs.rdx = stack_ptr[12];
        current->regs.rbx = stack_ptr[11];
        current->regs.rbp = stack_ptr[10];
        current->regs.rax = 0xAAAABBBB;
        current->regs.rsi = stack_ptr[9];
        current->regs.rdi = stack_ptr[8];
        current->regs.r8 = stack_ptr[7];
        current->regs.r9 = stack_ptr[6];
        current->regs.r10 = stack_ptr[5];
        current->regs.r11 = stack_ptr[4];
        current->regs.r12 = stack_ptr[3];
        current->regs.r13 = stack_ptr[2];
        current->regs.r14 = stack_ptr[1];
        current->regs.r15 = stack_ptr[0];
        u16 sreg;
        asm("mov %%ds, %%rax; mov %%rax, %0"::"m"(sreg):"rax");
        current->regs.ds = sreg;
        asm("mov %%es, %%rax; mov %%rax, %0"::"m"(sreg):"rax");
        current->regs.es = sreg;
        asm("mov %%fs, %%rax; mov %%rax, %0"::"m"(sreg):"rax");
        current->regs.fs = sreg;
        asm("mov %%gs, %%rax; mov %%rax, %0"::"m"(sreg):"rax");
        current->regs.gs = sreg;
        //error("SCHEDULER EFLAGS:0x%x, CS:0x%x, EIP:0x%x\n",current->regs.eflags,current->regs.cs,current->regs.rip);

         // Sauvegarde le contenu des registres de pile (ss, esp)
         // au moment de l'interruption. Necessaire car le processeur
         // empile ou non ces valeurs selon le contexte de l'interruption.
        if (current->regs.cs != 0x08) // user mode
        {
            current->regs.rsp = stack_ptr[18];
            current->regs.ss = stack_ptr[19];
        }
        else // pendant un appel systeme
        {
            //current->regs.rsp = stack_ptr[9] + 12;	// vaut : &stack_ptr[18]
            // current->regs.ss =
            current->regs.rsp = stack_ptr[18];
            current->regs.ss = GDT::getDefaultSS();
        }

        // Sauver le TSS de l'ancien processus
        current->kstack.ss0 = GDT::getDefaultSS();
        current->kstack.rsp0 = TSS::getDefaultTss()->rsp0;
    }

    // Choix du nouveau processus (un simple roundrobin)
    newpid = 0;
    for (i = current->pid + 1; i < MAXPID && newpid == 0; i++)
    {
        if (pList[i].state == 1)
            newpid = i;
    }

    if (!newpid) {
        for (i = 1; i < current->pid && newpid == 0; i++)
        {
            if (pList[i].state == 1)
                newpid = i;
        }
    }

    p = &pList[newpid];

    //VGAText::printf("Prepare to switch to process %d\n", p->pid); // Debug
    //VGAText::printf("New ss:0x%x, esp:0x%x, eip:0x%x\n", p->regs.ss, p->regs.rsp, p->regs.rip); // Debug
    //VGAText::printf("Cur ss:0x%x, esp:0x%x, eip:0x%x\n", current->regs.ss, current->regs.rsp, current->regs.rip); // Debug

    // Commutation
    if (p->regs.cs == 0x08) // Segment 1 : Kernel code
        switchToTask(p->pid, KERNELMODE);
    else
        switchToTask(p->pid, USERMODE);
}

}

// switchToTask(): Prepare la commutation de tache effectuee par doTaskSwitch().
// Le premier parametre indique le pid du processus a charger.
// Le mode indique si ce processus etait en mode utilisateur ou en mode kernel
// quand il a ete precedement interrompu par le scheduler.
// L'empilement des registres sur la pile change selon le cas.
void TaskManager::switchToTask(int n, int mode)
{
    //VGAText::printf("Switching to task %d mode %d !\n",n,mode);
    //int sig;

    TSS* defaultTss = TSS::getDefaultTss();

    // Commence le changement de processus
    current = &pList[n];

    /*
    // Traite les signaux
    if ((sig = Signal::dequeueSignal(current->signal)))
        Signal::handleSignal(sig);
    */

    // Load the new process's TSS
    //defaultTss->ss0 = current->kstack.ss0;
    defaultTss->rsp0 = current->kstack.rsp0;

    // Prepare le changement de pile noyau
    if (mode == USERMODE)
        doTaskSwitch(current, current->kstack.ss0, current->kstack.rsp0);
    else // KERNELMODE
        doTaskSwitch(current, current->regs.ss, current->regs.rsp);
}

