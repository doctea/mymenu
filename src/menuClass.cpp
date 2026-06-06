#ifdef ENABLE_SCREEN

#include "menu.h"
#include "menuitems.h"

#include <LinkedList.h>
#include <profiling.h>

int freeRam();
#ifdef ARDUINO_TEENSY41
    int freeExtRam();
#endif

#include <string>

bool debug_flag = false;

#define MAX_KNOB 1024

#ifndef CORE_TEENSY
    // if no FLASHMEM then we're probably not running on Teensy platform, so define it empty
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif
    //#define F(x) { x }
#endif


int Menu::display_pinned() {
    int y = tft->getCursorY();
    if (profile_enable) {
        tft->setTextSize(tft->default_textsize);
        const int row_h = tft->getRowHeight();
        const int profile_h = row_h + 3;
        tft->fillRect(0, y, tft->width(), profile_h, BLACK);
        tft->setCursor(0, y);
        Serial.println(profile_string);
        bool was_wrap = tft->isTextWrap();
        tft->setTextWrap(false);
        tft->printf(tft->get_message_format(), profile_string);
        tft->setTextWrap(was_wrap);

        int target_y = tft->getCursorY();
        if (target_y < y + row_h)
            target_y = y + row_h;
        y = target_y + 3;
    }

    if (pinned_panel!=nullptr) {
        #if MENU_PERF_PARTIAL_UPDATES
            pinned_panel->refresh_redraw_state();
            if (pinned_panel->should_redraw()) {
                //if (debug) { Serial.println("display()=> doing pinned_panel display"); Serial_flush(); }
                const int new_y = pinned_panel->display(Coord(0,y), false, false);
                pinned_panel->mark_drawn(new_y - y);
                y = new_y;
            } else {
                y += pinned_panel->get_cached_draw_height();
            }
        #else
            y = pinned_panel->display(Coord(0,y), false, false);
        #endif
    }

    return y;
}

// draw the menu display
int Menu::display() {
    PROFILE_SLOT(p_menu_display, "menu display()");
    PROFILE_SET_SPIKE_THRESHOLD(p_menu_display, 18000);
    PROFILE_SET_SPIKE_MODULO(p_menu_display, 96);  // last known tick at time of spike
    PROFILE_SCOPE(p_menu_display);
    bool debug = this->debug;

    // Reset deferred overlay request for this frame.
    this->pending_overlay_item = nullptr;
    this->pending_overlay_y = 0;

    // early return if display isn't ready for writing (mostly used for dma checks)
    if (!this->tft->ready()) {
	    return 0;
    }

    uint32_t display_started = 0;
    static int frames_drawn = 0;
    static int frames_in_last_second = 0;
    static uint32_t current_second = 0, last_second = 0;

    if (profile_enable) {
        display_started = micros();
        current_second = micros()/(1000*1000);
        if (current_second!=last_second) {
            frames_in_last_second = frames_drawn;
            frames_drawn = 0;
            last_second = current_second;
        }
    }

    //Serial.printf("display !");
    int y = 0;
    
    if (debug) { Serial.println("display()=> about to setTextColor()"); Serial_flush(); }
    tft->setTextColor(C_WHITE, BLACK);

    int currently_opened = selected_page->currently_opened;
    int currently_selected = selected_page->currently_selected;
    //if (debug) { Serial.printf("display()=> about to get the selected_page items.. "); Serial_flush(); }
    MenuItemList *items = selected_page->items;
    //if (debug) { Serial.printf("currently_opened = %i/%i, currently_selected = %i/%i..\n", currently_opened+1, selected_page->items->size(), currently_selected+1, selected_page->items->size()); Serial_flush(); }

    #if MENU_PERF_PARTIAL_UPDATES
        static page_t *last_selected_page = nullptr;
        static int last_selected_page_index_rendered = -1;
        static int last_opened_page_index_rendered = -2;
        static int last_currently_selected = -2;
        static int last_currently_opened = -2;
        const bool page_changed = (last_selected_page != selected_page);
        const bool page_index_changed = (last_selected_page_index_rendered != selected_page_index);
        const bool opened_page_changed = (last_opened_page_index_rendered != opened_page_index);
        const bool selection_changed = (last_currently_selected != currently_selected);
        const bool opened_changed = (last_currently_opened != currently_opened);
        last_selected_page = selected_page;
        last_selected_page_index_rendered = selected_page_index;
        last_opened_page_index_rendered = opened_page_index;

        const bool is_takeover = is_item_opened() && items->get(currently_opened)->allow_takeover();
        // opened_changed is included because an item changing height (open/close) shifts all items
        // below it — those items must repaint at their new Y positions even if their content is stale.
        const bool requires_full_clear = is_takeover || mode==DISPLAY_ONE || page_changed || page_index_changed || opened_page_changed || opened_changed || recalculate_bottoms;
        if (requires_full_clear) {
            tft->clear();
            mark_message_dirty();
            if (pinned_panel!=nullptr) {
                pinned_panel->request_redraw();
            }
            // Post PAGE_ENTER to all items on the incoming page
            for (auto* item : *items) {
                item->post_event(REDRAW_ON_PAGE_ENTER);
            }
        } else {
            // Post selection/deselection/open/close events to affected items
            if (selection_changed) {
                if (last_currently_selected >= 0 && last_currently_selected < (int)items->size())
                    items->get(last_currently_selected)->post_event(REDRAW_ON_DESELECTION);
                if (currently_selected >= 0 && currently_selected < (int)items->size())
                    items->get(currently_selected)->post_event(REDRAW_ON_SELECTION);
            }
            if (opened_changed) {
                if (last_currently_opened >= 0 && last_currently_opened < (int)items->size())
                    items->get(last_currently_opened)->post_event(REDRAW_ON_CLOSE);
                if (currently_opened >= 0 && currently_opened < (int)items->size())
                    items->get(currently_opened)->post_event(REDRAW_ON_OPEN);
            }
        }
        last_currently_selected = currently_selected;
        last_currently_opened = currently_opened;

        // Drain any pending timing/transport events into the current page's items.
        // These are accumulated by on_step() / on_beat() / on_bar() / on_bpm_clock_change()
        // between display() calls and delivered here so only visible items receive them.
        if (pending_page_events != REDRAW_NEVER) {
            for (auto* item : *items)
                item->post_event(pending_page_events);
            pending_page_events = REDRAW_NEVER;
        }

        // Initialize dirty region tracking for selective Y-range DMA push
        tft->reset_dirty_region();
        // After a full clear the entire framebuffer is black; seed dirty region so
        // everything gets pushed once, including sections that skip their own redraw check.
        if (requires_full_clear) {
            tft->set_dirty_region(0, tft->height());
        }
    #else
        tft->clear();
        tft->reset_dirty_region();
    #endif

    // now draw the menu
    //if (debug) { Serial.println("display()=> about to check display mode and branch.."); Serial_flush(); }
    if (is_item_opened() && items->get(currently_opened)->allow_takeover()) {
        //if (debug) { Serial.println("display()=> takeover branch"); Serial_flush(); }
        // takeover -- focus on this menu item only
        tft->setCursor(0, y);

        y = display_pinned();
        y = draw_message();
        // let the currently opened item take care of drawing all of the display
        //Serial.println("allow_takeover!");
        //if (debug) { Serial.printf("display()=> doing display of item %i\n", currently_opened); Serial_flush(); }
        //if (debug) { Serial.printf("display()=> got item to display '%s'\n", items->get(currently_opened)->label); Serial_flush(); }
        items->get(currently_opened)->display(Coord(0,y), true, true);
        //if (debug) { Serial.println("display()=> finished display\n"); Serial_flush(); }
        
        // Mark entire screen as dirty for takeover mode
        tft->set_dirty_region(0, tft->height());
    } else if (mode==DISPLAY_ONE) {
        //if (debug) { Serial.println("display()=> DISPLAY_ONE branch"); Serial_flush(); }
        // for screens that can only display one option at a time
        // static int last_displayed = 0;
        // if (last_displayed!=currently_selected)
        //     tft->clear();
        tft->setCursor(0, y);

        y = display_pinned();

        tft->setCursor(0, y);
        
        //if (debug) { Serial.println("display()=> about to draw_message()"); Serial_flush(); }
        y = draw_message();

        if (currently_selected>=0 && currently_selected < (int)items->size()) {
            tft->setTextSize(0);
            items->get(currently_selected)->display(Coord(0,y), true, currently_opened==currently_selected);
            // last_displayed = currently_selected;
        }

        tft->setTextColor(C_WHITE, BLACK);
        while(tft->getCursorY() < tft->height())
            tft->println("");
        tft->println("");
        
        // Mark entire screen as dirty for DISPLAY_ONE mode
        tft->set_dirty_region(0, tft->height());

    } else {
        //if (debug) { Serial.println("display()=> default branch"); Serial_flush(); }
        //static bool first_display = true;
        //static int *panel_bottom = nullptr;
        bool bottoms_computed = false;
        if (selected_page->panel_bottom == nullptr) {
            selected_page->panel_bottom = (int*)CALLOC_FUNC(this->get_num_panels(), sizeof(int));
        } else {
            if (!this->recalculate_bottoms)
                bottoms_computed = true;
            else
                this->recalculate_bottoms = false;
        }

        int *panel_bottom = selected_page->panel_bottom;
        if (panel_bottom == nullptr) {
            return 0;
        }

        // find number of panels to offset in order to ensure that selected panel is on screen?
        int start_panel = 0;
        if (bottoms_computed && this->selected_page->scrollable && currently_selected >= 0 && currently_selected < (int)items->size() && panel_bottom[currently_selected] >= screen_height_cutoff) {
            start_panel = currently_selected - 1;
            //#ifdef OLD_SCROLL_METHOD
            // count backwards to find number of panels we have to go to fit currently_selected on screen...
            int count_y = panel_bottom[currently_selected];
            for (unsigned int i = currently_selected ; i > 0 ; i--) {
                count_y -= panel_bottom[i];
                if (count_y + panel_bottom[currently_selected] < tft->height() / 2) {
                    start_panel = i;
                    break;
                }
            }
            //#endif
            #ifdef NEW_SCROLL_METHOD_NONWORKING
            // count forward until we find the first item we can start on that will include the item on screen
            int target_y = panel_bottom[currently_selected];
            int adj_y = 0;
            for (unsigned int i = 0 ; i < items.size() ; i++) {
                adj_y += panel_bottom[i];
                Debug_printf(F("item %i accumulated height %i trying to fit into %i\n"), i, adj_y, tft->height());
                if (target_y - adj_y < tft->height()) {
                    Serial.println(F("\tyes!"));
                    start_panel = i;
                    break;
                }                
            }
            #endif

            //tft.fillWindow(ST77XX_BLACK);
            // tft->clear();
            //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
            start_panel = constrain(start_panel, 0, (int)items->size()-1);
        } else {
            start_panel = 0;
            // tft->clear();
            //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
        }
        //if (panel_bottom[currently_selected] >= tft->height()/2)
        //    start_panel = currently_selected - 1;

        #ifndef ALT_MENU_POSITIONING
            if(this->selected_page->scrollable)
                start_panel = constrain(start_panel-2, 0, (int)items->size()-1);
            // ^^^ whether this is useful appears to depend on what menus we're on... disabled it for better behaviour on the ParameterInput pages on Microlidian
            // TODO: fix this behaviour once and for all
        #endif

        // static int last_start_panel = -1;
        // if (last_start_panel!=start_panel) {
        //     tft->clear(true);
        // }
        // last_start_panel = start_panel;

        tft->setCursor(0,y);

        int pinned_start_y = y;
        #if MENU_PERF_PARTIAL_UPDATES
            // Peek at should_redraw() AFTER refresh so we know if display_pinned() will actually re-render
            if (pinned_panel != nullptr) pinned_panel->refresh_redraw_state();
            const bool pinned_did_redraw = (pinned_panel == nullptr) || pinned_panel->should_redraw();
        #else
            const bool pinned_did_redraw = true;
        #endif
        y = display_pinned();
        // Only mark dirty if pinned panel actually re-rendered
        if (pinned_did_redraw && y > pinned_start_y) {
            tft->set_dirty_region(pinned_start_y, y);
        }
        tft->setCursor(0, y);
        
        //if (debug) { Debug_println(F("display()=> about to draw_message()")); Serial_flush(); }
        int msg_start_y = y;
        #if MENU_PERF_PARTIAL_UPDATES
            const bool msg_did_redraw = should_redraw_message_row();
        #else
            const bool msg_did_redraw = true;
        #endif
        int new_y = draw_message();
        if (new_y==y)
            tft->println(); // force dropping a line if draw_message hasn't for some reason (needed to get the tabs line to show on bodmer/rp2040?)
        y = tft->getCursorY();
        // Only mark dirty if message row actually re-rendered
        if (msg_did_redraw && y > msg_start_y) {
            tft->set_dirty_region(msg_start_y, y);
        }
        //tft->setCursor(0, new_y);

        tft->setTextSize(tab_textsize);

        const int tft_width = tft->width();
        const int current_character_width = tft->currentCharacterWidth();
        
        // Track tabs section for dirty region
        int tabs_start_y = y;

        /////// draw tabs for the pages
        // NOTE: left as index-based loop — starts at selected_page_index and wraps with modulo,
        // so requires random-access get(ci) to compute the wrapped display index each iteration.
        for (unsigned int i = selected_page_index ; i < pages->size() + selected_page_index ; i++) {
            int ci = i;
            if (ci >= (int)pages->size())        // wrap around to start of list if we get to the end
                ci = ci % pages->size();

            page_t *page = pages->get(ci);
            if (page==nullptr) {
                Serial.printf("ERROR: page %i is nullptr!\n", ci);
                continue;
            }
            if (page->title==nullptr) {
                Serial.printf("ERROR: page %i has nullptr title!\n", ci);
                continue;
            }

            if ((int)i==selected_page_index && !is_page_opened())
                tft->setTextColor(BLACK, page->colour);
            else
                tft->setTextColor(page->colour, BLACK);

            int cursor_x = tft->getCursorX();
            //int characters_left = ((tft->width() - tft->getCursorX()) / tft->currentCharacterWidth()) - 1; // + (tft->characterWidth() * (strlen(pages->get(ci)->title)+1)));
            int characters_left = ((tft_width - cursor_x) / current_character_width) - 1;
            if (characters_left<2) {
                break;
            } else if (characters_left < (int)page->title_len + 1) {
                char title[MENU_C_MAX];
                strncpy(title, page->title, characters_left);
                title[characters_left] = '\0';
                title[characters_left-2] = '.';
                title[characters_left-1] = '.';
                //title[characters_left] = '\0';
                tft->print(title);
                break;  // break if we'd go off the screen rendering this item TODO: draw the available characters
            }

            tft->print(page->title);
            tft->setTextColor(tft->rgb(196,196,196), BLACK);
            tft->print("|");
            tft->setTextColor(C_WHITE, BLACK);
        }
        if (y == tft->getCursorY()) {
            // we havent had enough tabs to move down a line, so force one
            tft->println();
        }
        y = tft->getCursorY();
        
        // Mark tabs dirty only if page selection changed (tabs text changed)
        #if MENU_PERF_PARTIAL_UPDATES
            if (page_index_changed || opened_page_changed) {
                tft->set_dirty_region(tabs_start_y, y);
            }
        #else
            tft->set_dirty_region(tabs_start_y, y);
        #endif
        
        tft->setTextSize(0);
        /// finished drawing tabs

        // draw pinned page header (column labels etc) if set
        int header_start_y = y;
        if (selected_page->header_text != nullptr) {
            tft->setTextColor(C_WHITE, BLACK);
            tft->setTextSize(selected_page->header_text_size);
            tft->setCursor(0, y);
            bool wasTextWrap = tft->isTextWrap();
            tft->setTextWrap(false);
            tft->println(selected_page->header_text);
            tft->setTextWrap(wasTextWrap);
            y = tft->getCursorY();
        } else {
            // Keep a consistent visual boundary before item rows on pages that
            // do not render a dedicated page header line.
            tft->drawLine(0, y, tft->width(), y, C_WHITE);
            y += 2;
        }
        
        // Mark header dirty only if page changed
        #if MENU_PERF_PARTIAL_UPDATES
            if (page_changed || page_index_changed) {
                tft->set_dirty_region(header_start_y, y);
            }
        #else
            tft->set_dirty_region(header_start_y, y);
        #endif

        const int list_start_y = y;
        #if !MENU_PERF_PARTIAL_UPDATES
            // Without selective redraw, blank the whole list area up front
            const int list_h = tft->height() - list_start_y;
            if (list_h > 0) {
                tft->fillRect(0, list_start_y, tft->width(), list_h, BLACK);
            }
        #endif

        // When the scroll position changes, items render at different Y coordinates than last frame.
        // Clear the list area and force all items to repaint so no stale content is left behind.
        #if MENU_PERF_PARTIAL_UPDATES
        {
            static int last_start_panel_rendered = -1;
            if (start_panel != last_start_panel_rendered) {
                const int clear_h = tft->height() - list_start_y;
                if (clear_h > 0)
                    tft->fillRect(0, list_start_y, tft->width(), clear_h, BLACK);
                tft->set_dirty_region(list_start_y, tft->height());
                for (auto* item : *items)
                    item->request_redraw();
                last_start_panel_rendered = start_panel;
            }
        }
        #endif

        #if MENU_PERF_PARTIAL_UPDATES
        if (this->prev_overlay_item_tracking != nullptr) {
            // An overlay was active last frame. First, black-fill the entire overlay region
            // so that stale overlay pixels (graph fragments, box borders, gaps between items)
            // are gone before anything renders. Items then repaint cleanly on top of the black.
            // This is done in the same pass as the normal item renders so only one frame is sent.
            const int prev_fill_h = tft->height() - this->prev_overlay_y_tracking;
            if (prev_fill_h > 0) {
                tft->fillRect(0, this->prev_overlay_y_tracking, tft->width(), prev_fill_h, BLACK);
                tft->set_dirty_region(this->prev_overlay_y_tracking, tft->height());
            }
            // Force all items to redraw so their pixels are repainted over the now-black region.
            // Covers both cases with no blank-frame transition:
            //   • Overlay still open:  items render under the live overlay as normal.
            //   • Overlay just closed: items cover the stale overlay pixels immediately.
            for (auto* item : *items)
                item->request_redraw();
        }
        #endif

        // draw each menu item
        //int start_y = 0;
        //if (debug) { Serial.println("display()=> about to start drawing the items.."); Serial_flush(); }
        // const uint_fast16_t size = items->size();
        auto it = items->begin();
        for (uint_fast16_t s = 0; s < start_panel && it != items->end(); ++s, ++it) {}
        
        int list_bottom_y = list_start_y; // track where list actually ends
        
        for (uint_fast16_t i = start_panel; it != items->end(); ++it, ++i) {
            //if (debug) { Serial.printf("display()=> about to get item %i\n", i); Serial_flush(); }
            MenuItem *item = *it;

            //int time = millis();
            //Serial.printf("Menu rendering item %i [selected #%i, opened #%i]\n", i, currently_selected, currently_opened);
            Coord pos = Coord(0,y);
            //if (debug) { Serial.printf("display()=> about to display() item %i aka %s\n", i, item->label); Serial_flush(); }

            unsigned long time_micros = 0;
            if (this->debug_times) time_micros = micros();
            tft->setTextSize(0);

            const bool item_selected = ((int)i==currently_selected);
            const bool item_opened = ((int)i==currently_opened);
            int item_start_y = y;

            #if MENU_PERF_PARTIAL_UPDATES
                if (item->needs_redraw(item_selected, item_opened)) {
                    // Blank this item's area. For opened items use full remaining height
                    // to prevent ghost pixels from the previous (shorter) closed state.
                    const int16_t cached_h = item->get_cached_draw_height();
                    // cached_h == -1: first render, erase a default amount
                    // cached_h ==  0: zero-height item, erase nothing (avoids clobbering adjacent items)
                    // cached_h  > 0: normal item, erase its previous footprint
                    //
                    // Only use full-screen-height erase for genuine takeover items (e.g. SubMenuItem
                    // opened as a full page). Bars and inline selectors do NOT allow_takeover() and
                    // must only erase their own row; over-erasing blanks items below and wipes the
                    // screen region that overlays rely on already having content in it.
                    const int erase_h = (item_opened && item->allow_takeover())
                        ? (tft->height() - item_start_y)
                        : (cached_h > 0 ? cached_h : (cached_h < 0 ? tft->getRowHeight() * 2 : 0));
                    if (erase_h > 0)
                        tft->fillRect(0, item_start_y, tft->width(), erase_h, BLACK);
                    
                    y = item->display(pos, item_selected, item_opened) + 1;
                    item->mark_rendered(item_selected, item_opened, y - item_start_y - 1);
                    tft->set_dirty_region(item_start_y, y);
                } else {
                    // Skip render, advance y by cached height
                    const int16_t cached_h = item->get_cached_draw_height();
                    if (cached_h >= 0) {
                        // cached_h == 0: zero-height item, advance y by just the 1-pixel spacing
                        y = item_start_y + cached_h + 1;
                    } else {
                        // cached_h == -1: never rendered yet, must render now
                        tft->fillRect(0, item_start_y, tft->width(), tft->getRowHeight() * 2, BLACK);
                        y = item->display(pos, item_selected, item_opened) + 1;
                        item->mark_rendered(item_selected, item_opened, y - item_start_y - 1);
                        tft->set_dirty_region(item_start_y, y);
                    }
                }
            #else
                y = item->display(pos, item_selected, item_opened) + 1;
                tft->set_dirty_region(item_start_y, y);
            #endif

            //Serial.printf("after rendering MenuItem %i, return y is %i, cursor coords are (%i,%i)\n", y, tft->getCursorX(), tft->getCursorY());
            //if (debug) { Serial.printf("display()=> just did display() item %i aka %s\n", i, item->label); Serial_flush(); }

            if (this->debug_times) {
                tft->setTextSize(1);
                tft->printf("Took: %lu\n", micros() - time_micros);
                y = y + tft->getRowHeight();
            }

            list_bottom_y = y;

            // panel_bottom stores absolute cumulative bottoms from a full top-to-bottom
            // pass. During partial viewport renders (bottoms_computed=true), y is
            // viewport-local and would corrupt the cache, causing scroll snap-back.
            if (!bottoms_computed) {
                panel_bottom[i] = y;
            }

            if (bottoms_computed && y >= this->tft->height())
                break;
        }
        
        #if MENU_PERF_PARTIAL_UPDATES
            // Clear and dirty blank space only when list height actually changes.
            static int last_list_bottom_y = -1;
            if (list_bottom_y != last_list_bottom_y) {
                if (list_bottom_y < tft->height()) {
                    tft->fillRect(0, list_bottom_y, tft->width(), tft->height() - list_bottom_y, BLACK);
                    tft->set_dirty_region(list_bottom_y, tft->height());
                }
                last_list_bottom_y = list_bottom_y;
            }
        #else
            if (y < tft->height()) {
                tft->set_dirty_region(y, tft->height());
            }
        #endif
        
        bottoms_computed = true;
        
        // control debug output (knob positions / button presses)
        /*if (y < tft->height()) {
            tft->setCursor(0, y);
            tft->setTextColor(C_WHITE, BLACK);
            //tft->println();
            tft->setTextSize(2);
            //tft->printf("K:%2i B:%2i\n", last_knob_position, button_count);
            //tft->printf("S:%2i O:%2i\n", currently_selected, currently_opened);
            tft->printf("Mem: %i\n"), freeRam();
        }*/
        if (debug) { Debug_println("Done in main draw part"); Serial_flush(); }
    }

    // Draw deferred overlay after the page has fully rendered so it appears on top.
    if (this->pending_overlay_item!=nullptr) {
        this->pending_overlay_item->display(Coord(0, this->pending_overlay_y), true, true);
        // The overlay paints on top of the already-rendered list, potentially covering
        // a region taller than the base item's cached height. Mark from the overlay
        // start to the bottom of the screen dirty so the DMA push includes it all.
        tft->set_dirty_region(this->pending_overlay_y, tft->height());
    }

    // Update overlay tracking for next frame (read at the START of the next display() call,
    // before the items loop, so items can be pre-dirtied when the overlay closes).
    this->prev_overlay_item_tracking = this->pending_overlay_item;
    this->prev_overlay_y_tracking    = this->pending_overlay_y;

    //tft->updateScreenAsync(false);
    if (debug) { Debug_println("display()=> about to tft->updateDisplay()"); Serial_flush(); }
    if (auto_update)
        this->updateDisplay();

    if (profile_enable) {
        /*char * ptr = (char*)extmem_malloc(1024 << 10);
        if (!ptr)
          Serial.println("failed to allocate from extmem!");
        else
          sprintf(ptr, "hello");*/
        #ifdef ARDUINO_TEENSY41
            snprintf(
                profile_string, 
                MENU_C_MAX, 
                "%-2ifps | RAM2 %uK EXT %uK free", 
                frames_in_last_second,
                freeRam()/1024,
                freeExtRam()/1024
            );
        #else
            snprintf(
                profile_string, 
                MENU_C_MAX, 
                "%-2ifps | RAM2 %uK free", 
                frames_in_last_second,
                freeRam()/1024
            );
        #endif
        frames_drawn++;
    }

    if (send_frame) {
        bool should_send = true;
        #ifdef ENABLE_REMOTE_VIEWER
            if (send_frame_live) {
                static uint32_t last_live_frame_sent_at = 0;
                #ifndef REMOTE_VIEWER_LIVE_MAX_FPS
                    #define REMOTE_VIEWER_LIVE_MAX_FPS 8
                #endif
                const uint32_t max_fps = (REMOTE_VIEWER_LIVE_MAX_FPS > 0) ? (uint32_t)REMOTE_VIEWER_LIVE_MAX_FPS : 1u;
                const uint32_t min_interval_ms = 1000u / max_fps;
                uint32_t now_ms = millis();
                if (now_ms - last_live_frame_sent_at < min_interval_ms) {
                    should_send = false;
                } else {
                    last_live_frame_sent_at = now_ms;
                }
            }
        #endif

        if (should_send) {
            this->tft->push_framebuffer_serial();
            if(!send_frame_live) {
                send_frame = false;
            }
        }
    }

    return y;
}


#include "menuitems_quickpage.h"

void Menu::setup_quickjump() {
    all_page_index = add_page("All Pages Index");
    menu->add(new AllPagesIndexMenuItem("All Pages Index"));
    menu->remember_opened_page(-1, true);  // add the All Pages Index page to the all pages index by default

    quick_page_index = add_page("QuickJump");
    menu->add(new QuickPagesMenuItem("QuickJump history"));
}


void menu_set_last_message(const char *msg, int colour) {
    menu->set_last_message(msg);
    menu->set_message_colour(colour);
}

#endif