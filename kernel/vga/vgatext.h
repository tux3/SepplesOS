#ifndef _SCREEN_H_
#define _SCREEN_H_

//#define NO_MALLOC
#define LOG_TO_E9 1

#include <arch/pinio.h>
#include <lib/string.h>
#include <mm/memmap.h>

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
        VGAText() = delete;
        //VGAText(int width, int height);
        static void print(const char* s);
        template <class T, class... Args> static void printf(const char* format, const T& value, const Args&... args);
        static void printf(const char* s);

        // Terminal low-level virtual functions
        static void put(const char c); ///< Can handle special chars (backspace, enter, tab, ...)
        static void clear();
        //void scan(char* buf);
        //void nget(char* buf, unsigned int n);
        static void scroll(int n); ///< Scrolls n lines in the log. If n<0, scroll up. Will not scroll past the end
        static void scrollDown(); ///< Scrolls to the end.
        static void rubout();

        ///< Block erasing characters before the limit. If a parameter is -1, it is set to the current position
        static void setRuboutLimit(i8 x=-1, i8 y=-1);

        static void enableLog(bool state = true); ///< Enable or disable the log. Faster when disabled.
        static bool isLogEnabled();
        static void showCursor();
        static void hideCursor();
        static void setCurStyle(char txtColor = CUR_WHITE, bool txtHighlight = false, char bgdColor = CUR_BLACK,  bool blink = false);

    protected:
        static void moveCursor(int x, int y); ///< Moves the cursor. (-1,-1) to hide it.
        static void newLines(unsigned n); ///< Scrolls to the end and adds n new lines

        static u8 getWidth();
        static u8 getHeight();

        template <class T, class... Args> static void _printf(const char* format, const T& value, const Args&... args);
        static void _printf(const char* s); ///< For printf recursion

    private:
        static u8* m_logBase;       // Debut du log VGA texte
        static u8* m_logCur;       // Debut du dernier ecran du log VGA texte
        static u8* m_logLim;        // Fin du log VGA texte
        static bool logEnabled;         // When true, write to the log and screen. If false write directly to the screen.
        static i8 m_curX;			// Position curseur : x
        static i8 m_curY;			// Position curseur : y
        static i8 m_curStyle;		// Attributs video des caracteres a afficher
        static i8 m_tabSize;			// Taille d'un caractère TAB
        static constexpr u8 m_width = 80;			// Resolution : largeur
        static constexpr u8 m_height = 25;			// Resolution : hauteur
        static constexpr u8* m_memBase = (u8*)VGA_TEXT_START; // Début de la mémoire video VGA texte
        static constexpr u8* m_memLim = m_memBase + (2 * m_width * m_height);	// Fin de la mémoire video VGA texte
        static i8 ruboutLimitX;
        static i8 ruboutLimitY;
};

/// Templates
template <class T, class... Args>
void VGAText::printf(const char* format, const T& value, const Args&... args)
{
    bool IF = gti; cli; // Screen operations should be atomic
    _printf(format,value,args...); // Renvois les arguments à _printf !
    if (IF) sti;
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
            else if(*format == 'p')
            {
                char buf[32];
                utoa((u64)value, buf, 16);
                put('0'); put('x');
                _printf(buf);
            }
            else if(*format == 'b')
            {
                char buf[32];
                itoa((i64)value, buf, 2);
                _printf(buf);
            }
            else if(*format == 'c')
                put((long)value);
            else if(*format == 's')
                _printf((const char*)(long)value);

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

#endif
