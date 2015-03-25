#ifndef GDT_H_INCLUDED
#define GDT_H_INCLUDED

namespace GDT
{
    // Segment selector values
    static constexpr int
        SS_NULL =       8*0,
        SS_DATA =       8*1,
        SS_CODE64 =     8*2,
        SS_CODE32 =     8*3,
        SS_DATA16 =     8*4,
        SS_CODE16 =     8*5,
        SS_USRDATA =    8*6+0b11,
        SS_USRSTACK =   8*7+0b11,
        SS_USRCODE64 =  8*8+0b11,
        SS_USRCODE32 =  8*9+0b11,
        SS_TSS =        8*10, // Takes up descriptors 10 and 11
        SS_TSS_STACK =  8*12;
}

#endif // GDT_H_INCLUDED
