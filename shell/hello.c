int main()
{
    const char* str = "Hello from userland!\n";
    asm("   mov %0, %%rax\n\
            mov %1, %%rbx\n\
            syscall":"=m"(1),"=m"(str));
    return 0;
}
