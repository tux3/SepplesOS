#ifndef BINARYPREFIX_H_INCLUDED
#define BINARYPREFIX_H_INCLUDED

#include <std/types.h>

char *toIEC(char *buf, u32 bufSize, u64 size); // Convertit en unité IEC (kibioctet Kio, mibioctet Mio, ...), retourne un buffer de la forme "XXX Mio"
char *toSI(char *buf, u32 bufSize, u64 size); // Convertit en unité SI (kilo-octet Ko, mégaoctet Mo, ...), retourne un buffer de la forme "XXX Mo"

#endif // BINARYPREFIX_H_INCLUDED
