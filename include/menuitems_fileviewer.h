#ifndef MENUITEMS_FILEVIEWER__INCLUDED
#define MENUITEMS_FILEVIEWER__INCLUDED

#include <Arduino.h>

#include "LinkedList.h"
#include "menuitems.h"

#include "SD.h"

class FileViewerMenuItem : public MenuItem {
    String filename = "";
    unsigned int height_lines = 0;
    unsigned int start_line = 0;

    LinkedList<String> *file_contents = new LinkedList<String>();

    public:
    FileViewerMenuItem(const char *label) : MenuItem(label) {}
    virtual void on_add() {
        MenuItem::on_add();
        this->height_lines = (tft->height() / tft->getRowHeight());
    }

    void setFilename(String filename) {
        this->filename = filename;
    }
    void readFile() {
        Serial.println("readFile()");
        file_contents->clear();
        File f = SD.open(filename.c_str(), FILE_READ);
        f.setTimeout(0);
        if (!f) {
            Serial.println("error loading file for viewing!"); Serial_flush();
            filename = String("(err)") + filename;
            return;
        }
        while (String line = f.readStringUntil('\n')) {
            file_contents->add(line);
        }
        f.close();
        Serial.println("finished readFile()!");
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setCursor(pos.x,pos.y);
        //char label[MENU_C_MAX];
        //sprintf(label, "%s: %s (%i)", this->label, filename.c_str(), file_contents->size());
        header(label, pos, selected, opened);

        if (filename!="") {
            tft->printf("Filename: %s\n", (char*)filename.c_str());
            tft->printf("Lines: %i\n", file_contents->size());

            unsigned int i = 0;
            for (i = start_line ; i < start_line + height_lines && i < file_contents->size() ; i++) {
                /*char buffer[MENU_C_MAX] = "";
                sprintf(buffer, "%-3i: %s", i, file_contents->get(i).c_str());
                tft->println(buffer);*/
                tft->printf("%-3i: ", i+1);
                tft->println(file_contents->get(i).c_str());
            }
            if (i < height_lines)
                tft->println("...more...");
        } else {
            tft->println("Error loading?");
        }

        return tft->getCursorY();
    }

    virtual bool knob_left() override {
        if (start_line == 0)
            start_line = file_contents->size() - 1;
        else 
            start_line--;
        return true;
    }

    virtual bool knob_right() override {
        start_line++;
        start_line %= file_contents->size();
        return true;
    }
};

#endif