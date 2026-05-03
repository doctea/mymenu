#pragma once
#ifdef ENABLE_BUTTON_MATRIX

#include <Wire.h>
#include <MCP23008.h>

#define KEYPAD_ADDRESS 32

extern MCP23008 mcp;
extern bool button_status_matrix[4][4]; // store the current status of each button in the matrix

struct key_pressed_t {
    int row;
    int col;
};

static int buildLookup(uint8_t gpioValue);
void setup_keypad();
key_pressed_t readKeys();
int getKey();

#endif