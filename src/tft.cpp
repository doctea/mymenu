#ifdef ENABLE_SCREEN
    #include "menu.h"
#endif
#include <Arduino.h>

#include "tft.h"

void tft_print (const char *text) {
    Serial.print(text);
    //Serial_flush();
    #ifdef ENABLE_SCREEN
        if (menu!=nullptr && menu->tft!=nullptr)
            menu->tft->print(text);
            #if (defined(TFT_ILI9341_T3N) || defined(TFT_BODMER)) && !defined(ARDUINO_ARCH_RP2040)
                // this is needed to get power-on display messages on usb_midi_clocker
                // but seems to crash Microlidian for some reasons, so don't do it if we're on RP2040
                menu->tft->updateDisplay();
            #endif
    #endif
    //Debug_printf("just did tft print '%s'\n", text); 
    //Serial_flush();
}
void tft_clear() {
    #ifdef ENABLE_SCREEN
        if (menu!=nullptr && menu->tft!=nullptr)
            menu->tft->clear();
    #endif
}
