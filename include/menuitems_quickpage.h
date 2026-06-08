#pragma once

#include <Arduino.h>
#include "menuitems.h"
#include "menu.h"

class QuickPagesMenuItem : public MenuItem {
    public:
        QuickPagesMenuItem(const char *label) : MenuItem(label) {}

        int selected_value_index = -1;
        //int start_at = 0;

        virtual int display_entry(int o, bool opened = false) {
            page_t *page = menu->get_quick_page(o);
            if (page==nullptr) 
                return tft->getCursorY();

            colours(opened && o==selected_value_index, page->colour);
            tft->println(page->title);
            colours(C_WHITE);

            return tft->getCursorY();
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            if (selected_value_index<0 && menu->quick_page_history_total>0) {
                selected_value_index = 0;
            }
            for (int i = 0 ; i < menu->NUM_QUICK_PAGE_HISTORY ; i++) {
                int o = (i + selected_value_index) % menu->NUM_QUICK_PAGE_HISTORY;

                pos.y = this->display_entry(o, opened);

                if (tft->getCursorY()>=tft->height())
                    break;
            }
            return tft->getCursorY();
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
            if (Serial && this->debug) Serial.printf("button_select for value %i (%i, %s)\n", selected_value_index, menu->quick_pages[selected_value_index], menu->get_quick_page(selected_value_index)->title);
            menu->quickjump_to_page(menu->quick_pages[selected_value_index].page_index);
            return false;
        }

};


class CustomQuickPagesMenuItem : virtual public QuickPagesMenuItem {
    public:
        // TODO: replace with a more memory-efficient structure
        LinkedList<page_t*> *pages = new LinkedList<page_t*>();

        CustomQuickPagesMenuItem(const char *label) : QuickPagesMenuItem(label) {}

        int selected_value_index = -1;
        //int start_at = 0;

        virtual int display_entry(int o, bool opened = false) {
            page_t *page = pages->get(o);
            if (page==nullptr) 
                return tft->getCursorY();

            colours(opened && o==selected_value_index, page->colour);
            if (page==nullptr)
                tft->println("Null page?!");
            else if (page->title==nullptr)
                tft->println("Null page title?!");
            else
                tft->println(page->title);
            colours(C_WHITE);

            return tft->getCursorY();
        }

        virtual void add_page(page_t *p) {
            for (auto* page : *pages) {
                if (page==p)   // already exists 
                    return;
            }
            pages->add(p);
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            uint_fast16_t size = pages->size();

            if (selected_value_index<0 && size>0) {
                selected_value_index = 0;
            }
            for (uint_fast16_t i = 0 ; i < size ; i++) {
                uint_fast16_t o = (i + selected_value_index) % size;

                pos.y = this->display_entry(o, opened);
               
                if (tft->getCursorY()>=tft->height())
                    break;
            }
            return pos.y;
        }

        virtual bool knob_left() override {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = pages->size()-1;
            return true;
        }

        virtual bool knob_right() override {
            selected_value_index++;
            if (selected_value_index >= (int)pages->size())
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() override {
            if (selected_value_index<0)
                return false;
            if(Serial && this->debug) Serial.printf("button_select for value %i (%i, %s)\n", selected_value_index, pages->get(selected_value_index), menu->get_quick_page(selected_value_index)->title);
            // todo: why isn't this properly opening the page and selecting first item?!
            menu->quickjump_to_page(menu->get_page_index_for_name(pages->get(selected_value_index)->title));
            //page_t * selected_page = menu->get_selected_page();
            return false;
        }

};


class AllPagesIndexMenuItem : public QuickPagesMenuItem {
    public:
        AllPagesIndexMenuItem(const char *label) : QuickPagesMenuItem(label) {}

        virtual int display_entry(int o, bool opened = false) override {
            page_t *page = menu->get_page(o);
            if (page==nullptr) 
                return tft->getCursorY();

            colours(opened && o==selected_value_index, page->colour);
            tft->println(page->title);
            colours(C_WHITE);

            return tft->getCursorY();
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //pos.y = header(label, pos, selected, opened);
            // todo: make the header show the page group
            tft->setTextSize(2);

            uint_fast8_t size = menu->get_number_pages();

            if (selected_value_index<0 && size>0) {
                selected_value_index = 0;
            }
            for (uint_fast8_t i = 0 ; i < size ; i++) {
                uint_fast8_t o = (i + selected_value_index) % size;

                pos.y = this->display_entry(o, opened);

                if (tft->getCursorY()>=tft->height())
                    break;
            }
            return pos.y;
        }

        virtual bool knob_left() override {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = menu->get_number_pages()-1;
            return true;
        }

        virtual bool knob_right() override {
            selected_value_index++;
            if (selected_value_index >= (int)menu->get_number_pages())
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() override {
            if (selected_value_index<0)
                return false;
            if (Serial && this->debug) Serial.printf("button_select for value %i (%i, %s)\n", selected_value_index, menu->quick_pages[selected_value_index], menu->get_quick_page(selected_value_index)->title);
            menu->quickjump_to_page(selected_value_index);
            return false;
        }

};


// ok, so the idea of this one is to list first all of the groups, and then to be able to expand those groups to show the pages within them, and then to select a page to open it
class PageGroupIndexMenuItem : public AllPagesIndexMenuItem {
    public:
        uint16_t selected_group_index = 0;
        uint16_t selected_page_within_group_index = 0;

        bool group_opened = false;
        
        PageGroupIndexMenuItem(const char *label) : AllPagesIndexMenuItem(label) {}

        page_group_t *get_selected_group() {
            return menu->groups->get(selected_group_index);
        }

        page_t *get_selected_page_within_group() {
            page_group_t *group = get_selected_group();
            if (group == nullptr) return nullptr;
            return group->pages.get(selected_page_within_group_index);
        }

        virtual int display_entry(int o, bool opened = false) override {
            page_group_t *group = get_selected_group();
            if (group == nullptr) return tft->getCursorY();
            int overall_o = menu->get_index_for_page(group->pages.get(o));

            page_t *page = menu->get_page(overall_o);
            if (page==nullptr) 
                return tft->getCursorY();

            if (o==0)
                tft->print("-");
            else
                tft->print(" ");
                
            colours(opened && o==selected_page_within_group_index, page->colour);
            tft->println(page->title);
            colours(false);

            return tft->getCursorY();
        }

        virtual int display_entry_group(int o, bool opened = false) {
            page_group_t *group = menu->groups->get(o);
            if (group==nullptr) 
                return tft->getCursorY();

            colours(opened && o==selected_group_index, group->colour);
            
            char count_indicator[8] = "";
            if (group_opened) {
                sprintf(count_indicator, "[%i/%i]", selected_page_within_group_index+1, group->pages.size());
            } else if (o==selected_group_index) {
                sprintf(count_indicator, "[x/%i]", group->pages.size());
            }

            tft->printf(
                "%s % *s\n", 
                group->group_name, 
                (tft->width() / tft->currentCharacterWidth())-strlen(group->group_name)-1, //-strlen(count_indicator), 
                count_indicator
            );

            colours(false);

            return tft->getCursorY();
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            if (group_opened)
                pos.y = header(get_selected_page_within_group()->title, pos, selected, opened);
            else
                pos.y = header("Select a group...", pos, selected, opened);

            tft->setTextSize(2);

            if (!group_opened) {
                // if group isn't opened, show list of groups ready for selection
                uint_fast8_t size = menu->groups->size();
                if (selected_group_index<0 && size>0) {
                    selected_group_index = 0;
                }
                selected_value_index = selected_group_index;
                for (uint_fast8_t i = 0 ; i < size ; i++) {
                    uint_fast8_t o = (i + selected_group_index) % size;
                    pos.y = this->display_entry_group(o, opened);
                    if (tft->getCursorY()>=tft->height())
                        break;
                }
            } else {
                // if group is opened, show list of pages within the selected group ready for selection

                this->display_entry_group(selected_group_index, opened);   // show the group name as a header above the pages within that group

                page_group_t *group = get_selected_group();
                if (group == nullptr) return pos.y;
                uint_fast8_t size = group->pages.size();
                if (selected_page_within_group_index<0 && size>0) {
                    selected_page_within_group_index = 0;
                }
                selected_value_index = menu->get_index_for_page(group->pages.get(selected_page_within_group_index));
                for (uint_fast8_t i = 0 ; i < size ; i++) {
                    uint_fast8_t o = (i + selected_page_within_group_index) % size;
                    pos.y = this->display_entry(o, opened);
                    if (tft->getCursorY()>=tft->height())
                        break;
                }
            }

            return pos.y;
        }

        virtual bool knob_left() override {
            if (!group_opened) {
                selected_group_index--;
                if (selected_group_index < 0 || selected_group_index >= menu->groups->size())
                    selected_group_index = menu->groups->size()-1;
            } else {
                selected_page_within_group_index--;
                page_group_t *group = get_selected_group();
                if (group == nullptr) return false;
                if (selected_page_within_group_index < 0 || selected_page_within_group_index >= group->pages.size())
                    selected_page_within_group_index = group->pages.size()-1;
            }
            return true;
        }

        virtual bool knob_right() override {
            if (!group_opened) {
                selected_group_index++;
                if (selected_group_index >= (int)menu->groups->size())
                    selected_group_index = 0;
            } else {
                selected_page_within_group_index++;
                page_group_t *group = get_selected_group();
                if (group == nullptr) return false;
                if (selected_page_within_group_index >= (int)group->pages.size())
                    selected_page_within_group_index = 0;
            }
            return true;
        }

        virtual bool button_select() override {
            if (!group_opened) {
                group_opened = true;
                selected_page_within_group_index = 0;
                return false;
            } else {
                if (Serial && this->debug) Serial.printf("button_select for value %i (%i, %s)\n", selected_value_index, menu->quick_pages[selected_value_index], menu->get_quick_page(selected_value_index)->title);
                menu->quickjump_to_page(menu->get_index_for_page(this->get_selected_page_within_group()));
                return false;
            }
        }

        virtual bool button_back() override {
            if (group_opened) {
                group_opened = false;
                IF_MENU_PERF_PARTIAL_UPDATES(this->post_event(REDRAW_ON_INVALIDATE);)
                return true;
            }
            return false;    // if we're not in a group, back button should do its normal thing of going up to page level
        }

};