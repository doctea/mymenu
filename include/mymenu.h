#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 4) && (__GNUC_PATCHLEVEL__ > 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

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

#include "colours.h"

#include "menu.h"
#include "menuitems.h"


#endif