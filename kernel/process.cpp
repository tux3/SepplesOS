#include <process.h>
#include <error.h>
#include <paging.h>
#include <elf.h>
#include <screen.h>
#include <signal.h>
#include <gdt.h>
//#include <disk/ext2.h>
//#include <disk/filesystem.h>
#include <syscalls.h>

/// TaskManager
TaskManager::TaskManager()
{
    nProc = 0;
    pList = new Process[MAXPID+1];
}

void TaskManager::init()
{
    // Init kernel thread
    current = &pList[0];
    current->pid = 0;
    current->state = 1;
    current->regs.cr3 = (u32) gPaging.getPageDir0();
//    current->terminal = &gTerm;
    current->parent = current;

    // Init selectors
    int ss;
    asm("mov %%ss, %0":"=m"(ss));
    current->regs.ss = ss;
}

/*
int TaskManager::loadTask(const char* path)
{
    return loadTask(path,0,0);
}

int TaskManager::loadTask(const char* path, const int argc, const char **argv)
{
    IO::Disk::Node* node = globalFilesystem.pathToNode(path);
    if (!node)
    {
        errorHandler::error("Can't load task, file not found.\n");
        return 0;
    }
    else
        return loadTask(node, argc, argv);
}

int TaskManager::loadTask(struct Dusk::IO::Disk::Node *node, const int argc, const char **argv)
{
    struct page *kstack;
    Process *previous;
    Dusk::IO::Terminal *terminal;

    char **param, **uparam;
    char *file;
    uint32 stackp;
    uint32 eEntry;

    int pid;
    int i;

    if (!node)
    {
        errorHandler::error("loadTask: Null node ptr. Probably a file not found.\n");
        return 0;
    }

    // Calcul du pid du nouveau processus. Assume qu'on n'atteindra jamais
    // la limite imposee par la taille de pList[]
    // FIXME: reutiliser slots libres
    pid = 1;
    while (pList[pid].state != 0 && pid++ < MAXPID);
    if (pList[pid].state != 0)
    {
        errorHandler::error("load_task(): Not enough slot for processes, MAXPID=%d\n",MAXPID);
        return 0;
    }

    cli; /// <TEST> Not a good time to let the scheduler switch to another task

    nProc++;
    pList[pid].pid = pid;

    // On copie les arguments a passer au programme
    if (argc)
    {
        param = (char**) globalPaging.kmalloc(sizeof(char*) * (argc+1));
        for (i=0 ; i<argc ; i++)
        {
            param[i] = (char*) globalPaging.kmalloc(strlen(argv[i]) + 1);
            strcpy(param[i], argv[i]);
        }
        param[i] = 0;
    }

    // Cree un repertoire de pages
    pList[pid].pageDir = globalPaging.pageDirCreate();


    // On change d'espace d'adressage pour passer sur celui du nouveau
    // processus.
    // Il faut faire pointer 'current' sur le nouveau processus afin que
    // les defauts de pages mettent correctement a jour les informations
    // de la struct process.
    //errorHandler::error("Starting task %d from task %d\n",pid,current->pid);
    previous = current;
    current = &pList[pid];
    asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(pList[pid].pageDir->base->pAddr));

    // Charge l'executable en memoire. En cas d'echec, on libere les
    // ressources precedement allouees.
    node->open();
    file = (char*) globalPaging.kmalloc(sizeof(char) * node->getSize());
    node->read(file, node->getSize());
    node->close();
    eEntry = (uint32) Dusk::Elf::loadElf(file, &pList[pid]); // Load elf in memory and return entry point in memory.
    globalPaging.kfree(file);
    if (eEntry == 0) // echec
    {
        for (i=0 ; i<argc ; i++)
            globalPaging.kfree(param[i]);
        globalPaging.kfree(param);
        current = previous;
        asm("mov %0, %%eax ;mov %%eax, %%cr3"::"m" (current->regs.cr3));
        globalPaging.pageDirDestroy(pList[pid].pageDir);
        errorHandler::error("loadTask: Unable to load task in memory\n");
        return 0;
    }

    // Cree la pile utilisateur. La memoire est allouee dynamiquement a
    // chaque defaut de page.
    stackp = USER_STACK - 16;

    // Copie des parametres sur la pile utilisateur
    if (argc)
    {
        uparam = (char**) globalPaging.kmalloc(sizeof(char*) * argc);
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
            globalPaging.kfree(param[i]);

        globalPaging.kfree(param);
        globalPaging.kfree(uparam);
    }

    // Cree la pile noyau
    kstack = globalPaging.getPageFromHeap();

    // On associe un terminal au processus
    terminal = new Dusk::IO::Video::VGAText(); // Creer un nouveau Terminal, de type VGAText

    // Initialise le reste des registres et des attributs
    pList[pid].regs.ss = 0x33;
    pList[pid].regs.esp = stackp;
    pList[pid].regs.eflags = 0x0;
    pList[pid].regs.cs = 0x23;
    pList[pid].regs.eip = eEntry;
    pList[pid].regs.ds = 0x2B;
    pList[pid].regs.es = 0x2B;
    pList[pid].regs.fs = 0x2B;
    pList[pid].regs.gs = 0x2B;

    pList[pid].regs.cr3 = (uint32) pList[pid].pageDir->base->pAddr;

    pList[pid].kstack.ss0 = 0x18;
    pList[pid].kstack.esp0 = (uint32) (kstack->vAddr + PAGESIZE - 16);

    pList[pid].regs.eax = 0;
    pList[pid].regs.ecx = 0;
    pList[pid].regs.edx = 0;
    pList[pid].regs.ebx = 0;

    pList[pid].regs.ebp = 0;
    pList[pid].regs.esi = 0;
    pList[pid].regs.edi = 0;

    // Attributs deja initialises par load_elf():
    // pList[pid].b_exec;
    // pList[pid].e_exec;
    // pList[pid].b_bss;
    // pList[pid].e_bss;
    pList[pid].bHeap = (char*) ((uint32) pList[pid].eBss & 0xFFFFF000) + PAGESIZE;
    pList[pid].eHeap = pList[pid].bHeap;

    pList[pid].pwd = node->getParent(); // Working directory is containing folder.

    if (previous->state != 0)
        pList[pid].parent = previous;
    else
        pList[pid].parent = &pList[0];


    // Ajoute toi aux enfants de ton parent.
    if (previous->state != 0)
        previous->childs << &pList[pid];
    else
        pList[0].childs << &pList[pid];

    pList[pid].terminal = terminal;

    pList[pid].signal = 0;
    for(i=0 ; i<32 ; i++)
        pList[pid].sigfn[i] = (char*) SIG_DFL;

    pList[pid].status = 0;

    pList[pid].state = 1;

    current = previous;
    asm("mov %0, %%eax ;mov %%eax, %%cr3":: "m"(current->regs.cr3));

    sti; /// <TEST> Restart scheduler
    //schedule();/// <TEST> force switch now

    return pid;
}

void TaskManager::exitTask(int status)
{
    uint16 kss;
    uint32 kesp;
    //struct list_head *p, *n;
    struct page *page;
    //struct Dusk::IO::Disk::open_file *fd, *fdn;
    llist<struct Process::openFile*> fd;
    Process *proc;

    //errorHandler::error("Exiting task %d\n",current->pid);

    if (!current->pid) // Killing the kernel task ? Umad ?
        errorHandler::fatalError("The kernel task was terminated, halting.\n");

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
        globalPaging.releasePageFrame((uint32)page->pAddr);
        current->pageList.removeFirst();
        globalPaging.kfree(page);
    }

    // Libere la liste des fichiers ouverts
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

    // Libere la pile noyau
    kss = pList[0].regs.ss;
    kesp = pList[0].regs.esp;

    /// We switched the stack manually, thus we know the value of ESP, so we can use it as a pointer to fetch things on the stack since DS and SS references the same memory !
    /// So we know that "status" is "EBP+12", we set EBP = (ESP+X)+12, and we use status to fetch things on the stack !
    asm("mov %0, %%ss; mov %1, %%esp; "::"m"(kss), "m"(kesp)); // Switch stack (TODO CHECK IF DANGEROUS WE ASSUME kss WILL BE THE SAME SEGMENT: WE STILL FETCH kesp ON THE STACK, BUT AFTER SWITCHING SS !)

    /// Mov return value on stack and push 2 arguments
    asm("mov %ebp, %eax; add $4, %eax; mov (%eax), %ebx; mov %ebx, (%esp); sub $16, %esp");
    *((uint32*)(kesp-4)) = (uint32)globalPaging.getPageDir0();
    *((uint32*)(kesp-8)) = (uint32)current->pageDir;

    globalPaging.releasePageFromHeap((char *) ((uint32) current->kstack.esp0 & 0xFFFFF000));

    asm("add $8, %esp; mov %esp, %ebp"); // Switch stack frame (BECAUSE IT NEEDS TO BE DONE MANUALLY IN C++ FOR SOME REASEON)

    // Libere le repertoire de pages et les tables associees (Using local var status as an ESP pointer !)
    asm("sub $8, %ebp");
    asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(status)); // status = globalPaging.getPageDir0();
    asm("sub $4, %ebp");
    globalPaging.pageDirDestroy((pageDirectory)status); // status = current->pageDir

    asm("add $8, %esp; ret;");
}

void TaskManager::schedule(void)
{
    Process *p;
    uint32 *stack_ptr;
    int i, newpid;

    // Stocke dans stack_ptr le pointeur vers les registres sauvegardes
    asm("mov (%%ebp), %%eax; mov %%eax, %0": "=m"(stack_ptr):);

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
        globalTerm.print(";");
        // Sauver les 14 registres du processus courant
        current->regs.eflags = stack_ptr[16];
        current->regs.cs = stack_ptr[15];
        current->regs.eip = stack_ptr[14];
        current->regs.eax = stack_ptr[13];
        current->regs.ecx = stack_ptr[12];
        current->regs.edx = stack_ptr[11];
        current->regs.ebx = stack_ptr[10];
        current->regs.ebp = stack_ptr[8];
        current->regs.esi = stack_ptr[7];
        current->regs.edi = stack_ptr[6];
        current->regs.ds = stack_ptr[5];
        current->regs.es = stack_ptr[4];
        current->regs.fs = stack_ptr[3];
        current->regs.gs = stack_ptr[2];
        errorHandler::error("SCHEDULER EFLAGS:0x%x, CS:0x%x, EIP:0x%x\n",current->regs.eflags,current->regs.cs,current->regs.eip);
        BOCHSBREAK

         // Sauvegarde le contenu des registres de pile (ss, esp)
         // au moment de l'interruption. Necessaire car le processeur
         // empile ou non ces valeurs selon le contexte de l'interruption.
        if (current->regs.cs != 0x08) // mode utilisateur
        {
            current->regs.esp = stack_ptr[17];
            current->regs.ss = stack_ptr[18];
        }
        else // pendant un appel systeme
        {
            current->regs.esp = stack_ptr[9] + 12;	// vaut : &stack_ptr[17]
            current->regs.ss = globalGdt.getDefaultTss()->ss0;
        }

        // Sauver le TSS de l'ancien processus
        current->kstack.ss0 = globalGdt.getDefaultTss()->ss0;
        current->kstack.esp0 = globalGdt.getDefaultTss()->esp0;
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

    //globalTerm.printf("Prepare to switch to process %d\n", p->pid); // Debug
    //globalTerm.printf("New ss:0x%x, esp:0x%x, eip:0x%x\n", p->regs.ss, p->regs.esp, p->regs.eip); // Debug
    //globalTerm.printf("Cur ss:0x%x, esp:0x%x, eip:0x%x\n", current->regs.ss, current->regs.esp, current->regs.eip); // Debug

    // Commutation
    if (p->regs.cs == 0x08) // Segment 1 : Kernel code
        switchToTask(p->pid, KERNELMODE);
    else
        switchToTask(p->pid, USERMODE);
}


// switch_to_task(): Prepare la commutation de tache effectuee par do_switch().
// Le premier parametre indique le pid du processus a charger.
// Le mode indique si ce processus etait en mode utilisateur ou en mode kernel
// quand il a ete precedement interrompu par le scheduler.
// L'empilement des registres sur la pile change selon le cas.
void TaskManager::switchToTask(int n, int mode)
{
    globalTerm.printf("Switching to task %d mode %d !\n",n,mode);
    BOCHSBREAK

    uint32 kesp, eflags;
    uint32 kss, ss, cs;
    int sig;

    struct tss* defaultTss = globalGdt.getDefaultTss();

    // Commence le changement de processus
    current = &pList[n];

    // Traite les signaux
    if ((sig = Signal::dequeueSignal(current->signal)))
    {
        Signal::handleSignal(sig);
    }

    // Charge le TSS du nouveau processus
    defaultTss->ss0 = current->kstack.ss0;
    defaultTss->esp0 = current->kstack.esp0;

    // Empile les registres ss, esp, eflags, cs et eip necessaires a la
    // commutation. Ensuite, la fonction do_switch() restaure les
    // registres, la table de page du nouveau processus courant et commute avec l'instruction iret.
    ss = current->regs.ss;
    cs = current->regs.cs;
    eflags = (current->regs.eflags | 0x200) & 0xFFFFBFFF;

    // Prepare le changement de pile noyau
    if (mode == USERMODE)
    {
        kss = current->kstack.ss0;
        kesp = current->kstack.esp0;

        asm("mov %0, %%ss;"::"m"(kss));
        char* stackPtr = (char*)kesp;
        stackPtr-=4;
        memcpy(stackPtr, (char*)&ss, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&current->regs.esp, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&eflags, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&cs, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&current->regs.eip, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&current, sizeof(uint32));
        //globalTerm.print("Stack ready, jumping !\n");

        asm("mov %0, %%esp; \
        ljmp $0x08, $do_switch" \
        ::"m"(stackPtr));
    }
    else // KERNELMODE
    {
        kss = current->regs.ss;
        kesp = current->regs.esp;

        //errorHandler::error("SWITCH EFLAGS:0x%x, CS:0x%x, EIP:0x%x\n",current->regs.eflags,current->regs.cs,current->regs.eip);

        asm("mov %0, %%ss;"::"m"(kss));
        char* stackPtr = (char*)kesp;
        stackPtr-=4;
        memcpy(stackPtr, (char*)&eflags, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&cs, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&current->regs.eip, sizeof(uint32));
        stackPtr-=4;
        memcpy(stackPtr, (char*)&current, sizeof(uint32));
        //globalTerm.print("Stack ready, jumping !\n");

        asm("mov %0, %%esp; \
        ljmp $0x08, $do_switch" \
        ::"m"(stackPtr));
    }
}

*/

Process* TaskManager::getCurrent()
{
    return current;
}

int TaskManager::getNProc()
{
    return nProc;
}
