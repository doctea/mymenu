#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#ifndef OBJECT_MULTITOGGLED__INCLUDED
#define OBJECT_MULTITOGGLED__INCLUDED

#include <GenericList.h>

#include "menuitems.h"

// objects that present a a bunch of on/off controls in a row
// handles splitting the labels over multiple lines to fit the available space
// and also provides an (optional) 'toggle-all' option that will set them all to a particular state on/off

// base multi-toggle sub-select item
class MultiToggleItemBase {
    public:
        bool invert_colours = false;
        unsigned int label_len = 0;

        MultiToggleItemBase(const char *label, bool invert_colours = false) {
            this->label = label;
            this->invert_colours = invert_colours;
            this->label_len = (label != nullptr) ? strlen(label) : 0;
        }
        const char *label;
        inline virtual const char *get_label() {
            return this->label;
        }
        inline virtual unsigned int get_label_len() {
            return this->label_len;
        }
        virtual bool do_getter() = 0;
        virtual void do_setter(bool value) = 0;
        inline virtual uint16_t get_colour() {
            return do_getter() ? 
                invert_colours ? RED : GREEN 
                : 
                invert_colours ? GREEN : RED;
        }
};

// multi-toggle item that targts getters & setters on an object
template<class TargetClass>
class MultiToggleItemClass : public MultiToggleItemBase {
    public:
        TargetClass *target;
        void(TargetClass::*setter)(bool);
        bool(TargetClass::*getter)();

        MultiToggleItemClass(const char *label, TargetClass *target, void(TargetClass::*setter)(bool), bool(TargetClass::*getter)(), bool invert_colours = false) 
            : MultiToggleItemBase(label, invert_colours) 
        {
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

// version of class that calls the underlying target item to get the "activated" colour - used by eg Pattern and Output selectors
template<class TargetClass>
class MultiToggleColourItemClass : public MultiToggleItemClass<TargetClass> {
    public:
        MultiToggleColourItemClass(const char *label, TargetClass *target, void(TargetClass::*setter)(bool), bool(TargetClass::*getter)(), bool invert_colours = false) 
            : MultiToggleItemClass<TargetClass>(label, target, setter, getter, invert_colours) 
            {}

        virtual uint16_t get_colour() {
            return this->invert_colours ? 
                (this->do_getter() ? GREY : this->target->get_colour()) :
                (this->do_getter() ? this->target->get_colour() : GREY)
                ;
        }
};

// multi-toggle item that targets a getter & setter function
class MultiToggleItemFunction : public MultiToggleItemBase {
    public:
        void(*setter)(bool) = nullptr;
        bool(*getter)() = nullptr;

        MultiToggleItemFunction(char *label, void(*setter)(bool), bool(*getter)(), bool invert_colours = false) : MultiToggleItemBase(label, invert_colours) {
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
class ObjectMultiToggleControl : public MenuItem {
    public:
        bool all_option = false;    // whether to add an 'all' toggle option
        bool all_status = false;    // current status of the 'all' toggle
        
        int initial_on_count = 0;
        
        GenericList<MultiToggleItemBase*> items;

        ObjectMultiToggleControl(const char *label) : MenuItem(label) {}
        ObjectMultiToggleControl(const char *label, bool enable_all_option, bool show_header = false) : ObjectMultiToggleControl(label) {
            this->flags.show_header = show_header;
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

            uint max_height_reached = 0;
            uint x = 0;
            uint start_y = pos.y;

            const uint_fast8_t items_size = items.size();
            if (items_size == 0) {
                return pos.y;
            }

            bool all_selected = false;

            uint effectively_selected = 0;

            //const char *all_label = all_status ? "[*]" : "[-]"; //"[ALL]";
            const char *all_label = all_status ? "*" : "-"; //"[ALL]";

            if (all_option) {
                effectively_selected = currently_selected - 1;
                if (currently_selected==0) all_selected = true;
                colours(all_selected && opened, all_status ? GREEN : RED, this->default_bg);
                tft->setCursor(x, pos.y);
                tft->println(all_label);
                x += (2 * FONT_WIDTH);  // single-char label plus trailing spacing
                tft->setCursor(x, pos.y);
            }

            //int width_per_item = (tft->width() / FONT_WIDTH) / (items.size() + (all_option ? 1 : 0));    // max size to be used for each item
            const int width_per_item = ((tft->width()-x) / FONT_WIDTH) / items.size(); // max size to be used for each item after the 'all' item is taken into account

            //Serial.printf("for tft width=%i,\tfont_width=%i,\titems.size=%i\n", tft->width(), FONT_WIDTH, items.size());
            //Serial.printf("so got width_per_item %i\n", width_per_item);
            //width_per_item = constrain(width_per_item, 1, tft->width()/FONT_WIDTH);
            
            uint_fast8_t i = 0;
            for (auto* item : items) {
                //Serial.printf("processing item %s\n", item->label);

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==effectively_selected) && opened, item->get_colour(), this->default_bg);

                // segment the label of the item up over multiple lines of `width_per_item` chars each
                char tmp[width_per_item+1];
                unsigned int last_length = max(1,width_per_item-1);
                if (last_length>1 && i+1==items_size)    // cheeky little mod to use all the remaining space if we get to the end
                    last_length = ((tft->width() - tft->getCursorX()) / FONT_WIDTH) - 1;

                const char *item_label = item->get_label();
                const unsigned int item_label_len = item->get_label_len();
                for (unsigned int segment_start = 0 ; segment_start < item_label_len ; segment_start += last_length) {
                    if (item_label[segment_start]==' ' && last_length>1)   // if the first character is a space, and column is wider than 1, skip the space to improve alignment
                        segment_start++;
                    const unsigned int remaining = item_label_len - segment_start;
                    const unsigned int segment_len = min(last_length, remaining);
                    strncpy(tmp, &item_label[segment_start], segment_len);
                    tft->setCursor(x, tft->getCursorY());
                    tmp[segment_len] = '\0';
                    //Serial.printf("got '%s'\n", tmp);
                    //tft->printf("%s\n", tmp);
                    tft->println(tmp);
                }
 
                // remember how far down the screen we've drawn
                if (tft->getCursorY()>(int)max_height_reached)
                    max_height_reached = tft->getCursorY();

                x += (width_per_item * FONT_WIDTH); // move to the start of the next item along
                pos.y = start_y; // reset cursor position ready to draw the next item
                tft->setCursor(x, pos.y);
                ++i;
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
            return flags.go_back_on_select;
        }
        virtual bool button_select_released() override {
            this->held = false;
            return false;
        }
        virtual bool switch_all(bool on = true) {
            //all_status = !all_status;
            all_status = on;
            for (auto* item : items) {
                item->do_setter(all_status);
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
        uint_fast8_t num_columns = 2;

        ObjectMultiToggleColumnControl(const char *label, bool enable_all_option = false, int num_columns = 2, bool show_header = true) : ObjectMultiToggleControl(label, enable_all_option, show_header) {
            this->num_columns = num_columns;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            pos.y = tft->getCursorY();

            colours(opened, opened ? GREEN : this->default_fg, this->default_bg);
            tft->setTextSize(1);

            uint_fast16_t max_height_reached = 0;
            uint_fast16_t x = 0;
            uint_fast16_t start_y = pos.y;

            const uint_fast8_t items_size = items.size();
            if (items_size == 0) {
                return pos.y;
            }

            bool all_selected = false;

            uint effectively_selected = 0;

            const char *all_label = all_status ? "[*]" : "[-]"; 

            if (all_option) {
                effectively_selected = currently_selected - 1;
                if (currently_selected==0) all_selected = true;
                colours(all_selected && opened, all_status ? GREEN : RED, this->default_bg);
                tft->setCursor(x, pos.y);
                tft->println(all_label);
                x += (2 * tft->currentCharacterWidth());
                tft->setCursor(x, pos.y);
                tft->println();
            }

            const uint_fast8_t safe_columns = this->num_columns > 0 ? this->num_columns : 1;
            const uint_fast8_t char_width = tft->currentCharacterWidth() > 0 ? tft->currentCharacterWidth() : 1;
            const uint width_per_item = (tft->width() / safe_columns);
            const uint items_per_column = 1 + (items_size / safe_columns);
            const uint chars_per_column = width_per_item / char_width;

            start_y = tft->getCursorY();

            uint_fast8_t i = 0;
            for (auto* item : items) {
                int column_number = i / items_per_column;
                tft->setCursor(
                    column_number * width_per_item, 
                    start_y + ((i % items_per_column) * (tft->getSingleRowHeight()+2))
                );

                // green or red according to whether underlying item is on or off, inverted if widget opened and item selected
                colours((i==effectively_selected) && opened, item->get_colour(), this->default_bg);

                //tft->printf((const char*)fmt, (char*)item->get_label());  // limits size but is LOADS 7fps slower!
                //tft->println(item->get_label());
                char tmp[MENU_C_MAX];
                strncpy(tmp, item->get_label(), MENU_C_MAX - 1);
                tmp[MENU_C_MAX - 1] = '\0';
                const uint max_chars = min(chars_per_column, (uint)(MENU_C_MAX - 1));
                tmp[max_chars] = '\0';
                tft->println(tmp);
                //tft->println();

                pos.y = tft->getCursorY();
                if ((uint_fast16_t)pos.y > max_height_reached)
                    max_height_reached = (uint_fast16_t)pos.y;
                ++i;
            }
            
            return max_height_reached; //tft->getCursorY();
        }
};


#endif