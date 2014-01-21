#ifndef __STRING_H__
#define __STRING_H__

#include <std/types.h>

size_t strlen(const char* str); // Return size of *str -1
int strcmp (const char* s1, const char* s2); // Return the difference between two strings
char* strcpy(char* dest, const char* src); // Copy *src into *dest
char* strcat(char* dest, const char* src); // Concatenate src in dest
char* strchr (const char* s, int c); // Return a pointer to the first occurence of c in str

char* itoa(i64 value, char* result, int base);
char* utoa(u64 value, char* result, int base);
i64 atoi ( const char * str );
void reverse(char s[]); // Reverse a char[]

#endif
