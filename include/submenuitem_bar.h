#ifndef SUBMENUTITEMBAR__INCLUDED
#define SUBMENUTITEMBAR__INCLUDED
#include "submenuitem.h"

// originally adapted from ParameterMenuItem
class SubMenuItemBar : public SubMenuItem {
    public:

    bool show_sub_headers = true;

    SubMenuItemBar(const char *label, bool show_sub_headers = true) : SubMenuItem(label) {
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

    SubMenuItemColumns(const char *label, int columns = 1, bool show_sub_headers = true) : SubMenuItemBar(label, show_sub_headers) {
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
        DualMenuItem(const char *label, bool show_sub_headers = true) : SubMenuItemColumns(label, 2, show_sub_headers) {
        }
};

#endif