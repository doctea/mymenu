#pragma once

// Remote viewer serial protocol — shared between all mymenu-based projects.
//
// Include this in exactly ONE translation unit per project (your main .cpp).
// Requires: extern Menu *menu  (provided by all ENABLE_SCREEN projects).
// Activate by defining ENABLE_REMOTE_VIEWER in build flags.
//
// Wire protocol (device → host):
//   ==VIEWER-HELLO==\nwidth:W\nheight:H\nprefix:1\ncontrols:...\n==END-VIEWER-HELLO==
//   ==START-FRAME== [w:2][h:2][enc:1][size:4][payload] ==END-FRAME==
//
// Wire protocol (host → device):
//   '^'            (bare, no prefix) — request capability announcement
//   '\x01' + cmd   — command byte (d/f/a/b/n/e/L/X)

#ifdef ENABLE_REMOTE_VIEWER

#include "menu.h"

extern Menu *menu;

#ifndef ARDUINO_BOARD
    #define ARDUINO_BOARD "unknown"
#endif

#define VIEWER_PREFIX_BYTE  ((char)0x01)
#define VIEWER_MAX_PENDING_KNOB_STEPS    32
#define VIEWER_MAX_PENDING_BUTTON_PRESSES 8

#ifndef REMOTE_VIEWER_HELLO_RESEND_MS
    #define REMOTE_VIEWER_HELLO_RESEND_MS 2000
#endif
#ifndef REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS
    #define REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS 1
#endif

#define RV_STRINGIFY_HELPER(x) #x
#define RV_STRINGIFY(x) RV_STRINGIFY_HELPER(x)

static const char *remote_viewer_feature_flags() {
    return
            "MENU_PERF_PARTIAL_UPDATES="
        #if defined(MENU_PERF_PARTIAL_UPDATES)
            "1;"
        #else
            "0;"
        #endif
        "REMOTE_VIEWER_LIVE_MAX_FPS="
        #ifdef REMOTE_VIEWER_LIVE_MAX_FPS
            "1;"
        #else
            "0;"
        #endif
        ;
}

static const char *remote_viewer_build_flags() {
    return
        "ENABLE_REMOTE_VIEWER=1;"
        "ENABLE_SCREEN=1;"
        "TFT_BODMER=1;"
        "BODMER_SPRITE=1;"
        "MENU_PERF_PARTIAL_UPDATES="
        #if defined(MENU_PERF_PARTIAL_UPDATES)
            RV_STRINGIFY(MENU_PERF_PARTIAL_UPDATES) ";"
        #else
            "0;"
        #endif
        "REMOTE_VIEWER_LIVE_MAX_FPS="
        #ifdef REMOTE_VIEWER_LIVE_MAX_FPS
            RV_STRINGIFY(REMOTE_VIEWER_LIVE_MAX_FPS) ";"
        #else
            "default(8);"
        #endif
        ;
}

struct _ViewerPendingState {
    int32_t  knob_steps      = 0;
    uint16_t back_presses    = 0;
    uint16_t select_presses  = 0;
    uint16_t right_presses   = 0;
    bool     send_frame      = false;
    bool     toggle_live     = false;
    bool     hello_sent      = false;
    uint32_t last_hello_sent_at = 0;
    uint8_t  hello_auto_send_count = 0;
};

static _ViewerPendingState _viewer_state;

// Send the capability announcement to the host over Serial.
// Called automatically on first serial connection and whenever '^' is received.
static void send_viewer_hello() {
    Serial.println("==VIEWER-HELLO==");
    Serial.printf("width:%d\n",  menu->tft->width());
    Serial.printf("height:%d\n", menu->tft->height());
    Serial.println("prefix:1");
    Serial.printf("pixel_format:%s\n", menu->tft->viewer_pixel_format());
    Serial.printf("board:%s\n", ARDUINO_BOARD);
    Serial.printf("cpu_mhz:%lu\n", (unsigned long)(F_CPU / 1000000UL));
    Serial.printf("pages:%d\n", menu->get_number_pages());
    Serial.printf("page_index:%d\n", menu->get_selected_page_index());
    Serial.printf("page_title:%s\n", menu->get_selected_page_title());
    Serial.printf("profile:%s\n", menu->get_profile_string());
    Serial.printf("flags:%s\n", remote_viewer_feature_flags());
    Serial.printf("build_flags:%s\n", remote_viewer_build_flags());
    Serial.print("controls:d=knob_left,f=knob_right,a=button_back,b=button_select");
    #ifdef PIN_BUTTON_C
    Serial.print(",n=button_right");
    #endif
    Serial.print(",[]=page_prev_next,?=summary");
    Serial.println();
    Serial.println("==END-VIEWER-HELLO==");
}

static void _handle_viewer_command(char cmd) {
    switch (cmd) {
        case 'd':
            if (_viewer_state.knob_steps > -VIEWER_MAX_PENDING_KNOB_STEPS)
                _viewer_state.knob_steps--;
            _viewer_state.send_frame = true;
            break;
        case 'f':
            if (_viewer_state.knob_steps < VIEWER_MAX_PENDING_KNOB_STEPS)
                _viewer_state.knob_steps++;
            _viewer_state.send_frame = true;
            break;
        case 'a':
            if (_viewer_state.back_presses < VIEWER_MAX_PENDING_BUTTON_PRESSES)
                _viewer_state.back_presses++;
            _viewer_state.send_frame = true;
            break;
        case 'b':
            if (_viewer_state.select_presses < VIEWER_MAX_PENDING_BUTTON_PRESSES)
                _viewer_state.select_presses++;
            _viewer_state.send_frame = true;
            break;
        case 'n':
            if (_viewer_state.right_presses < VIEWER_MAX_PENDING_BUTTON_PRESSES)
                _viewer_state.right_presses++;
            _viewer_state.send_frame = true;
            break;
        case 'e':
            _viewer_state.send_frame = true;
            break;
        case 'L':
            _viewer_state.toggle_live = !_viewer_state.toggle_live;
            _viewer_state.send_frame = true;
            break;
        case '[':
            if (menu) menu->select_previous_page();
            send_viewer_hello();
            _viewer_state.send_frame = true;
            break;
        case ']':
            if (menu) menu->select_next_page();
            send_viewer_hello();
            _viewer_state.send_frame = true;
            break;
        case '?':
            send_viewer_hello();
            break;
        case 'X':
            _viewer_state.toggle_live = false;
            if (menu) menu->send_frame_live = false;
            break;
        default:
            break;
    }
}

// Call once per loop() BEFORE any text-console serial reader (e.g. update_serial /
// debug_console).  Intercepts '^' and '\x01'+cmd sequences before they can be
// absorbed by the debug-console line-buffer.  Leaves all other bytes untouched.
static void read_viewer_serial() {
    if (!Serial) {
        _viewer_state.hello_sent = false;
        _viewer_state.last_hello_sent_at = 0;
        _viewer_state.hello_auto_send_count = 0;
        // Stop live mode so that a reconnecting serial monitor isn't flooded
        // with binary frame data.  Live mode restarts when '^' is received.
        if (menu) menu->send_frame_live = false;
        return;
    }
    uint32_t now_ms = millis();
    if (!_viewer_state.hello_sent ||
        (_viewer_state.hello_auto_send_count < REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS &&
         now_ms - _viewer_state.last_hello_sent_at > REMOTE_VIEWER_HELLO_RESEND_MS)) {
        send_viewer_hello();
        _viewer_state.hello_sent = true;
        _viewer_state.last_hello_sent_at = now_ms;
        if (_viewer_state.hello_auto_send_count < 255)
            _viewer_state.hello_auto_send_count++;
    }
    while (Serial.available() > 0) {
        char c = (char)Serial.peek();
        if (c == '^') {
            Serial.read();           // consume '^'
            send_viewer_hello();
            _viewer_state.hello_sent = true;
            _viewer_state.last_hello_sent_at = now_ms;
            _viewer_state.hello_auto_send_count = REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS;
            // viewer explicitly connected — start live mode immediately
            if (menu) menu->send_frame_live = true;
            _viewer_state.send_frame = true;
        } else if (c == VIEWER_PREFIX_BYTE) {
            if (Serial.available() < 2) break;  // wait for the command byte too
            Serial.read();           // consume prefix
            char cmd = (char)Serial.read();
            _handle_viewer_command(cmd);
            _viewer_state.hello_sent = true;
            _viewer_state.last_hello_sent_at = now_ms;
            _viewer_state.hello_auto_send_count = REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS;
        } else {
            break;  // leave unrecognised bytes for the debug console
        }
    }
}

// Call once per loop() AFTER read_viewer_serial() and only when safe to call
// menu methods (e.g. when the menu lock is free).
static void dispatch_viewer_commands() {
    while (_viewer_state.knob_steps < 0) {
        menu->knob_left();
        _viewer_state.knob_steps++;
    }
    while (_viewer_state.knob_steps > 0) {
        menu->knob_right();
        _viewer_state.knob_steps--;
    }
    while (_viewer_state.back_presses > 0) {
        menu->button_back();
        _viewer_state.back_presses--;
    }
    while (_viewer_state.select_presses > 0) {
        menu->button_select();
        menu->button_select_released();
        _viewer_state.select_presses--;
    }
    while (_viewer_state.right_presses > 0) {
        menu->button_right();
        _viewer_state.right_presses--;
    }
    if (_viewer_state.toggle_live) {
        menu->send_frame_live = !menu->send_frame_live;
        _viewer_state.toggle_live = false;
        _viewer_state.send_frame = true;
    }
    if (_viewer_state.send_frame) {
        menu->send_frame = true;
        _viewer_state.send_frame = false;
    }
}

#endif // ENABLE_REMOTE_VIEWER
