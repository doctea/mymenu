#ifdef ENABLE_SCREEN

#include <Arduino.h>

#include "menu_io.h"

int InterruptButton::num_interrupts = 0;

InterruptButton *InterruptButton::isr_target[3];

#endif