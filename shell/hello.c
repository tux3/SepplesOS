int main()
{
    const char* str = "Hello from userland!\n";
    asm("   mov $0x10001, %%eax\n\
            mov %0, %%ebx\n\
            int $0x30"::"m"(str):"eax","ebx");
    return 0;
}
