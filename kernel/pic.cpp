#include <pic.h>
#include <io.h>
#include <error.h>

PIC::PIC()
{
    frequency = 0;
    ready = false;
}

void PIC::init()
{
    // Init ICW1
    outb(0x20, 0x11);   // Master PIC
    outb(0xA0, 0x11);   // Slave PIC

    // Init ICW2
    outb(0x21, 0x20);	// Start vector = 32 (the 32 first are reserved to exceptions), range 0x20-0x27
    outb(0xA1, 0x70);	// Start vector = 96 (64 vectors further), range 0x70-0x77

    // Init ICW3
    outb(0x21, 0x04);
    outb(0xA1, 0x02);   // Pic esclave rataché à la patte 2

    // Init ICW4
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Mask interrupts
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    ready=true;
}

void PIC::setFrequency(unsigned int hz)
{
    if (!ready)
        error("PIC::setFrequency: The PIC is not ready, call init() first.\n");

    int divisor;
    if (hz != 0)
        divisor = 1193180 / hz;       // Calculate our divisor
    else
        divisor = 0;
    outb(0x43, 0x36);             // Set our command byte 0x36
    outb(0x40, divisor & 0xFF);   // Set low byte of divisor
    outb(0x40, divisor >> 8);     // Set high byte of divisor

    frequency = hz;
}

unsigned int PIC::getFrequency()
{
    return frequency;
}
