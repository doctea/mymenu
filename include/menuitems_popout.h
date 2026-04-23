#pragma once

#include "menuitems.h"

struct SelectorTakeoverOverlaySpec {
    const char *title = nullptr;
    const char *subtitle = nullptr;
    const char *value = nullptr;
    const char *left_hint = nullptr;
    const char *right_hint = nullptr;

    uint16_t frame_colour = C_WHITE;
    uint16_t title_fg = BLACK;
    uint16_t value_fg = C_WHITE;
    uint16_t subtitle_fg = C_WHITE;
    uint16_t left_hint_fg = C_WHITE;
    uint16_t right_hint_fg = C_WHITE;

    int box_padding = 2;
    int min_box_h = 26;
    int subtitle_top_gap = 3;
    int max_title_text_size = 1;
    int max_value_text_size = 3;
    int max_subtitle_text_size = 1;

    bool has_extra = false;
    int extra_height = 0;
    vl::Func<void(DisplayTranslator *tft, int x, int y, int max_width, int max_height)> draw_extra;
};

inline void menu_draw_centered_text(
    DisplayTranslator *tft,
    const char *txt,
    int center_x,
    int center_y,
    int pixel_width,
    uint16_t fg,
    uint16_t bg,
    uint8_t max_text_size
) {
    if (tft==nullptr || txt==nullptr || txt[0]=='\0')
        return;

    uint8_t text_size = tft->get_textsize_for_width(txt, pixel_width);
    if (text_size > max_text_size)
        text_size = max_text_size;
    tft->setTextSize(text_size);

    const int text_pixel_width = strlen(txt) * tft->characterWidth() * (text_size>0 ? text_size : 1);
    const int text_pixel_height = tft->getSingleRowHeight() * (1 + text_size);
    const int draw_x = center_x - (text_pixel_width / 2);
    const int draw_y = center_y - (text_pixel_height / 2);

    tft->setCursor(draw_x, draw_y);
    tft->setTextColor(fg, bg);
    tft->print(txt);

    tft->setCursor(0, draw_y + text_pixel_height);
}

inline int menu_draw_selector_takeover_overlay(
    DisplayTranslator *tft,
    Coord pos,
    const SelectorTakeoverOverlaySpec &spec
) {
    if (tft==nullptr)
        return pos.y;

    const bool has_subtitle = spec.subtitle!=nullptr && spec.subtitle[0]!='\0';
    const int screen_w = tft->width();
    const int screen_h = tft->height();
    const int margin_x = max(3, screen_w / 16);
    const int box_w = screen_w - (margin_x * 2);
    const int subtitle_extra_h = has_subtitle ? max(3, tft->getSingleRowHeight()/2) : 0;
    const int box_h = constrain(
        (screen_h / 3) + tft->getSingleRowHeight() + spec.box_padding + subtitle_extra_h + spec.extra_height,
        spec.min_box_h, 
        max(spec.min_box_h, screen_h - 4)
    );
    const int anchor_y = pos.y + tft->getRowHeight() + 2;

    // Prefer drawing directly below the source row; if that overflows, flip above it.
    const int max_box_y = max(2, screen_h - box_h - 2);
    int box_y = anchor_y;
    if (box_y > max_box_y)
        //box_y = anchor_y - box_h - 2;
        box_y = screen_h - box_h - 2;
    box_y = constrain(box_y, 2, max_box_y);

    const int box_x = (screen_w - box_w) / 2;

    const int title_h = max(8, tft->getSingleRowHeight() + 4);
    const int subtitle_h = has_subtitle ? max(7, tft->getSingleRowHeight() + 1) : 0;
    const int subtitle_y = box_y + title_h + max(2, spec.subtitle_top_gap) + 2;
    const int hints_y = box_y + box_h - max(7, tft->getSingleRowHeight() + 2) - 1;
    const int value_top_y = has_subtitle ? (subtitle_y + subtitle_h) : (box_y + title_h + 2);
    const int value_center_y = (value_top_y + hints_y - spec.extra_height) / 2;

    tft->fillRect(box_x, box_y, box_w, box_h, BLACK);
    tft->drawRect(box_x, box_y, box_w, box_h, spec.frame_colour);
    tft->fillRect(box_x + 1, box_y + 1, box_w - 2, title_h, spec.frame_colour);

    menu_draw_centered_text(tft, spec.title, box_x + (box_w / 2), box_y + 1 + (title_h / 2), box_w - 6, spec.title_fg, spec.frame_colour, spec.max_title_text_size);
    if (has_subtitle) {
        menu_draw_centered_text(tft, spec.subtitle, box_x + (box_w / 2), subtitle_y + (subtitle_h / 2), box_w - 6, spec.subtitle_fg, BLACK, spec.max_subtitle_text_size);
    }

    menu_draw_centered_text(tft, spec.value, box_x + (box_w / 2), value_center_y, box_w - 10, spec.value_fg, BLACK, spec.max_value_text_size);

    if (spec.has_extra) {
        spec.draw_extra(tft, box_x + 4, tft->getCursorY() + 2, box_w-8, spec.extra_height);
    }

    tft->setTextSize(0);
    tft->setTextColor(spec.left_hint_fg, BLACK);
    tft->setCursor(box_x + 4, hints_y);
    tft->print(spec.left_hint!=nullptr ? spec.left_hint : "");

    const char *right_hint = spec.right_hint!=nullptr ? spec.right_hint : "";
    const int right_text_w = strlen(right_hint) * tft->currentCharacterWidth();
    tft->setTextColor(spec.right_hint_fg, BLACK);
    tft->setCursor((box_x + box_w - 4) - right_text_w, hints_y);
    tft->print(right_hint);

    tft->setCursor(0, box_y + box_h + 1);
    return tft->getCursorY();
}
