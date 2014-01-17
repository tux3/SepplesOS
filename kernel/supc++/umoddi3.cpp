#include <supc++/umoddi3.h>
#include <supc++/udivmoddi4.h>

extern "C" u64 __umoddi3(u64 a, u64 b)
{
    u64 r;
    __udivmoddi4(a, b, &r);
    return r;
}
