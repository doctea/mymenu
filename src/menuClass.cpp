#include "menu.h"
#include "menuitems.h"

#include <LinkedList.h>

#define MAX_KNOB 1024

#ifndef CORE_TEENSY
    // if no FLASHMEM then we're probably not running on Teensy platform, so define it empty
    #define FLASHMEM
    //#define F(x) { x }
#endif

// draw the menu display
int Menu::display() {
    bool debug = this->debug;
    //Serial.printf("display !");
    int y = 0;
    
    if (debug) { Serial.println("display()=> about to setTextColor()"); Serial_flush(); }
    tft->setTextColor(C_WHITE, BLACK);

    int currently_opened = selected_page->currently_opened;
    int currently_selected = selected_page->currently_selected;
    LinkedList<MenuItem*> *items = selected_page->items;

    // now draw the menu
    if (currently_opened>=0 && items->get(currently_opened)->allow_takeover()) {
        // takeover -- focus on this menu item only
        if (pinned_panel!=nullptr)
            y = pinned_panel->display(Coord(0,0), false, false);
            //y = draw_message();
        // let the currently opened item take care of drawing all of the display
        //Serial.println("allow_takeover!");
        items->get(currently_opened)->display(Coord(0,y), true, true);
    } else if (mode==DISPLAY_ONE) {
        // for screens that can only display one option at a time
        static int last_displayed = 0;
        if (last_displayed!=currently_selected)
            tft->clear();
        tft->setCursor(0, 0);

        if (pinned_panel!=nullptr) {
            Serial.println("display()=> about to pinned_panel->display()");
            y = pinned_panel->display(Coord(0,0), false, false);
            Serial.println("display()=> did pinned_panel->display()!"); 
        }

        tft->setCursor(0, y);
        
        if (debug) { Serial.println("display()=> about to draw_message()"); Serial_flush(); }
        y = draw_message();

        if (currently_selected>=0 && currently_selected < (int)items->size()) {
            items->get(currently_selected)->display(Coord(0,y), true, currently_opened==currently_selected);
            last_displayed = currently_selected;
        }

        tft->setTextColor(C_WHITE, BLACK);
        while(tft->getCursorY() < tft->height())
            tft->println("");
        tft->println("");

    } else {
        //static bool first_display = true;
        //static int *panel_bottom = nullptr;
        bool bottoms_computed = false;
        if (selected_page->panel_bottom == nullptr) {
            selected_page->panel_bottom = (int*)calloc(this->get_num_panels(), sizeof(int));
        } else {
            bottoms_computed = true;
        }
        int *panel_bottom = selected_page->panel_bottom;
        /*if (first_display) {
            panel_bottom = (int*)malloc(this->get_num_panels() * sizeof(int));
            first_display = false;
        }*/
        //static int panel_bottom[MENU_MAX_PANELS];
        //static bool bottoms_computed = false;

        // find number of panels to offset in order to ensure that selected panel is on screen?
        int start_panel = 0;
        if (currently_selected>0 && panel_bottom[currently_selected] >= (0.75*tft->height())) {
            start_panel = currently_selected - 1;
            //#ifdef OLD_SCROLL_METHOD
            // count backwards to find number of panels we have to go to fit currently_selected on screen...
            int count_y = panel_bottom[currently_selected];
            for (unsigned int i = currently_selected ; i > 0 ; i--) {
                count_y -= panel_bottom[i];
                if (count_y + panel_bottom[currently_selected] < tft->height() / 2) {
                    start_panel = i;
                    break;
                }
            }
            //#endif
            #ifdef NEW_SCROLL_METHOD_NONWORKING
            // count forward until we find the first item we can start on that will include the item on screen
            int target_y = panel_bottom[currently_selected];
            int adj_y = 0;
            for (unsigned int i = 0 ; i < items.size() ; i++) {
                adj_y += panel_bottom[i];
                Debug_printf(F("item %i accumulated height %i trying to fit into %i\n"), i, adj_y, tft->height());
                if (target_y - adj_y < tft->height()) {
                    Serial.println(F("\tyes!"));
                    start_panel = i;
                    break;
                }                
            }
            #endif

            //tft.fillWindow(ST77XX_BLACK);
            tft->clear();
            //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
            start_panel = constrain(start_panel, 0, (int)items->size()-1);
        } else {
            start_panel = 0;
            tft->clear();
            //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
        }
        //if (panel_bottom[currently_selected] >= tft->height()/2)
        //    start_panel = currently_selected - 1;
        start_panel = constrain(start_panel-2, 0, (int)items->size()-1);

        static int last_start_panel = -1;
        if (last_start_panel!=start_panel) {
            tft->clear(true);
        }
        last_start_panel = start_panel;

        tft->setCursor(0,0);

        if (pinned_panel!=nullptr) {
            //if (debug) { Debug_println(F("display()=> about to pinned_panel->display()")); Serial_flush(); }
            y = pinned_panel->display(Coord(0,0), false, false);
            //if (debug) { Debug_println(F("display()=> did pinned_panel->display()!")); Serial_flush(); }
        }

        tft->setCursor(0, y);
        
        //if (debug) { Debug_println(F("display()=> about to draw_message()")); Serial_flush(); }
        y = draw_message();

        // draw tabs for pages
        for (unsigned int i = selected_page_index ; i < pages->size() + selected_page_index ; i++) {
            int ci = i;
            if (ci >= (int)pages->size())        // wrap around to start of list if we get to the end
                ci = ci % pages->size();

            //if (tft->getCursorX() + (tft->characterWidth() * (strlen(pages->get(ci)->title)+1)) >= tft->width())
            //    break;  // break if we'd go off the screen rendering this item TODO: draw the available characters

            if ((int)i==selected_page_index)
                tft->setTextColor(BLACK, pages->get(ci)->colour);
            else
                tft->setTextColor(pages->get(ci)->colour, BLACK);

            int characters_left = ((tft->width() - tft->getCursorX()) / tft->characterWidth()) - 1; // + (tft->characterWidth() * (strlen(pages->get(ci)->title)+1)));
            if (characters_left<2) {
                break;
            } else if (characters_left < (int)strlen(pages->get(ci)->title)+1) {
                char title[MENU_C_MAX];
                strncpy(title, pages->get(ci)->title, characters_left);
                title[characters_left] = '\0';
                title[characters_left-2] = '.';
                title[characters_left-1] = '.';
                //title[characters_left] = '\0';
                tft->print(title);
                break;  // break if we'd go off the screen rendering this item TODO: draw the available characters
            }

            tft->print(pages->get(ci)->title);
            tft->setTextColor(tft->rgb(196,196,196), BLACK);
            tft->print("|");
            tft->setTextColor(C_WHITE, BLACK);
        }
        if (y == tft->getCursorY()) {
            // we havent had enough panels to move down a line, so force one
            tft->println();
        }
        //tft->println();
        y = tft->getCursorY();

        // draw each menu item's panel
        //int start_y = 0;
        for (unsigned int i = start_panel ; i < items->size() ; i++) {
            MenuItem *item = items->get(i);
            /*if(item->label[0]=='T')
                delay(5000);*/

            //int time = millis();
            //Serial.printf("Menu rendering item %i [selected #%i, opened #%i]\n", i, currently_selected, currently_opened);
            Coord pos = Coord(0,y);
            if (debug) { Serial.printf("display()=> about to display() item %i aka %s\n", i, item->label); Serial_flush(); }

            unsigned long time_micros = 0;
            if (this->debug_times) time_micros = micros();
            y = item->display(pos, (int)i==currently_selected, (int)i==currently_opened) + 1;
            //Serial.printf("after rendering MenuItem %i, return y is %i, cursor coords are (%i,%i)\n", y, tft->getCursorX(), tft->getCursorY());
            if (debug) { Serial.printf("display()=> just did display() item %i aka %s\n", i, item->label); Serial_flush(); }

            if (this->debug_times) {
                tft->setTextSize(1);
                tft->printf("Took: %u\n", micros() - time_micros);
                y = y + tft->getRowHeight();
            }

            if (!bottoms_computed) {
                panel_bottom[i] = y;// - start_y;
                //start_y = y;
            }

            if (bottoms_computed && y >= this->tft->height())
                break;
            //Serial.printf("menuitem %i took %i to refresh\n", i, millis()-time);
            //if (y >= tft->height()) // stop rendering if we've reached the bottom of the screen?
            //    break;
        }
        bottoms_computed = true;

        // control debug output (knob positions / button presses)
        /*if (y < tft->height()) {
            tft->setCursor(0, y);
            tft->setTextColor(C_WHITE, BLACK);
            //tft->println();
            tft->setTextSize(2);
            //tft->printf("K:%2i B:%2i\n", last_knob_position, button_count);
            //tft->printf("S:%2i O:%2i\n", currently_selected, currently_opened);
            tft->printf("Mem: %i\n"), freeRam();
        }*/
        if (debug) { Debug_println("Done in main draw part"); Serial_flush(); }
    }

    //tft->updateScreenAsync(false);
    if (debug) { Debug_println("display()=> about to tft->updateDisplay()"); Serial_flush(); }
    tft->updateDisplay();

    return y;
}

void menu_set_last_message(const char *msg, int colour) {
    menu->set_last_message(msg);
    menu->set_message_colour(colour);
}

