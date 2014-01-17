#include <lib/string.h>

/* strlen : Retourne la taille d'un null-terminated char* -1 (donc '\0' non compté */
size_t strlen(const char * str)
{
    const char *s;
    for (s = str; *s; ++s);
    return(s - str);
}

int strcmp (const char * s1, const char * s2)
{
    for(; *s1 == *s2; ++s1, ++s2)
        if(*s1 == 0)
            return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}


/* Copie un null-terminated char[] dans un autre, mais sans filet */
char* strcpy(char *dest, const char *src)
{
   char *save = dest;
   while ((*dest++ = *src++));
   return save;
}

// reverse:  reverse string s in place
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// Concatenate src in dest
char* strcat(char *dest, const char *src)
{
    char *rdest = dest;

    while (*dest)
      dest++;
    while ((*dest++ = *src++));
    return rdest;
}

char* strchr(const char* s, int c)
{
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

// Convert int value into a C-String
char* itoa(i64 value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	i64 tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

i64 atoi(const char* str)
{
	i64 num = 0;
	char c;
	bool negative = *str == '-' ? true : false;
	if (negative)
		str++;
	while ((c = *str++))
	{
		if (c < '0' || c > '9')
			return negative ? -num : num;  // No valid conversion possible
		num *= 10;
		num += c - '0';
	}
	return negative ? -num : num;
}


