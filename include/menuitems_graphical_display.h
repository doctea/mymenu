#pragma once

#include "menuitems.h"

#include "midihelpers.h"

template<class DataType>
class GraphicalValueDisplay : public MenuItem {
    public:

    float *stored_values = nullptr;
    DataType *target_variable = nullptr;
    DataType min, max;
    int head = 0, tail;
    int8_t height = 15;
    int8_t memory = 100;

    GraphicalValueDisplay(const char *label, DataType *target_variable, DataType min, DataType max, int height = 15) : MenuItem(label) {
        this->target_variable = target_variable;
        this->height = 15;

        this->min = min;
        this->max = max;

        this->stored_values = calloc(TICKS_PER_PHRASE, sizeof(float));
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(this->label, pos, selected, opened);

        renderValue(selected, opened, tft->width()/tft->currentCharacterWidth());

        return pos.y;
    }

    virtual void update_ticks(uint32_t tick) override {
        stored_values[head++] = (float)(*target_variable - min) / (float)max;
        if (head>=memory)
            head = 0;
    }

    // render the current value at current position
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        int start = head;

        int pixel_width = max_character_width * tft->characterWidth();
        int mem_per_pixel = pixel_width / memory;

        for (int i = 0 ; i < pixel_width ; i++) {
            int index = (head + i) % memory;
            float value = stored_values[index / mem_per_pixel];
            float last_value = i>0 ? stored_values[index-1 / mem_per_pixel] : stored_values[index / mem_per_pixel];
            if (i>0)
                tft->drawLine(i-1, last_value * (float)height, i, value, this->default_fg_colour);            
        }

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