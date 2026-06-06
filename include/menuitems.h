#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include <Arduino.h>

//#include "menu.h"
#include "display_abstract.h"
#include "colours.h"

#define MAX_LABEL_LENGTH 40

extern const char *fired_message;
extern const char *sure_message;
extern const char *set_message;
extern const char *label_on;
extern const char *label_off;

// Feature flag: enable partial-update rendering optimisations (selective item redraw,
// dirty-region DMA push). Set to 1 in your build flags to enable.
// #ifndef MENU_PERF_PARTIAL_UPDATES
//     #define MENU_PERF_PARTIAL_UPDATES 0
// #endif

#if MENU_PERF_PARTIAL_UPDATES
    // Wrap optional calls so call sites stay readable while methods only exist in partial-update builds.
    #define IF_MENU_PERF_PARTIAL_UPDATES(code) code
#else
    #define IF_MENU_PERF_PARTIAL_UPDATES(code)
#endif

// ---------------------------------------------------------------------------
// MenuItem redraw policy — bitmask of events that should trigger a redraw.
//
// Items carry a `pending_redraw_events` bitmask. The menu posts event bits
// to items at the appropriate callsites (page switch, selection change, tick,
// input, etc.). needs_redraw() returns true if any pending bit matches the
// item's policy. mark_rendered() clears the pending bits.
//
// Bit assignments (uint16_t):
typedef uint16_t MenuItem_RedrawPolicy;

// Individual event bits -------------------------------------------------------
static const MenuItem_RedrawPolicy REDRAW_ON_TICK         = 1 << 0;  ///< Any clock tick (update_ticks)
static const MenuItem_RedrawPolicy REDRAW_ON_SELECTION    = 1 << 1;  ///< Cursor moved onto this item
static const MenuItem_RedrawPolicy REDRAW_ON_DESELECTION  = 1 << 2;  ///< Cursor moved off this item
static const MenuItem_RedrawPolicy REDRAW_ON_OPEN         = 1 << 3;  ///< Item was opened
static const MenuItem_RedrawPolicy REDRAW_ON_CLOSE        = 1 << 4;  ///< Item was closed
static const MenuItem_RedrawPolicy REDRAW_ON_OWN_INPUT    = 1 << 5;  ///< Knob/button event targeted at this item (selected/opened)
static const MenuItem_RedrawPolicy REDRAW_ON_ANY_INPUT    = 1 << 6;  ///< Any input on the current page (global)
static const MenuItem_RedrawPolicy REDRAW_ON_PAGE_ENTER   = 1 << 7;  ///< Page containing this item became active
static const MenuItem_RedrawPolicy REDRAW_ON_INVALIDATE   = 1 << 8;  ///< Explicit request_redraw() call
static const MenuItem_RedrawPolicy REDRAW_ON_CUSTOM            = 1 << 9;  ///< Gates check_needs_redraw_custom() virtual hook
// Musical timing / transport events (post via menu or subclass) ---------------
static const MenuItem_RedrawPolicy REDRAW_ON_STEP              = 1 << 10; ///< Clock step/tick (sub-beat resolution)
static const MenuItem_RedrawPolicy REDRAW_ON_BEAT              = 1 << 11; ///< Quarter-note beat boundary
static const MenuItem_RedrawPolicy REDRAW_ON_BAR               = 1 << 12; ///< Bar/measure boundary
static const MenuItem_RedrawPolicy REDRAW_ON_BPM_CLOCK_CHANGE  = 1 << 13; ///< Transport state change (start/stop/continue/restart) or BPM change
// Bit 14 reserved for user-defined project-specific events
static const MenuItem_RedrawPolicy REDRAW_NEVER           = 0;       ///< Never redraw (use sparingly)
static const MenuItem_RedrawPolicy REDRAW_ALWAYS          = 0x7FFF;  ///< Always redraw (backwards compat / escape hatch — covers bits 0-14)

// Preset composite policies ---------------------------------------------------
/// Default for most items: redraws on all state-change events but NOT on every tick.
static const MenuItem_RedrawPolicy REDRAW_STATIC =
    REDRAW_ON_PAGE_ENTER | REDRAW_ON_SELECTION | REDRAW_ON_DESELECTION |
    REDRAW_ON_OPEN | REDRAW_ON_CLOSE | REDRAW_ON_INVALIDATE | REDRAW_ON_OWN_INPUT | REDRAW_ON_INVALIDATE;

/// For items displaying live data (BPM, clock position, graphs, etc.)
static const MenuItem_RedrawPolicy REDRAW_LIVE = REDRAW_STATIC | REDRAW_ON_TICK | REDRAW_ON_BPM_CLOCK_CHANGE;

// Backward-compatible aliases for old enum names ---------------------------------
static const MenuItem_RedrawPolicy REDRAW_ON_OPEN_STATE         = REDRAW_ON_OPEN | REDRAW_ON_CLOSE;
static const MenuItem_RedrawPolicy REDRAW_ON_SELECTION_OR_OPEN  = REDRAW_ON_SELECTION | REDRAW_ON_DESELECTION | REDRAW_ON_OPEN | REDRAW_ON_CLOSE;
static const MenuItem_RedrawPolicy REDRAW_ON_VALUE_CHANGE       = REDRAW_ON_CUSTOM;  ///< Use check_needs_redraw_custom() hook

char *get_redraw_policies_description(MenuItem_RedrawPolicy policy);
class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};

void menu_set_last_message(const char *msg, int colour);

// Lightweight flat-array list to replace LinkedList<option> in selector controls.
// Eliminates per-entry malloc overhead (LinkedList paid 8-byte alloc overhead per 8-byte option node).
template<typename T>
class OptionList {
    T* _items = nullptr;
    uint16_t _count = 0;
    uint16_t _capacity = 0;

    void grow() {
        uint16_t new_cap = (_capacity == 0) ? 8 : (_capacity + 8);
        T* new_items = (T*)realloc(_items, new_cap * sizeof(T));
        if (new_items == nullptr) return;
        _items = new_items;
        _capacity = new_cap;
    }

public:
    OptionList() = default;
    ~OptionList() { free(_items); }
    OptionList(const OptionList&) = delete;
    OptionList& operator=(const OptionList&) = delete;

    void add(T item) {
        if (_count >= _capacity) grow();
        if (_count < _capacity) _items[_count++] = item;
    }
    T& get(int idx) { return _items[idx]; }
    const T& get(int idx) const { return _items[idx]; }
    uint16_t size() const { return _count; }
    bool empty() const { return _count == 0; }
    void clear() { _count = 0; }

    T* begin() { return _items; }
    T* end()   { return _items + _count; }
    const T* begin() const { return _items; }
    const T* end()   const { return _items + _count; }
};

// basic line
class MenuItem {
    public:
        bool debug = false;

        DisplayTranslator *tft = nullptr;
        int menu_c_max = MENU_C_MAX;

        char label[MAX_LABEL_LENGTH];
        unsigned int label_len = 0;

        uint16_t default_fg = C_WHITE; //0xFFFF;
        uint16_t default_bg = BLACK;

        bool show_header = true;
        bool selectable = true;
        bool go_back_on_select = false;

        // Cache text-size decisions for static labels to avoid repeated width scans.
        int8_t cached_label_textsize = -1;
        uint16_t cached_label_width_px = 0;

        #if MENU_PERF_PARTIAL_UPDATES
            /// Policy: bitmask of REDRAW_ON_* bits that trigger a redraw.
            /// Default is REDRAW_STATIC — redraws on state changes but not every tick.
            MenuItem_RedrawPolicy redraw_policy = REDRAW_STATIC;
            /// Accumulated event bits since last mark_rendered(). Checked by needs_redraw().
            MenuItem_RedrawPolicy pending_redraw_events = REDRAW_ON_PAGE_ENTER; // force first render
            int8_t last_rendered_selected = -1;
            int8_t last_rendered_opened = -1;
            int16_t cached_draw_height = -1; // -1 = not yet rendered; 0 = rendered with zero height; >0 = normal

            virtual void set_redraw_policy(MenuItem_RedrawPolicy policy) {
                // Always preserve PAGE_ENTER and INVALIDATE so items respond to full-screen
                // clears and explicit request_redraw() calls regardless of the custom policy set.
                redraw_policy = policy | REDRAW_ON_PAGE_ENTER | REDRAW_ON_INVALIDATE;
            }
            virtual void add_redraw_policy(MenuItem_RedrawPolicy policy) {
                redraw_policy |= policy;
            }
            virtual MenuItem_RedrawPolicy get_redraw_policy() const {
                return redraw_policy;
            }
            /// Post an event bit to this item. The menu calls this at the right callsites.
            virtual void post_event(MenuItem_RedrawPolicy event_bit) {
                pending_redraw_events |= event_bit;
            }
            /// Explicit invalidation — always triggers a redraw regardless of policy.
            virtual void request_redraw(MenuItem_RedrawPolicy reason = REDRAW_ON_INVALIDATE) {
                pending_redraw_events |= reason;
            }
            /// Returns true if this item should be redrawn this frame.
            virtual bool needs_redraw(bool selected, bool opened) {
                bool debug = true;
                if (cached_draw_height < 0) {
                    if (debug) Serial.printf("%s: needs_redraw() true because cached_draw_height is %i\n", this->label, cached_draw_height);
                    return true;
                }
                if (redraw_policy == REDRAW_ALWAYS) {
                    if (debug) Serial.printf("%s: needs_redraw() true because redraw_policy is REDRAW_ALWAYS\n", this->label);
                    return true;
                }
                if (pending_redraw_events & redraw_policy) {
                    if (debug) Serial.printf("%s: needs_redraw() true because policy %s met by %s\n", this->label, get_redraw_policies_description(redraw_policy), get_redraw_policies_description(pending_redraw_events));
                    return true;
                }
                if ((redraw_policy & REDRAW_ON_CUSTOM) && check_needs_redraw_custom(selected, opened)) {
                    if (debug) Serial.printf("%s: needs_redraw() true because REDRAW_ON_CUSTOM is set and check_needs_redraw_custom() returned true\n", this->label);
                    return true;
                }
                return false;
            }
            /// Override to implement custom redraw logic (checked when policy has REDRAW_ON_CUSTOM).
            virtual bool check_needs_redraw_custom(bool selected, bool opened) { return false; }
            virtual void mark_rendered(bool selected, bool opened) {
                last_rendered_selected = (int8_t)selected;
                last_rendered_opened = (int8_t)opened;
                pending_redraw_events = REDRAW_NEVER;
            }
            virtual void mark_rendered(bool selected, bool opened, int16_t draw_height) {
                last_rendered_selected = (int8_t)selected;
                last_rendered_opened = (int8_t)opened;
                cached_draw_height = (draw_height > 0) ? draw_height : 0;
                pending_redraw_events = REDRAW_NEVER;
            }
            virtual int16_t get_cached_draw_height() const {
                return cached_draw_height;
            }
        #endif

        MenuItem set_tft(DisplayTranslator *tft) {
            this->tft = tft;
            return *this;
        }

        MenuItem(const char *in_label, bool selectable = true, bool show_header = true) {
            strncpy(label, in_label, MAX_LABEL_LENGTH);
            label[MAX_LABEL_LENGTH - 1] = '\0';
            label_len = strlen(label);
            this->selectable = selectable;
            this->show_header = show_header;
        }
        virtual void on_add();
        virtual void update_label(const char *new_label);
        virtual void invalidate_render_cache();
        virtual int get_textsize_for_label(uint16_t max_width_px);

        MenuItem *set_default_colours(uint16_t fg, uint16_t bg = BLACK);

        // called every tick, in case anything needs doing
        virtual void update_ticks(unsigned long ticks) {
            // nothing to do by default
        };

        virtual const char *get_label() {
            return this->label;
        }
        
        virtual int display(Coord pos, bool selected, bool opened);
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width);

        virtual void colours(bool inverted);
        virtual void colours(bool inverted, uint16_t fg);
        virtual void colours(bool inverted, uint16_t fg, uint16_t bg);
        
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0, unsigned int text_len = (unsigned int)-1);

        // called when item is selected ie opened from the main menu - return true to open, return false to 'refuse to open'
        virtual bool action_opened();

        // default to returning true to exit out to main menu after setting (IF OPENED, otherwise button_select is not sent!)
        virtual bool button_select();
        virtual bool button_select_released();
        virtual bool button_back();
        virtual bool button_right();
        virtual bool knob_left();
        virtual bool knob_right();
        virtual bool allow_takeover();
        // When true, SubMenuItemBar will render this control once more after row layout
        // so it can draw an overlay/popout while opened.
        virtual bool wants_fullscreen_overlay_when_opened_in_bar() { return false; }
        /// Redraw policy the parent bar should apply while this item is live as a fullscreen overlay.
        /// REDRAW_ON_OWN_INPUT (default) keeps the overlay current when the user interacts with it.
        /// Override to REDRAW_ON_TICK for overlays that display live/animated data (graphs, etc.).
        virtual MenuItem_RedrawPolicy get_overlay_redraw_policy() const { return REDRAW_ON_OWN_INPUT; }
        // whether we should be allowed to hover over this one
        virtual bool is_selectable ();
        // whether 'tis openable -- ie, that it can be 'opened' without having an effect, eg submenuitem... basically anything except an action?!
        virtual bool is_openable () {
            return true;
        }
};

// TODO: verify that this is actually what happens!
#define SELECT_EXIT true
#define SELECT_DONTEXIT false

// this IS actually what happens
#define BACK_EXIT false
#define BACK_DONTEXIT true

class FixedSizeMenuItem : public MenuItem {
    public:
    int fixed_size = 0;
    FixedSizeMenuItem(const char *label, int size) : MenuItem(label) {
        this->fixed_size = size;
    }
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width);
};

class PinnedPanelMenuItem : public MenuItem {
    public:
        unsigned long ticks = 0;
        #if MENU_PERF_PARTIAL_UPDATES
            bool redraw_needed = true;
            int16_t cached_draw_height = 0;

            virtual void request_redraw() {
                redraw_needed = true;
            }
            virtual void refresh_redraw_state() {
                // default no-op
            }
            virtual bool should_redraw() const {
                return redraw_needed || cached_draw_height <= 0;
            }
            virtual int16_t get_cached_draw_height() const {
                return cached_draw_height;
            }
            virtual void mark_drawn(int16_t draw_height) {
                cached_draw_height = (draw_height > 0) ? draw_height : 0;
                redraw_needed = false;
            }
        #endif

        PinnedPanelMenuItem(const char *label) : MenuItem(label) {
            IF_MENU_PERF_PARTIAL_UPDATES(
                redraw_policy |= REDRAW_ON_TICK | REDRAW_ON_CUSTOM | REDRAW_ON_BPM_CLOCK_CHANGE;
            )
        };

        virtual void update_ticks(unsigned long ticks) override {
            this->ticks = ticks;
        }
};

class DoublePinnedPanelMenuItem : public PinnedPanelMenuItem {
    public:
        unsigned long ticks = 0;
        PinnedPanelMenuItem *item1, *item2;

        DoublePinnedPanelMenuItem(PinnedPanelMenuItem *item1, PinnedPanelMenuItem *item2) : PinnedPanelMenuItem("") {
            this->item1 = item1;
            this->item2 = item2;
        };

        virtual void update_ticks(unsigned long ticks) override {
            if (this->item1!=nullptr) this->item1->update_ticks(ticks);
            if (this->item2!=nullptr) this->item2->update_ticks(ticks);
            this->ticks = ticks;

            #if MENU_PERF_PARTIAL_UPDATES
                if ((this->item1!=nullptr && this->item1->should_redraw()) ||
                    (this->item2!=nullptr && this->item2->should_redraw())) {
                    this->request_redraw();
                }
            #endif
        }

        #if MENU_PERF_PARTIAL_UPDATES
            virtual void refresh_redraw_state() override {
                if (this->item1!=nullptr) this->item1->refresh_redraw_state();
                if (this->item2!=nullptr) this->item2->refresh_redraw_state();

                if ((this->item1!=nullptr && this->item1->should_redraw()) ||
                    (this->item2!=nullptr && this->item2->should_redraw())) {
                    this->request_redraw();
                }
            }
        #endif

        int display(Coord pos, bool selected, bool opened) override {
            int y = pos.y;
            if (this->item1!=nullptr) {
                y += this->item1->display(Coord(pos.x, y), selected, opened);
            }
            if (this->item2!=nullptr) {
                y += this->item2->display(Coord(pos.x, y), selected, opened);
            }
            return y;
        }
};


#include "menuitems_numbers.h"
#include "menuitems_selector.h"
#include "menuitems_action.h"
//#include "menuitems_pinned.h"

class SeparatorMenuItem : virtual public MenuItem {
    public:
        bool draw_lines = true;
        int textSize = 0;
        //int16_t colour = C_WHITE;
        SeparatorMenuItem(const char *label, int textSize = 0, bool draw_lines = true) : MenuItem(label, false) {
            this->textSize = textSize;
            this->draw_lines = draw_lines;
            // Separators can't be selected, so use PAGE_ENTER + INVALIDATE only.
            // They will be redrawn when the page is entered or explicitly invalidated.
            IF_MENU_PERF_PARTIAL_UPDATES(set_redraw_policy(REDRAW_ON_PAGE_ENTER | REDRAW_ON_INVALIDATE);)
        }
        SeparatorMenuItem(const char *label, uint16_t default_fg, int textSize = 0, bool draw_lines = true) : SeparatorMenuItem(label, textSize, draw_lines) {
            this->default_fg = default_fg;
            // redraw_policy already set in primary constructor
        }

        virtual int display(Coord pos, bool selected, bool opened) override;
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0, unsigned int text_len = (unsigned int)-1);
};

#include "menuitems_toggle.h"
#include "menuitems_object.h"
#include "menuitems_fileviewer.h"

#endif