#include <keyboard.h>
#include <vga/vgatext.h>
#include <lib/string.h>
#include <debug.h>

bool volatile Keyboard::ilock=false;
char* Keyboard::ubuf=0;
unsigned Keyboard::pos=0;
int Keyboard::stop=-2;
bool Keyboard::logInput=false;

void Keyboard::scan(char* buf)
{
    while (ilock); // Wait for the lock
    ilock=true;
    // We've got the lock, but while ubuf and stop aren't correct, we're not receiving anything
    VGAText::setRuboutLimit();
    ubuf = buf;
    pos = 0;
    stop = -1; // Buffered mode (for scan())
    while(ilock); // Now we wait and when it'll unlock when the buffer is full
    VGAText::setRuboutLimit(0,0);
}

void Keyboard::nget(char* buf, unsigned int n)
{
    while (ilock); // Wait for the lock
    ilock=true;
    // We've got the lock, but while ubuf and stop aren't correct, we're not receiving anything
    VGAText::setRuboutLimit();
    ubuf = buf;
    pos = 0;
    stop = n; // Unbuffered mode (for nget())
    while(ilock); // Now we wait and when it'll unlock when the buffer is full
    VGAText::setRuboutLimit(0,0);
}

void Keyboard::input(const char c)
{
    if (ilock && ubuf != 0 && stop != -2) // Scan or nget active
    {
        if (stop < 0) // scan()
        {
            if (c==8) // Backspace
            {
                if (pos)
                    pos--;
                else
                    ubuf[0]=0;
            }
            else
            {
                ubuf[pos] = c;
                pos++;
                ubuf[pos] = '\0';
            }

            if (c == 10 || c == 13) // We got a '\n', end of scan()
                unlock();
        }
        else if (stop == 0) // End of nget()
            unlock();
        else // nget()
        {
            if (c==8) // Backspace
            {
                if (pos)
                    pos--;
                else
                    ubuf[0]=0;
            }
            else
            {
                ubuf[pos] = c;
                pos++;
                ubuf[pos] = '\0';
            }

            // Update stop (number of chars remaining)
            stop--;
            if (c == 10 || c == 13)
                stop=0;
            if (!stop)
                unlock();
        }
    }

    // VGAText is the closest we have to a standard output!
    if(logInput)
        VGAText::put(c);
}

void Keyboard::setLogInput(bool state)
{
    logInput = state;
}

void Keyboard::unlock()
{
    ilock=false;
    ubuf = 0;
    stop = -2;
    pos = 0;
}
