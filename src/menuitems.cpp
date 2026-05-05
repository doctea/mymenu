#ifdef ENABLE_SCREEN

#include "menuitems.h"

const char *fired_message = "Fired %s";
const char *sure_message = "Sure ???";
const char *set_message = "Set %s to %s (%i)";
const char *label_on    = "On";
const char *label_off   = "Off";

// called when this menuitem is added to menu
void MenuItem::on_add() {
    //Debug_printf(F("MenuItem#on_add in %s\n"), this->label);
    menu_c_max = tft->get_c_max();
}    
void MenuItem::update_label(const char *new_label) {
    //Debug_printf("%s#update_label('%s')\n", this->label, new_label);
    strncpy(this->label, new_label, MAX_LABEL_LENGTH);
}
MenuItem *MenuItem::set_default_colours(uint16_t fg, uint16_t bg) {
    this->default_fg = fg;
    this->default_bg = bg;
    return this;
}
int MenuItem::renderValue(bool selected, bool opened, uint16_t max_character_width) {
    //tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
    colours(selected);
    //tft->setTextSize((strlen(label) < max_character_width/2) ? 2 : 1 );
    tft->setTextSize(tft->get_textsize_for_width(label, max_character_width*tft->characterWidth()));
    tft->println(label);
    return tft->getCursorY();
}
void MenuItem::colours(bool inverted) {
    colours(inverted, /*tft->WHITE*/this->default_fg, this->default_bg);
}
void MenuItem::colours(bool inverted, uint16_t fg) {
    colours(inverted, fg, this->default_bg);
}
void MenuItem::colours(bool inverted, uint16_t fg, uint16_t bg) {
    if (!inverted) {
        tft->setTextColor(fg, bg);
    } else {
        //Serial.printf("%s selected, setting colours %02x, %02x\n", label, bg, fg);
        tft->setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
    }
}
bool MenuItem::action_opened() {
    return true;
}
bool MenuItem::button_select() {
    return go_back_on_select;
}
bool MenuItem::button_select_released() {
    return false;
}
bool MenuItem::button_back() {
    return false;
}
bool MenuItem::button_right() {
    return false;
}
bool MenuItem::knob_left() {
    return false;
}
bool MenuItem::knob_right() {
    return false;
}
bool MenuItem::allow_takeover() {
    return false;
}
bool MenuItem::is_selectable () {
    return this->selectable;
}
int MenuItem::display(Coord pos, bool selected, bool opened) {
    //Serial.printf("MenuItem display()")
    //tft_print("hello?");
    //char state[10];
    //if (selected) sprintf(state,"Sel");
    //if (opened) sprintf(&state[3]," Open");
    //sprintf(state,"%s%s",selected?'Sel':'   ', opened?'Ope':'   ');
    // display this item however that may be
    //tft->fillRect(random(20), random(20), random(20), random(20), C_WHITE);
    //Serial.printf("MenuItem: base display for %s at (%i,%i) [s:%i o:%i]\n", label, tft->getCursorX(), tft->getCursorY(), selected, opened);
    tft->setTextSize(0);
    tft->setCursor(pos.x,pos.y);
    //tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    colours(selected);
    //tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
    this->renderValue(selected, opened, MENU_C_MAX);
    this->tft->println();
    //return (tft->getTextSizeY() * 8) + 2;
    return tft->getCursorY();
}
int MenuItem::header(const char *text, Coord pos, bool selected, bool opened, int textSize) {
    if (!this->show_header) return pos.y;

    tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
    pos.y+=2;
    tft->setCursor(pos.x, pos.y);
    colours(selected, this->default_fg, this->default_bg);
    tft->setTextSize(textSize);
    if (opened) {
        //tft->print(">>>");
        //tft->printf((char*)"%-19s",(char*)text);   // \n not needed as reaching to edge
        tft->printf((char*)tft->get_header_open_format(), (char*)text);
    } else if (selected) {
        //tft->printf((char*)"%-22s",(char*)text);   // \n not needed as reaching to edge
        tft->printf((char*)tft->get_header_selected_format(), (char*)text);
        //tft->println();
    } else {
        tft->printf((char*)tft->get_header_format(), (char*)text);
    }
    colours(false);
    //return (tft->getTextSize()+1)*6;
    /*if (tft->getCursorY() <= pos.y) {
        // we havent had enough characters to move down a line, so force one
        tft->println();
    }*/
    tft->setCursor(0, tft->getCursorY()+2);
    return tft->getCursorY(); // + 2;
}

int FixedSizeMenuItem::renderValue(bool selected, bool opened, uint16_t max_character_width) {
    //tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
    colours(selected);
    //tft->setTextSize((strlen(label) < max_character_width/2) ? 2 : 1 );
    //tft->setTextSize(tft->get_textsize_for_width(label, max_character_width*tft->characterWidth()));
    tft->setTextSize(this->fixed_size);
    tft->println(label);
    return tft->getCursorY();
}

int SeparatorMenuItem::header(const char *text, Coord pos, bool selected, bool opened, int textSize) {
    if (!this->show_header) return pos.y;

    tft->setTextSize(textSize);
    colours(false, this->default_fg, this->default_bg);

    int end_x;

    if (this->draw_lines) {
        tft->setCursor(pos.x, pos.y+1);

        end_x = (tft->width() - (tft->currentCharacterWidth() * strlen(text))) - 4;
        tft->drawLine(0, pos.y,   end_x, pos.y,   this->default_fg);
        tft->drawLine(0, pos.y+2, end_x, pos.y+2, this->default_fg);
        tft->drawLine(0, pos.y+4, end_x, pos.y+4, this->default_fg);

        tft->setCursor(end_x+4, pos.y);
    }
    bool wasTextWrap = tft->isTextWrap();
    tft->setTextWrap(false);
    tft->printf((char*)tft->get_header_format(), (char*)text);
    tft->println();
    tft->setTextWrap(wasTextWrap);
    // have to draw this after the text for some reason, otherwise it doesn't get drawn/gets wiped out??
    if (draw_lines) {
        tft->drawLine(0, pos.y+6, end_x, pos.y+6, this->default_fg);
        tft->setCursor(0, tft->getCursorY()+2);
    }
    colours(false);

    return tft->getCursorY();
}

int SeparatorMenuItem::display(Coord pos, bool selected, bool opened) {
    tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
    pos.y += 2;
    
    /*int old_y = pos.y;*/
    pos.y = header(label, pos, selected, opened, this->textSize);
    /*if (old_y==pos.y) /{
        pos.y += tft->getRowHeight(); // force display down a lne
    }*/

    return pos.y;
}

#endif