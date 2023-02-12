#ifndef MENU_IO__INCLUDED
#define MENU_IO__INCLUDED

#include <Encoder.h>
#include <Bounce2.h>

#ifdef ENCODER_KNOB_L
    //Encoder knob(ENCODER_KNOB_L, ENCODER_KNOB_R);
    extern Encoder knob;
#endif
#ifdef PIN_BUTTON_A
    //Bounce pushButtonA = Bounce(PIN_BUTTON_A, 10); // 10ms debounce
    extern Bounce pushButtonA;
#endif
#ifdef PIN_BUTTON_B
    //Bounce pushButtonB = Bounce(PIN_BUTTON_B, 10); // 10ms debounce
    extern Bounce pushButtonB; 
#endif
#ifdef PIN_BUTTON_C
    //Bounce pushButtonC = Bounce(PIN_BUTTON_C, 10); // 10ms debounce
    extern Bounce pushButtonC;
#endif

#endif