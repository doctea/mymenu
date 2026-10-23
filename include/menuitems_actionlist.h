#pragma once

// A menu item that shows a list of rows from an ActionListSource and lets the
// user act on them. Sources can push further sources (sub-menus, pickers) onto a
// small stack; back pops a level, and closes the item at the root.
//
// Closed: a compact preview of the root list (up to compact_rows rows).
// Opened: by default expands in place as a fixed-height scrolling window of
// open_rows rows, so the page doesn't reflow while navigating. With
// takeover=true it instead uses the whole screen below the message bar.

#include "menuitems.h"
#include "menuitems_actionlist_core.h"

class ActionListMenuItem : public MenuItem {
    public:
        ActionListMenuItem(const char *label, ActionListSource *root, uint8_t compact_rows = 4,
                           uint8_t open_rows = 8, bool takeover = false)
            : MenuItem(label), root_source(root), compact_rows(compact_rows),
              open_rows(open_rows > 0 ? open_rows : 1), takeover(takeover) {
            IF_MENU_PERF_PARTIAL_UPDATES(add_redraw_policy(REDRAW_ON_CUSTOM);)
        }

        void set_root(ActionListSource *root) {
            root_source = root;
            navigator.reset(nullptr);
            IF_MENU_PERF_PARTIAL_UPDATES(request_redraw();)
        }

        ActionListNavigator &get_navigator() { return navigator; }

        virtual bool allow_takeover() override { return takeover; }

        virtual bool action_opened() override {
            if (root_source == nullptr) return false;
            navigator.reset(root_source);
            return true;
        }

        virtual bool knob_left() override {
            navigator.move(-1);
            return true;
        }

        virtual bool knob_right() override {
            navigator.move(1);
            return true;
        }

        virtual bool button_select() override {
            if (navigator.get_depth() == 0) return true;
            if (navigator.select()) {
                // collapse so the menu's follow-up button_back() closes the item
                navigator.reset(root_source);
                return true;
            }
            return false;
        }

        virtual bool button_right() override {
            if (navigator.get_depth() == 0) return false;
            navigator.right();
            return true;
        }

        virtual bool button_back() override {
            return navigator.back();
        }

        #ifdef MENU_PERF_PARTIAL_UPDATES
        virtual bool check_needs_redraw_custom(bool selected, bool opened) override {
            const uint32_t signature = content_signature(opened);
            if (signature == last_signature) return false;
            last_signature = signature;
            return true;
        }
        #endif

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setTextSize(1);
            if (opened && navigator.get_depth() > 0)
                return display_opened(pos, selected);
            return display_compact(pos, selected, opened);
        }

    protected:
        ActionListSource *root_source = nullptr;
        ActionListNavigator navigator;
        uint8_t compact_rows = 4;
        uint8_t open_rows = 8;
        bool takeover = false;
        uint32_t last_signature = 0;

        uint16_t default_colour() const { return C_WHITE; }

        uint8_t text_columns() const {
            int columns = tft->characterWidth() > 0 ? tft->width() / tft->characterWidth() : MENU_C_MAX;
            if (columns > MENU_C_MAX) columns = MENU_C_MAX;
            if (columns < 4) columns = 4;
            return (uint8_t)columns;
        }

        virtual uint32_t content_signature(bool opened) {
            ActionListSource *source = (opened && navigator.get_depth() > 0) ? navigator.current() : root_source;
            if (source == nullptr) return 0;
            uint32_t signature = source->generation();
            if (opened) {
                signature = signature * 31u + source->count();
                signature = signature * 31u + navigator.get_depth();
                signature = signature * 31u + (uint16_t)navigator.cursor();
                signature = signature * 31u + navigator.scroll();
            }
            return signature;
        }

        uint16_t colour_for_row(ActionListSource *source, uint8_t index) {
            uint16_t colour = source->row_colour(index);
            if (colour == 0) colour = default_colour();
            if (!source->row_enabled(index) || source->row_dimmed(index)) colour = tft->halfbright_565(colour);
            return colour;
        }

        // vertical gap between rows, matching other list widgets (eg ObjectMultiToggle)
        static const int ROW_GAP = 2;

        // Row layout tracks its own y rather than reading the cursor back: some displays
        // (eg ILI9341_t3n) clamp setCursor() to the screen, so an item running off the
        // bottom would report a too-short height and the menu would never scroll to it.
        int line_y = 0;
        int line_advance = 8;

        void begin_lines(int y) { line_y = y; }
        int end_lines() const { return line_y; }

        // Prints text without auto-wrap, then moves to the next line plus ROW_GAP.
        // Auto-wrap at the right edge advances the cursor by less than a full row
        // on some displays, which made full-width rows overlap. Rows below the
        // screen aren't drawn, but still count towards the item's height.
        void print_line(const char *text, uint8_t columns, int x = 0) {
            if (line_y < tft->height()) {
                tft->setCursor(x, line_y);
                const bool was_wrap = tft->isTextWrap();
                tft->setTextWrap(false);
                tft->printf("%-*.*s", columns, columns, text);
                tft->setTextWrap(was_wrap);
                tft->println();
                const int advanced = tft->getCursorY() - line_y;
                if (advanced > 0) line_advance = advanced;
            }
            line_y += line_advance + ROW_GAP;
            if (line_y < tft->height()) tft->setCursor(x, line_y);
        }

        int row_pitch() {
            const int row_height = tft->getRowHeight() > 0 ? tft->getRowHeight() : 8;
            return row_height + ROW_GAP;
        }

        // prints one row padded/truncated to the display width
        void print_row(ActionListSource *source, uint8_t index, bool highlighted, uint8_t columns, int x = 0) {
            char text[MENU_C_MAX + 1];
            text[0] = '\0';
            source->row_text(index, text, sizeof(text));
            colours(highlighted, colour_for_row(source, index), BLACK);
            print_line(text, columns, x);
        }

        virtual int display_compact(Coord pos, bool selected, bool opened) {
            tft->setCursor(pos.x, pos.y);
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x, pos.y);
            begin_lines(pos.y);
            if (root_source == nullptr) return end_lines();

            const uint8_t columns = text_columns();
            const uint8_t count = root_source->count();
            const uint8_t shown = count < compact_rows ? count : compact_rows;
            for (uint8_t i = 0; i < shown; ++i)
                print_row(root_source, i, false, columns, pos.x);

            colours(false, tft->halfbright_565(default_colour()), BLACK);
            if (count == 0) {
                print_line("(none)", columns, pos.x);
            } else if (count > shown) {
                char more[16];
                snprintf(more, sizeof(more), "+%u more", (unsigned)(count - shown));
                print_line(more, columns, pos.x);
            }
            colours(false, default_colour(), BLACK);
            return end_lines();
        }

        int display_opened(Coord pos, bool selected) {
            ActionListSource *source = navigator.current();
            const uint8_t columns = text_columns();

            // the open header is prefixed with ">>>" and pads but doesn't truncate,
            // so clip long titles to keep the header on one line
            char title[MENU_C_MAX + 1];
            const char *raw_title = source->title();
            snprintf(title, sizeof(title), "%.*s", columns > 3 ? columns - 3 : 1,
                raw_title != nullptr ? raw_title : label);
            tft->setCursor(pos.x, pos.y);
            pos.y = header(title, pos, selected, true);
            tft->setCursor(pos.x, pos.y);
            begin_lines(pos.y);

            int available = open_rows;
            if (takeover) {
                available = (tft->height() - pos.y) / row_pitch() - 1;  // leave room for the hint
                if (available < 1) available = 1;
            }

            navigator.ensure_visible((uint8_t)available);
            const uint8_t count = source->count();
            const uint8_t first = navigator.scroll();
            const int16_t cursor = navigator.cursor();

            int printed = 0;
            if (count == 0) {
                colours(false, tft->halfbright_565(default_colour()), BLACK);
                print_line("(none)", columns, pos.x);
                printed++;
            }
            for (uint8_t i = first; i < count && i < first + available; ++i, ++printed)
                print_row(source, i, i == cursor, columns, pos.x);

            // pad inline windows to a constant height so the page below doesn't jump
            colours(false, default_colour(), BLACK);
            if (!takeover)
                for (; printed < available; ++printed)
                    print_line("", columns, pos.x);

            colours(false, tft->halfbright_565(default_colour()), BLACK);
            const bool more_above = first > 0;
            const bool more_below = first + available < count;
            const char *hint = source->hint();
            char hint_line[MENU_C_MAX + 1];
            snprintf(hint_line, sizeof(hint_line), "%s%s%s%s[back]=%s",
                more_above ? "^ " : "",
                more_below ? "v " : "",
                hint != nullptr ? hint : "",
                hint != nullptr ? " " : "",
                navigator.get_depth() > 1 ? "up" : "close");
            print_line(hint_line, columns, pos.x);
            colours(false, default_colour(), BLACK);
            return end_lines();
        }
};
