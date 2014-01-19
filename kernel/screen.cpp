#include <screen.h>
#include <paging.h>
#include <memmap.h>

namespace IO
{
    VGAText::VGAText()
    {
        // Attributs texte
        m_curX = 0;		// Position curseur : x
        m_curY = 0;		// Position curseur : y
        m_curStyle = 0x07;	// Attributs video des caracteres a afficher
        m_tabSize = 4;

        // Limites de l'écran
        m_width = 80;
        m_height = 25;
        m_memBase = (u8*)VGA_FRAMEBUFFER;
        m_memLim = m_memBase + (2 * m_width * m_height);

        // Start with the log disabled
        logEnabled=false;

        return;
    }

    bool VGAText::isLogEnabled()
    {
        return logEnabled;
    }

    void VGAText::enableLog(bool state)
    {
        bool IF = gti; cli; // Screen operations should be atomic
        if (state)
        {
            // Init the log with the current state of the screen
            m_logBase = new u8[2*m_width*m_height];
            m_logCur = m_logBase;
            m_logLim = m_logBase + 2*m_width*m_height;
            memcpy((char*)m_logBase, (char*)m_memBase, 2*m_width*m_height);
            logEnabled=true;
        }
        else
        {
            logEnabled=false;
            delete[] m_logBase;
        }
        if (IF) sti;
    }

    void VGAText::print(const char* s)
    {
        bool IF = gti; cli; // Screen operations should be atomic
        for(;*s;s++)
            put(*s);
        if (IF) sti;
    }

    void VGAText::printf(const char* format)
    {
        bool IF = gti; cli; // Screen operations should be atomic
        for(;*format;format++)
            put(*format);
        if (IF) sti;
    }

    void VGAText::_printf(const char* format)
    {
        for(;*format;format++)
            put(*format);
    }

    void VGAText::setCurStyle(char txtColor, bool txtHighlight, char bgdColor, bool blink)
    {
        // 8   7                4 3          0
        // |blk|background color|i|text color|
        int style = 0b00000000;

        if (blink)
            style += 0b10000000;
        if (txtHighlight)
            style += 0b00001000;
        style += ((bgdColor&0b111) << 4) + (txtColor&0b111);

        m_curStyle = style;

        //printf("Style : %b\n", style);
    }

    void VGAText::moveCursor(int x, int y)
    {
        if (((x<0 || y<0) && (x!=-1 || y!=-1)) || x > m_width || y > m_height)
        {
            error("VGAText::moveCursor: Invalid position (%d,%d)\n",x,y);
            return;
        }

        // If we just want to hide the cursor, we don't need to change the pos
        if (x >= 0 && y >= 0)
        {
            m_curX = x;
            m_curY = y;
        }

        u16 c_pos = y * m_width + x;

        // cursor LOW port to vga INDEX register
        outb(0x3d4, 0x0f);
        outb(0x3d5, (u8) c_pos);
        // cursor HIGH port to vga INDEX register
        outb(0x3d4, 0x0e);
        outb(0x3d5, (u8) (c_pos >> 8));

        return;
    }

    void VGAText::showCursor()
    {
        moveCursor(m_curX, m_curY);
    }

    void VGAText::hideCursor()
    {
        moveCursor(-1, -1);
    }

    /*
    void VGAText::scan(char* buf)
    {
        globalKeyboard.scan(buf);
    }

    void VGAText::nget(char* buf, unsigned int n)
    {
        globalKeyboard.nget(buf,n);
    }
    */

    void VGAText::put(const char c)
    {
        int videoOff;

        // We can't write to the pos of the cursor if we're scrolled up.
        if (logEnabled && m_logCur != m_logLim - 2*m_height*m_width)
            scrollDown();

        if (c == 10)  		// NL (interpreted as CR + NL)
        {
            m_curX = 0;
            m_curY++;
        }
        else if (c == 8)    // BACKSPACE
        {
            rubout();
        }
        else if (c == 9)  	// TAB
        {
            m_curX = m_curX + m_tabSize - (m_curX % m_tabSize);
        }
        else if (c == 13)  	// CR
        {
            m_curX = 0;
        }
        else
        {
            videoOff = (2 * m_curX + 2 * m_width * m_curY);
            if (logEnabled)
            {
                *(m_logCur+videoOff) = c;
                *(m_logCur+videoOff + 1) = m_curStyle;
            }
            *(m_memBase+videoOff) = c;
            *(m_memBase+videoOff + 1) = m_curStyle;

            m_curX++;
            if (m_curX >= m_width)
            {
                m_curX = 0;
                m_curY++;
            }
        }

        if (m_curY >= m_height)
            newLines(1 + m_curY - m_height); // Add new lines
    }

    void VGAText::clear()
    {
        bool IF = gti; cli; // Screen operations should be atomic
        for (u8* videomem=(u8*)m_memBase; videomem<m_memLim; videomem+=2)
        {
            *videomem = 0;
            *(videomem+1) = CUR_WHITE;
        }
        if (logEnabled)
        {
            enableLog(false);
            enableLog(true);
        }

        m_curX = 0;
        m_curY = 0;
        if (IF) sti;
    }

    void VGAText::scroll(int n)
    {
        if (!logEnabled)
        {
            error("VGAText::scroll: Can't scroll with the log disabled.\n");
            return;
        }

        if (!n)
            return;
        else if (n<0) // Move n lines back in the log and render to the screen
        {
            if (m_logCur == m_logBase) // Can't scroll higher than the top
                return;
            hideCursor(); // While we're scrolled up, the cursor disappears
            m_logCur += n * 2 * m_width;
            if (m_logCur < m_logBase)
                m_logCur = m_logBase;
            memcpy((char*) m_memBase, (char*)m_logCur, 2 * m_width * m_height); // Render
        }
        else // Move forward n lines
        {
            m_logCur += n * 2 * m_width;
            u8* curLim = m_logLim - 2*m_width*m_height;
            if (m_logCur >= curLim) // Don't scroll past the end
            {
                m_logCur = curLim;
                showCursor(); // If we're back to the end, show the cursor again
            }
            memcpy((char*) m_memBase, (char*)m_logCur, 2 * m_width * m_height); // Render
        }
    }

    void VGAText::scrollDown()
    {
        m_logCur = m_logLim - 2*m_width*m_height;
        memcpy((char*) m_memBase, (char*)m_logCur, 2 * m_width * m_height); // Render
    }

    void VGAText::newLines(unsigned n)
    {
        u8 *videomem, *membuf;

        // Fix the cursor as soon as possible.
        m_curY -= n;
        if (m_curY < 0)
            m_curY = 0;
        else if (m_curY > m_height)
            m_curY = m_height;

        // Now point in adding more lines that the user can see.
        if (n>m_height)
            n=m_height;

        // Update the framebuffer first
        for (videomem=(u8*)m_memBase; videomem<m_memLim; videomem+=2)
        {
            membuf = (u8*) (videomem + n * 2 * m_width);

            if (membuf < m_memLim)   // Copy the char n*160 chars further (soit n lignes plus bas)
            {
                *videomem = *membuf;
                *(videomem + 1) = *(membuf + 1);
            }
            else // The new lines are empty.
            {
                *videomem = 0;
                *(videomem+1) = CUR_WHITE;
            }
        }

        // Update the log if needed
        if (logEnabled)
        {
            // Alloc a new log big enough for the n new lines and fill it with the old log
            size_t newsize = m_logLim - m_logBase + n*2*m_width;
            u8* newLog = new u8[newsize];
            memcpy((char*)newLog,(char*)m_logBase, m_logLim - m_logBase);
            delete[] m_logBase;
            m_logBase = newLog;
            m_logLim = newLog + newsize;
            m_logCur = m_logLim - (2*m_width*m_height);

            // Empty the n new lines
            u8* ptr = m_logLim - n*2*m_width;
            if (ptr<m_logCur) ptr = m_logCur; // careful not to erase before the start
            for(;ptr<m_logLim; ptr+=2)
            {
                *ptr = 0;
                *(ptr+1) = CUR_WHITE;
            }
        }
    }

    void VGAText::rubout()
    {
        int videoOff;

        // Cursor pos
        videoOff = 2 * m_curX + 2 * m_width * m_curY;

        // Either we're at the start of a line, or we're not
        if (m_curX > 0)
        {
            // Erase the char before
            videoOff -= 2;
            m_curX--;
            if (logEnabled)
            {
                *(m_logCur + videoOff) = 0;
                *(m_logCur + videoOff + 1) = CUR_WHITE;
            }
            *(m_memBase + videoOff) = 0;
            *(m_memBase + videoOff + 1) = CUR_WHITE;
        }
        else
        {
            // Go to the end of the line
            if (m_curY <= 0)
                return;

            m_curX=(m_width -1);
            videoOff-=2;

            // Scroll up and remove the line from the log if we can
            // (without actually freeing the memory yet, but there's no leak)
            u8* newLogCur = m_logCur - 2*m_width;
            if (logEnabled && newLogCur >= m_logBase)
            {
                m_logCur = newLogCur;
                m_logLim -= 2*m_width;
                memcpy((char*) m_memBase, (char*)m_logCur, 2 * m_width * m_height); // Render
                videoOff += m_width*2; // Since we scrolled up, we need to fix the offset to point to the same line
            }
            else
            {
                m_curY--; // We're not scrolling up when the log is disabled
                // Erase the char
                *(m_memBase + videoOff) = 0;
                *(m_memBase + videoOff + 1) = CUR_WHITE;
            }

            // Go back in the line until we find a char that isn't a space (0)
            while ( *(m_memBase+videoOff) == 0 )
            {
                // Go back 1 char
                videoOff -=2;
                m_curX--;

                // Stop if we erased a whole line
                if (m_curX == 0)
                {
                    if ( *(m_memBase+videoOff) == 0 )
                        return;
                    else
                        break;
                }
            }
            m_curX++;
        }
    }

    u8 VGAText::getWidth(void)
    {
        return m_width;
    }

    u8 VGAText::getHeight(void)
    {
        return m_height;
    }
}
