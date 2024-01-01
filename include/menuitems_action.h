#ifndef MENUITEMS_ACTION__INCLUDED
#define MENUITEMS_ACTION__INCLUDED

#include "menuitems.h"

extern const char *fired_message;
extern const char *sure_message;

class ActionItem : public MenuItem {
    public:

    char button_label[MENU_C_MAX] = "";
    void(*on_open)() = nullptr;

    char *what_to_render = nullptr;

    ActionItem(const char *label, void (*on_open)(), bool show_header = false) : MenuItem(label) {
        this->on_open = on_open;
        this->show_header = show_header;
        snprintf(button_label, MENU_C_MAX, ">> %s <<", label);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //int textSize = ((int)strlen(button_label)*tft->currentCharacterWidth() < tft->width()/2 );
        //int textSize = tft->get_textsize_for_width(button_label, tft->width()-1);
        //colours(opened, opened ? GREEN : this->default_fg, this->default_bg);
        int textSize = tft->get_textsize_for_width(button_label, tft->width());
        pos.y = header(button_label, pos, selected, opened, textSize);

        //pos.y = this->renderValue(selected, opened, tft->width());
        // if doing full display, render button_label (with >> %s << etc)
        //int textSize = tft->get_textsize_for_width(button_label, tft->width());
        tft->setTextSize(textSize);
        if (selected) colours(true);
        tft->printf(tft->header_format, button_label);
        if (selected) colours(false);
        //tft->setCursor(tft->getCursorX(), tft->getCursorY()+1);

        //tft->setCursor(pos.x,pos.y);

        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        // render bare label (without >> %s << etc) if using renderValue
        int textSize = tft->get_textsize_for_width(label, max_character_width*tft->characterWidth());
        tft->setTextSize(textSize);
        tft->print(label);
        //tft->setCursor(tft->getCursorX(), tft->getCursorY()+tft->getRowHeight()+1);
        tft->setCursor(tft->getCursorX(), tft->getCursorY()+1);

        return tft->getCursorY() + 1;
    }

    virtual bool action_opened() override {
        //Debug_println(F("ActionItem#action_opened"));
        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }
};

class ActionFeedbackItem : public MenuItem {
    public:

    char button_label_false[20] = "";
    char button_label_true[20] = "";
    //void(*on_open)() = nullptr;

    using setter_def_2 = void(*)();
    using setter_def = void(*)(bool);
    //using get_label_def = char*(TargetClass::*)(bool); //, get_label_def *get_label
    using getter_def = bool(*)();

    setter_def setter = nullptr;
    setter_def_2 setter2 = nullptr;
    getter_def getter = nullptr;

    ActionFeedbackItem(const char *label, setter_def setter) : 
        MenuItem(label) {
            this->setter = setter;
        }
    ActionFeedbackItem(const char *label, setter_def_2 setter) : 
        MenuItem(label) {
            this->setter2 = setter;
        }

    ActionFeedbackItem(const char *label, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : MenuItem(label) {
        this->getter = getter;

        if (button_label_true!=nullptr)
            strncpy(this->button_label_true, button_label_true, 20);
        else
            snprintf(this->button_label_true, 20, label);

        if (button_label_false!=nullptr)
            snprintf(this->button_label_false, 20, button_label_false);
        else
            snprintf(this->button_label_false, 20, label);
    }

    ActionFeedbackItem(const char *label, setter_def setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : ActionFeedbackItem(label, getter, button_label_true, button_label_false) {
            this->setter = setter;
        }

    ActionFeedbackItem(const char *label, setter_def_2 setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : ActionFeedbackItem(label, getter, button_label_true, button_label_false) {
            this->setter2 = setter;
        }

    virtual int display(Coord pos, bool selected, bool opened) override {
        this->tft->setCursor(pos.x, pos.y);
        return this->renderValue(selected, opened, MENU_C_MAX);
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        char *button_label = nullptr;
        if (this->getter!=nullptr && this->button_label_true[0]) {
            button_label = (this->getter)() ? this->button_label_true : this->button_label_false;
        } else if (button_label_false[0]) {
            //Serial.printf(F("%s: rendering button_label_false '%s'\n"), this->label, this->button_label_false);
            button_label = this->button_label_false;
        } else {
            button_label = label;
        }
        //int y = header(button_label, Coord(this->tft->getCursorX(), this->tft->getCursorY()), selected, opened);
        colours(selected);

        // determine size font to use
        //bool use_small = strlen(button_label) <= (max_character_width/2);
        //int textSize = use_small ? 2 : 1;
        int textSize = tft->get_textsize_for_width(button_label, max_character_width*tft->characterWidth());
        tft->setTextSize(textSize);
        tft->print(button_label);
        tft->setCursor(tft->getCursorX(), tft->getCursorY()+tft->getRowHeight()+1);

        const int y = tft->getCursorY();
        return y;
    }

    virtual bool action_opened() override {
        //Serial.println(F("ObjectActionItem#action_opened"));
        //this->on_open();
        if (this->setter!=nullptr)
            (this->setter)(true);
        else if (this->setter2!=nullptr)
            (this->setter2)();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }

};

class ActionConfirmItem : public ActionItem {
    public:

    ActionConfirmItem(const char *label, void (*on_open)(), bool show_header = true) : ActionItem(label, on_open, show_header) {
        go_back_on_select = true;
    }

    /*virtual int display(Coord pos, bool selected, bool opened) override {
        const char *text_to_render = opened ? sure_message : button_label;

        int textSize = tft->get_textsize_for_width(text_to_render, tft->width());
        pos.y = header(text_to_render, pos, selected, opened, textSize);

        //tft->setCursor(pos.x,pos.y);
        //tft->setTextSize(textSize);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        return pos.y; //tft->getCursorY();
    }*/
    virtual int display(Coord pos, bool selected, bool opened) override {
        this->tft->setCursor(pos.x, pos.y);
        return this->renderValue(selected, opened, MENU_C_MAX);
    }
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        colours(selected, opened ? GREEN : this->default_fg, this->default_bg);
        const char *text_to_render = opened ? sure_message : button_label;

        int textSize = tft->get_textsize_for_width(text_to_render, tft->width());
        tft->setTextSize(textSize);
        tft->print(text_to_render);
        tft->setCursor(tft->getCursorX(), tft->getCursorY()+tft->getRowHeight()+1);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        //Debug_println(F("ActionConfirmItem#action_opened"));
        //this->on_open();
        return true; 
    }

    virtual bool button_select() override {
        //Debug_println(F("ActionConfirmItem#button_select"));

        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;    // return to menu
    }

};

#endif