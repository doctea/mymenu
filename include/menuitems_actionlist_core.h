#pragma once

// Display-independent core of ActionListMenuItem: list sources and a small
// navigation stack. Kept free of Arduino/display dependencies so the cursor and
// stacking behaviour can be tested on a host compiler.

#include <stdint.h>
#include <stddef.h>

class ActionListNavigator;

enum class ActionListResult : uint8_t {
    STAY = 0,   // remain on the current list
    POP,        // return to the previous list (or close the item when at the root)
    CLOSE,      // close the whole item
};

// A list of rows the user can scroll and act on. Rows are addressed by index and
// queried on demand; sources should not cache text unless they need a snapshot.
class ActionListSource {
    public:
        virtual ~ActionListSource() = default;

        virtual uint8_t count() = 0;
        virtual void row_text(uint8_t index, char *out, size_t size) = 0;

        // false for headings/info rows: the cursor skips them
        virtual bool row_selectable(uint8_t) { return true; }
        // false draws the row dimmed; select/right on it do nothing
        virtual bool row_enabled(uint8_t) { return true; }
        // true draws the row dimmed but leaves it actionable (eg a paused item)
        virtual bool row_dimmed(uint8_t) { return false; }
        // 0 = the item's default colour
        virtual uint16_t row_colour(uint8_t) { return 0; }

        // title shown while this list is the current level; nullptr = item label
        virtual const char *title() { return nullptr; }
        // short key help shown under the list; nullptr = just the back hint
        virtual const char *hint() { return nullptr; }

        virtual ActionListResult on_select(uint8_t, ActionListNavigator &) { return ActionListResult::STAY; }
        // return true when handled
        virtual bool on_right(uint8_t, ActionListNavigator &) { return false; }

        // called when this list becomes the current level (pushed, or returned to)
        virtual void on_enter() {}

        // changes whenever visible content changes; used to trigger redraws of live data
        virtual uint32_t generation() { return 0; }
};

class ActionListNavigator {
    public:
        static const uint8_t MAX_DEPTH = 4;
        static const int16_t NO_ROW = -1;

        struct level_t {
            ActionListSource *source = nullptr;
            int16_t cursor = NO_ROW;
            uint8_t scroll = 0;
        };

        void reset(ActionListSource *root) {
            depth = 0;
            if (root == nullptr) return;
            levels[0] = level_t{};
            levels[0].source = root;
            depth = 1;
            enter_current(true);
        }

        // false if the stack is full or source is null
        bool push(ActionListSource *source) {
            if (source == nullptr || depth >= MAX_DEPTH) return false;
            levels[depth] = level_t{};
            levels[depth].source = source;
            ++depth;
            enter_current(true);
            return true;
        }

        // false when already at the root (nothing popped)
        bool pop() {
            if (depth <= 1) return false;
            levels[--depth] = level_t{};
            enter_current(false);
            return true;
        }

        uint8_t get_depth() const { return depth; }
        ActionListSource *current() const { return depth > 0 ? levels[depth - 1].source : nullptr; }
        ActionListSource *root() const { return depth > 0 ? levels[0].source : nullptr; }
        int16_t cursor() const { return depth > 0 ? levels[depth - 1].cursor : NO_ROW; }
        uint8_t scroll() const { return depth > 0 ? levels[depth - 1].scroll : 0; }

        // step to the next selectable row in direction (+1/-1), wrapping
        bool move(int8_t direction) {
            ActionListSource *source = current();
            if (source == nullptr) return false;
            const uint8_t count = source->count();
            level_t &level = levels[depth - 1];
            if (count == 0) { level.cursor = NO_ROW; return false; }
            int16_t start = level.cursor;
            if (start < 0 || start >= count) start = direction >= 0 ? -1 : count;
            for (uint8_t step = 1; step <= count; ++step) {
                const int16_t index = (int16_t)(((start + direction * step) % count + count) % count);
                if (source->row_selectable((uint8_t)index)) {
                    level.cursor = index;
                    return true;
                }
            }
            level.cursor = NO_ROW;
            return false;
        }

        // Re-validate the cursor after rows were added/removed; prefers staying at
        // the same index, then the nearest selectable row above, then below.
        void clamp() {
            ActionListSource *source = current();
            if (source == nullptr) return;
            const uint8_t count = source->count();
            level_t &level = levels[depth - 1];
            if (count == 0) { level.cursor = NO_ROW; level.scroll = 0; return; }
            int16_t index = level.cursor;
            if (index >= count) index = count - 1;
            if (index < 0) index = 0;
            for (int16_t probe = index; probe >= 0; --probe)
                if (source->row_selectable((uint8_t)probe)) { level.cursor = probe; return; }
            for (int16_t probe = index + 1; probe < count; ++probe)
                if (source->row_selectable((uint8_t)probe)) { level.cursor = probe; return; }
            level.cursor = NO_ROW;
        }

        // adjust scroll so the cursor is within a window of visible_rows rows
        void ensure_visible(uint8_t visible_rows) {
            if (depth == 0 || visible_rows == 0) return;
            level_t &level = levels[depth - 1];
            const uint8_t count = level.source->count();
            if (level.cursor >= 0) {
                if (level.cursor < level.scroll) level.scroll = (uint8_t)level.cursor;
                else if (level.cursor >= level.scroll + visible_rows) level.scroll = (uint8_t)(level.cursor - visible_rows + 1);
            }
            // keep headings above the first selectable row visible when possible
            if (level.cursor >= 0 && level.cursor < visible_rows) level.scroll = 0;
            if (count <= visible_rows) level.scroll = 0;
            else if (level.scroll > count - visible_rows) level.scroll = count - visible_rows;
        }

        // act on the cursor row; returns whether the owning item should close
        bool select() {
            ActionListSource *source = current();
            const int16_t index = cursor();
            if (source == nullptr || index < 0 || index >= source->count()) return false;
            if (!source->row_selectable((uint8_t)index) || !source->row_enabled((uint8_t)index)) return false;
            const uint8_t depth_before = depth;
            const ActionListResult result = source->on_select((uint8_t)index, *this);
            if (result == ActionListResult::CLOSE) return true;
            if (result == ActionListResult::POP) {
                // a source that pushed and popped in one action must not pop its parent too
                if (depth == depth_before && !pop()) return true;
            }
            clamp();
            return false;
        }

        bool right() {
            ActionListSource *source = current();
            const int16_t index = cursor();
            if (source == nullptr || index < 0 || index >= source->count()) return false;
            if (!source->row_selectable((uint8_t)index) || !source->row_enabled((uint8_t)index)) return false;
            const bool handled = source->on_right((uint8_t)index, *this);
            clamp();
            return handled;
        }

        // true when the back press was consumed by popping a level
        bool back() { return pop(); }

    private:
        level_t levels[MAX_DEPTH];
        uint8_t depth = 0;

        void enter_current(bool fresh) {
            ActionListSource *source = current();
            source->on_enter();
            if (fresh) {
                levels[depth - 1].cursor = NO_ROW;
                move(1);
            } else {
                clamp();
            }
        }
};
