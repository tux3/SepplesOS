#ifndef PIC_H_INCLUDED
#define PIC_H_INCLUDED

class PIC
{
public:
    PIC()=delete;
    static void init(); ///< Init the PIC (programmable interrupt controller)
    static void setFrequency(unsigned hz); ///< Set the frequency of the clock's IRQs
    static unsigned getFrequency(); ///< Return the last frequency we requested (no checking if it was accepted)
private:
    static unsigned frequency; ///< Last frequency requested. Constructed to 0
    static bool ready; ///< True after init() was called.
};

#endif // PIC_H_INCLUDED
