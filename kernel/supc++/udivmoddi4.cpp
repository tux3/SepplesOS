#include <std/limits.h>
#include <supc++/int_lib.h>
#include <supc++/udivmoddi4.h>

u64 __udivmoddi4(u64 a, u64 b, u64* rem)
{
    const unsigned n_uword_bits = sizeof(u32) * CHAR_BIT;
    const unsigned n_udword_bits = sizeof(u64) * CHAR_BIT;
    udwords n;
    n.all = a;
    udwords d;
    d.all = b;
    udwords q;
    udwords r;
    unsigned sr;
    /* special cases, X is unknown, K != 0 */
    if (n.s.high == 0)
    {
        if (d.s.high == 0)
        {
            /* 0 X
             * ---
             * 0 X
             */
            if (rem)
                *rem = n.s.low % d.s.low;
            return n.s.low / d.s.low;
        }
        /* 0 X
         * ---
         * K X
         */
        if (rem)
            *rem = n.s.low;
        return 0;
    }
    /* n.s.high != 0 */
    if (d.s.low == 0)
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 0
             */
            if (rem)
                *rem = n.s.high % d.s.low;
            return n.s.high / d.s.low;
        }
        /* d.s.high != 0 */
        if (n.s.low == 0)
        {
            /* K 0
             * ---
             * K 0
             */
            if (rem)
            {
                r.s.high = n.s.high % d.s.high;
                r.s.low = 0;
                *rem = r.all;
            }
            return n.s.high / d.s.high;
        }
        /* K K
         * ---
         * K 0
         */
        if ((d.s.high & (d.s.high - 1)) == 0)     /* if d is a power of 2 */
        {
            if (rem)
            {
                r.s.low = n.s.low;
                r.s.high = n.s.high & (d.s.high - 1);
                *rem = r.all;
            }
            return n.s.high >> __builtin_ctz(d.s.high);
        }
        /* K K
         * ---
         * K 0
         */
        sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
        /* 0 <= sr <= n_uword_bits - 2 or sr large */
        if (sr > n_uword_bits - 2)
        {
           if (rem)
                *rem = n.all;
            return 0;
        }
        ++sr;
        /* 1 <= sr <= n_uword_bits - 1 */
        /* q.all = n.all << (n_udword_bits - sr); */
        q.s.low = 0;
        q.s.high = n.s.low << (n_uword_bits - sr);
        /* r.all = n.all >> sr; */
        r.s.high = n.s.high >> sr;
        r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
    }
    else  /* d.s.low != 0 */
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 K
             */
            if ((d.s.low & (d.s.low - 1)) == 0)     /* if d is a power of 2 */
            {
                if (rem)
                    *rem = n.s.low & (d.s.low - 1);
                if (d.s.low == 1)
                    return n.all;
                unsigned sr = __builtin_ctz(d.s.low);
                q.s.high = n.s.high >> sr;
                q.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
                return q.all;
            }
            /* K X
             * ---
             *0 K
             */
            sr = 1 + n_uword_bits + __builtin_clz(d.s.low) - __builtin_clz(n.s.high);
            /* 2 <= sr <= n_udword_bits - 1
             * q.all = n.all << (n_udword_bits - sr);
             * r.all = n.all >> sr;
             * if (sr == n_uword_bits)
             * {
             *     q.s.low = 0;
             *     q.s.high = n.s.low;
             *     r.s.high = 0;
             *     r.s.low = n.s.high;
             * }
             * else if (sr < n_uword_bits)  // 2 <= sr <= n_uword_bits - 1
             * {
             *     q.s.low = 0;
             *     q.s.high = n.s.low << (n_uword_bits - sr);
             *     r.s.high = n.s.high >> sr;
             *     r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
             * }
             * else              // n_uword_bits + 1 <= sr <= n_udword_bits - 1
             * {
             *     q.s.low = n.s.low << (n_udword_bits - sr);
             *     q.s.high = (n.s.high << (n_udword_bits - sr)) |
             *              (n.s.low >> (sr - n_uword_bits));
             *     r.s.high = 0;
             *     r.s.low = n.s.high >> (sr - n_uword_bits);
             * }
             */
            q.s.low =  (n.s.low << (n_udword_bits - sr)) &
                     ((i32)(n_uword_bits - sr) >> (n_uword_bits-1));
            q.s.high = ((n.s.low << ( n_uword_bits - sr))                       &
                     ((i32)(sr - n_uword_bits - 1) >> (n_uword_bits-1))) |
                     (((n.s.high << (n_udword_bits - sr))                     |
                     (n.s.low >> (sr - n_uword_bits)))                        &
                     ((i32)(n_uword_bits - sr) >> (n_uword_bits-1)));
            r.s.high = (n.s.high >> sr) &
                     ((i32)(sr - n_uword_bits) >> (n_uword_bits-1));
            r.s.low =  ((n.s.high >> (sr - n_uword_bits))                       &
                     ((i32)(n_uword_bits - sr - 1) >> (n_uword_bits-1))) |
                     (((n.s.high << (n_uword_bits - sr))                      |
                     (n.s.low >> sr))                                         &
                     ((i32)(sr - n_uword_bits) >> (n_uword_bits-1)));
        }
        else
        {
            /* K X
             * ---
             * K K
             */
            sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
            /* 0 <= sr <= n_uword_bits - 1 or sr large */
            if (sr > n_uword_bits - 1)
            {
               if (rem)
                    *rem = n.all;
                return 0;
            }
            ++sr;
            /* 1 <= sr <= n_uword_bits */
            /*  q.all = n.all << (n_udword_bits - sr); */
            q.s.low = 0;
            q.s.high = n.s.low << (n_uword_bits - sr);
            /* r.all = n.all >> sr;
             * if (sr < n_uword_bits)
             * {
             *     r.s.high = n.s.high >> sr;
             *     r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
             * }
             * else
             * {
             *     r.s.high = 0;
             *     r.s.low = n.s.high;
             * }
             */
            r.s.high = (n.s.high >> sr) &
                     ((i32)(sr - n_uword_bits) >> (n_uword_bits-1));
            r.s.low = (n.s.high << (n_uword_bits - sr)) |
                    ((n.s.low >> sr)                  &
                    ((i32)(sr - n_uword_bits) >> (n_uword_bits-1)));
        }
    }
    /* Not a special case
     * q and r are initialized with:
     * q.all = n.all << (n_udword_bits - sr);
     * r.all = n.all >> sr;
     * 1 <= sr <= n_udword_bits - 1
     */
    u32 carry = 0;
    for (; sr > 0; --sr)
    {
        /* r:q = ((r:q)  << 1) | carry */
        r.s.high = (r.s.high << 1) | (r.s.low  >> (n_uword_bits - 1));
        r.s.low  = (r.s.low  << 1) | (q.s.high >> (n_uword_bits - 1));
        q.s.high = (q.s.high << 1) | (q.s.low  >> (n_uword_bits - 1));
        q.s.low  = (q.s.low  << 1) | carry;
        /* carry = 0;
         * if (r.all >= d.all)
         * {
         *      r.all -= d.all;
         *      carry = 1;
         * }
         */
        const i64 s = (i64)(d.all - r.all - 1) >> (n_udword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}
