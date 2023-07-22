#ifndef SUBMENUTITEMBAR__INCLUDED
#define SUBMENUTITEMBAR__INCLUDED
#include "submenuitem.h"

// originally adapted from ParameterMenuItem
class SubMenuItemBar : public SubMenuItem {
    public:

    bool show_sub_headers = true;

    SubMenuItemBar(const char *label) : SubMenuItem(label) {

    }

    virtual bool allow_takeover() override {
        return false;
    }

    virtual inline int get_max_pixel_width(int item_number) {
        return this->tft->width() / (this->items->size() /*+1*/);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (this->debug) Serial.printf("Start of display in SubMenuItemBar at %u, passed in %i,%i\n", millis(), pos.x, pos.y);
        pos.y = header(label, pos, selected, opened);
        if (this->debug) Serial.printf(F("\tafter header, y=%i\n"), pos.y);
        tft->setCursor(pos.x, pos.y);
        //tft->setTextSize(1);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        int start_y = pos.y;        // y to start drawing at (just under header)
        int finish_y = pos.y;       // highest y that we finished drawing at

        // draw all the sub-widgets
        //int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
        int start_x = 0;
        //Debug_printf(F("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n"), width_per_item, this->tft->width(), this->items->size());
        for (unsigned int item_index = 0 ; item_index < this->items->size() ; item_index++) {
            const int width = this->get_max_pixel_width(item_index);
            const int temp_y = this->small_display(
                item_index, 
                start_x, 
                start_y, 
                width, //width_per_item, 
                this->currently_selected==(int)item_index, 
                this->currently_opened==(int)item_index,
                !opened && selected
            );
            start_x += width;
            if (temp_y>finish_y)
                finish_y = temp_y;
        }

        tft->setTextColor(this->default_fg, this->default_bg);
        tft->setTextSize(0);

        if (this->debug) Serial.printf(F("End of display, y=%i\n--------\n"), finish_y);
        return finish_y;//tft->getCursorY();
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) {
        if (this->debug) Serial.printf(F("\tSubMenuItemBar: start of small_display for index %i, passed in x,y=%i,%i and width=%i\n"), index, x, y, width_in_pixels);

        MenuItem *ctrl = items->get(index);
        const int character_width_in_pixels = tft->characterWidth(); // presumed font width
        const uint16_t max_display_width = (int)(width_in_pixels/character_width_in_pixels);
        //Serial.printf("(width_in_pixels %i/character_width_in_pixels %i) = %i\n", width_in_pixels, character_width_in_pixels, max_display_width);
        /*Serial.printf("small_display in %s, item %i (%s), got colour %4x!\n", 
            this->label, 
            index, 
            ctrl->label,
            ctrl->default_fg
        );*/

        const uint16_t colour = (ctrl->default_fg != C_WHITE) ? ctrl->default_fg : this->default_fg;

        // prepare label header format
        char fmt[10];
        snprintf(fmt, 10, "%%-%is\n", max_display_width);    // becomes eg "%-6s\n"
        //if (this->debug) Serial.printf("\tGot format '%s'\n", fmt);

        // print label header // TODO: either move this to be the responsibility of the control, or use something like get_label() instead, so that can override in subclass for things like parameter controls where this might change
        if (this->show_sub_headers) {
            //if (this->debug) Serial.printf("\tdrawing header at %i,%i\n", x, y);
            tft->setCursor(x, y);
            colours(is_selected, is_opened ? GREEN : colour, ctrl->default_bg);
            tft->setTextSize(0);
            //if (x + width_in_pixels>=tft->width())
            //    fmt[strlen(fmt)-2] = '\0';  // cut off the \n if we've reached the width of the display in order to fix wraparound?
            tft->printf(fmt, ctrl->label);
            //y = tft->getCursorY();
            y += tft->getRowHeight();
            //if (this->debug) Serial.printf("\t bottom of header is %i\n", y);
        }

        // get position ready for value
        tft->setCursor(x, y);   // reset cursor to underneath the label
        /*if (this->debug) {
            Serial.printf("\tdoing renderValue at %i,%i on @%p\n", x, y, ctrl);
            Serial.printf("\taka '%s'\n", ctrl->label);
        }*/

        if (this->debug) Serial.printf("SubMenuItem for %s:\t rendering item index %i (named %s)\n", label, index, ctrl->label);

        // actually render the item
        y = ctrl->renderValue((!this->show_sub_headers && outer_selected) || is_selected, is_opened, max_display_width); //width/width_in_chars);

        if (this->debug) Serial.printf("\tend of small_display, returning y=%i\n", y);
        return y;
    }
};


// todo: probably move the column functionality into SubMenuItemBar to save bytes on duplicated logic
class SubMenuItemColumns : public SubMenuItemBar {
    public:

    bool show_sub_headers = true;
    int columns = 1;

    SubMenuItemColumns(const char *label, int columns = 1) : SubMenuItemBar(label) {
        this->columns = columns;
    }

    virtual bool allow_takeover() override {
        return false;
    }

    virtual inline int get_max_pixel_width(int item_number) {
        return this->tft->width() / columns; //(this->items->size() /*+1*/);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        Debug_printf("Start of display in SubMenuItemBar, passed in %i,%i\n", pos.x, pos.y);
        pos.y = header(label, pos, selected, opened);
        Debug_printf(F("\tafter header, y=%i\n"), pos.y);
        tft->setCursor(pos.x, pos.y);
        //tft->setTextSize(1);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        //int start_y = pos.y;        // y to start drawing at (just under header)
        int finish_y = pos.y;       // highest y that we finished drawing at

        // draw all the sub-widgets
        //int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
        int start_x = 0;
        //Debug_printf(F("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n"), width_per_item, this->tft->width(), this->items->size());
        int last_y = 0;
        for (unsigned int item_index = 0 ; item_index < this->items->size() ; item_index++) {
            if (item_index % columns == 0) {
                start_x = 0; 
                pos.y = tft->getCursorY();
                last_y = pos.y;
            } else {
                start_x = (item_index%columns) * (menu->tft->width() / columns);
                pos.y = last_y;
            }
            const int width = this->get_max_pixel_width(item_index);
            const int temp_y = this->small_display(
                item_index, 
                start_x, 
                pos.y, 
                width, //width_per_item, 
                this->currently_selected==(int)item_index, 
                this->currently_opened==(int)item_index,
                !opened && selected
            );
            //start_x += width;
            if (temp_y>finish_y)
                finish_y = temp_y;
        }

        tft->setTextColor(this->default_fg, this->default_bg);
        tft->setTextSize(0);

        Debug_printf(F("End of display, y=%i\n--------\n"), finish_y);
        return finish_y;//tft->getCursorY();
    }
};

#endif