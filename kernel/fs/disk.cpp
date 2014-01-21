#include <fs/disk.h>
#include <io.h>
#include <paging.h>
#include <screen.h>
#include <error.h> // For debugging

namespace IO
{
    namespace FS
    {
        int blCommon(int drive, int numblock, int count)
        {
            outb(0x1F1, 0x00);	/* NULL byte to port 0x1F1 */
            outb(0x1F2, count);	/* Sector count */
            outb(0x1F3, (unsigned char) numblock);	/* Low 8 bits of the block address */
            outb(0x1F4, (unsigned char) (numblock >> 8));	/* Next 8 bits of the block address */
            outb(0x1F5, (unsigned char) (numblock >> 16));	/* Next 8 bits of the block address */

            /* Drive indicator, magic bits, and highest 4 bits of the block address */
            outb(0x1F6, 0xE0 | (drive << 4) | ((numblock >> 24) & 0x0F));

            return 0;
        }

        int blRead(int drive, int numblock, int count, char *buf)
        {
            u16 tmpword;
            int idx;

            blCommon(drive, numblock, count);
            outb(0x1F7, 0x20);

            /* Wait for the drive to signal that it's ready: */
            while (!(inb(0x1F7) & 0x08));

            for (idx = 0; idx < 256 * count; idx++) {
                tmpword = inw(0x1F0);
                buf[idx * 2] = (unsigned char) tmpword;
                buf[idx * 2 + 1] = (unsigned char) (tmpword >> 8);
            }

            return count;
        }

        int blWrite(int drive, int numblock, int count, char *buf)
        {
            u16 tmpword;
            int idx;

            blCommon(drive, numblock, count);
            outb(0x1F7, 0x30);

            /* Wait for the drive to signal that it's ready: */
            while (!(inb(0x1F7) & 0x08));

            for (idx = 0; idx < 256 * count; idx++) {
                tmpword = (buf[idx * 2 + 1] << 8) | buf[idx * 2];
                outw(0x1F0, tmpword);
            }

            return count;
        }

        // Read count bytes on disk
        int diskRead(int drive, u64 offset, char *buf, u64 count)
        {
            char *blBuffer;
            u32 blBegin, blEnd, blocks;

            blBegin = offset / 512;
            blEnd = (offset + count) / 512;
            blocks = blEnd - blBegin + 1;

            //gTerm.printf("diskRead: offset %p, size %p\n", offset, count);

            blBuffer = (char *) gPaging.kmalloc(blocks * 512);

            blRead(drive, blBegin, blocks, blBuffer);
            memcpy(buf, (char *) (blBuffer + offset % 512), count);

            gPaging.kfree(blBuffer);
            return count;
        }

        /*
         * Read count blocks on disk
         */
        int diskReadBl(int drive, int blOffset, int byteOffset, char *buf, int blCount, int byteCount)
        {
            char *blBuffer;

            while (byteOffset >= 512)
            {
                byteOffset -= 512;
                blOffset++;
            }

            while (byteCount >= 512)
            {
                byteCount -= 512;
                blCount++;
            }

        //	kernelScreen.printf("disk_read_bl():bl_offset : %d\n", bl_offset);
        //	kernelScreen.printf("disk_read_bl():bl_count : %d\n", bl_count);
        //	kernelScreen.printf("disk_read_bl():byte_offset : %d\n", byte_offset);
        //	kernelScreen.printf("disk_read_bl():byte_count : %d\n", byte_count);

            if (byteOffset > 0 || byteCount > 0)
                blCount++;

            if ((byteOffset + byteCount)>512)
                blCount++;

            blBuffer = (char *) gPaging.kmalloc(blCount * 512);

            blRead(drive, blOffset, blCount, blBuffer);
        //	kernelScreen.print("disk_read_bl():readed, calling memcpy\n");

            if ((byteOffset + byteCount)>512)
                blCount--;

            if (byteOffset > 0 || byteCount > 0)
                blCount--;

        //	kernelScreen.printf("disk_read_bl():bl_offset : %d\n", bl_offset);
        //	kernelScreen.printf("disk_read_bl():bl_count : %d\n", bl_count);
        //	kernelScreen.printf("disk_read_bl():byte_offset : %d\n", byte_offset);
        //	kernelScreen.printf("disk_read_bl():byte_count : %d\n", byte_count);

            memcpy(buf, (char *) (blBuffer+byteOffset), (blCount*512)+byteCount);

            gPaging.kfree(blBuffer);

            return blCount;
        }

    }
}
