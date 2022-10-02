#ifndef MENUITEM_PINNED__INCLUDED
#define MENUITEM_PINNED__INCLUDED

class PinnedPanelMenuItem : public MenuItem {
    public:
        unsigned long ticks = 0;

        PinnedPanelMenuItem(const char *label) : MenuItem(label) {};

        virtual void update_ticks(unsigned long ticks) override {
            this->ticks = ticks;
        }
};

class LoopMarkerPanel : public PinnedPanelMenuItem {
    unsigned long loop_length;
    int beats_per_bar = 4;
    int bars_per_phrase = 4;
    int ppqn;

    public:
        LoopMarkerPanel(int loop_length, int ppqn, int beats_per_bar = 4, int bars_per_phrase = 4) : PinnedPanelMenuItem("Loop Position Header") {
            this->loop_length = loop_length;
            this->beats_per_bar = beats_per_bar;
            this->bars_per_phrase = bars_per_phrase;
            this->ppqn = ppqn;
        };

        void set_loop_length(unsigned long loop_length) {
            this->loop_length = loop_length;
        }
        void set_beats_per_bar(unsigned long beats_per_bar) {
            this->beats_per_bar = beats_per_bar;
        }        

        virtual int display(Coord pos, bool selected = false, bool opened = false) override {
            //Serial.printf("PinnedPanel display colour RED is %4x, WHITE is %4x\n", RED, C_WHITE);

            tft->setTextColor(C_WHITE, BLACK);
            //tft.setCursor(pos.x,pos.y);
            //int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            int y = pos.y; //0;
            //y+=2;
            static unsigned long last_serviced_tick;
            static int last_position_width;

            // save some float maths by only recalculating if tick is different from last time
            if (last_serviced_tick != ticks) {
                float percent = float(ticks % loop_length) / (float)loop_length;
                int new_position_width = (percent*(float)tft->width());
                //Serial.printf("ticks %i: ticks%loop_length = %i: ", ticks, ticks%loop_length);
                //if (ticks%loop_length==0)   // if we're at the start of loop then blank out the display 
                if (new_position_width < last_position_width){
                    //Serial.println("so drawing black?");
                    tft->fillRect(0,y,tft->width(), y+6, BLACK);
                } /*else {
                    Serial.println();
                }*/
                last_position_width = new_position_width;
            }
            tft->fillRect(0, y, last_position_width, y+6, RED);

            //float percent = float(ticks % loop_length) / (float)loop_length;
            //tft->fillRect(0, y, (percent*(float)tft->width()), 6, RED);

            /*static float px_per_pos = tft->width() / loop_length;
            int loop_position = ticks % loop_length;
            tft->fillRect(0, y, px_per_pos * loop_position, 6, RED);*/

            static int tft_width = tft->width();

            static int step_size_beats = tft_width / (beats_per_bar*bars_per_phrase);  // safe to make static so long as beats_per_bar/bars_per_phrase is not configurable!
            for (int i = 0 ; i < tft_width ; i += step_size_beats) {
                tft->drawLine(i, y, i, y+2, C_WHITE);
                //if (i%BEATS_PER_BAR==0)
                    //tft.drawLine(i, y, i, y+4, ST7735_CYAN);
            }

            static int step_size_bars = tft_width / bars_per_phrase;
            for (int i = 0 ; i < tft_width ; i += step_size_bars) {
                //tft.drawLine(i, y, i, y+4, ST7735_WHITE);
                tft->fillRect(i, y+1, 2, 5, C_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft->width());
            y += 6;
            return y;
        }
        //#endif
};

#endif