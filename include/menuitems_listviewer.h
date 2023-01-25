#ifndef MENUITEMS_LISTVIEWER__INCLUDED
#define MENUITEMS_LISTVIEWER__INCLUDED

#include <Arduino.h>

#include "LinkedList.h"
#include "menuitems.h"

#include "SD.h"

class ListViewerMenuItem : public MenuItem {
    public:

    unsigned int height_lines = 0;
    unsigned int start_line = 0;

    LinkedList<String> *list_contents = nullptr; //new LinkedList<String>();

    ListViewerMenuItem(const char *label, LinkedList<String> *list_contents = nullptr) : MenuItem(label) {
        this->list_contents = list_contents;
    }

    int render_list_header(Coord pos) {
        tft->printf("Lines: %i\n", list_contents->size());
        return tft->getCursorY();
    }

    virtual void on_add() override {
        MenuItem::on_add();
        this->height_lines = (tft->height() / tft->getRowHeight());
    }


    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setCursor(pos.x,pos.y);
        //char label[MENU_C_MAX];
        //sprintf(label, "%s: %s (%i)", this->label, filename.c_str(), file_contents->size());
        pos.y = header(label, pos, selected, opened);

        pos.y = this->render_list_header(pos);

        if (list_contents->size()>0) {
            unsigned int i = 0;
            for (i = start_line ; i < start_line + height_lines && i < list_contents->size() ; i++) {
                /*char buffer[MENU_C_MAX] = "";
                sprintf(buffer, "%-3i: %s", i, file_contents->get(i).c_str());
                tft->println(buffer);*/
                tft->printf("%-3i: ", i+1);
                tft->println(list_contents->get(i).c_str());
            }
            if (i < height_lines && list_contents->size() > height_lines)
                tft->println("...more...");
        } else {
            tft->println("(Nothing to show)");
        }

        return tft->getCursorY();
    }

    virtual bool knob_left() override {
        if (start_line == 0)
            start_line = list_contents->size() - 1;
        else 
            start_line--;
        return true;
    }

    virtual bool knob_right() override {
        start_line++;
        start_line %= list_contents->size();
        return true;
    }

};


#endif