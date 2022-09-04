#include <LinkedList.h>

#include "menuitems.h"

class MultiToggleItemBase {
    public:
        MultiToggleItemBase(char *label) {
            this->label = label;
        }
        char *label;
        virtual bool do_getter();
        virtual void do_setter(bool value);
};

template<class TargetClass>
class MultiToggleItemClass : public MultiToggleItemBase{
    public:
        TargetClass *target;
        void(TargetClass::*setter)(bool);
        bool(TargetClass::*getter)();

        MultiToggleItemClass(char *label, TargetClass *target, void(TargetClass::*setter)(bool), bool(TargetClass::*getter)()) : MultiToggleItemBase(label) {
            this->target = target;
            this->setter = setter;
            this->getter = getter;
        }

        virtual bool do_getter() override {
            return ((*this->target).*(this->getter))();
            //return (*this->target)*(this->getter)();
            //(item.target->*item.setter)
            //(item.target->*item.setter)
            //return (this->target*)->getter();
        }
        virtual void do_setter(bool value) override {
            ((*this->target).*(this->setter))( value );
        }
};

class MultiToggleItemFunction : public MultiToggleItemBase {
    public:
        void(*setter)(bool) = nullptr;
        bool(*getter)() = nullptr;

        MultiToggleItemFunction(char *label, void(*setter)(bool), bool(*getter)()) : MultiToggleItemBase(label) {
            this->setter = setter;
            this->getter = getter;
        }

        virtual bool do_getter() override {
            return this->getter();
            //return (this->target*)->getter();
        }
        virtual void do_setter(bool value) override {
            this->setter(value);
        }
};

class ObjectMultiToggleControl : public MenuItem {
    public:
        bool all_option = false;    // whether to add an 'all' toggle option
        bool all_status = false;    // current status of the 'all' toggle

        int initial_on_count = 0;
        
        LinkedList<MultiToggleItemBase*> items = LinkedList<MultiToggleItemBase*>();

        ObjectMultiToggleControl(const char *label ) : MenuItem(label) {}
        ObjectMultiToggleControl(const char *label, bool enable_all_option) : ObjectMultiToggleControl(label) {
            this->all_option = enable_all_option;
        }

        virtual void addItem(MultiToggleItemBase *item) {
            this->items.add(item);
            currently_selected = 0; // set first item as selected

            // set the default 'all' option to whichever is the most common of the sub-items
            if (all_option) {
                if (item->do_getter()) {
                    initial_on_count++;
                }
                all_status = initial_on_count > this->items.size()/2;
            }
        }

        int currently_selected = -1;

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            #define FONT_WIDTH 6

            //tft->printf("width_per_item: %i\n", width_per_item);
            pos.y = tft->getCursorY();

            colours(opened, opened ? GREEN : C_WHITE, BLACK);
            tft->setTextSize(1);

            int max_height_reached = 0;
            int x = 0;
            int start_y = pos.y;

            const uint8_t items_size = items.size();// + (all_option ? 1 : 0);

            bool all_selected = false;

            int effectively_selected = 0;

            if (all_option) {
                effectively_selected = currently_selected - 1;
                if (currently_selected==0) all_selected = true;
                colours(all_selected && opened, all_status ? GREEN : RED, BLACK);
                tft->setCursor(x, pos.y);
                tft->println("[ALL] ");
                x += (7 * FONT_WIDTH);  // 7 being the size if "[ALL] " + 1
                tft->setCursor(x, pos.y);
            }

            //int width_per_item = (tft->width() / FONT_WIDTH) / (items.size() + (all_option ? 1 : 0));    // max size to be used for each item
            int width_per_item = ((tft->width()-x) / FONT_WIDTH) / items.size(); // max size to be used for each item after the 'all' item is taken into account

            for (uint8_t i = 0 ; i < items_size ; i++) {
                MultiToggleItemBase *item = items.get(i);
                //Serial.printf("processing item %s\n", item->label);

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==effectively_selected) && opened, item->do_getter() ? GREEN : RED, BLACK);

                // segment the label of the item up over multiple lines of `width_per_item` chars each
                char tmp[width_per_item];
                for (uint8_t segment_start = 0 ; segment_start < strlen(item->label) ; segment_start += width_per_item-1) {
                    // copy width_per_item characters or max remaining
                    int len_to_copy = min(strlen(item->label) - segment_start, width_per_item-1);

                    //Serial.printf("\tcpos = %i, width_per_item = %i, len_to_copy = %i, strlen(item.label) = %i\n", segment_start, width_per_item, len_to_copy, strlen(item.label));
                    memcpy(&tmp[0], &item->label[segment_start], len_to_copy);
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
            if (currently_selected < 0) 
                currently_selected = (this->all_option?1:0) + items.size() - 1;
            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool knob_right() {
            currently_selected++;
            if (currently_selected >= (this->all_option?1:0) + items.size())
                currently_selected = 0;
            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool button_select() {
            if (all_option && currently_selected == 0) { //items.size()) {
                all_status = !all_status;
                for (int i = 0 ; i < items.size() ; i++) {
                    items.get(i)->do_setter(all_status);
                }
                static char tmp[40];
                sprintf(tmp, "Toggled all to %s", all_status?"on":"off");
                menu->set_last_message(tmp);
            } else if (currently_selected>=0) {
                /*MultiToggleItem<TargetClass> item = items.get(currently_selected);
                (item.target->*item.setter)( ! (item.target->*item.getter)() );*/
                int effectively_selected = all_option ? currently_selected-1 : currently_selected;
                MultiToggleItemBase *item = items.get(effectively_selected);
                bool new_mode = !item->do_getter();
                item->do_setter(new_mode);
                static char tmp[40];
                sprintf(tmp, "Toggled %s to %s", item->label, new_mode?"on":"off");
                menu->set_last_message(tmp);
            }
            return false; //go_back_on_select;
        }
};