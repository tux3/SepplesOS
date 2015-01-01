#include "sepples.h"

void exit(int status)
{
	asm("   mov $1, %%eax\n\
            mov %0, %%ebx\n\
            int $0x30"::"m"(status):"eax","ebx");
}

int run(char* path)
{
	int ret;
	asm("   mov $0x10003, %%eax\n\
            mov %1, %%ebx\n\
            int $0x30\n\
            mov %%eax, %0":"=m"(ret):"m"(path):"eax","ebx");
	return ret;
}

void print(const char* str)
{
	asm("   mov $0x10001, %%eax\n\
            mov %0, %%ebx\n\
            int $0x30"::"m"(str):"eax","ebx");
}

void nget(char* buf, unsigned n)
{
	asm("   mov $0x10002, %%eax\n\
            mov %0, %%ebx\n\
            mov %1, %%ecx\n\
            int $0x30"::"m"(buf),"m"(n):"eax","ebx");
}

