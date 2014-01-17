#include <power.h>
#include <error.h>
#include <io.h>
#include <std/types.h>

// Attendre quelques microsecondes (environ)
static void udelay(int loops)
{
	const u16 DELAY_PORT = 0x80;
    while (loops--)
        asm volatile("outb %%al,%0" : : "dN" (DELAY_PORT)); // Around 1µs
}

void halt(void)
{
    cli;
	asm("hlt");

	// Should never go there (bochs may !)
	error("Computer woke up, looping...\n");
	while(1);

}

void reboot(void)
{
    // 1.Keyboard reboot (Ask the CPU to ask the Keyboard to ask the BIOS to reboot !)
    int i;
    for (i = 0; i < 10; i++)
    {
        int j;
        for (j = 0; j < 0x10000; j++)
        {
            if ((inb(0x64) & 0x02) == 0)
                break;
            udelay(2);
        }
        udelay(50);
        outb(0xfe, 0x64); /* pulse reset low */
        udelay(50);
    }
    //globalTerm.print("Keyboard reboot failure\n");


    // 2.CF9 reboot
    u8 cf9 = inb(0xcf9) & ~6;
    outb(cf9|2, 0xcf9); /* Request hard reset */
    udelay(50);
    outb(cf9|6, 0xcf9); /* Actually do the reset */
    udelay(50);
    //globalTerm.print("CF9 reboot failure\n");

    //globalTerm.print("Unable to reboot, last try ...\n");
	//globalTerm.print("Triplefaulting happilly\n");
	cli;
    for (u16 i=0; i<1024; i++) // Erase the IDT and GDT
       *((u64*)(0x80000 + i)) = 0;
    sti;
    asm("int $0"::);
	//fatalError("OH SHI-");
}
