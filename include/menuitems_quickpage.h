#pragma once

#include <Arduino.h>

#include "menuitems.h"

#include "menu.h"

class QuickPagesMenuItem : public MenuItem {
    public:
        QuickPagesMenuItem(const char *label) : MenuItem(label) {}

        int selected_value_index = -1;

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            if (selected_value_index<0 && menu->quick_page_history_total>0) {
                selected_value_index = 0;
            }
            for (int i = 0 ; i < menu->NUM_QUICK_PAGE_HISTORY ; i++) {
                page_t *page = menu->get_quick_page(i);
                if (page==nullptr) 
                    continue;
                colours(opened && i==selected_value_index, page->colour);
                tft->println(page->title);
                colours(C_WHITE);
            }
            return pos.y;
        }

        virtual bool knob_left() override {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = menu->quick_page_history_total-1;
            return true;
        }

        virtual bool knob_right() override {
            selected_value_index++;
            if (selected_value_index >= (int)menu->quick_page_history_total)
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() override {
            if (selected_value_index<0)
                return false;
            if(Serial) Serial.printf("button_select for value %i (%i, %s)\n", selected_value_index, menu->quick_pages[selected_value_index], menu->get_quick_page(selected_value_index)->title);
            // todo: why isn't this properly opening the page and selecting first item?!
            menu->open_page(menu->quick_pages[selected_value_index]);
            page_t * selected_page = menu->get_selected_page();
            return false;
        }

};