#include "submenuitem.h"

// originally adapted from ParameterMenuItem
class SubMenuItemBar : public SubMenuItem {
    public:

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
        colours(opened, opened ? GREEN : C_WHITE, BLACK);

        int start_y = pos.y;        // y to start drawing at (just under header)
        int finish_y = pos.y;       // highest y that we finished drawing at

        // draw all the sub-widgets
        // todo: include a widget to select the modulation source for the slot
        int width_per_item = this->tft->width()/(this->items.size() /*+1*/);
        if (this->debug) Serial.printf("display in SubMenuItemBar got width_per_item=%i from tftwidth %i / itemsize %i\n", width_per_item, this->tft->width(), this->items.size());
        for (int i = 0 ; i < this->items.size() ; i++) {
            int temp_y = this->small_display(i, i * width_per_item, start_y, width_per_item, this->currently_selected==i, this->currently_opened==i);
            if (temp_y>finish_y)
                finish_y = temp_y;
        }

        tft->setTextColor(C_WHITE, BLACK);
        tft->setTextSize(0);

        if (this->debug) Serial.printf("End of display, y=%i\n--------\n", finish_y);
        return finish_y;//tft->getCursorY();
    }

    virtual int small_display(int index, int x, int y, int width, bool is_selected, bool is_opened) {
        if (this->debug) Serial.printf("\tSubMenuItemBar: start of small_display for index %i, passed in x,y=%i,%i and width=%i\n", index, x, y, width);

        MenuItem *ctrl = items.get(index);
        int width_in_chars = 8; // presumed font width
        char fmt[10];

        // prepare label header format
        colours(false, ctrl->colour, BLACK);
        byte display_width = min((int)(width/width_in_chars), (int)strlen(ctrl->label));
        sprintf(fmt, "%%-%is\n", display_width);    // becomes eg "%-6s\n"
        if (this->debug) Serial.printf("\tGot format '%s'\n", fmt);

        // print label header
        if (this->debug) Serial.printf("\tdrawing header at %i,%i\n", x, y);
        tft->setCursor(x, y);
        colours(is_selected, is_opened ? GREEN : ctrl->colour, BLACK);
        tft->setTextSize(0);
        tft->printf(fmt, ctrl->label);
        y = tft->getCursorY();

        if (this->debug) Serial.printf("\t bottom of header is %i\n", y);
        // position for value
        tft->setCursor(x, y);   // reset cursor to underneath the label

        if (this->debug) Serial.printf("\tdoing renderValue at %i,%i\n", x, y);

        // render the item
        y = ctrl->renderValue(is_selected, is_opened, width/width_in_chars);

        if (this->debug) Serial.printf("\tend of small_display, returning y=%i\n", y);
        return y;
    }
};
