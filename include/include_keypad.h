#pragma once
#ifdef ENABLE_BUTTON_MATRIX

#include <Wire.h>
#include <MCP23008.h>

#define KEYPAD_ADDRESS 32

MCP23008 mcp(KEYPAD_ADDRESS);

struct key_pressed_t {
    int row;
    int col;
};

void setup_keypad() {
    mcp.begin(KEYPAD_ADDRESS);

    mcp.setPolarity8(0xFF);  // invert all pins
    mcp.setPullup8(0xFF);    // enable pull-up on all pins
}

bool button_status_matrix[4][4] = {0}; // store the current status of each button in the matrix

// Lookup table: gpio_value -> key index (0-15), -1 = no key
// Mirrors the Python gpio_lookup dict built in __init__
// row_mask  = 1 << (3 - (key & 3))        -> bit in low nibble
// col_mask  = 16 << (3 - (key >> 2))       -> bit in high nibble
// combined  = row_mask | col_mask
static int buildLookup(uint8_t gpioValue) {
    for (uint_fast8_t key = 0; key < 16; key++) {
        uint_fast8_t rowMask    = 1  << (3 - (key & 3));
        uint_fast8_t columnMask = 16 << (3 - (key >> 2));
        if ((rowMask | columnMask) == gpioValue) {
            return key;
        }
    }
    return -1;  // no key matched
}

// int buildStatusMatrix(uint8_t gpioValue) {
//     Serial.printf("buildStatusMatrix: ");
//     int statusMatrix = 0;
//     for (uint_fast8_t key = 0; key < 16; key++) {
//         uint_fast8_t rowMask    = 1  << (3 - (key & 3));
//         uint_fast8_t columnMask = 16 << (3 - (key >> 2));
//         if ((rowMask | columnMask) & gpioValue) {
//             statusMatrix |= (1 << key);
//             Serial.printf("%i=ON\t", key);
//         } else {
//             Serial.printf("%i=OFF\t", key);
//         }
//     }
//     Serial.println();

//     return statusMatrix;
// }

int getKey() {
    // --- Scan rows ---
    // Python: self.i2c_write([0, 240])  -> IODIR = 0xF0 -> top 4 = inputs, bottom 4 = outputs
    // Python: self.i2c_write([9])       -> point at GPIO register
    // Python: row_value = self.i2c_read(1)[0]
    mcp.pinMode8(0xF0);      // 1 = input, 0 = output  (0xF0 -> pins 4-7 input, 0-3 output)
    mcp.write8(0x00);        // drive output pins LOW to let current flow through pressed key
    uint8_t rowValue = mcp.read8();

    // --- Scan columns ---
    // Python: self.i2c_write([0, 15])   -> IODIR = 0x0F -> bottom 4 = inputs, top 4 = outputs
    mcp.pinMode8(0x0F);      // pins 0-3 input, 4-7 output
    mcp.write8(0x00);
    uint8_t colValue = mcp.read8();

    uint8_t combined = rowValue | colValue;
    // buildStatusMatrix(combined);

    if (combined == 0) {
        return -1;  // nothing pressed
    }

    return buildLookup(combined);
}


key_pressed_t readKeys() {
    int key = getKey();

    if (key == -1) {
        return {-1, -1};  // nothing pressed
    }

    int row = key / 4;
    int col = key % 4;

    return {row, col};
}

#endif