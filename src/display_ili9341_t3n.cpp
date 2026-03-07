#include <display_ili9341_t3n.h>

#include <Arduino.h>

int current_fb = 0;
bool done = true;
bool ready_to_change_fb = true;
uint16_t *fb1[2] = { nullptr, nullptr };

ILI9341_t3n *ili_tft = nullptr;

void frame_complete_callback() {
    //current_fb = !current_fb;
    ready_to_change_fb = true;
    ili_tft->endUpdateAsync(); 
    if (Serial) Serial.printf("frame complete callback! current_fb=%i, setting ready_to_change_fb=%i\n", current_fb, ready_to_change_fb); Serial_flush();
}