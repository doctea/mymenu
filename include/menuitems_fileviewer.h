#ifndef MENUITEMS_FILEVIEWER__INCLUDED
#define MENUITEMS_FILEVIEWER__INCLUDED

#ifdef ENABLE_SD

#include <Arduino.h>

#include <util/atomic.h>

#include "LinkedList.h"
#include "menuitems.h"

#include "SD.h"

int freeRam();

#include "menuitems_listviewer.h"

class FileViewerMenuItem : public ListViewerMenuItem {
    String filename = "";

    //LinkedList<String> *file_contents = new LinkedList<String>();

    public:
    FileViewerMenuItem(const char *label) : ListViewerMenuItem(label, new LinkedList<String>()) {
    }
    
    virtual void on_add() override {
        ListViewerMenuItem::on_add();
    }

    void setFilename(String filename) {
        this->filename = filename;
    }
    void readFile() {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (debug) {
                Serial.println("readFile()..."); Serial_flush();
                Serial.printf("Currently free RAM is %i\n", freeRam()); Serial_flush();
            }
            list_contents->clear();
            if (debug) { Serial.println("readFile(): did list_contents->clear();"); Serial_flush(); }
            File f = SD.open(filename.c_str(), FILE_READ);
            // ^^ intermittent crashing here when auto_advancing...
            if (debug) { Serial.printf("readFile(): did SD.open(%s);", filename.c_str()); Serial_flush(); }
            f.setTimeout(0);
            if (!f) {
                Serial.println("error loading file for viewing!"); Serial_flush();
                filename = String("(err)") + filename;
                f.close();
                return;
            }
            String line;
            while (f.available()) {
                line = f.readStringUntil('\n');
                list_contents->add(line);
                if (debug) { 
                    Serial.printf("readFile(): read a line '%s'\n", line.c_str()); 
                    Serial.printf("free ram is now %u\n", freeRam());
                    Serial_flush(); 
                }
            }
            if (debug) Serial.println("readFile(): closing.."); Serial_flush();
            f.close();
            if (debug) Serial.println("readFile(): did f.close();"); Serial_flush();
            Serial.println("finished readFile()!");
        }
    }

    int render_list_header(Coord pos) {
        pos.y = ListViewerMenuItem::render_list_header(pos);
        tft->printf("Filename: %s\n", (char*)filename.c_str());
        return tft->getCursorY();
    }

};

#endif

#endif