#ifndef SUBMENUTITEMBAR__INCLUDED
#define SUBMENUTITEMBAR__INCLUDED
#include "submenuitem.h"

// originally adapted from ParameterAmountControls
class SubMenuItemBar : public SubMenuItem {
    public:

    bool show_sub_headers = true;
    int_fast16_t cached_pixel_width_per_item = 0;
    int_fast16_t cached_screen_width = 0;
    int_fast16_t cached_column_count = 0;

    SubMenuItemBar(const char *label, bool show_sub_headers = true, bool show_header = true) : SubMenuItem(label, show_header) {
        this->show_sub_headers = show_sub_headers;
    }

    virtual int number_columns() {
        return this->items->size();
    }

    virtual bool allow_takeover() override {
        return false;
    }

    virtual int get_max_pixel_width(int item_number);
    virtual uint_fast16_t get_max_character_width(int item_number);

    virtual int display(Coord pos, bool selected, bool opened) override;
    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected);

    virtual bool needs_redraw(bool selected, bool opened) override {
        if (SubMenuItem::needs_redraw(selected, opened)) return true;

        // While a fullscreen-overlay child is open, only force a bar redraw when the
        // overlay's declared policy fires (e.g. OWN_INPUT for static selectors, TICK for
        // graphs). This prevents the bar from re-rendering every frame for static overlays,
        // which was the main cause of fps drop. The overlay itself is kept alive via
        // Menu::active_overlay_item, so it can still be drawn even when the bar skips.
        #if MENU_PERF_PARTIAL_UPDATES
            if (opened && this->currently_opened >= 0 && this->currently_opened < (int)this->items->size()) {
                MenuItem *overlay_item = this->items->get(this->currently_opened);
                if (overlay_item != nullptr && overlay_item->wants_fullscreen_overlay_when_opened_in_bar()) {
                    // Structural events (OPEN, PAGE_ENTER, INVALIDATE) always require the bar to run
                    // so active_overlay_item gets set on first open and after full-screen clears.
                    // The overlay's declared policy governs live-update redraws beyond that.
                    const MenuItem_RedrawPolicy structural =
                        REDRAW_ON_OPEN | REDRAW_ON_PAGE_ENTER | REDRAW_ON_INVALIDATE | REDRAW_ON_CLOSE;
                    const MenuItem_RedrawPolicy ovl_policy = overlay_item->get_overlay_redraw_policy() | structural;
                    if (this->pending_redraw_events & ovl_policy)
                        return true;
                    // Bar does not need to run — overlay is handled via active_overlay_item.
                    return false;
                }
            }
        #endif

        // Check children, so that we redraw if any of them need redraw
        for (auto* item : *this->items) {
            if (item->needs_redraw(
                item == this->items->get(this->currently_selected), 
                item == this->items->get(this->currently_opened)
            )) return true;
        }
        return false;
    }
    virtual void mark_rendered(bool selected, bool opened, int16_t draw_height) override {
        SubMenuItem::mark_rendered(selected, opened, draw_height);
        for (auto* item : *this->items) {
            item->mark_rendered(
                item == this->items->get(this->currently_selected), 
                item == this->items->get(this->currently_opened),
                draw_height // we need to pass something in so that the draw_height check doesn't force a render
            );
        }
    }
    virtual void post_event(MenuItem_RedrawPolicy event_bit) override {
        SubMenuItem::post_event(event_bit);
        for (auto* item : *this->items) {
            item->post_event(event_bit);
        }
    }

};

class SubMenuItemBarCustomProportions : public SubMenuItemBar {
    public:
    float *number_columns_proportions; // array of proportions for each column, should add up to 1.0
    int num_columns;
    SubMenuItemBarCustomProportions(const char *label, int num_columns, bool show_sub_headers = true, bool show_header = true) : SubMenuItemBar(label, show_sub_headers, show_header) {
        this->num_columns = num_columns;
        this->number_columns_proportions = new float[num_columns];
        for (int i = 0 ; i < num_columns ; i++) {
            this->number_columns_proportions[i] = 1.0 / num_columns;
        }
    }

    void set_column_proportion(uint8_t column_index, float proportion) {
        if (column_index < num_columns) {
            this->number_columns_proportions[column_index] = proportion;
            // invalidate cached pixel width so it gets recalculated with new proportions
            this->cached_pixel_width_per_item = 0;
            this->cached_screen_width = 0;
            this->cached_column_count = 0;
        }
    }

    virtual int get_max_pixel_width(int item_number) override {
        // todo: cache these or calculate at set time, to avoid a bunch of float maths every render loop
        return ((float)this->tft->width()) * this->number_columns_proportions[item_number]; 
    }
};


// todo: probably move the column functionality into SubMenuItemBar to save bytes on duplicated logic
class SubMenuItemColumns : public SubMenuItemBar {
    public:

    bool show_sub_headers = true;
    int columns = 1;

    SubMenuItemColumns(const char *label, int columns = 1, bool show_sub_headers = true, bool show_header = true) : SubMenuItemBar(label, show_sub_headers, show_header) {
        this->columns = columns;
    }

    virtual int number_columns() override {
        return this->columns;
    }

    virtual bool allow_takeover() override {
        return false;
    }

    /*virtual inline int get_max_pixel_width(int item_number) override {
        return this->tft->width() / columns; 
    }*/

    virtual int display(Coord pos, bool selected, bool opened) override;
};


class DualMenuItem : public SubMenuItemColumns {
    public:
        int item_1_width = 0;
        DualMenuItem(const char *label, bool show_sub_headers = true, bool show_header = true, int item_1_width = 0) : SubMenuItemColumns(label, 2, show_sub_headers, show_header) {
            this->item_1_width = item_1_width;
        }

        int get_max_pixel_width(int item_number) override {
            if (item_1_width==0) {
                if (this->cached_pixel_width_per_item==0 ||
                    this->cached_screen_width != (int_fast16_t)this->tft->width() ||
                    this->cached_column_count != (int_fast16_t)this->number_columns()) {
                    this->cached_pixel_width_per_item = this->tft->width() / this->number_columns();
                    this->cached_screen_width = this->tft->width();
                    this->cached_column_count = this->number_columns();
                }
                return this->cached_pixel_width_per_item - (item_number==number_columns()-1?3:0);
            } else {
                if (item_number==0) {
                    return item_1_width;
                } else {
                    return this->tft->width() - item_1_width;
                }
            }
        }
};

#endif