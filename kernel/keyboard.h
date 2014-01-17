#ifndef KEYBOARD_H
#define KEYBOARD_H

namespace IO
{
    class Keyboard
    {
        public:
            Keyboard();
            void scan(char* buf); ///< Read a line (buffered, waits for a  '\n')
            void nget(char* buf, unsigned int n); ///< Read n characters

            void input(const char c); ///< Adds c to the input buffer. Mostly called by the keyboard interrupt handler.

        protected:
            void unlock(); ///< Unlock and send characters to the standard output

        private:
            bool ilock; ///< Lock for scan/nget. Signals the request to input(). Unlocked by input().
            char* ubuf; ///< Ptr to the buffer to fill
            unsigned int pos; ///< Position in the buffer
            /// If >= 0 : Numbers of chars left to read for nget
            /// If == -1 : We're in a scan
            /// If == -2 : Send to the standard output (gTerm)
            int stop;
    };
};

/// Singleton
extern IO::Keyboard gKbd;

#endif // KEYBOARD_H
