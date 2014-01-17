#ifndef SYSCALLS_H_INCLUDED
#define SYSCALLS_H_INCLUDED

#define SYS_SCAN_MAX 4096

namespace Dusk
{
	void sys_exit(int);
	int sys_open(char*);
	char* sys_sbrk(int);
	int sys_exec(char *, char **);
	int sys_scan(char*);
	int sys_wait(int*);
	void sys_sigreturn(void);
	int sys_nothing(int);
}

#endif // SYSCALLS_H_INCLUDED
