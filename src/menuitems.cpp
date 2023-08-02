#include "menuitems.h"

int SeparatorMenuItem::header(const char *text, Coord pos, bool selected, bool opened) {
    if (!this->show_header) return pos.y;

    /*tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
    tft->setCursor(pos.x, pos.y+1);
    //colours(!selected, this->default_fg, this->default_bg);
    colours(false, this->default_fg, this->default_bg);
    tft->setTextSize(0);
    tft->printf((char*)tft->get_header_format(), (char*)text);
    int start_x = tft->characterWidth() * strlen(text);
    //tft->drawLine(start_x, pos.y, tft->width(), pos.y, this->default_fg);
    tft->drawLine(start_x, pos.y+3, tft->width(), pos.y+2, this->default_fg);
    tft->drawLine(start_x, pos.y+6, tft->width(), pos.y+4, this->default_fg);
    tft->drawLine(start_x, pos.y+8, tft->width(), pos.y+6, this->default_fg);*/
    tft->setTextSize(0);

    colours(false, this->default_fg, this->default_bg);
    //tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
    tft->setCursor(pos.x, pos.y+1);
    int end_x = (tft->width() - (tft->characterWidth() * strlen(text))) - 2;
    tft->drawLine(0, pos.y,   end_x-2, pos.y,   this->default_fg);
    tft->drawLine(0, pos.y+2, end_x-2, pos.y+2, this->default_fg);
    tft->drawLine(0, pos.y+4, end_x-2, pos.y+4, this->default_fg);

    //tft->drawLine(0, pos.y, end_x-1, pos.y+12, YELLOW); // debu g disalgonal line
    tft->setCursor(end_x+2, pos.y);
    //int initial_position = tft->getCursorY();
    tft->printf((char*)tft->get_header_format(), (char*)text);
    // have to draw this after the text for some reason, otherwise it doesn't get drawn/gets wiped out??
    tft->drawLine(0, pos.y+6, end_x-2, pos.y+6, this->default_fg);

    //tft->drawLine(8, pos.y+8, end_x-1, pos.y+8, this->default_fg);
    //tft->drawLine(10, pos.y+10, end_x-1, pos.y+12, this->default_fg);
    tft->setCursor(0, tft->getCursorY()+1);

    //int ret_height = tft->getCursorY();
    /*if (ret_height==initial_position) {      // force display down a row if we haven't moved
        //Serial.printf("ret_height matches initial_position of %i, so incrementing by %i\t(x is %i)\n", initial_position, tft->getRowHeight(), tft->getCursorX());
        ret_height += tft->getRowHeight();
    } else {
        //Serial.printf("ret_height different already (%i vs %i) so not moving!\t(x is %i)\n", initial_position, tft->getCursorY(), tft->getCursorX());
    }*/

    colours(false);

    //return (tft->getTextSize()+1)*6;
    return tft->getCursorY();
    //return ret_height + 2;
}