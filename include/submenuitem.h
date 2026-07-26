#ifndef SUBMENUITEM__INCLUDED
#define SUBMENUITEM__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "menuitems.h"

//#include "debug.h"

class SubMenuItem : public MenuItem {
    public:
        bool always_show = false;       // whether to hide items until menu is opened or not
        bool scrollable = true;           // whether to scroll to keep currently_selected in view
        bool anchor_to_separator = true; // whether to anchor scroll so the last SeparatorMenuItem before currently_selected stays at the top
        int currently_selected = -1;
        int currently_opened = -1;
        MenuItemList *items = nullptr;

        SubMenuItem(const char *label, bool show_header = true, bool scrollable = true) : MenuItem(label) {
            this->items = new MenuItemList();
            this->flags.go_back_on_select = true;
            this->flags.show_header = show_header;
            this->scrollable = scrollable;
        }
        // always_show argument determines whether to show items even when menu isn't opened
        /*SubMenuItem(const char *label, bool show_sub_headers = true) : SubMenuItem(label, show_sub_headers) {
            this->always_show = always_show;            
        }*/

        virtual bool allow_takeover() override {
            return this->always_show==false;
        }

        virtual bool action_opened() override {
            //debug_flag = true;
            //this->debug = true;
            Debug_println("submenuitem#action_opened"); Serial_flush();
            //if (this->allow_takeover())
            //    tft->clear();
            this->currently_selected = 0;
            
            // find first selectable item
            while ((unsigned int)currently_selected < items->size() && !items->get(currently_selected)->is_selectable()) {
                currently_selected++;
            }
            // if (items->size()==1 && items->get(0)->is_openable())   // if there's only one item, open it
            if (items->selectable_count()==1 && items->get(currently_selected)->is_openable())   // if there's only one selectable+openable item, open it
                button_select();

            // if (!always_show) 
            //     this->needs_redraw = true;

            Debug_println("calling MenuItem::action_opened"); Serial_flush();
            return MenuItem::action_opened();
        }

        bool is_opened() {
            return this->currently_opened!=-1;
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
        virtual void add(MenuItemList *items) {
            for (auto* item : *items) {
                this->add(item);
            }
            items->clear();
            delete items;
        }

        virtual void on_add() override {
            for (auto* item : *this->items) {
                item->set_tft(this->tft);
                item->on_add();
                item->set_default_colours(this->default_fg, this->default_bg); // inherit colours .. but breaks when we set custom colours eg in ParameterAmountControls..
            }
        }

        virtual void update_ticks(unsigned long ticks) override {
            for (auto* item : *items) {
                item->update_ticks(ticks);
            }
        }

        //bool needs_redraw = true;
        int previously_selected = -2;
        virtual int display(Coord pos, bool selected, bool opened) override {
            previously_selected = currently_selected;

            int y = header(this->label, pos, selected, opened);
            colours(false,this->default_fg,this->default_bg);

            if (is_opened() && this->items->get(currently_opened)->allow_takeover())
                return this->items->get(currently_selected)->display(Coord(0,y), true, true);

            int start_item;
            if (!scrollable) {
                start_item = 0;
            } else {
                const int available_height = tft->height() - y;

                // Conservative per-item height for non-PERF estimates: getSingleRowHeight()*3 (=24px).
                // getRowHeight() is NOT used here because it depends on the current text-size state,
                // which is undefined at this point in the call and varies frame to frame, causing
                // the height check to non-deterministically pass or fail (flickering).
                const int min_item_h = tft->getSingleRowHeight() * 3;

                // If all items fit in the available space, no scrolling is needed
                int total_height = 0;
                #if MENU_PERF_PARTIAL_UPDATES
                    for (int i = 0; i < (int)items->size(); ++i) {
                        const int16_t h = items->get(i)->get_cached_draw_height();
                        total_height += (h > 0) ? h : min_item_h;
                    }
                #else
                    total_height = (int)items->size() * min_item_h;
                #endif

                if (total_height <= available_height) {
                    start_item = 0;
                } else {
                    const int default_start = constrain(currently_selected - 2, 0, (int)this->items->size() - 1);
                    int sep_idx = -1;
                    if (anchor_to_separator && currently_selected > 0) {
                        for (int i = currently_selected - 1; i >= 0; --i) {
                            if (items->get(i)->is_separator()) {
                                sep_idx = i;
                                break;
                            }
                        }
                    }
                    if (sep_idx >= 0) {
                        // Estimate pixel height of items sep_idx..currently_selected inclusive
                        int estimated_height = 0;
                        for (int i = sep_idx; i <= currently_selected; ++i) {
                            #if MENU_PERF_PARTIAL_UPDATES
                                const int16_t h = items->get(i)->get_cached_draw_height();
                                estimated_height += (h > 0) ? h : min_item_h;
                            #else
                                estimated_height += min_item_h;
                            #endif
                        }
                        start_item = (estimated_height <= available_height) ? sep_idx : default_start;
                    } else {
                        start_item = default_start;
                    }
                }
            }

            if (opened || this->always_show) {
                auto it = items->begin();
                for (int s = 0; s < start_item && it != items->end(); ++s, ++it) {}  // fast-forward through the iterator to the start_item index
                // Serial.printf("submenuitem#display starting at item %i of %i\n", start_item, items->size()); Serial.flush();
                for (int i = start_item; it != items->end(); ++it, ++i) {
                    if (this->debug) { Serial.printf("submenuitem#display rendering item %i..\n", i); Serial.flush(); }
                    y = tft->getCursorY();

                    tft->setTextColor(this->default_fg, this->default_bg);
                    pos.x = 0; pos.y = tft->getCursorY();
                    MenuItem *item = *it;
                    // Serial.printf("got item %i: %s\n", i, item->label); Serial.flush();
                    // Serial.printf("submenuitem#display about to call display on item %i; currently_selected=%i, currently_opened=%i\n", i, this->currently_selected, this->currently_opened); Serial.flush();
                    y = item->display(
                        pos, i==this->currently_selected, i==this->currently_opened
                    );
                    //Serial.printf("submenuitem#display finished display on item %i\n", i); Serial.flush();            
                    tft->setTextColor(this->default_fg, this->default_bg);
                    y = this->tft->getCursorY();

                    if (y>=this->tft->height()) 
                        break;
                }
                // Serial.printf("submenuitem#display finished rendering items, y=%i, tft->height()=%i\n", y, this->tft->height()); Serial.flush();
                // blank to bottom of screen
                //if (this->debug) { Serial.printf("submenuitem#display blanking\n"); Serial.flush(); }
                if (!always_show && y < tft->height()) {
                    tft->drawRect(0, y, 0, tft->height(), BLACK);
                }
                //if (this->debug) { Serial.printf("submenuitem#display finished\n"); Serial.flush(); }
            } else {
                tft->printf("[%i sub-items...]\n", this->items->size());
                y = tft->getCursorY();
            }

            return y;
        }


        virtual void set_overlay_display(bool opened, Coord pos) override {
            if (menu==nullptr) return;

            // Match SubMenuItemBar behavior: defer opened overlay draw until end of frame.
            // pending_overlay_item, for the deferred draw so the overlay survives skip frames.
            if (opened && this->currently_opened>=0 && this->currently_opened < (int)this->items->size()) {
                MenuItem *opened_item = this->items->get(this->currently_opened);
                if (opened_item!=nullptr && opened_item->wants_fullscreen_overlay_when_opened_in_bar()) {
                    menu->active_overlay_item  = opened_item;
                    menu->active_overlay_y     = pos.y;
                } else {
                    menu->close_overlay();
                }
            } else {
                menu->close_overlay();
            }
        }

        virtual bool knob_left() override {
            if (!is_opened()) {
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
            if (!is_opened()) {
                currently_selected++;
                if (currently_selected>=(int)items->size())
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
            if (!is_opened()) {
                if (currently_selected>=0) {
                    if (items->get(currently_selected)->action_opened()) {
                        currently_opened = currently_selected;
                        IF_MENU_PERF_PARTIAL_UPDATES(items->get(currently_opened)->post_event(REDRAW_ON_OPEN);)
                        return false;
                    } else {
                        IF_MENU_PERF_PARTIAL_UPDATES(items->get(currently_selected)->post_event(REDRAW_ON_OWN_INPUT);)
                        return false;
                    }
                }
            } else {
                //Serial.printf("in submenuitem(%s)#button_select() on currently_opened=%i (%s)\n", this->get_label(), currently_opened, items->get(currently_opened)->get_label());
                // an item is currently opened, so call select on that item
                IF_MENU_PERF_PARTIAL_UPDATES(items->get(currently_opened)->post_event(REDRAW_ON_OWN_INPUT);)
                if (items->get(currently_opened)->button_select()) {
                    IF_MENU_PERF_PARTIAL_UPDATES(items->get(currently_opened)->post_event(REDRAW_ON_CLOSE | REDRAW_ON_DESELECTION);)
                    currently_selected = currently_opened;
                    currently_opened = -1;
                    return false;
                } else {
                    return false;
                }
            }
            return this->flags.go_back_on_select;
        }

        virtual bool button_back() override {
            //needs_redraw = true;    // force a redraw if we've selected
            if (is_opened() && !items->get(currently_opened)->button_back()) {
                //Serial.println("submenuitem#button_back() got a false back from the selected item's button_back, setting currently_opened etc then returning true");
                #if MENU_PERF_PARTIAL_UPDATES
                    // Post close+deselection so the item redraws out of its highlighted/opened visual state.
                    const int just_closed = currently_opened;
                    items->get(just_closed)->post_event(REDRAW_ON_CLOSE | REDRAW_ON_DESELECTION | REDRAW_ON_OWN_INPUT);
                #endif
                currently_selected = currently_opened;
                currently_opened = -1;
                if (items->openable_count()==1)       // if there's only one item, exit out of the submenu
                    return button_back();   // todo: recursive?! maybe we meant to call parent?
            } else if (!is_opened()) {
                //Serial.println("submenuitem#button_back() nothing selected so settiong currently_selected then returning false");
                //currently_opened = 0;
                currently_selected = -1;
                return false;
            }
            return true;
        }

        virtual bool button_right() override {
            if (is_opened()) {
                if (items->get(currently_opened)->button_right()) {

                } else {

                }
            } else {

            }
            return true;
        }
};



// two options side-by-side (actually probably works for multiple items, but doesn't do any scaling)
/*class DualMenuItem : public SubMenuItem {
    public: 
        DualMenuItem(const char *label) : SubMenuItem(label, true) {
        }

        virtual void add(MenuItem *item) override {
            item->flags.show_header = false;
            SubMenuItem::add(item);
        }

        virtual bool action_opened() {
            this->currently_selected = 0;
            return SubMenuItem::action_opened();
        }

        bool needs_redraw = true;
        int previously_selected = -2;
        virtual int display(Coord pos, bool selected, bool opened) override {
            needs_redraw = false;

            //tft->setTextSize(0);
            int y = header(this->label, pos, selected, opened, 0);
            colours(false,this->default_fg,this->default_bg);

            if (currently_opened>=0 && this->items->get(currently_opened)->allow_takeover())
                return this->items->get(currently_selected)->display(Coord(0,y), true, true);

            int start_item = 0; 

            int width_per_item = tft->width() / items->size();
            int start_y = y;
            int highest_y = y;

            if (opened || this->always_show) {
                //tft->clear();
                //colours(false, C_WHITE, BLACK);
                int count = 0;
                int i = start_item;
                for (auto* sub_item : *this->items) {
                    pos.x = width_per_item * count;
                    pos.y = start_y;

                    // draw fake headers for subitem
                    tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
                    tft->setCursor(pos.x, pos.y+1);
                    colours((!opened && selected) || (opened && i==this->currently_selected), this->default_fg, this->default_bg);
                    //tft->setTextSize(0);
                    int textSize = tft->get_textsize_for_width(sub_item->label, tft->width()/2);
                    tft->setTextSize(textSize);
                    tft->println(sub_item->label);
                    colours(false);
                    pos.y = tft->getCursorY();  // set position to just under the fake header

                    //if (this->debug) Serial.printf("%i: Drawing %s\tat\t%i,%i\t selected=%s\t and opened=%s\n", count, sub_item->label, pos.x, pos.y, i==this->currently_selected?"true":"false", i==this->currently_opened?"true":"false");
                    y = sub_item->display(
                        pos, ((int)i)==this->currently_selected, i==this->currently_opened
                    );
                    if (y>highest_y) {
                        //if (this->debug) Serial.printf("count %i: Subitem %s\t%i has x,y of\t%i,%i, higher than previous record\t%i (with is %i)\n", count, sub_item->label, i, pos.x, y, highest_y, width_per_item);
                        highest_y = y;
                    }
                    //this->tft->setTextColor(C_WHITE, BLACK);
                    y = this->tft->getCursorY();
                    
                    count++;
                    ++i;
                }
                // blank to bottom of screen
                if (!always_show && y < tft->height()) {
                    while (y < tft->height()) {
                        for (unsigned int i = 0 ; i < tft->get_c_max() ; i++)
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
};*/

#endif
