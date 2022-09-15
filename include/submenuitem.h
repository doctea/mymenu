#ifndef SUBMENUITEM__INCLUDED
#define SUBMENUITEM__INCLUDED

#include "menuitems.h"

class SubMenuItem : public MenuItem {
    public:
        bool always_show = false;       // whether to hide items until menu is opened or not
        int currently_selected = -1;
        int currently_opened = -1;
        LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

        // always_show argument determines whether to show items even when menu isn't opened
        SubMenuItem(char *label, bool always_show = false) : MenuItem(label) {
            this->always_show = always_show;
        }

        bool allow_takeover() override {
            return this->always_show==false;
        }
        bool action_opened() override {
            if (this->allow_takeover())
                tft->clear();
            return MenuItem::action_opened();
        }

        void on_add() override {
            for (int i = 0 ; i < this->items.size() ; i++) {
                this->items.get(i)->set_tft(this->tft);
                this->items.get(i)->on_add();
            }
        }

        void add(MenuItem *item) {
            if (item!=nullptr)
                this->items.add(item);
        }

        int display(Coord pos, bool selected, bool opened) {
            static int last_opened = -2;
            if (opened!=last_opened)
                tft->clear();
            last_opened = opened;

            int y = header(this->label, pos, selected, opened);

            int start_item = currently_selected>=0 ? currently_selected : 0;

            if (opened || this->always_show) {
                //tft->clear();
                for (int i = start_item ; i < this->items.size() ; i++) {
                    y = this->items.get(i)->display(Coord(0,y), i==this->currently_selected, i==this->currently_opened);
                    if (y>=tft->height()) 
                        break;
                }
                // blank to bottom of screen
                if (y < tft->height()) {
                    while (y < tft->height()) {
                        for (int i = 0 ; i < tft->get_c_max() ; i++)
                            tft->print(" ");
                        y = tft->getCursorY();
                    }
                }
            } else {
                tft->printf("[%i sub-items...]\n", this->items.size());
                y = tft->getCursorY();
            }

            return y;
        }

        bool knob_left() override {
            if (currently_opened==-1) {
                currently_selected--;
                if (currently_selected<0)
                    currently_selected = items.size()-1;
                return true;
            } else {
                return this->items.get(currently_opened)->knob_left();
            }
        }
        bool knob_right() override {
            if (currently_opened==-1) {
                currently_selected++;
                if (currently_selected>=items.size())
                    currently_selected = 0;
                return true;
            } else {
                return this->items.get(currently_opened)->knob_right();
            }
        }

        bool button_select() override {
            if (currently_opened==-1) {
                if (currently_selected>=0) {
                    if (items.get(currently_selected)->action_opened()) {
                        currently_opened = currently_selected;
                        Serial.printf("submenuitem#button_select() opened subitem %i\n", currently_opened);
                        return false;
                    } else {
                        return true;
                    }
                }
            } else {
                if (items.get(currently_opened)->button_select()) {
                    button_back();
                    return false;
                } else
                    return false;
            }
            return true;
        }

        bool button_back() override {
            if (currently_opened!=-1 && !items.get(currently_opened)->button_back()) {
                currently_selected = currently_opened;
                currently_opened = -1;
            } else if (currently_opened==-1) {
                //currently_opened = 0;
                currently_selected = -1;
                return false;
            }
            return true;
        }

        bool button_right() {
            if (currently_opened!=-1) {
                if (items.get(currently_opened)->button_right()) {

                } else {

                }
            } else {

            }
            return true;
        }
};

#endif