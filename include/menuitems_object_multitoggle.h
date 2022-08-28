#include <LinkedList.h>

#include "menuitems.h"

template<class TargetClass>
struct MultiToggleItem {
    char *label;
    TargetClass *target;
    void(TargetClass::*setter)(bool);
    bool(TargetClass::*getter)();
};

template<class TargetClass>
class ObjectMultiToggleControl : public MenuItem {
    public:

        LinkedList<MultiToggleItem<TargetClass>> items = LinkedList<MultiToggleItem<TargetClass>>();

        ObjectMultiToggleControl(const char *label ) : MenuItem(label) {}

        virtual void addItem(MultiToggleItem<TargetClass> item) {
            this->items.add(item);
            currently_selected = 0; // set first item as selected
        }

        int currently_selected = -1;

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            #define FONT_WIDTH 6

            int width_per_item = (tft->width() / FONT_WIDTH) / items.size();    // max size to be used for each item
            //tft->printf("width_per_item: %i\n", width_per_item);
            pos.y = tft->getCursorY();

            colours(opened, opened ? GREEN : C_WHITE, BLACK);
            tft->setTextSize(1);

            int max_height_reached = 0;
            int x = 0;
            int start_y = pos.y;

            for (uint8_t i = 0 ; i < items.size() ; i++) {
                MultiToggleItem<TargetClass> item = items.get(i);

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==currently_selected) && opened, (item.target->*item.getter)() ? GREEN : RED, BLACK);

                // segment the label of the item up over multiple lines of `width_per_item` chars each
                char tmp[width_per_item+1];
                for (uint8_t segment_start = 0 ; segment_start < strlen(item.label) ; segment_start += width_per_item) {
                    // copy width_per_item characters or max remaining
                    int len_to_copy = min(strlen(item.label) - segment_start, width_per_item);

                    //Serial.printf("\tcpos = %i, width_per_item = %i, len_to_copy = %i, strlen(item.label) = %i\n", segment_start, width_per_item, len_to_copy, strlen(item.label));
                    memcpy(&tmp[0], &item.label[segment_start], len_to_copy);
                    tmp[len_to_copy] = '\0';
                    
                    //Serial.printf("\tdraw tmp = '%s' at %i,%i\n", tmp, x, pos.y);
                    tft->setCursor(x, tft->getCursorY());
                    tft->printf("%s\n", tmp);
                }
                // remember how far down the screen we've drawn
                if (tft->getCursorY()>max_height_reached)
                    max_height_reached = tft->getCursorY();

                x += (width_per_item * FONT_WIDTH); // move to the start of the next item along
                pos.y = start_y; // reset cursor position ready to draw the next item
                tft->setCursor(x, pos.y);
            }
            
            return max_height_reached; //tft->getCursorY();
        }

        virtual bool knob_left() {
            currently_selected--;
            if (currently_selected<0) 
                currently_selected = items.size() - 1;
            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool knob_right() {
            currently_selected++;
            if (currently_selected>=items.size())
                currently_selected = 0;
            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool button_select() {
            if (currently_selected>=0) {
                MultiToggleItem<TargetClass> item = items.get(currently_selected);
                (item.target->*item.setter)( ! (item.target->*item.getter)() );
            }
            return false; //go_back_on_select;
        }
};