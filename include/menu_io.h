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
    virtual void resetStateChange() {
      //this->stateChangeLastTime = 0; //millis();
      this->durationOfPreviousState = 0;
      this->stateChangeLastTime = millis();
      this->update();
    }
};

class InterruptButton : public ResettableButton {

    public:
    int my_interrupt = -1;
    bool current_state = !stateForPressed;

    inline void setPressedState(bool state) {
        Button::setPressedState(state);
        this->current_state = !state;
    }

    static InterruptButton *isr_target[3];

    static int num_interrupts;
    static void isr_0_change () {
        InterruptButton::isr_target[0]->current_state = digitalRead(InterruptButton::isr_target[0]->pin);
    }
    static void isr_1_change () {
        InterruptButton::isr_target[1]->current_state = digitalRead(InterruptButton::isr_target[1]->pin);
    }
    static void isr_2_change () {
        InterruptButton::isr_target[2]->current_state = digitalRead(InterruptButton::isr_target[2]->pin);
    }

    InterruptButton() {}
    InterruptButton(uint8_t pin, unsigned long interval_millis) : InterruptButton() {
        this->pin = pin;
        attach(pin);
        interval(interval_millis);
    }

    void attach(int pin, int mode = INPUT) {
        this->pin = pin;
        setPinMode(pin, mode);
        
        my_interrupt = InterruptButton::num_interrupts;
        switch (my_interrupt) {
            case 0:
                attachInterrupt(digitalPinToInterrupt(pin), InterruptButton::isr_0_change,  CHANGE);
                break;
            case 1:
                attachInterrupt(digitalPinToInterrupt(pin), InterruptButton::isr_1_change,  CHANGE);
                break;
            case 2:
                attachInterrupt(digitalPinToInterrupt(pin), InterruptButton::isr_2_change,  CHANGE);
                break;
        }

        InterruptButton::isr_target[my_interrupt] = this;

        num_interrupts++;
    }

    protected:
        virtual bool readCurrentState() override {
            return getPressedState() ? !this->current_state : this->current_state;
        }

};

using Button = InterruptButton;

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