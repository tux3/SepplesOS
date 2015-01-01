#include <disk/disk.h>
#include <arch/pinio.h>
#include <mm/malloc.h>
#include <mm/memcpy.h>
#include <vga/vgatext.h>
#include <error.h>
#include <debug.h>

namespace ATA
{

/**
 *
 * Ports :
 * 0x1F0 - Data
 * 0x1F1 - Deprecated, send a zero if you want, it's ignored
 * 0x1F2 - Sector count
 * 0x1F3 - Low LBA
 * 0x1F4 - Mid LBA
 * 0x1F5 - High LBA
 * 0x1F6 - Drive ID, magic, high high LBA
 * 0x1F7 - Status read/command write (Command Block register)
 * 0x3F6 - Device control write/Alt status read
 *
 * Status byte :
 * BSY - DRDY - DF - <Command dependant> - DRQ - Obsolete - Obsolete - ERR
 *
 * If BSY is one, the device has control of the register, we shouldn't write anything
 * or this is UD, and the other bits are not valid
 *
 * If BSY is zero, we can write to the Command Block register and the device shouldn't edit anything
 *
 **/

// Configuration data for device 0 and 1
// returned by the reg_config() function.
static int regConfigInfo[2];
#define REG_CONFIG_TYPE_NONE  0
#define REG_CONFIG_TYPE_UNKN  1
#define REG_CONFIG_TYPE_ATA   2
#define REG_CONFIG_TYPE_ATAPI 3

#define CB_DATA  0x1F0   // data reg         in/out cmd_blk_base1+0
#define CB_ERR   0x1F1   // error            in     cmd_blk_base1+1
#define CB_FR    0x1F1   // feature reg         out cmd_blk_base1+1
#define CB_SC    0x1F2   // sector count     in/out cmd_blk_base1+2
#define CB_SN    0x1F3   // sector number/low LBA    in/out cmd_blk_base1+3
#define CB_CL    0x1F4   // cylinder low/med LBA     in/out cmd_blk_base1+4
#define CB_CH    0x1F5   // cylinder high/high LBA    in/out cmd_blk_base1+5
#define CB_DR    0x1F6   // device register: DEV ID+high LBA+LBA magic      in/out cmd_blk_base1+6
#define CB_STAT  0x1F7   // primary status   in     cmd_blk_base1+7
#define CB_CMD   0x1F7   // command             out cmd_blk_base1+7
#define CB_ASTAT 0x3F6   // alternate status in     ctrl_blk_base2+6
#define CB_DC    0x3F6   // device control      out ctrl_blk_base2+6

// device control reg (CB_DC) bits
#define CB_DC_HOB    0x80  // High Order Byte (48-bit LBA)
#define CB_DC_SRST   0x04  // soft reset
#define CB_DC_NIEN   0x02  // disable interrupts

// bits 7-4 of the device (CB_DR) register
#define CB_DH_LBA  0x40    // LBA bit
#define CB_DH_DEV0 0x00    // select device 0
#define CB_DH_DEV1 0x10    // select device 1

#define CB_STAT_BSY  0x80  // busy
#define CB_STAT_RDY  0x40  // ready
#define CB_STAT_DF   0x20  // device fault
#define CB_STAT_WFT  0x20  // write fault (old name)
#define CB_STAT_SKC  0x10  // seek complete (only SEEK command)
#define CB_STAT_SERV 0x10  // service (overlap/queued commands)
#define CB_STAT_DRQ  0x08  // data request
#define CB_STAT_CORR 0x04  // corrected (obsolete)
#define CB_STAT_IDX  0x02  // index (obsolete)
#define CB_STAT_ERR  0x01  // error (ATA)
#define CB_STAT_CHK  0x01  // check (ATAPI)

#define TIMEOUT 0x100000     // If a loop waiting for a device spins longer than that, it's a timeout

void wait400ns()
{
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
}

// Wait until !BSY or until we timeout, return true if !BSY
bool waitReady()
{
    for (u64 wait=0;;wait++)
    {
       u8 status = inb(CB_ASTAT);
       if ((status & CB_STAT_BSY) == 0)
          return true;
       if (wait >= TIMEOUT)
       {
          error("waitReady: Timed out with status %p\n",status);
          return false;
       }
    }
    fatalError("waitReady: Reached unreachable code!\n");
}

// Sends a soft reset on the bus to go back to a safe state
void softReset()
{
    u8 sc, sn, status;

    // Set and then reset the soft reset bit in the Device Control
    // register.  This causes device 0 be selected.
    outb(CB_DC, (u8)(CB_DC_NIEN | CB_DC_SRST));
    wait400ns();
    outb(CB_DC, CB_DC_NIEN);
    wait400ns();

    // If there is a device 0, wait for device 0 to set BSY=0.
    if (regConfigInfo[0] != REG_CONFIG_TYPE_NONE)
    {
        for(u64 wait=0;;wait++)
        {
            status = inb(CB_STAT);
            if ((status & CB_STAT_BSY) == 0)
                break;
            if (wait >= TIMEOUT)
            {
                error("softReset: Device 0 timed out\n");
                break;
            }
        }
    }

    // If there is a device 1, wait until device 1 allows
    // register access.
    if (regConfigInfo[1] != REG_CONFIG_TYPE_NONE)
    {
        u64 wait=0;
        for(;;wait++)
        {
            outb(CB_DR, CB_DH_DEV0);
            wait400ns();
            sc = inb(CB_SC);
            sn = inb(CB_SN);
            if ((sc == 0x01) && (sn == 0x01))
                break;
            if (wait >= TIMEOUT)
            {
                error("softReset: Device 1 timed out\n");
                break;
            }
        }

        // Now check if drive 1 set BSY=0.
        if (wait < TIMEOUT)
            if (inb(CB_STAT) & CB_STAT_BSY)
                error("softReset: Device 1 is still busy after reset\n");
    }
}

// Selects a device and checks that it reacts correctly
bool selectDevice(int devId)
{
    u8 status;

    if (devId > 1)
        fatalError("selectDevice: Trying to select out-of-bounds device %d !\n",devId);

    if (regConfigInfo[devId] < REG_CONFIG_TYPE_ATA)
    {
        error("selectDevice: Trying to select unknown or undetected device %d, this might not work\n", devId);

        outb(CB_DR, (u8)(devId ? CB_DH_DEV1 : CB_DH_DEV0));
        wait400ns();
        return true; // Hopefully works, but hey, you asked for it
    }

    // We don't know which drive is currently selected but we should
    // wait until BSY=0 and DRQ=0. Normally both BSY=0 and DRQ=0
    // unless something is very wrong!
    for(u64 wait=0;;wait++)
    {
        status = inb(CB_STAT);
        if ((status & (CB_STAT_BSY | CB_STAT_DRQ)) == 0)
            break;
        if (wait>TIMEOUT)
        {
            error("selectDevice: Timed-out waiting for current device, forcing a soft-reset and aborting\n");
            softReset();
            return false;
        }
    }

    // Here we select the drive we really want to work with by
    // setting the DEV bit in the Drive/Head register.
    outb(CB_DR, (u8)(devId ? CB_DH_DEV1 : CB_DH_DEV0));
    wait400ns();

    // Wait for the selected device to have BSY=0 and DRQ=0.
    // Normally the drive should be in this state unless
    // something is very wrong (or initial power up is still in
    // progress).
    for(u64 wait=0;;wait++)
    {
        status = inb(CB_STAT);
        if ((status & (CB_STAT_BSY | CB_STAT_DRQ)) == 0)
            break;
        if (wait>TIMEOUT)
        {
            error("selectDevice: Timed-out waiting for selected device, forcing a soft-reset and aborting\n");
            softReset();
            return false;
        }
    }
    return true;
}

void init()
{
    logE9("ATA init\n");

    // First assume we don't have any device
    regConfigInfo[0] = REG_CONFIG_TYPE_NONE;
    regConfigInfo[1] = REG_CONFIG_TYPE_NONE;

    unsigned char sc;
    unsigned char sn;
    unsigned char cl;
    unsigned char ch;
    unsigned char st;

    outb(CB_DC, CB_DC_NIEN); // Disable interrupts, we'll be polling

    // Poke device 0
    outb(CB_DR, CB_DH_DEV0 );
    wait400ns();
    outb(CB_SC, 0x55);
    outb(CB_SN, 0xaa);
    outb(CB_SC, 0xaa);
    outb(CB_SN, 0x55);
    outb(CB_SC, 0x55);
    outb(CB_SN, 0xaa);
    sc = inb(CB_SC);
    sn = inb(CB_SN);
    if ((sc == 0x55) && (sn == 0xaa))
        regConfigInfo[0] = REG_CONFIG_TYPE_UNKN;

    // Poke device 1
    outb(CB_DR, CB_DH_DEV1);
    wait400ns();
    outb(CB_SC, 0x55);
    outb(CB_SN, 0xaa);
    outb(CB_SC, 0xaa);
    outb(CB_SN, 0x55);
    outb(CB_SC, 0x55);
    outb(CB_SN, 0xaa);
    sc = inb(CB_SC);
    sn = inb(CB_SN);
    if ((sc == 0x55) && (sn == 0xaa))
        regConfigInfo[1] = REG_CONFIG_TYPE_UNKN;

    // Send a software reset
    outb(CB_DR, CB_DH_DEV0);
    wait400ns();
    softReset();

    // Now we can probe device 0 and try to find its type
    outb(CB_DR, CB_DH_DEV0);
    wait400ns();
    sc = inb(CB_SC);
    sn = inb(CB_SN);
    if ((sc == 0x01) && (sn == 0x01))
    {
        regConfigInfo[0] = REG_CONFIG_TYPE_UNKN;
        st = inb(CB_STAT);
        cl = inb(CB_CL);
        ch = inb(CB_CH);
        if (((cl == 0x14) && (ch == 0xeb))       // PATAPI
          ||((cl == 0x69) && (ch == 0x96)))      // SATAPI
        {
            regConfigInfo[0] = REG_CONFIG_TYPE_ATAPI;
        }
        else if ((st != 0) &&
                 (((cl == 0x00) && (ch == 0x00))     // PATA
                ||((cl == 0x3c) && (ch == 0xc3))))   // SATA
        {
            regConfigInfo[0] = REG_CONFIG_TYPE_ATA;
        }
    }

    // Now we can probe device 1 and try to find its type
    outb(CB_DR, CB_DH_DEV1);
    wait400ns();
    sc = inb(CB_SC);
    sn = inb(CB_SN);
    if ((sc == 0x01) && (sn == 0x01))
    {
        regConfigInfo[1] = REG_CONFIG_TYPE_UNKN;
        st = inb(CB_STAT);
        cl = inb(CB_CL);
        ch = inb(CB_CH);
        if (((cl == 0x14) && (ch == 0xeb))       // PATAPI
          ||((cl == 0x69) && (ch == 0x96)))      // SATAPI
        {
            regConfigInfo[1] = REG_CONFIG_TYPE_ATAPI;
        }
        else if ((st != 0) &&
                 (((cl == 0x00) && (ch == 0x00))     // PATA
                ||((cl == 0x3c) && (ch == 0xc3))))   // SATA
        {
            regConfigInfo[1] = REG_CONFIG_TYPE_ATA;
        }
    }

    VGAText::setCurStyle(VGAText::CUR_BLUE, true);
    VGAText::print("Drive 0 : ");
    if (regConfigInfo[0] == REG_CONFIG_TYPE_NONE)
        VGAText::print("None\n");
    else if (regConfigInfo[0] == REG_CONFIG_TYPE_ATA)
        VGAText::print("Ready (ATA)\n");
    else if (regConfigInfo[0] == REG_CONFIG_TYPE_ATAPI)
        VGAText::print("Ready (ATAPI)\n");
    else
        VGAText::print("Not detected\n");

    VGAText::print("Drive 1 : ");
    if (regConfigInfo[1] == REG_CONFIG_TYPE_NONE)
        VGAText::print("None\n");
    else if (regConfigInfo[1] == REG_CONFIG_TYPE_ATA)
        VGAText::print("Ready (ATA)\n");
    else if (regConfigInfo[1] == REG_CONFIG_TYPE_ATAPI)
        VGAText::print("Ready (ATAPI)\n");
    else
        VGAText::print("Not detected\n");
    VGAText::setCurStyle();
}

int prepareCommand(int drive, int numblock, int count)
{
    if (!count || count>255)
        fatalError("blCommon: Invalid count of %p\n",count);
    if (numblock>=16*1024*1024) // We can only adress 24 bits
        fatalError("blCommon: Invalid block address of %p\n",numblock);

    outb(CB_DC, CB_DC_NIEN); // Disable interrupts, we'll be polling

    // For the moment we only impement ATA LBA28
    outb(0x1F2, count);	// Sector count
    outb(0x1F3, (u8) numblock);	// Low 8 bits of the block address
    outb(0x1F4, (u8) (numblock >> 8));	// Next 8 bits of the block address
    outb(0x1F5, (u8) (numblock >> 16));	// Next 8 bits of the block address
    // Drive indicator, enable LBA, and highest 4 bits of the block address
    outb(0x1F6, (u8)(0xE0 | (drive << 4) | ((numblock >> 24) & 0x0F)));

    return 0;
}

bool readSector(int drive, int numblock, void *_buf)
{
    u8* buf=(u8*)_buf;

    if (!selectDevice(drive))
        return false;

    // Prepare and send our command, the drive should set BSY asap
    prepareCommand(drive, numblock, 1);
    outb(0x1F7, 0x20);
    wait400ns();
    if (!waitReady())
        return false;

    for (int idx = 0; idx < 256; idx++) {
        u16 tmpword = inw(0x1F0);
        buf[idx * 2] = (unsigned char) tmpword;
        buf[idx * 2 + 1] = (unsigned char) (tmpword >> 8);
    }

    return true;
}

/** This is wrong as hell, go see how blRead is done, and just adapt the command
 * You'll also need to always clear the cache after writing, because some drives are stupid
int blWrite(int drive, int numblock, int count, char *buf)
{
    u16 tmpword;
    int idx;

    blCommon(drive, numblock, count);
    outb(0x1F7, 0x30);

    // Wait for the drive to signal that it's ready:
    while (!(inb(0x1F7) & 0x08));

    for (idx = 0; idx < 256 * count; idx++) {
        tmpword = (buf[idx * 2 + 1] << 8) | buf[idx * 2];
        outw(0x1F0, tmpword);
    }

    return count;
}
**/

// Read count bytes on disk
int diskRead(int drive, u64 offset, u64 count, void *dest)
{
    u64 blBegin = offset / 512;
    u64 blEnd = (offset + count - 1) / 512;
    u64 blocks = blEnd - blBegin + 1;

    //VGAText::printf("diskRead: offset %p, size %p, blocks %p\n", offset, count, blocks);

    u8* blBuffer = new u8[blocks * 512];

    for (u64 i=0; i<blocks; ++i)
    {
        if (!readSector(drive, blBegin+i, blBuffer+i*512))
        {
            // Try to read that sector again, just in case
            if (!readSector(drive, blBegin+i, blBuffer+i*512))
            {
                error("diskRead: readSector failed\n");
                return 0;
            }
        }
    }

    //u64 chksum=0;
    //for (u64 i=0; i<blocks*512;++i)
    //    chksum += (u8)blBuffer[i];
    //VGAText::printf("diskRead: at %p size %p chksum %p\n",offset,count,chksum);
    //if (!chksum) fatalError("Chksum status is %p\n",inb(0x1F7));

    memcpy((char*)dest, (char *) (blBuffer + offset % 512), count);

    delete[] blBuffer;
    return count;
}

}
