#include <error.h>

using namespace IO;

void error(const char* msg)
{
    gTerm.setCurStyle(VGAText::CUR_RED); // Style error ! Pour faire un peu peur =]
    gTerm.print(msg);
    gTerm.setCurStyle(); // Remet style normal
}

void fatalError(const char* msg)
{
    gTerm.setCurStyle(VGAText::CUR_BLACK, false, VGAText::CUR_RED); // Style critical error ! Pour bien faire peur =]
    gTerm.print(msg);
    gTerm.setCurStyle(); // Remet style normal
    halt();
}

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
    fatalError("Call to pure virtual function. System hatled.\n");
}
