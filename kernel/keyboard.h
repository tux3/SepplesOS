#ifndef KEYBOARD_H
#define KEYBOARD_H

class Keyboard
{
    public:
        Keyboard()=delete;
        static void scan(char* buf); ///< Read a line (buffered, waits for a  '\n')
        static void nget(char* buf, unsigned int n); ///< Read a line of max n characters, not counting the '\0'

        static void input(const char c); ///< Adds c to the input buffer. Mostly called by the keyboard interrupt handler.

        static void setLogInput(bool state); ///< If state, forward every input character to VGAText

    protected:
        static void unlock(); ///< Unlock and send characters to the standard output

    private:
        static volatile bool ilock; ///< Lock for scan/nget. Signals the request to input(). Unlocked by input().
        static char* ubuf; ///< Ptr to the buffer to fill
        static unsigned pos; ///< Position in the buffer
        /// If >= 0 : Numbers of chars left to read for nget
        /// If == -1 : We're in a scan
        /// If == -2 : Send to the standard output (gTerm)
        static int stop;
        static bool logInput;
};

#endif // KEYBOARD_H
