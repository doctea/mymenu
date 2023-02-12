#include "menu.h"

void tft_print (const char *text) {
    Serial.print(text);
    Serial_flush();
    if (menu!=nullptr && menu->tft!=nullptr)
        menu->tft->print(text);
    Debug_printf("just did tft print '%s'\n", text); 
    Serial_flush();
}
void tft_clear() {
    if (menu!=nullptr && menu->tft!=nullptr)
        menu->tft->clear();
}