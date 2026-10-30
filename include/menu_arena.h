#ifndef MENU_ARENA_H
#define MENU_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <new>

class MenuItemArena {
    char *buffer_;
    size_t capacity_;
    size_t used_;
    size_t peak_used_;
    size_t heap_fallback_bytes_;
    uint32_t allocation_count_;
    uint32_t heap_fallback_count_;
    bool allocations_open_;

public:
    MenuItemArena(char *buffer, size_t capacity)
        : buffer_(buffer), capacity_(capacity), used_(0), peak_used_(0),
          heap_fallback_bytes_(0), allocation_count_(0), heap_fallback_count_(0),
          allocations_open_(false) {}

    MenuItemArena(const MenuItemArena&) = delete;
    MenuItemArena& operator=(const MenuItemArena&) = delete;

    void begin_allocations() { allocations_open_ = true; }
    void close_allocations() { allocations_open_ = false; }
    bool allocations_open() const { return allocations_open_; }

    void *allocate(size_t size, size_t alignment = alignof(max_align_t)) {
        if (!allocations_open_ || buffer_ == nullptr || alignment == 0 ||
            (alignment & (alignment - 1)) != 0)
            return nullptr;

        if (size == 0)
            size = 1;

        const uintptr_t start = reinterpret_cast<uintptr_t>(buffer_);
        const uintptr_t max_value = static_cast<uintptr_t>(-1);
        if (used_ > capacity_ || start > max_value - used_)
            return nullptr;

        const uintptr_t current = start + used_;
        if (current > max_value - (alignment - 1))
            return nullptr;
        const uintptr_t aligned = (current + alignment - 1) &
                                  ~static_cast<uintptr_t>(alignment - 1);
        if (aligned < start)
            return nullptr;

        const uintptr_t offset = aligned - start;
        if (offset > capacity_ || size > capacity_ - offset)
            return nullptr;

        used_ = static_cast<size_t>(offset) + size;
        if (used_ > peak_used_)
            peak_used_ = used_;
        ++allocation_count_;
        return reinterpret_cast<void*>(aligned);
    }

    bool owns(const void *pointer) const {
        if (pointer == nullptr || buffer_ == nullptr || used_ == 0)
            return false;
        const uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
        const uintptr_t start = reinterpret_cast<uintptr_t>(buffer_);
        return address >= start && address - start < used_;
    }

    void record_heap_fallback(size_t size) {
        ++heap_fallback_count_;
        heap_fallback_bytes_ += size;
    }

    size_t bytes_used() const { return used_; }
    size_t bytes_capacity() const { return capacity_; }
    size_t peak_bytes_used() const { return peak_used_; }
    size_t heap_fallback_bytes() const { return heap_fallback_bytes_; }
    uint32_t allocation_count() const { return allocation_count_; }
    uint32_t heap_fallback_count() const { return heap_fallback_count_; }
};

extern MenuItemArena *menu_item_arena;

inline void menu_item_set_arena(MenuItemArena *arena) {
    menu_item_arena = arena;
}

inline MenuItemArena *menu_item_get_arena() {
    return menu_item_arena;
}

inline void *menu_item_allocate(size_t size, bool nothrow = false) {
    MenuItemArena *arena = menu_item_get_arena();
    if (arena != nullptr && arena->allocations_open()) {
        void *pointer = arena->allocate(size);
        if (pointer != nullptr)
            return pointer;
        arena->record_heap_fallback(size);
    }

    return nothrow ? ::operator new(size, std::nothrow) : ::operator new(size);
}

inline void menu_item_deallocate(void *pointer) noexcept {
    MenuItemArena *arena = menu_item_get_arena();
    if (arena != nullptr && arena->owns(pointer))
        return;
    ::operator delete(pointer);
}

#endif