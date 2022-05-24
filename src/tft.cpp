#include "menu.h"

void tft_print (char *text) {
    Serial.print(text);
    Serial.flush();
    menu->tft->print(text);
    Serial.print("just did tft print?"); Serial.flush();
}
void tft_clear() {
    menu->tft->clear();
}