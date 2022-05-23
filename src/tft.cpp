#include "menu.h"

void tft_print (char *text) {
    Serial.print(text);
    menu->tft->print(text);
}
void tft_clear() {
    menu->tft->clear();
}