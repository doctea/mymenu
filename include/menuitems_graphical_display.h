#pragma once

#include "menuitems.h"

#include "midi_helpers.h"

template<class DataType>
class GraphicalValueDisplay : public MenuItem {
    public:

    float *stored_values = nullptr;
    DataType *target_variable = nullptr;
    DataType min_limit, max_limit;
    int head = 0, tail;
    int8_t height = 15;
    int16_t memory = BEATS_PER_BAR * PPQN;

    GraphicalValueDisplay(const char *label, DataType *target_variable, DataType min, DataType max, int height = 15) : MenuItem(label) {
        this->target_variable = target_variable;
        this->height = 15;

        this->min_limit = min;
        this->max_limit = max;

        this->stored_values = (float*)calloc(memory, sizeof(float));
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(this->label, pos, selected, opened);

        renderValue(selected, opened, tft->width()/tft->currentCharacterWidth());

        return pos.y;
    }

    virtual void update_ticks(uint32_t tick) override {
        //float value = ((float)(*target_variable - min_limit)) / (float)max_limit;
        float value;
        
        //if (tick % 192 > 96) {
            value = *target_variable;
            //if (Serial) Serial.printf("got variable value\t%3.3f\n", value);
        /*} else {
            value = (float)(tick%96)/96.0;
            if (Serial) Serial.printf("got ticked value\t%3.3f\n", value);
        }*/
        //Serial.printf("update_ticks() got value %3.3f to store into %i\n", value, head);
        stored_values[head++] = value;
        if (head>=memory)
            head = 0;
    }

    int pixel_width = 0;
    float mem_per_pixel = 0.0f;

    // render the current value at current position
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        int start = head;

        if (pixel_width==0)
            this->pixel_width = max_character_width * tft->characterWidth();
        if (mem_per_pixel==0.0f)
            mem_per_pixel = max((float)pixel_width / (float)memory, 1.0);

        /*for (int i = 0 ; i < pixel_width ; i++) {
            int index = (head + i) % memory;
            float value = stored_values[index / mem_per_pixel];
            float last_value = i>0 ? stored_values[index-1 / mem_per_pixel] : stored_values[index / mem_per_pixel];
            Serial.printf("i=%i, value=%3.3f, last_value=%3.3f\n", index, value, last_value);
            if (i>0)
                tft->drawLine(i-1, tft->getCursorY() + (last_value * (float)height), i, tft->getCursorY()+value, this->default_fg);
        }*/

        const int_fast16_t base_row = tft->getCursorY();

        tft->drawLine(0, base_row, pixel_width, base_row, GREY);

        int_fast16_t last_y = 0;
        for (int screen_x = 0 ; screen_x < pixel_width ; screen_x++) {
            //const int_fast16_t tick_for_screen_X = screen_x / mem_per_pixel; //ticks_to_memory_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
            const int_fast16_t tick_for_screen_X = (head + (int)((float)screen_x/mem_per_pixel)) % memory;
            //Serial.printf("mem_per_pixel=%i: for screen_x=%i, got tick_for_screen_X=%i\n", mem_per_pixel, screen_x, tick_for_screen_X);
            const float value = stored_values[tick_for_screen_X];
            const int_fast16_t y = height - (value * height);
            if (screen_x != 0) {
                //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, this->default_fg);                    
            }
            last_y = y;
        }

        tft->drawLine(0, base_row+height, pixel_width, base_row+height, GREY);

        return tft->getCursorY();
    }

};

/*
  char history[11] = "JIHGFEDCBA";

  for (int head = 0 ; head < 15 ; head++) {
      for (int i = 0 ; i < 10 ; i++) {
        //int index = abs((i - head) % 10);
        //int index = abs(10 - (i - head));
        int index = abs(head + i) % 10;
           
        std::cout << history[index] << " ";
      }
      std::cout << "\n";
      history[head%10] = std::rand();
  }
  
    J I H G F E D C B A 
    I H G F E D C B A ﾦ 
    H G F E D C B A ﾦ ﾉ 
    G F E D C B A ﾦ ﾉ  
    F E D C B A ﾦ ﾉ  ￝ 
    E D C B A ﾦ ﾉ  ￝     
    D C B A ﾦ ﾉ  ￝   ￕ 
    C B A ﾦ ﾉ  ￝   ￕ I 
    B A ﾦ ﾉ  ￝   ￕ I  
    A ﾦ ﾉ  ￝   ￕ I  ﾶ 
    ﾦ ﾉ  ￝   ￕ I  ﾶ ￧ 
    ﾉ  ￝   ￕ I  ﾶ ￧ ﾇ 
     ￝   ￕ I  ﾶ ￧ ﾇ  
    ￝  ￕ I  ﾶ ￧ ﾇ   
    ￕ I  ﾶ ￧ ﾇ   ~ 

/////

  #define	PI		3.141592654 
  #define TICKS_PER_PHRASE 96
 
  std::cout.precision(3);
  
  for (int tick = 0 ; tick < 2000 ; tick++) {
      float normal = (tick % TICKS_PER_PHRASE)/((float)TICKS_PER_PHRASE);
      std::cout << tick << "\t = normal\t" << normal << ":\t" << sin(normal*(2.0*PI)) << "\n";
  }


*/