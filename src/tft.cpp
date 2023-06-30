#ifdef ENABLE_SCREEN
    #include "menu.h"
#endif
#include <Arduino.h>

void tft_print (const char *text) {
    Serial.print(text);
    //Serial_flush();
    #ifdef ENABLE_SCREEN
        if (menu!=nullptr && menu->tft!=nullptr)
            menu->tft->print(text);
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
