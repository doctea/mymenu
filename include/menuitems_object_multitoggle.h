#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#ifndef OBJECT_MULTITOGGLED__INCLUDED
#define OBJECT_MULTITOGGLED__INCLUDED

#include <LinkedList.h>

#include "menuitems.h"

// objects that present a a bunch of on/off controls in a row
// handles splitting the labels over multiple lines to fit the available space
// and also provides an (optional) 'toggle-all' option that will set them all to a particular state on/off

// base multi-toggle sub-select item
class MultiToggleItemBase {
    public:
        MultiToggleItemBase(const char *label) {
            this->label = label;
        }
        const char *label;
        inline virtual const char *get_label() {
            return this->label;
        }
        virtual bool do_getter() = 0;
        virtual void do_setter(bool value) = 0;
};

// multi-toggle item that targts getters & setters on an object
template<class TargetClass>
class MultiToggleItemClass : public MultiToggleItemBase{
    public:
        TargetClass *target;
        void(TargetClass::*setter)(bool);
        bool(TargetClass::*getter)();

        MultiToggleItemClass(const char *label, TargetClass *target, void(TargetClass::*setter)(bool), bool(TargetClass::*getter)()) : MultiToggleItemBase(label) {
            this->target = target;
            this->setter = setter;
            this->getter = getter;
        }

        virtual bool do_getter() override {
            return ((*this->target).*(this->getter))();
        }
        virtual void do_setter(bool value) override {
            ((*this->target).*(this->setter))( value );
        }
};

// multi-toggle item that targets a getter & setter function
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

// actual menuitem that holds multi-toggle items
// TODO: multi-column mode
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
                all_status = initial_on_count > ((int)this->items.size())/2;
            }
        }

        int currently_selected = -1;

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            #define FONT_WIDTH 6

            //tft->printf("width_per_item: %i\n", width_per_item);
            pos.y = tft->getCursorY();

            colours(opened, opened ? GREEN : this->default_fg, this->default_bg);
            tft->setTextSize(1);

            int max_height_reached = 0;
            int x = 0;
            int start_y = pos.y;

            const uint_fast8_t items_size = items.size();

            bool all_selected = false;

            int effectively_selected = 0;

            //const char *all_label = all_status ? "[*]" : "[-]"; //"[ALL]";
            const char *all_label = all_status ? "*" : "-"; //"[ALL]";

            if (all_option) {
                effectively_selected = currently_selected - 1;
                if (currently_selected==0) all_selected = true;
                colours(all_selected && opened, all_status ? GREEN : RED, this->default_bg);
                tft->setCursor(x, pos.y);
                tft->println(all_label);
                x += ((strlen(all_label)+1) * FONT_WIDTH);  // 7 being the size if "[ALL] " + 1
                tft->setCursor(x, pos.y);
            }

            //int width_per_item = (tft->width() / FONT_WIDTH) / (items.size() + (all_option ? 1 : 0));    // max size to be used for each item
            const int width_per_item = ((tft->width()-x) / FONT_WIDTH) / items.size(); // max size to be used for each item after the 'all' item is taken into account

            //Serial.printf("for tft width=%i,\tfont_width=%i,\titems.size=%i\n", tft->width(), FONT_WIDTH, items.size());
            //Serial.printf("so got width_per_item %i\n", width_per_item);
            //width_per_item = constrain(width_per_item, 1, tft->width()/FONT_WIDTH);
            
            for (uint_fast8_t i = 0 ; i < (uint_fast8_t)items_size ; i++) {
                MultiToggleItemBase *item = items.get(i);
                //Serial.printf("processing item %s\n", item->label);

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==effectively_selected) && opened, item->do_getter() ? GREEN : RED, this->default_bg);

                // segment the label of the item up over multiple lines of `width_per_item` chars each
                char tmp[width_per_item+1];
                unsigned int last_length = max(1,width_per_item-1);
                if (last_length>1 && i+1==items_size)    // cheeky little mod to use all the remaining space if we get to the end
                    last_length = ((tft->width() - tft->getCursorX()) / FONT_WIDTH) - 1;

                const char *item_label = item->get_label();
                for (unsigned int segment_start = 0 ; segment_start < strlen(item_label) ; segment_start += last_length) {
                    if (item_label[segment_start]==' ' && last_length>1)   // if the first character is a space, and column is wider than 1, skip the space to improve alignment
                        segment_start++;
                    strncpy(tmp, &item_label[segment_start], last_length);
                    last_length = min(last_length, strlen(tmp));
                    tft->setCursor(x, tft->getCursorY());
                    tmp[last_length] = '\0';
                    //Serial.printf("got '%s'\n", tmp);
                    //tft->printf("%s\n", tmp);
                    tft->println(tmp);
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

        bool held = false;      // whether button is held
        bool held_mode = false; // what mode we should set held ones to

        virtual void update_held(int currently_selected) {
            if (all_option && currently_selected==0) // do nothing if the selected item is the 'all' option
                return;
            int effectively_selected = all_option ? currently_selected-1 : currently_selected;
            MultiToggleItemBase *item = this->items.get(effectively_selected);
            if (item->do_getter()!=this->held_mode) {
                //Serial.printf("update_held setting %i on %i\n", this->held_mode, effectively_selected);
                item->do_setter(this->held_mode);
            }
        }

        virtual bool knob_left() override {
            currently_selected--;
            if (currently_selected < 0) 
                currently_selected = (this->all_option?1:0) + (int)items.size() - 1;
            if (this->held)
                this->update_held(currently_selected);
            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool knob_right() override {
            currently_selected++;
            if (currently_selected >= (this->all_option?1:0) + (int)items.size())
                currently_selected = 0;
            if (this->held) 
                this->update_held(currently_selected);

            //Serial.printf("knob_right: selected %i\n", currently_selected);
            return true;
        }
        virtual bool button_select() override {
            if (all_option && currently_selected == 0) { // toggle all select
                this->toggle_all();
            } else if (currently_selected>=0) {
                this->held = true;
                /*MultiToggleItem<TargetClass> item = items.get(currently_selected);
                (item.target->*item.setter)( ! (item.target->*item.getter)() );*/
                int effectively_selected = all_option ? currently_selected-1 : currently_selected;
                MultiToggleItemBase *item = items.get(effectively_selected);
                bool new_mode = !item->do_getter();
                this->held_mode = new_mode;
                item->do_setter(new_mode);
                static char tmp[40];
                snprintf(tmp, 40, "Toggled %s to %s", item->label, new_mode?label_on:label_off);
                menu->set_last_message(tmp);
            }
            return go_back_on_select;
        }
        virtual bool button_select_released() override {
            this->held = false;
            return false;
        }
        virtual bool switch_all(bool on = true) {
            //all_status = !all_status;
            all_status = on;
            for (unsigned int i = 0 ; i < items.size() ; i++) {
                items.get(i)->do_setter(all_status);
            }
            static char tmp[MENU_C_MAX];
            snprintf(tmp, MENU_C_MAX, "Toggled all %s to %s", this->label, all_status?label_on:label_off);
            menu->set_last_message(tmp);
            return all_status;
        }
        virtual bool toggle_all() {
            return this->switch_all(!all_status);
        }
};

class ObjectMultiToggleColumnControl : public ObjectMultiToggleControl {
    public:

        ObjectMultiToggleColumnControl(const char *label ) : ObjectMultiToggleControl(label) {}
        ObjectMultiToggleColumnControl(const char *label, bool enable_all_option) : ObjectMultiToggleColumnControl(label) {
            this->all_option = enable_all_option;
        }

        char fmt[MENU_C_MAX] = "-";
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            pos.y = tft->getCursorY();

            colours(opened, opened ? GREEN : this->default_fg, this->default_bg);
            tft->setTextSize(1);

            uint_fast16_t max_height_reached = 0;
            uint_fast16_t x = 0;
            uint_fast16_t start_y = pos.y;

            const uint_fast8_t num_columns = 2;
            const uint_fast8_t items_size = items.size();

            bool all_selected = false;

            int effectively_selected = 0;

            const char *all_label = all_status ? "[*]" : "[-]"; 

            if (all_option) {
                effectively_selected = currently_selected - 1;
                if (currently_selected==0) all_selected = true;
                colours(all_selected && opened, all_status ? GREEN : RED, this->default_bg);
                tft->setCursor(x, pos.y);
                tft->println(all_label);
                x += ((strlen(all_label)+1) * tft->currentCharacterWidth());
                tft->setCursor(x, pos.y);
                tft->println();
            }

            const int width_per_item = (tft->width() / num_columns);
            const int items_per_column = items_size / num_columns;

            if (fmt[0]=='-')
                snprintf(fmt, MENU_C_MAX, "%%-%is\n\0", width_per_item/tft->currentCharacterWidth()); // becomes eg "%-6s\n"
            //tft->printf("got column width %i and format '%s'\n", width_per_item, fmt);

            start_y = tft->getCursorY();

            for (uint_fast8_t i = 0 ; i < (uint_fast8_t)items_size ; i++) {
                MultiToggleItemBase *item = items.get(i);
                // move to next column if we need to
                if (i == items_per_column) {
                    if (tft->getCursorY()>max_height_reached)                
                        max_height_reached = tft->getCursorY(); // remember how far down the screen we've drawn

                    tft->setCursor(width_per_item, start_y);
                } else if (i > items_per_column) {
                    tft->setCursor(tft->width()/num_columns, pos.y);
                }

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==effectively_selected) && opened, item->do_getter() ? GREEN : RED, this->default_bg);

                //tft->printf((const char*)fmt, (char*)item->get_label());  // limits size but is LOADS 7fps slower!
                tft->println(item->get_label());

                pos.y = tft->getCursorY();
            }
            
            return max_height_reached; //tft->getCursorY();
        }
};


#endif