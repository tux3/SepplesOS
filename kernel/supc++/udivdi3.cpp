#include <supc++/udivdi3.h>
#include <supc++/udivmoddi4.h>

extern "C" u64 __udivdi3(u64 a, u64 b)
{
    return __udivmoddi4(a, b, 0);
}
