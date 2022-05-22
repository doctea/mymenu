#ifndef MYMENU__INCLUDED
#define MYMENU__INCLUDED

//#include "Config.h"

#include <LinkedList.h>
//#include <Adafruit_GFX.h>
//#include "ST7789_t3.h"
#include "tft.h"
//#include "menu.h"

//#include "midi_helpers.h"
//#include "debug.h"
//#include "bpm.h"
//#include "project.h"

class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};

#include "menu.h"
#include "menuitems.h"

/*
#ifdef ENCODER_KNOB_L
extern Encoder knob;
#endif
#ifdef PIN_BUTTON_A
extern Bounce pushButtonA;
#endif
#ifdef PIN_BUTTON_B
extern Bounce pushButtonB;
#endif
#ifdef PIN_BUTTON_C
extern Bounce pushButtonC;
#endif
*/

//class Menu;
//extern Menu menu;

#endif