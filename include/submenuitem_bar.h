#ifndef SUBMENUTITEMBAR__INCLUDED
#define SUBMENUTITEMBAR__INCLUDED
#include "submenuitem.h"

// originally adapted from ParameterAmountControls
class SubMenuItemBar : public SubMenuItem {
    public:

    bool show_sub_headers = true;
    int_fast16_t cached_pixel_width_per_item = 0;

    SubMenuItemBar(const char *label, bool show_sub_headers = true, bool show_header = true) : SubMenuItem(label, show_header) {
        this->show_sub_headers = show_sub_headers;
    }

    virtual int number_columns() {
        return this->items->size();
    }

    virtual bool allow_takeover() override {
        return false;
    }

    virtual int get_max_pixel_width(int item_number);
    virtual uint_fast16_t get_max_character_width(int item_number);

    virtual int display(Coord pos, bool selected, bool opened) override;
    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected);
};


// todo: probably move the column functionality into SubMenuItemBar to save bytes on duplicated logic
class SubMenuItemColumns : public SubMenuItemBar {
    public:

    bool show_sub_headers = true;
    int columns = 1;

    SubMenuItemColumns(const char *label, int columns = 1, bool show_sub_headers = true, bool show_header = true) : SubMenuItemBar(label, show_sub_headers, show_header) {
        this->columns = columns;
    }

    virtual int number_columns() override {
        return this->columns;
    }

    virtual bool allow_takeover() override {
        return false;
    }

    /*virtual inline int get_max_pixel_width(int item_number) override {
        return this->tft->width() / columns; 
    }*/

    virtual int display(Coord pos, bool selected, bool opened) override;
};


class DualMenuItem : public SubMenuItemColumns {
    public:
        int item_1_width = 0;
        DualMenuItem(const char *label, bool show_sub_headers = true, bool show_header = true, int item_1_width = 0) : SubMenuItemColumns(label, 2, show_sub_headers, show_header) {
            this->item_1_width = item_1_width;
        }

        int get_max_pixel_width(int item_number) override {
            if (item_1_width==0) {
                if (this->cached_pixel_width_per_item==0)
                    this->cached_pixel_width_per_item = this->tft->width() / this->number_columns();
                return this->cached_pixel_width_per_item - (item_number==number_columns()-1?3:0);
            } else {
                if (item_number==0) {
                    return item_1_width;
                } else {
                    return this->tft->width() - item_1_width;
                }
            }
        }
};

#endif