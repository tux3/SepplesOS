#include <std/types.h>

/// TODO: Rename memcpy -> memmove, fix all the users, THEN implement an optimized memcpy
/// And then maybe if we find safe uses for memcpy (guaranteed no overlap), use it

/// Bytewise copy, overlapping pointers are okay.
void* memcpy(void* dst, const void* src, size_t n);

