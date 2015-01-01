#ifndef CXA_H
#define CXA_H

extern "C"
{

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);
void __cxa_finalize(void *f);

}

#endif // CXA_H
