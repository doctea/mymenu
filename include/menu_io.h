#ifndef MENU_IO__INCLUDED
#define MENU_IO__INCLUDED

#include <Encoder.h>
#include <Bounce2.h>

using Button = Bounce2::Button;

#ifdef ENCODER_KNOB_L
    #ifdef ENCODER_DURING_SETUP
        extern Encoder *knob;
    #else
        extern Encoder knob;
    #endif    
#endif
#ifdef PIN_BUTTON_A
    //Bounce pushButtonA = Bounce(PIN_BUTTON_A, 10); // 10ms debounce
    extern Button pushButtonA;
#endif
#ifdef PIN_BUTTON_B
    //Bounce pushButtonB = Bounce(PIN_BUTTON_B, 10); // 10ms debounce
    extern Button pushButtonB; 
#endif
#ifdef PIN_BUTTON_C
    //Bounce pushButtonC = Bounce(PIN_BUTTON_C, 10); // 10ms debounce
    extern Button pushButtonC;
#endif

#endif