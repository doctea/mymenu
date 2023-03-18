#ifndef MENUITEMS_FILEVIEWER__INCLUDED
#define MENUITEMS_FILEVIEWER__INCLUDED

#ifdef ENABLE_SD

#include <Arduino.h>

#include "LinkedList.h"
#include "menuitems.h"

#include "SD.h"

#include "menuitems_listviewer.h"

class FileViewerMenuItem : public ListViewerMenuItem {
    String filename = "";

    //LinkedList<String> *file_contents = new LinkedList<String>();

    public:
    FileViewerMenuItem(const char *label) : ListViewerMenuItem(label) {
        this->list_contents = new LinkedList<String>();
    }
    
    virtual void on_add() override {
        ListViewerMenuItem::on_add();
    }

    void setFilename(String filename) {
        this->filename = filename;
    }
    void readFile() {
        Serial.println("readFile()");
        list_contents->clear();
        File f = SD.open(filename.c_str(), FILE_READ);
        f.setTimeout(0);
        if (!f) {
            Serial.println("error loading file for viewing!"); Serial_flush();
            filename = String("(err)") + filename;
            return;
        }
        while (f.available()) {
            String line = f.readStringUntil('\n');
            list_contents->add(line);
        }
        f.close();
        Serial.println("finished readFile()!");
    }

    int render_list_header(Coord pos) {
        pos.y = ListViewerMenuItem::render_list_header(pos);
        tft->printf("Filename: %s\n", (char*)filename.c_str());
        return tft->getCursorY();
    }

};

#endif

#endif