#ifndef SUBMENUTITEMBAR__INCLUDED
#define SUBMENUTITEMBAR__INCLUDED
#include "submenuitem.h"

// originally adapted from ParameterMenuItem
class SubMenuItemBar : public SubMenuItem {
    public:

    bool show_sub_headers = true;

    SubMenuItemBar(const char *label) : SubMenuItem(label) {

    }

    virtual int number_columns() {
        return this->items->size();
    }

    virtual bool allow_takeover() override {
        return false;
    }

    virtual inline int get_max_pixel_width(int item_number) {
        //return (this->tft->width() / this->number_columns());
        return this->get_max_character_width(item_number) * tft->currentCharacterWidth();
    }

    virtual inline uint_fast16_t get_max_character_width(int item_number) {
        //return (get_max_pixel_width(item_number)/tft->characterWidth()) - (item_number==number_columns()-1?1:0);
        int_fast16_t screen_width_in_characters = this->tft->width() / tft->currentCharacterWidth();
        int_fast16_t character_width_per_column = screen_width_in_characters / this->number_columns();
        return character_width_per_column - (item_number==number_columns()-1?1:0);
    }

    virtual int display(Coord pos, bool selected, bool opened) override;
    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected);
};


// todo: probably move the column functionality into SubMenuItemBar to save bytes on duplicated logic
class SubMenuItemColumns : public SubMenuItemBar {
    public:

    bool show_sub_headers = true;
    int columns = 1;

    SubMenuItemColumns(const char *label, int columns = 1) : SubMenuItemBar(label) {
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

#endif