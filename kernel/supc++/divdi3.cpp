#include <std/limits.h>
#include <supc++/divdi3.h>
#include <supc++/udivmoddi4.h>

extern "C" i64 __divdi3(i64 a, i64 b)
{
    const int bits_in_dword_m1 = (int)(sizeof(i64) * CHAR_BIT) - 1;
    i64 s_a = a >> bits_in_dword_m1;           /* s_a = a < 0 ? -1 : 0 */
    i64 s_b = b >> bits_in_dword_m1;           /* s_b = b < 0 ? -1 : 0 */
    a = (a ^ s_a) - s_a;                         /* negate if s_a == -1 */
    b = (b ^ s_b) - s_b;                         /* negate if s_b == -1 */
    s_a ^= s_b;                                  /*sign of quotient */
    return (__udivmoddi4(a, b, (u64*)0) ^ s_a) - s_a;  /* negate if s_a == -1 */
}
