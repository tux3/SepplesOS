#ifndef BINARYPREFIX_H_INCLUDED
#define BINARYPREFIX_H_INCLUDED

#include <std/types.h>

u8* toIEC(u8* buf, u32 bufSize, u64 size); // Convertit en unité IEC (kibioctet Kio, mibioctet Mio, ...), retourne un buffer de la forme "XXX Mio"
u8* toSI(u8* buf, u32 bufSize, u64 size); // Convertit en unité SI (kilo-octet Ko, mégaoctet Mo, ...), retourne un buffer de la forme "XXX Mo"

#endif // BINARYPREFIX_H_INCLUDED
