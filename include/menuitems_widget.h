// want to make a new kind of menuitem that can draw a nice widget in a square grid, using an icon instead of text.
// it'll eventually be used to handle accessing Parameters, and modulation too; but maybe a generic 
// implementation that can draw a grid of icons will be the first step.

#pragma once

#include "mymenu.h"

#include "menuitems.h"

class Icon {
    public:
        // for now, just a placeholder for a bitmap or something, but eventually this could be extended to include different states (eg for toggles)
        const uint8_t *bitmap;
        uint8_t *unpacked;

        int icon_width = 32;
        int icon_height = 32;
        
        Icon(const uint8_t *bitmap, int input_width, int input_height) 
            : bitmap(bitmap)
        {
            // unpack the bitmap into a more easily drawable format (for now, just a 1D array of bytes where each byte represents a pixel, but this could be extended to support different colour depths or something)
            unpacked = new uint8_t[this->icon_width * this->icon_height];
            for (int y = 0 ; y < this->icon_height ; y++) {    
                for (int x = 0 ; x < this->icon_width ; x++) {
                    int byte_index = y / (icon_height/input_height);
                    int bit_index = x / (icon_width/input_width);
                    unpacked[x + (y * this->icon_width)] = (bitmap[byte_index] >> bit_index) & 1;
                }
            }
        };


        void draw(DisplayTranslator *tft, int x, int y, int width, int height, uint16_t fg, uint16_t bg, bool inverted = false) {
            // for now, just draw a filled rectangle as a placeholder for the icon
            if (inverted) {
                uint16_t temp = fg;
                fg = bg;
                bg = temp;
            }

            tft->fillRect(x+1, y+1, width-2, height-2, fg);
            tft->drawRect(x, y, width, height, bg);

            for (int yy = 0 ; yy < height ; yy++) {
                for (int xx = 0; xx < width ; xx++) {
                    int px_from_x = (int)(xx);
                    int px_from_y = (int)(yy);
                    int value = unpacked[px_from_x + (px_from_y * icon_width)];
                    if (value) {
                        tft->drawFastHLine(x + xx, y + yy, 1, !inverted ? fg : bg);
                    } else {
                        tft->drawFastHLine(x + xx, y + yy, 1, inverted ? fg : bg);
                    }
                }
            }
        }
};

class MenuItems_Widget : public MenuItem {

    int width = 32, height = 32;
    const int BORDER_WIDTH = 2;

    Icon *icon = nullptr;

    public:
        MenuItems_Widget(const char *in_label, bool selectable = true, bool show_header = true, int w = 32, int h = 32, Icon *icon = nullptr) 
            : MenuItem(in_label, selectable, show_header), width(w), height(h), icon(icon) {
        }        

        MenuItems_Widget(const char *in_label, Icon *icon) 
            : MenuItem(in_label), icon(icon) {
                this->width = icon->icon_width;
                this->height = icon->icon_height;
        }

        MenuItems_Widget(const char *in_label, Icon *icon, int w = 32, int h = 32) 
            : MenuItem(in_label), icon(icon) {
                this->width = w;
                this->height = h;
        }

        void set_icon(Icon *icon) {
            this->icon = icon;
        }

        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            // just draw a big square for now, with the label in the middle
            int size = max_character_width * 2; // make it big enough to be a nice big button, but not so big that it takes up the whole screen
            int x = tft->getCursorX(); // just use the current cursor position for now
            int y = tft->getCursorY();

            
            // draw the label in the middle of the square
            colours(selected, default_fg, default_bg);
            //tft->setTextDatum(MC_DATUM); // center the text
            if (icon) {
                icon->draw(tft, x, y, width, height, selected ? default_fg : default_bg, !selected ? default_fg : default_bg, selected);
            } else {
                tft->fillRect(x, y, width, height, selected ? default_fg : default_bg);
                tft->drawRect(x, y, width, height, !selected ? default_fg : default_bg);

                int16_t text_x = x + BORDER_WIDTH;
                int16_t text_y = y + BORDER_WIDTH;

                tft->setCursor(text_x, text_y);
                tft->setTextSize(1);
                tft->print(label);
            }

            tft->setCursor(x, y + height + BORDER_WIDTH); // move the cursor to below the square for the next item to be drawn

            return height + 4; // return the height of the item (size of the square + some padding)
        }

};
