#ifdef ENABLE_SCREEN

#include "menu.h"
#include "menuitems.h"

#include <LinkedList.h>

int freeRam();
#ifdef ARDUINO_TEENSY41
    int freeExtRam();
#endif

#include <string>

bool debug_flag = false;

#define MAX_KNOB 1024

#ifndef CORE_TEENSY
    // if no FLASHMEM then we're probably not running on Teensy platform, so define it empty
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif
    //#define F(x) { x }
#endif


int Menu::display_pinned() {
    int y = tft->getCursorY();
    if (profile_enable) {
        tft->setCursor(0, y);
        Serial.println(profile_string);
        tft->setTextSize(0);
        tft->printf(tft->get_message_format(), profile_string);
        //tft->println("wtf?");
        y = tft->getCursorY() + 3;
    }

    if (pinned_panel!=nullptr) {
        //if (debug) { Serial.println("display()=> doing pinned_panel display"); Serial_flush(); }
        y = pinned_panel->display(Coord(0,y), false, false);
    }

    return y;
}

// draw the menu display
int Menu::display() {
    bool debug = this->debug;

    // early return if display isn't ready for writing (mostly used for dma checks)
    if (!this->tft->ready()) {
	    return 0;
    }

    uint32_t display_started = 0;
    static int frames_drawn = 0;
    static int frames_in_last_second = 0;
    static uint32_t current_second = 0, last_second = 0;

    if (profile_enable) {
        display_started = micros();
        current_second = micros()/(1000*1000);
        if (current_second!=last_second) {
            frames_in_last_second = frames_drawn;
            frames_drawn = 0;
            last_second = current_second;
        }
    }

    //Serial.printf("display !");
    int y = 0;
    
    if (debug) { Serial.println("display()=> about to setTextColor()"); Serial_flush(); }
    tft->setTextColor(C_WHITE, BLACK);

    int currently_opened = selected_page->currently_opened;
    int currently_selected = selected_page->currently_selected;
    //if (debug) { Serial.printf("display()=> about to get the selected_page items.. "); Serial_flush(); }
    LinkedList<MenuItem*> *items = selected_page->items;
    //if (debug) { Serial.printf("currently_opened = %i/%i, currently_selected = %i/%i..\n", currently_opened+1, selected_page->items->size(), currently_selected+1, selected_page->items->size()); Serial_flush(); }

    // now draw the menu
    //if (debug) { Serial.println("display()=> about to check display mode and branch.."); Serial_flush(); }
    if (is_item_opened() && items->get(currently_opened)->allow_takeover()) {
        //if (debug) { Serial.println("display()=> takeover branch"); Serial_flush(); }
        // takeover -- focus on this menu item only
        tft->setCursor(0, y);

        y = display_pinned();
        //y = draw_message();
        // let the currently opened item take care of drawing all of the display
        //Serial.println("allow_takeover!");
        //if (debug) { Serial.printf("display()=> doing display of item %i\n", currently_opened); Serial_flush(); }
        //if (debug) { Serial.printf("display()=> got item to display '%s'\n", items->get(currently_opened)->label); Serial_flush(); }
        items->get(currently_opened)->display(Coord(0,y), true, true);
        //if (debug) { Serial.println("display()=> finished display\n"); Serial_flush(); }
    } else if (mode==DISPLAY_ONE) {
        //if (debug) { Serial.println("display()=> DISPLAY_ONE branch"); Serial_flush(); }
        // for screens that can only display one option at a time
        static int last_displayed = 0;
        if (last_displayed!=currently_selected)
            tft->clear();
        tft->setCursor(0, y);

        y = display_pinned();

        tft->setCursor(0, y);
        
        //if (debug) { Serial.println("display()=> about to draw_message()"); Serial_flush(); }
        y = draw_message();

        if (currently_selected>=0 && currently_selected < (int)items->size()) {
            tft->setTextSize(0);
            items->get(currently_selected)->display(Coord(0,y), true, currently_opened==currently_selected);
            last_displayed = currently_selected;
        }

        tft->setTextColor(C_WHITE, BLACK);
        while(tft->getCursorY() < tft->height())
            tft->println("");
        tft->println("");

    } else {
        //if (debug) { Serial.println("display()=> default branch"); Serial_flush(); }
        //static bool first_display = true;
        //static int *panel_bottom = nullptr;
        bool bottoms_computed = false;
        if (selected_page->panel_bottom == nullptr) {
            selected_page->panel_bottom = (int*)CALLOC_FUNC(this->get_num_panels(), sizeof(int));
        } else {
            if (!this->recalculate_bottoms)
                bottoms_computed = true;
            else
                this->recalculate_bottoms = false;
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
        if (this->selected_page->scrollable && panel_bottom[currently_selected] >= screen_height_cutoff) {
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

        #ifndef ALT_MENU_POSITIONING
            if(this->selected_page->scrollable)
                start_panel = constrain(start_panel-2, 0, (int)items->size()-1);
            // ^^^ whether this is useful appears to depend on what menus we're on... disabled it for better behaviour on the ParameterInput pages on Microlidian
            // TODO: fix this behaviour once and for all
        #endif

        static int last_start_panel = -1;
        if (last_start_panel!=start_panel) {
            tft->clear(true);
        }
        last_start_panel = start_panel;

        tft->setCursor(0,y);

        y = display_pinned();

        tft->setCursor(0, y);
        
        //if (debug) { Debug_println(F("display()=> about to draw_message()")); Serial_flush(); }
        int new_y = draw_message();
        if (new_y==y)
            tft->println(); // force dropping a line if draw_message hasn't for some reason (needed to get the tabs line to show on bodmer/rp2040?)
        y = tft->getCursorY();
        //tft->setCursor(0, new_y);

        tft->setTextSize(tab_textsize);

        const int tft_width = tft->width();
        const int current_character_width = tft->currentCharacterWidth();

        /////// draw tabs for the pages
        for (unsigned int i = selected_page_index ; i < pages->size() + selected_page_index ; i++) {
            int ci = i;
            if (ci >= (int)pages->size())        // wrap around to start of list if we get to the end
                ci = ci % pages->size();

            if ((int)i==selected_page_index && !is_page_opened())
                tft->setTextColor(BLACK, pages->get(ci)->colour);
            else
                tft->setTextColor(pages->get(ci)->colour, BLACK);

            int cursor_x = tft->getCursorX();
            //int characters_left = ((tft->width() - tft->getCursorX()) / tft->currentCharacterWidth()) - 1; // + (tft->characterWidth() * (strlen(pages->get(ci)->title)+1)));
            int characters_left = ((tft_width - cursor_x) / current_character_width) - 1;
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
            // we havent had enough tabs to move down a line, so force one
            tft->println();
        }
        y = tft->getCursorY();
        tft->setTextSize(0);
        /// finished drawing tabs

        // draw each menu item
        //int start_y = 0;
        //if (debug) { Serial.println("display()=> about to start drawing the items.."); Serial_flush(); }
        const uint_fast16_t size = items->size();
        for (uint_fast16_t i = start_panel ; i < size ; i++) {
            //if (debug) { Serial.printf("display()=> about to get item %i\n", i); Serial_flush(); }
            MenuItem *item = items->get(i);
            /*if(item->label[0]=='T')
                delay(5000);*/

            //int time = millis();
            //Serial.printf("Menu rendering item %i [selected #%i, opened #%i]\n", i, currently_selected, currently_opened);
            Coord pos = Coord(0,y);
            //if (debug) { Serial.printf("display()=> about to display() item %i aka %s\n", i, item->label); Serial_flush(); }

            unsigned long time_micros = 0;
            if (this->debug_times) time_micros = micros();
            tft->setTextSize(0);
            y = item->display(pos, (int)i==currently_selected, (int)i==currently_opened) + 1;
            //Serial.printf("after rendering MenuItem %i, return y is %i, cursor coords are (%i,%i)\n", y, tft->getCursorX(), tft->getCursorY());
            //if (debug) { Serial.printf("display()=> just did display() item %i aka %s\n", i, item->label); Serial_flush(); }

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
    if (auto_update)
        this->updateDisplay();

    if (profile_enable) {
        /*char * ptr = (char*)extmem_malloc(1024 << 10);
        if (!ptr)
          Serial.println("failed to allocate from extmem!");
        else
          sprintf(ptr, "hello");*/
        #ifdef ARDUINO_TEENSY41
            snprintf(
                profile_string, 
                MENU_C_MAX, 
                "%-2ifps | RAM2 %uK EXT %uK free", 
                frames_in_last_second,
                freeRam()/1024,
                freeExtRam()/1024
            );
        #else
            snprintf(
                profile_string, 
                MENU_C_MAX, 
                "%-2ifps | RAM2 %uK free", 
                frames_in_last_second,
                freeRam()/1024
            );
        #endif
        frames_drawn++;
    }

    return y;
}


#include "menuitems_quickpage.h"

void Menu::setup_quickjump() {
    quick_page_index = add_page("QuickJump");
    menu->add(new QuickPagesMenuItem("QuickJump history"));
    menu->remember_opened_page(0);  // add the Main page to the quickjump list by default
}


void menu_set_last_message(const char *msg, int colour) {
    menu->set_last_message(msg);
    menu->set_message_colour(colour);
}

#endif