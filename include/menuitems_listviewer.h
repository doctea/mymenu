#ifndef MENUITEMS_LISTVIEWER__INCLUDED
#define MENUITEMS_LISTVIEWER__INCLUDED

#include <Arduino.h>

#include "LinkedList.h"
#include "menuitems.h"

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
            unsigned int i = start_line;
            auto it = list_contents->begin();
            for (unsigned int s = 0; s < start_line && it != list_contents->end(); ++s, ++it) {}
            for (; it != list_contents->end() && i < start_line + height_lines; ++it, ++i) {
                tft->printf("%-3i: ", i+1);
                tft->println((*it).c_str());
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