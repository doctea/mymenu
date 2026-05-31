#ifdef ENABLE_SCREEN

#include "submenuitem_bar.h"

static inline void print_fixed_width_label(DisplayTranslator *tft, const char *label, uint_fast16_t width_chars) {
    if (width_chars == 0) {
        tft->println();
        return;
    }

    if (label == nullptr) {
        label = "";
    }

    const uint_fast16_t max_chars = (width_chars >= (uint_fast16_t)MENU_C_MAX) ? (uint_fast16_t)(MENU_C_MAX - 1) : width_chars;
    char out[MENU_C_MAX];
    uint_fast16_t n = 0;
    while (n < max_chars && label[n] != '\0') {
        out[n] = label[n];
        ++n;
    }
    while (n < max_chars) {
        out[n] = ' ';
        ++n;
    }
    out[max_chars] = '\0';
    tft->println(out);
}

/*int SubMenuItemBar::get_max_pixel_width(int item_number) {
    //return (this->tft->width() / this->number_columns());
    return this->get_max_character_width(item_number) * tft->currentCharacterWidth();
}

uint_fast16_t SubMenuItemBar::get_max_character_width(int item_number) {
    //return (get_max_pixel_width(item_number)/tft->characterWidth()) - (item_number==number_columns()-1?1:0);
    int_fast16_t screen_width_in_characters = this->tft->width() / tft->currentCharacterWidth();
    int_fast16_t character_width_per_column = screen_width_in_characters / this->number_columns();
    return character_width_per_column - (item_number==number_columns()-1?1:0);
}*/

int SubMenuItemBar::get_max_pixel_width(int item_number) {
    const int current_width = this->tft->width();
    const int columns = this->number_columns();
    if (columns <= 0)
        return 0;

    if (this->cached_pixel_width_per_item==0 ||
        this->cached_screen_width != current_width ||
        this->cached_column_count != columns) {
        this->cached_pixel_width_per_item = this->tft->width() / this->number_columns();
        this->cached_screen_width = current_width;
        this->cached_column_count = columns;
    }
    return this->cached_pixel_width_per_item - (item_number==number_columns()-1?3:0);
}
//int SubMenuItemBar::get_max_characters(int item_)

uint_fast16_t SubMenuItemBar::get_max_character_width(int item_number) {
    return (this->get_max_pixel_width(item_number) / (tft->characterWidth()));// - (item_number==number_columns()-1?1:0);
}

int SubMenuItemBar::display(Coord pos, bool selected, bool opened) {
    // If an opened child requests full takeover (e.g. a nested SubMenuItem),
    // delegate display entirely to it — the bar itself is not rendered.
    // This is distinct from the overlay mechanism (wants_fullscreen_overlay_when_opened_in_bar),
    // which draws a popup on top of already-rendered content.
    if (opened && currently_opened >= 0 && currently_opened < (int)items->size()) {
        MenuItem *takeover_item = items->get(currently_opened);
        if (takeover_item != nullptr && takeover_item->allow_takeover()) {
            return takeover_item->display(Coord(0, pos.y), true, true);
        }
    }

    /*if (this->debug) {
        Serial.printf("Start of display in SubMenuItemBar at %u, passed in %i,%i\n", millis(), pos.x, pos.y);
        Serial.printf("rendering label @%p, ", label); Serial_flush();
        Serial.printf("'%s'\n", label); Serial_flush();
    }*/
    pos.y = header(label, pos, selected, opened);
    //if (this->debug) Serial.printf("\tafter header, y=%i\n", pos.y);    Serial_flush();
    tft->setCursor(pos.x, pos.y);
    //tft->setTextSize(1);
    colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

    uint_fast16_t start_y = pos.y;        // y to start drawing at (just under header)
    uint_fast16_t finish_y = pos.y;       // highest y that we finished drawing at

    // draw all the sub-widgets
    //int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
    uint_fast16_t start_x = 0;
    bool restore_wrap = false;
    bool previous_wrap = false;
    if (this->show_sub_headers) {
        previous_wrap = tft->isTextWrap();
        tft->setTextWrap(false);
        restore_wrap = true;
    }
    //Debug_printf(F("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n"), width_per_item, this->tft->width(), this->items->size());
    //if (this->debug) Serial.printf("\tstarting item index loop...\n");    Serial_flush();
    for (uint_fast16_t item_index = 0 ; item_index < this->items->size() ; item_index++) {
        //tft->setTextSize(0);
        const uint_fast16_t width = this->get_max_pixel_width(item_index);
        //const uint_fast16_t width = this->get_max_character_width(item_index);
        //if (this->debug) Serial.printf("about to small_display for item %i\n", item_index); Serial_flush();
        const uint_fast16_t temp_y = this->small_display(
            item_index, 
            start_x, 
            start_y, 
            width, //width_per_item, 
            this->currently_selected==(int)item_index || (!opened && selected && !this->show_header), 
            this->currently_opened==(int)item_index,
            !opened && selected
        );
        start_x += width;
        if (temp_y>finish_y)
            finish_y = temp_y;
    }
    if (restore_wrap) {
        tft->setTextWrap(previous_wrap);
    }

    // Some controls (e.g. compact selectors) need a second, full-width overlay pass.
    // Defer until end of Menu::display so this draws on top of everything else.
    if (opened && this->currently_opened>=0 && this->currently_opened < (int)this->items->size()) {
        MenuItem *opened_item = this->items->get(this->currently_opened);
        if (opened_item!=nullptr && opened_item->wants_fullscreen_overlay_when_opened_in_bar() && menu!=nullptr) {
            menu->pending_overlay_item = opened_item;
            menu->pending_overlay_y = start_y;
        }
    }

    tft->setTextColor(this->default_fg, this->default_bg);
    //tft->setTextSize(0);

    tft->setCursor(0, finish_y);

    //if (this->debug) Serial.printf(F("End of display, y=%i\n--------\n"), finish_y);
    return finish_y;//tft->getCursorY();
}

/*const char* SubMenuItemBar get_subitem_header_format(int max_display_width_characters) {

}*/

int SubMenuItemBar::small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) {
    //if (this->debug) Serial.printf(F("\tSubMenuItemBar: start of small_display for index %i, passed in x,y=%i,%i and width=%i\n"), index, x, y, width_in_pixels);

    MenuItem *ctrl = items->get(index);
    //const uint_fast16_t character_width_in_pixels = tft->currentCharacterWidth(); // presumed font width
    tft->setTextSize(0);
    const uint_fast16_t current_char_width = tft->currentCharacterWidth() > 0 ? tft->currentCharacterWidth() : 1;
    uint_fast16_t max_display_width_characters = width_in_pixels / current_char_width; //get_max_character_width(index);
    if (max_display_width_characters == 0)
        max_display_width_characters = 1;
    //if (index==this->number_columns()-1)
    //    max_display_width_characters-=1;   // for the last column, use one less character, to avoid wrapping unnecessarily

    //Serial.printf("(width_in_pixels %i/character_width_in_pixels %i) = %i\n", width_in_pixels, character_width_in_pixels, max_display_width_characters);
    /*Serial.printf("small_display in %s, item %i (%s), got colour %4x!\n", 
        this->label, 
        index, 
        ctrl->label,
        ctrl->default_fg
    );*/

    const uint_fast16_t colour = (ctrl->default_fg != C_WHITE) ? ctrl->default_fg : this->default_fg;

    // print label header // TODO: either move this to be the responsibility of the control, or use something like get_label() instead, so that can override in subclass for things like parameter controls where this might change
    if (this->show_sub_headers) {
        //if (this->debug) Serial.printf("\tdrawing header at %i,%i\n", x, y);
        tft->setCursor(x, y);
        colours(is_selected, is_opened ? GREEN : colour, ctrl->default_bg);
        //tft->setTextSize(0);
        //if (x + width_in_pixels>=tft->width())
        //    fmt[strlen(fmt)-2] = '\0';  // cut off the \n if we've reached the width of the display in order to fix wraparound?

        //int textsize = tft->get_textsize_for_width(ctrl->label, get_max_pixel_width(index));
        int textSize = 0;
        tft->setTextSize(textSize);
        print_fixed_width_label(tft, ctrl->get_label(), max_display_width_characters);

        if (x < tft->width())
            y = tft->getCursorY();
        //y += tft->getRowHeight();
        //if (this->debug) Serial.printf("\t bottom of header is %i\n", y);
    }

    // get position ready for value
    tft->setCursor(x, y);   // reset cursor to underneath the label
    /*if (this->debug) {
        Serial.printf("\tdoing renderValue at %i,%i on @%p\n", x, y, ctrl);
        Serial.printf("\taka '%s'\n", ctrl->label);
    }*/

    //if (this->debug) Serial.printf("SubMenuItem for %s:\t rendering item index %i (named %s)\n", label, index, ctrl->label);

    // actually render the item
    tft->setTextSize(0);
    const bool render_opened_inline = is_opened && !ctrl->wants_fullscreen_overlay_when_opened_in_bar();
    if (render_opened_inline)
        colours(is_opened, GREEN, this->default_bg);
    y = ctrl->renderValue((!this->show_sub_headers && outer_selected) || is_selected, render_opened_inline, max_display_width_characters); //width/width_in_chars);

    //if (this->debug) Serial.printf("\tend of small_display, returning y=%i\n", y);
    return y;
}

int SubMenuItemColumns::display(Coord pos, bool selected, bool opened) {
    //Debug_printf("Start of display in SubMenuItemBar, passed in %i,%i\n", pos.x, pos.y);
    pos.y = header(label, pos, selected, opened);
    //Debug_printf(F("\tafter header, y=%i\n"), pos.y);
    tft->setCursor(pos.x, pos.y);
    //tft->setTextSize(1);
    colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

    //int start_y = pos.y;        // y to start drawing at (just under header)
    uint_fast16_t finish_y = pos.y;       // highest y that we finished drawing at

    // draw all the sub-widgets
    //int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
    uint_fast16_t start_x = 0;
    bool restore_wrap = false;
    bool previous_wrap = false;
    if (this->show_sub_headers) {
        previous_wrap = tft->isTextWrap();
        tft->setTextWrap(false);
        restore_wrap = true;
    }
    //Debug_printf(F("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n"), width_per_item, this->tft->width(), this->items->size());
    uint_fast16_t last_y = 0;
    for (uint_fast16_t item_index = 0 ; item_index < this->items->size() ; item_index++) {
        if (item_index % columns == 0) {
            start_x = 0; 
            pos.y = tft->getCursorY();
            last_y = pos.y;
        } else {
            //start_x = (item_index%columns) * (menu->tft->width() / columns);
            start_x += this->get_max_pixel_width(item_index-1);
            pos.y = last_y;
        }
        const uint_fast16_t width = this->get_max_pixel_width(item_index);
        const uint_fast16_t temp_y = this->small_display(
            item_index, 
            start_x, 
            pos.y, 
            width, //width_per_item, 
            this->currently_selected==(int)item_index || (!opened && selected && !this->show_header), 
            this->currently_opened==(int)item_index,
            !opened && selected
        );
        //start_x += width;
        if (temp_y>finish_y)
            finish_y = temp_y;
    }
    if (restore_wrap) {
        tft->setTextWrap(previous_wrap);
    }

    tft->setTextColor(this->default_fg, this->default_bg);
    //tft->setTextSize(0);

    //Debug_printf(F("End of display, y=%i\n--------\n"), finish_y);
    return finish_y;//tft->getCursorY();
}

#endif