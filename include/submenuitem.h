#ifndef SUBMENUITEM__INCLUDED
#define SUBMENUITEM__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "menuitems.h"

class SubMenuItem : public MenuItem {
    public:
        bool always_show = false;       // whether to hide items until menu is opened or not
        int currently_selected = -1;
        int currently_opened = -1;
        LinkedList<MenuItem*> *items = nullptr; // = LinkedList<MenuItem*>();

        SubMenuItem(const char *label) : MenuItem(label) {
            this->items = new LinkedList<MenuItem*>();
            go_back_on_select = true;
        }
        // always_show argument determines whether to show items even when menu isn't opened
        SubMenuItem(const char *label, bool always_show) : SubMenuItem(label) {
            this->always_show = always_show;            
        }

        virtual bool allow_takeover() override {
            return this->always_show==false;
        }
        virtual bool action_opened() override {
            //if (this->allow_takeover())
            //    tft->clear();
            this->currently_selected = 0;
            if (!always_show) 
                this->needs_redraw = true;
                //this->tft->clear();
            return MenuItem::action_opened();
        }

        virtual void add(MenuItem *item) {
            if (item!=nullptr) {
                item->tft = this->tft;
                this->items->add(item);
                item->set_default_colours(this->default_fg, this->default_bg);  // do it here so that we can override it after being added, but this still isnt foolproof
            } else {
                //Serial.println("WARNING: SubMenuItem#add passed a nullptr!");
            }
        }
        virtual void add(LinkedList<MenuItem*> *items) {
            for (int i = 0 ; i < items->size() ; i++) {
                this->add(items->get(i));
            }
        }

        virtual void on_add() override {
            for (int i = 0 ; i < this->items->size() ; i++) {
                this->items->get(i)->set_tft(this->tft);
                this->items->get(i)->on_add();
                //this->items->get(i)->set_default_colours(this->default_fg, this->default_bg); // inherit colours .. but breaks when we set custom colours eg in ParameterMenuItem..
            }
        }

        virtual void update_ticks(unsigned long ticks) override {
            for (int i = 0 ; i < items->size() ; i++) {
                items->get(i)->update_ticks(ticks);
            }
        }

        bool needs_redraw = true;
        int previously_selected = -2;
        virtual int display(Coord pos, bool selected, bool opened) override {
            //static int previously_opened = -2;
            //static bool previously_opened = false;

            if (currently_selected!=previously_selected || needs_redraw) {
                //Serial.println("SubMenuItem#display: tft->clear ");
                this->tft->clear();
            }
            previously_selected = currently_selected;
            //previously_opened = opened;
            //if (currently_opened!=previously_opened || needs_redraw || (opened!=previously_opened)) {
                //Serial.printf("%s is clearing due to needs_redraw (%s) or opened!=previously_opened (currently_opened=%i, previously_opened=%i)\n", this->label, needs_redraw?"true":"false", opened, previously_opened);
                //tft->clear();
            //}
            needs_redraw = false;
            //previously_opened = currently_opened;
            //previously_opened = opened;

            int y = header(this->label, pos, selected, opened);
            colours(false,this->default_fg,this->default_bg);

            if (currently_opened>=0 && this->items->get(currently_opened)->allow_takeover())
                return this->items->get(currently_selected)->display(Coord(0,y), true, true);

            //int start_item = currently_selected>=0 ? currently_selected : 0;
            int start_item = constrain(currently_selected-2, 0, this->items->size()-1);

            if (opened || this->always_show) {
                //tft->clear();
                //colours(false, C_WHITE, BLACK);
                for (int i = start_item ; i < this->items->size() ; i++) {
                    //tft->println("wtf1?");
                    y = tft->getCursorY();

                    tft->setTextColor(this->default_fg, this->default_bg);
                    //Serial.printf("fg is %0x, bg is %0x\n")
                    //Serial.printf("SubMenuItem#display():\ttft@%p\n", this->tft);
                    //tft->println("wtf2?");
                    pos.x = 0; pos.y = tft->getCursorY();
                    y = this->items->get(i)->display(
                        pos, i==this->currently_selected, i==this->currently_opened
                    );
                    tft->setTextColor(this->default_fg, this->default_bg);
                    //tft->println("wtf3?");
                    y = this->tft->getCursorY();

                    if (y>=this->tft->height()) 
                        break;
                }
                // blank to bottom of screen
                if (!always_show && y < tft->height()) {
                    while (y < tft->height()) {
                        for (int i = 0 ; i < tft->get_c_max() ; i++)
                            tft->print((char*)" ");
                        y = tft->getCursorY();
                    }
                }
            } else {
                tft->printf("[%i sub-items...]\n", this->items->size());
                y = tft->getCursorY();
            }

            return y;
        }

        virtual bool knob_left() override {
            if (currently_opened==-1) {
                currently_selected--;
                if (currently_selected<0)
                    currently_selected = items->size()-1;
                if (!items->get(currently_selected)->is_selectable())
                    return this->knob_left();
                return true;
            } else {
                return this->items->get(currently_opened)->knob_left();
            }
        }
        virtual bool knob_right() override {
            if (currently_opened==-1) {
                currently_selected++;
                if (currently_selected>=items->size())
                    currently_selected = 0;
                if (!items->get(currently_selected)->is_selectable())
                    return this->knob_right();
                return true;
            } else {
                return this->items->get(currently_opened)->knob_right();
            }
        }

        virtual bool button_select() override {
            //Serial.printf("SubMenuItem#button_select(), currently_opened is %i\n", currently_opened);
            if (currently_opened==-1) {
                if (currently_selected>=0) {
                    if (items->get(currently_selected)->action_opened()) {
                        currently_opened = currently_selected;
                        //Serial.printf("submenuitem#button_select() opened subitem %i (%s)\n", currently_opened, items.get(currently_selected)->label);
                        return false;
                    } else {
                        return false;
                    }
                }
            } else {
                //Serial.printf("in submenuitem(%s)#button_select() on currently_opened=%i (%s)\n", this->label, currently_opened, items.get(currently_opened)->label);
                // an item is currently opened, so call select on that item
                if (items->get(currently_opened)->button_select()) {
                    //Serial.println("\tbutton_select returned true! setting currently_opened=-1 and returning false..");
                    //Serial.println("submenuitem#button_select() calling button_back and then returning false");
                    //button_back();
                    currently_selected = currently_opened;
                    currently_opened = -1;
                    return false;
                } else {
                    //Serial.println("\tbutton_select returned false! so also returning false!");
                    return false;
                }
            }
            return go_back_on_select;
        }

        virtual bool button_back() override {
            needs_redraw = true;    // force a redraw if we've selected
            if (currently_opened!=-1 && !items->get(currently_opened)->button_back()) {
                //Serial.println("submenuitem#button_back() got a false back from the selected item's button_back, setting currently_opened etc then returning true");
                currently_selected = currently_opened;
                currently_opened = -1;
            } else if (currently_opened==-1) {
                //Serial.println("submenuitem#button_back() nothing selected so settiong currently_selected then returning false");
                //currently_opened = 0;                
                currently_selected = -1;
                return false;
            }
            return true;
        }

        virtual bool button_right() override {
            if (currently_opened!=-1) {
                if (items->get(currently_opened)->button_right()) {

                } else {

                }
            } else {

            }
            return true;
        }
};


// two options side-by-side (actually probably works for multiple items, but doesn't do any scaling)
class DualMenuItem : public SubMenuItem {
    public: 
        DualMenuItem(const char *label) : SubMenuItem(label, true) {
        }

        virtual void add(MenuItem *item) override {
            item->show_header = false;
            SubMenuItem::add(item);
        }

        virtual bool action_opened() {
            this->currently_selected = 0;
            return SubMenuItem::action_opened();
        }

        bool needs_redraw = true;
        int previously_selected = -2;
        virtual int display(Coord pos, bool selected, bool opened) override {
            //if (this->debug) Serial.println("display doublesubmenu ===>");
            //static int previously_selected = -2;
            //static int previously_opened = -2;

            /*if (currently_selected!=previously_selected || needs_redraw) {
                Serial.printf("DualSubMenu clear with currently_selected %i and previously_selected %i, needs_redraw is %s\n", currently_selected, previously_selected, needs_redraw?"true":"false");
                this->tft->clear();
            }
            previously_selected = currently_selected;*/
            //previously_opened = currently_opened;
            //if (currently_opened!=previously_opened || needs_redraw || (opened!=previously_opened)) {
                //Serial.printf("%s is clearing due to needs_redraw (%s) or opened!=previously_opened (currently_opened=%i, previously_opened=%i)\n", this->label, needs_redraw?"true":"false", opened, previously_opened);
                //tft->clear();
            //}
            needs_redraw = false;
            //previously_opened = currently_opened;
            //previously_opened = opened;

            int y = header(this->label, pos, selected, opened);
            colours(false,this->default_fg,this->default_bg);

            if (currently_opened>=0 && this->items->get(currently_opened)->allow_takeover())
                return this->items->get(currently_selected)->display(Coord(0,y), true, true);

            int start_item = 0; //currently_selected>=0 ? currently_selected : 0;

            int width_per_item = tft->width() / items->size();
            int start_y = y;
            int highest_y = y;

            if (opened || this->always_show) {
                //tft->clear();
                //colours(false, C_WHITE, BLACK);
                int count = 0;
                for (int i = start_item ; i < this->items->size() ; i++) {
                    //tft->println("wtf1?");
                    //y = tft->getCursorY();

                    //tft->setTextColor(C_WHITE, BLACK);
                    //colours(is_opened)
                    //Serial.printf("fg is %0x, bg is %0x\n")
                    //Serial.printf("SubMenuItem#display():\ttft@%p\n", this->tft);
                    //tft->println("wtf2?");
                    //pos.x = 0; pos.y = tft->getCursorY();
                    pos.x = width_per_item * count;
                    pos.y = start_y;

                    // draw fake headers for subitem
                    tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
                    tft->setCursor(pos.x, pos.y+1);
                    colours((!opened && selected) || (opened && i==this->currently_selected), this->default_fg, this->default_bg);
                    tft->setTextSize(0);
                    tft->println(this->items->get(i)->label);
                    colours(false);
                    pos.y = tft->getCursorY();  // set position to just under the fake header

                    //if (this->debug) Serial.printf("%i: Drawing %s\tat\t%i,%i\t selected=%s\t and opened=%s\n", count, items->get(i)->label, pos.x, pos.y, i==this->currently_selected?"true":"false", i==this->currently_opened?"true":"false");
                    y = this->items->get(i)->display(
                        pos, i==this->currently_selected, i==this->currently_opened
                    );
                    if (y>highest_y) {
                        //if (this->debug) Serial.printf("count %i: Subitem %s\t%i has x,y of\t%i,%i, higher than previous record\t%i (with is %i)\n", count, this->items->get(i)->label, i, pos.x, y, highest_y, width_per_item);
                        highest_y = y;
                    }
                    //this->tft->setTextColor(C_WHITE, BLACK);
                    //tft->println("wtf3?");
                    y = this->tft->getCursorY();
                    
                    //if (pos.x >= this->tft->width() || y>=this->tft->height()) 
                    //    break;
                    count++;
                }
                // blank to bottom of screen
                if (!always_show && y < tft->height()) {
                    while (y < tft->height()) {
                        for (int i = 0 ; i < tft->get_c_max() ; i++)
                            tft->print((char*)" ");
                        y = tft->getCursorY();
                    }
                }
                y = highest_y;
            } else {
                tft->printf("[%i sub-items...]\n", this->items->size());
                y = tft->getCursorY();
            }

            //if (this->debug) Serial.printf("For item\t%s, returning y\t%i\n", this->label, y);
            //if (this->debug) Serial.println("<===display doublesubmenu");

            return y;
        }
};

#endif
