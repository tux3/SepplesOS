#ifndef INT_LIB_H_INCLUDED
#define INT_LIB_H_INCLUDED

#include <std/types.h>

/// This file assumes little-endianness (should be okay)

typedef union
{
    i64 all;
    struct
    {
        u32 low;
        i32 high;
    }s;
} dwords;

typedef union
{
    u64 all;
    struct
    {
        u32 low;
        u32 high;
    }s;
} udwords;

/// 128 bit (TI)
//typedef union
//{
//    int128 all;
//    struct
//    {
//        uint64 low;
//        int64 high;
//    }s;
//} twords;
//
//typedef union
//{
//    uint128 all;
//    struct
//    {
//        uint64 low;
//        uint64 high;
//    }s;
//} utwords;

#endif // INT_LIB_H_INCLUDED
