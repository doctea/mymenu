#include "menu.h"

void tft_print (char *text) {
    Serial.print(text);
    Serial.flush();
    menu->tft->print(text);
    Serial.printf("just did tft print '%s'\n", text); 
    Serial.flush();
}
void tft_clear() {
    menu->tft->clear();
}