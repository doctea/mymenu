#pragma once

#include <Encoder.h>
#include <Bounce2.h>

// wrapper class to allow resetting of button's state change memory, to work around problem where
// it thinks the back button has been held for 250ms when first powered on
class ResettableButton : public Bounce2::Button {
  public:
    /**
     * @brief Resets the 'last state change' time.
     */
    void resetStateChange() {
      this->stateChangeLastTime = millis();
    }  
};

using Button = ResettableButton;

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