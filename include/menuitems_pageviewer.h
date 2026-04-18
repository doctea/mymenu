#pragma once

#include <Arduino.h>

#include "LinkedList.h"
#include "menuitems.h"

// implement file viewer by copying entire file into extmem
// then display it line by line
// store a pointer to the start of the current line and render from there, counting lines
// if we need to wrap around then we can just reset the pointer to the start of the file

class PageFileViewerMenuItem : public MenuItem {
    public:

    char filename[256];

    unsigned int height_lines = 0;
    char *page_first_line = nullptr;
    //char *page_last_line = nullptr;

    char *start_at = nullptr;

    char *file_contents = nullptr;
    unsigned int file_size = 0;

    PageFileViewerMenuItem(const char *label) : MenuItem(label) {
    }

    void setFileContents(char *file_contents, unsigned int file_size) {
        this->file_contents = file_contents;
        this->file_size = file_size;

        // find the start of the last line we can display
        page_first_line = file_contents;
        //char *last_found_break = nullptr;
        unsigned int line_count = 0;
        char *search = page_first_line;
        while (search < file_contents + file_size && line_count < height_lines) {
            if (*search == '\n') {
                //last_found_break = search;
                line_count++;
            }
            search++;
        }
        start_at = file_contents;
        //page_last_line = last_found_break;
    }

    bool readFile(const char *filename) {
        const uint64_t MAX_FILE_SIZE = 1024 * 1024 * sizeof(char); // 1MB max file size for now, to avoid running out of memory
        #ifdef ENABLE_SD
        if (file_contents==nullptr) {
            Serial.printf("Allocating %llu bytes for file_contents buffer...\n", MAX_FILE_SIZE);
            file_contents = (char*)extmem_malloc(MAX_FILE_SIZE);  // allocate 1MB for file contents - todo: make this configurable, or at least define a constant for it
            if (!file_contents) {
                Serial.printf("Failed to allocate file_contents buffer of size %llu - won't be able to display file!\n", MAX_FILE_SIZE);
                return false;
            }
        }

        if (file_contents == nullptr) {
            Serial.println("ERROR: file_contents is nullptr, cannot read file!");
            return false;
        }

        strncpy(this->filename, filename, 255);
        this->filename[255] = '\0';

        if (!SD.exists(filename)) {
            Serial.printf("readFile: File %s does not exist!\n", filename);
            // Keep viewer in a safe, empty state so display/scroll cannot crash.
            file_size = 0;
            page_first_line = nullptr;
            start_at = nullptr;
            return false;
        } 
        
        // we should clear the file_contents buffer before reading new data into it, to avoid displaying old data if the new file is smaller than the previous one
        
        File f = SD.open(filename, FILE_READ);
        if (!f) {
            Serial.println("readFile: Error opening file for viewing!");
            file_size = 0;
            page_first_line = nullptr;
            start_at = nullptr;
            return false;
        }

        if (f.size() > MAX_FILE_SIZE) {
            Serial.printf("File %s exists and opened, but is %llu bytes, which exceeds the maximum allowed size of %llu bytes!\n", filename, SD.totalSize(), MAX_FILE_SIZE);
            f.close();
            file_size = 0;
            page_first_line = nullptr;
            start_at = nullptr;
            return false;
        }

        memset(file_contents, 0, MAX_FILE_SIZE);
        file_size = f.size();
        f.readBytes(file_contents, file_size);
        f.close();

        setFileContents(file_contents, file_size);
        return true;
        #endif
        return false;
    }

    virtual void on_add() override {
        MenuItem::on_add();
        this->height_lines = (tft->height() / tft->getRowHeight()) / 3;
    }

    int render_list_header(Coord pos) {
        tft->printf("%s\n(%u bytes)\n", filename, file_size, 0);
        //tft->printf("Lines: %i\n", list_contents->size());
        //tft->printf("...");
        return tft->getCursorY();
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setCursor(pos.x,pos.y);
        pos.y = header(label, pos, selected, opened);
        pos.y = this->render_list_header(pos);

        if (file_contents==nullptr || start_at==nullptr || file_size==0) {
            tft->println("(No file loaded)");
            return tft->getCursorY();
        }

        char line_buffer[256];

        char *cursor = start_at;
        for (unsigned int line_number = 0 ; line_number < height_lines ; line_number++) {
            int buf_idx = 0;
            while (cursor < file_contents+file_size && *cursor != '\n' && buf_idx < 256) {
                line_buffer[buf_idx++] = *cursor;
                cursor++;
                // indicate that we've truncated the line
                if (buf_idx >= 254)
                    line_buffer[buf_idx] = '~';
            }
            line_buffer[buf_idx] = '\0';
            tft->println(line_buffer);
            cursor++;
        }

        return tft->getCursorY();
    }

    virtual bool knob_left() override {
        if (file_contents == nullptr || start_at == nullptr || file_size == 0)
            return false;

        // already at start - don't go any further (todo: wrap around to eof)
        if (start_at == file_contents)
            return false;

        // step back 2 characters before the start of the current line, in order step over 
        // the newline character of the previous line
        // then look back to find the start of the previous line
        char *cursor = start_at-2;
        while (cursor > file_contents) {
            if (*cursor == '\n') {
                break;
            }
            cursor--;
        }
        if (cursor >= file_contents)
            start_at = cursor + 1;  // +1 to skip the newline character
        //Serial.printf("knob_left: start_at is now %p\n", start_at);
        return true;
    }

    virtual bool knob_right() override {
        if (file_contents == nullptr || start_at == nullptr || file_size == 0)
            return false;

        // already at end - don't go any further (todo: wrap around to start)
        if (start_at >= file_contents + file_size)
            return false;

        // look forward to find the next newline character or eof
        char *cursor = start_at;
        while (cursor < file_contents + file_size) {
            if (*cursor == '\n') {
                break;
            }
            cursor++;
        }
        if (cursor < file_contents + file_size && cursor != page_first_line)
            start_at = cursor+1; // +1 to skip the newline character
        //Serial.printf("knob_right: start_at is now %p\n", start_at);
        return true;
    }

};