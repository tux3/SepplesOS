#include <keyboard.h>
#include <screen.h>
#include <lib/string.h>

namespace IO
{
    Keyboard::Keyboard()
    {
        unlock();
    }

    void Keyboard::scan(char* buf)
    {
        while (ilock); // Wait for the lock
        ilock=true;
        // We've got the lock, but while ubuf and stop aren't correct, we're not receiving anything
        ubuf = buf;
        pos = 0;
        stop = -1; // Buffered mode (for scan())
        while(ilock); // Now we wait and when it'll unlock when the buffer is full
    }

    void Keyboard::nget(char* buf, unsigned int n)
    {
        while (ilock); // Wait for the lock
        ilock=true;
        // We've got the lock, but while ubuf and stop aren't correct, we're not receiving anything
        ubuf = buf;
        pos = 0;
        stop = n; // Unbuffered mode (for nget())
        while(ilock); // Now we wait and when it'll unlock when the buffer is full
    }

    void Keyboard::input(const char c)
    {
        if (ilock && ubuf != 0 && stop != -2) // Scan or nget active
        {
            if (stop < 0) // scan()
            {
                // Add the character to the user buffer
                ubuf[pos] = c;
                pos++;
                ubuf[pos] = '\0';


                if (c == 10 || c == 13) // We got a '\n', end of scan()
                    unlock();
            }
            else if (stop == 0) // End of nget()
                unlock();
            else // nget()
            {
                // Add the character to the user buffer
                ubuf[pos] = c;
                pos++;
                ubuf[pos] = '\0';

                // Update stop (number of chars remaining)
                stop--;
            }
        }

        // Send to the standard output. Wich happens to be the gTerm.
        gTerm.put(c);
    }

    void Keyboard::unlock()
    {
        ilock=false;
        ubuf = 0;
        stop = -2;
        pos = 0;
    }
}
