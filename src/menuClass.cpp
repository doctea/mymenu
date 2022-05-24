#include "menu.h"
#include "menuitems.h"

#define MAX_KNOB 1024

/*void tft_print(const char *text) {
    menu->tft->print(text);
}*/

// draw the menu display
int Menu::display() {
    int y = 0;
    
    tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);

    // now draw the menu
    if (currently_opened>=0 && items.get(currently_opened)->allow_takeover()) {
        #ifdef PPQN
            y = draw_loop_markers();
        #endif
        y = draw_message();
        // let the currently opened item take care of drawing all of the display
        items.get(currently_opened)->display(Coord(0,y), true, true);
    } else {
        static int panel_bottom[20];
        static bool bottoms_computed = false;

        // find number of panels to offset in order to ensure that selected panel is on screen?
        int start_panel = 0;
        if (currently_selected>0 && panel_bottom[currently_selected] > tft->height()) {
            start_panel = currently_selected - 1;
            // count backwards to find number of panels we have to go to fit currently_selected on screen...
            int count_y = panel_bottom[currently_selected];
            for (int i = currently_selected ; i > 0 ; i--) {
                count_y -= panel_bottom[i];
                if (count_y + panel_bottom[currently_selected] < tft->height()) {
                    start_panel = i;
                    break;
                }
            }
            //tft.fillWindow(ST77XX_BLACK);
            tft->fillRect(0, 0, tft->width(), tft->height(), tft->BLACK);
            start_panel = constrain(start_panel, 0, items.size()-1);
        } else {
            start_panel = 0;
            tft->fillRect(0, 0, tft->width(), tft->height(), tft->BLACK);
        }

        tft->setCursor(0,0);

        #ifdef LOOP_MARKERS
        y = draw_loop_markers();
        #endif
        tft->setCursor(0, y);
        y = draw_message();

        // draw each menu item's panel
        //int start_y = 0;
        for (int i = start_panel ; i < items.size() ; i++) {
            MenuItem *item = items.get(i);
            //int time = millis();
            y = item->display(Coord(0,y), i==currently_selected, i==currently_opened) + 1;

            if (!bottoms_computed) {
                panel_bottom[i] = y;// - start_y;
                //start_y = y;
            }
            //Serial.printf("menuitem %i took %i to refresh\n", i, millis()-time);
        }
        bottoms_computed = true;

        // control debug output (knob positions / button presses)
        tft->setCursor(0, y);
        tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        //tft->println();
        tft->setTextSize(2);
        tft->printf("K:%2i B:%2i\n", last_knob_position, button_count);
        tft->printf("S:%2i O:%2i\n", currently_selected, currently_opened);
        tft->printf("Mem:%i\n", freeRam());
    }

    //tft->updateScreenAsync(false);
    tft->updateDisplay();

    return y;
}