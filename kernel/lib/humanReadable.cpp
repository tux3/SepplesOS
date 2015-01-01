#include <lib/humanReadable.h>

// change to 1000 for SI-compatible numbers
#define KILOBYTE 1024

u8* toIEC(u8* buf, u32 bufSize, u64 size)
{
  static char indexes[] = " KMGTPE";
  // clean up the interface if you're using this
//  assert(n == 10);
  int idx = 0;
  int cidx = bufSize-1;
  while (size > 10*1024) { size /= 1024; idx++; }
  buf[cidx--] = '\0';
  buf[cidx--] = 'B';
  if (idx) buf[cidx--] = 'i';
  if (idx) buf[cidx--] = indexes[idx];
  buf[cidx--] = ' ';
  if (size == 0) {
    buf[cidx--] = '0';
  } else {
    while (size > 0) {
      buf[cidx--] = '0' + size % 10;
      size /= 10;
    }
  }
  return buf + cidx + 1;
}

u8* toSI(u8* buf, u32 bufSize, u64 size)
{
  static char indexes[] = " kMGTPE";
  // clean up the interface if you're using this
//  assert(n == 10);
  int idx = 0;
  int cidx = bufSize-1;
  while (size > 10*1000) { size /= 1000; idx++; }
  buf[cidx--] = '\0';
  buf[cidx--] = 'B';
  if (idx) buf[cidx--] = indexes[idx];
  buf[cidx--] = ' ';
  if (size == 0) {
    buf[cidx--] = '0';
  } else {
    while (size > 0) {
      buf[cidx--] = '0' + size % 10;
      size /= 10;
    }
  }
  return buf + cidx + 1;
}
