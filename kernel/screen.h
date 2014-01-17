#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <io.h>
#include <lib/string.h>

namespace IO
{
    // Access to the VGA Text Mode, writing directly into memory (by memory-mapped I/O)
    class VGAText
    {
        public:
            enum curStyleColor // TL;DR: 0 means black
            {
                CUR_BLACK		= 0b000,
                CUR_BLUE		= 0b001,
                CUR_GREEN		= 0b010,
                CUR_CYAN		= 0b011,
                CUR_RED			= 0b100,
                CUR_MAGENTA		= 0b101,
                CUR_YELLOW		= 0b110,
                CUR_WHITE		= 0b111
            };

        public:
            VGAText();
            ~VGAText();
            //VGAText(int width, int height);
            void print(const char* s);
            template <class T, class... Args> void printf(const char* format, const T& value, const Args&... args);
            void printf(const char* s);

            // Terminal low-level virtual functions
            void put(const char c); ///< Can handle special chars (backspace, enter, tab, ...)
            void clear();
            //void scan(char* buf);
            //void nget(char* buf, unsigned int n);
            void scroll(int n); ///< Scrolls n lines in the log. If n<0, scroll up. Will not scroll past the end
            void scrollDown(); ///< Scrolls to the end.
            void rubout();

            void enableLog(bool state); ///< Enable or disable the log. Faster when disabled.
            void showCursor();
            void hideCursor();
            void setCurStyle(char txtColor = CUR_WHITE, bool txtHighlight = false, char bgdColor = CUR_BLACK,  bool blink = false);

        protected:
            void moveCursor(int x, int y); ///< Moves the cursor. (-1,-1) to hide it.
            void newLines(unsigned n); ///< Scrolls to the end and adds n new lines

            u8 getWidth();
            u8 getHeight();

            template <class T, class... Args> void _printf(const char* format, const T& value, const Args&... args);
            void _printf(const char* s); ///< For printf recursion

        private:
            u8* m_memBase; // Début de la mémoire video VGA texte
            u8* m_memLim;	// Fin de la mémoire video VGA texte
            u8* m_logBase;       // Debut du log VGA texte
            u8* m_logCur;       // Debut du dernier ecran du log VGA texte
            u8* m_logLim;        // Fin du log VGA texte
            bool logEnabled;         // When true, write to the log, then memcpy to the screen. If false write directly to the screen.
            bool olock;		// Verrou en ecriture sur la mémoire
            i8 m_curX;			// Position curseur : x
            i8 m_curY;			// Position curseur : y
            i8 m_curStyle;		// Attributs video des caracteres a afficher
            i8 m_tabSize;			// Taille d'un caractère TAB
            u8 m_width;			// Resolution : largeur
            u8 m_height;			// Resolution : hauteur
    };

    /// Templates
    template <class T, class... Args>
    void VGAText::printf(const char* format, const T& value, const Args&... args)
    {
        while(olock);if(gti){cli;olock=true;} // Wait and Lock if interrupts are enabled
        _printf(format,value,args...); // Renvois les arguments à _printf !
        if(olock){olock=false;sti;} // Unlock if we locked
    }

    template <class T, class... Args>
    void VGAText::_printf(const char* format, const T& value, const Args&... args)
    {
        while (*format)
        {
            if (*format == '%')
            {
                ++format;
                if (*format == 'd')
                {
                    char buf[32];
                    itoa((i64)value, buf, 10);
                    _printf(buf);
                }
                else if(*format == 'x')
                {
                    char buf[32];
                    itoa((i64)value, buf, 16);
                    _printf(buf);
                }
                else if(*format == 'b')
                {
                    char buf[32];
                    itoa((i64)value, buf, 2);
                    _printf(buf);
                }
                else if(*format == 'c')
                    put((int)value);
                else if(*format == 's')
                    _printf((const char*)(int)value);

                // Si format inconu, continuing happily, on ne peut pas afficher d'erreur à l'intérieur d'un printf !
                _printf(++format, args...); // Récursion sans revenir (car return après)
                return;
            }
            else
                put(*format); // Fin de récursion
            ++format;
        }
        // Erreur : trop d'arguments par rapport au format, mais on ne peut pas afficher d'erreur à l'intérieur d'un printf !
        return;
    }
}

/// Singleton
extern IO::VGAText gTerm;

#endif
