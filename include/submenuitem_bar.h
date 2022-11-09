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

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (this->debug) Serial.printf("Start of display in SubMenuItemBar, passed in %i,%i\n", pos.x, pos.y);
        pos.y = header(label, pos, selected, opened);
        if (this->debug) Serial.printf("\tafter header, y=%i\n", pos.y);
        tft->setCursor(pos.x, pos.y);
        //tft->setTextSize(1);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        int start_y = pos.y;        // y to start drawing at (just under header)
        int finish_y = pos.y;       // highest y that we finished drawing at

        // draw all the sub-widgets
        int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
        if (this->debug) Serial.printf("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n", width_per_item, this->tft->width(), this->items->size());
        for (int item_index = 0 ; item_index < this->items->size() ; item_index++) {
            int temp_y = this->small_display(
                item_index, 
                item_index * width_per_item, 
                start_y, 
                width_per_item, 
                this->currently_selected==item_index, 
                this->currently_opened==item_index,
                !opened && selected
            );
            if (temp_y>finish_y)
                finish_y = temp_y;
        }

        tft->setTextColor(this->default_fg, this->default_bg);
        tft->setTextSize(0);

        if (this->debug) Serial.printf("End of display, y=%i\n--------\n", finish_y);
        return finish_y;//tft->getCursorY();
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) {
        if (this->debug) Serial.printf("\tSubMenuItemBar: start of small_display for index %i, passed in x,y=%i,%i and width=%i\n", index, x, y, width_in_pixels);

        MenuItem *ctrl = items->get(index);
        int character_width_in_pixels = tft->characterWidth(); // presumed font width
        char fmt[10];

        /*Serial.printf("small_display in %s, item %i (%s), got colour %4x!\n", 
            this->label, 
            index, 
            ctrl->label,
            ctrl->default_fg
        );*/

        /*uint16_t colour = C_WHITE;
        if (ctrl->default_fg != colour) {
            //Serial.printf("ctrl's default_fg of\t%04x doesn't match default white of\t%04x - taking ctrl\n", ctrl->default_fg, colour);
            colour = ctrl->default_fg;
        } else {
            //Serial.printf("ctrl's default_fg of\t%04x matches default white of\t%04x - taking bar\n", ctrl->default_fg, colour);
            colour = this->default_fg;
        }*/
        const uint16_t colour = (ctrl->default_fg != C_WHITE) ? ctrl->default_fg : this->default_fg;
        //Serial.printf("small_display %s\tcolour is\t%04x, bar's colour is\t%04x =>\t%04x\n", ctrl->label, ctrl->default_fg, this->default_fg, colour);
        //colours(false, colour, ctrl->default_bg);

        const uint16_t max_display_width = (int)(width_in_pixels/character_width_in_pixels);
        //uint16_t min_display_width = (int)strlen(ctrl->label);
        //Serial.printf("(width_in_pixels %i/character_width_in_pixels %i) = %i\n", width_in_pixels, character_width_in_pixels, max_display_width);
        // prepare label header format
        sprintf(fmt, "%%-%is\n", max_display_width);    // becomes eg "%-6s\n"
        if (this->debug) Serial.printf("\tGot format '%s'\n", fmt);

        // print label header
        if (this->show_sub_headers) {
            if (this->debug) Serial.printf("\tdrawing header at %i,%i\n", x, y);
            tft->setCursor(x, y);
            colours(is_selected, is_opened ? GREEN : colour, ctrl->default_bg);
            tft->setTextSize(0);
            //if (x + width_in_pixels>=tft->width())
            //    fmt[strlen(fmt)-2] = '\0';  // cut off the \n if we've reached the width of the display in order to fix wraparound?
            tft->printf(fmt, ctrl->label);
            //y = tft->getCursorY();
            y += tft->getRowHeight();
            if (this->debug) Serial.printf("\t bottom of header is %i\n", y);
        }

        // get position ready for value
        tft->setCursor(x, y);   // reset cursor to underneath the label
        if (this->debug) Serial.printf("\tdoing renderValue at %i,%i\n", x, y);

        // actually render the item
        y = ctrl->renderValue(outer_selected || is_selected, is_opened, max_display_width); //width/width_in_chars);

        if (this->debug) Serial.printf("\tend of small_display, returning y=%i\n", y);
        return y;
    }
};

#endif