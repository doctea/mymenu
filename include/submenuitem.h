#include "menuitems.h"

class SubMenuItem : public MenuItem {
    public:
        int currently_selected = -1;
        int currently_opened = -1;
        LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

        SubMenuItem(char *label) : MenuItem(label) {}

        void on_add() override {
            for (int i = 0 ; i < this->items.size() ; i++) {
                this->items.get(i)->set_tft(this->tft);
                this->items.get(i)->on_add();
            }
        }

        void add(MenuItem *item) {
            this->items.add(item);
        }

        int display(Coord pos, bool selected, bool opened) {
            int y = header(this->label, pos, selected, opened);

            for (int i = 0 ; i < this->items.size() ; i++) {
                y = this->items.get(i)->display(Coord(0,y), i==this->currently_selected, i==this->currently_opened);
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
                if (items.get(currently_opened)->button_select())
                    button_back();
                else
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