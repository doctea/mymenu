#ifndef MENUITEMS_LAMBDA__INCLUDED
#define MENUITEMS_LAMBDA__INCLUDED

#include "functional-vlpp.h"

#include "menuitems_numbers.h"

template<class DataType>
class LambdaNumberControl : public NumberControl<DataType> {
    public:

    using setter_func_def = vl::Func<void(DataType)>;
    using getter_func_def = vl::Func<DataType(void)>;

    setter_func_def setter_func;
    getter_func_def getter_func;

    LambdaNumberControl(
        const char* label, 
        setter_func_def setter_func,
        getter_func_def getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : NumberControl<DataType>(label, go_back_on_select, direct) {
        this->getter_func = getter_func;
        this->setter_func = setter_func;

        this->on_change_handler = on_change_handler;
        this->minimumDataValue = (DataType)0;
        this->maximumDataValue = (DataType)100;
        //this->go_back_on_select = go_back_on_select;

        this->set_internal_value( this->getter_func() );
    }
    LambdaNumberControl(
        const char* label, 
        setter_func_def setter_func,
        getter_func_def getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value),
        DataType minimumDataValue,
        DataType maximumDataValue,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaNumberControl<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select, direct) {
        this->minimumDataValue = minimumDataValue;
        this->maximumDataValue = maximumDataValue;
    }

    virtual void on_add() override {
        NumberControl<DataType>::on_add();
        this->set_internal_value( this->getter_func() );
    }

    /*virtual void increase_value() override {
        NumberControl<DataType>::increase_value();
    }

    virtual void decrease_value() override {
        NumberControl<DataType>::decrease_value();
    }*/

    virtual void change_value(DataType new_value) override {
        if (this->readOnly) return;
        //if (this->debug) { Serial.printf(F("LambdaNumberControl#change_value(%i)..\n"), new_value); Serial_flush(); }
        DataType last_value = this->get_current_value();
        //Serial.println("LambdaNumberControl#change_value about to call set_current_value");
        this->set_current_value(new_value);
        if (this->on_change_handler!=nullptr) {
            //if (this->debug)  { Serial.println(F("LambdaNumberControl calling on_change_handler")); Serial_flush(); }
            (this->on_change_handler)(last_value, this->internal_value);
        }
    }

    // override in subclass if need to do something special eg getter/setter
    virtual DataType get_current_value() override {
        DataType v = this->getter_func();
        if (this->debug) { Serial.printf("%s: LambdaNumberControl#get_current_value(): Called the getter and got value %3.3f!\n", this->label, (float)v); Serial_flush(); }
        return v;
    }

    /*virtual DataType get_internal_value() override {
        //if (this->direct) return NumberControl<DataType>::get_internal_value();
        return get_current_value();
    }*/
    /*virtual const char *getFormattedValue() override {
        Serial.printf("%s: LambdaNumberControl#getFormattedValue()!\n");
        if (this->direct) {
            return this->getFormattedInternalValue();
        }
        return NumberControl<DataType>::getFormattedValue();
        //if (this->debug) Serial.printf("%s:\tNumberControl#getFormattedValue() returning get_current_value=%3.3f\n", this->label, (float)this->get_current_value());
        //return this->getFormattedValue((DataType)this->get_current_value());
    }*/
    /*virtual const char *getFormattedInternalValue() override {
        static char tmp[MAX_LABEL_LENGTH];
        snprintf(tmp, MAX_LABEL_LENGTH-1, "%s", this->getFormattedValue((DataType)this->get_internal_value()));
        if (this->debug) { Serial.printf("%s: LambdaNumberControl#getFormattedInternalValue() returning '%s'\n", this->label, tmp); Serial_flush(); }
        return tmp;
    }*/
    /*virtual DataType get_internal_value() {
        return this->internal_value;
    }*/

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) override { 
        if (this->debug) { Serial.printf("%s: LambdaNumberControl#set_current_value(%3.3f) (%s)\n", this->label, (float)value, this->getFormattedValue(value)); Serial_flush(); }

        //this->set_internal_value(value);
        this->internal_value = value;         // TODO: see if this is needed??
        this->setter_func(value);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set %8s to %s", this->label, this->getFormattedValue(value));
        //Serial.printf("about to set_last_message!");
        menu_set_last_message(msg,GREEN);
    }
};

class LambdaToggleControl : public MenuItem {
    public:
        using setter_func_def = vl::Func<void(bool)>;
        using getter_func_def = vl::Func<bool(void)>;

        setter_func_def setter_func;
        getter_func_def getter_func;

        void (*on_change_handler)(bool last_value, bool new_value) = nullptr;

        LambdaToggleControl(
            const char *label, 
            setter_func_def setter,
            getter_func_def getter,
            void (*on_change_handler)(bool last_value, bool new_value) = nullptr
        ) : MenuItem(label) {
            this->setter_func = setter;
            this->getter_func = getter;
            this->on_change_handler = on_change_handler;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? GREEN : this->default_fg, BLACK);
            //tft->setTextSize(2);        // was 2 ?
            //char tmp[MENU_C_MAX] = "";
            tft->setTextSize(2);

            this->renderValue(selected, opened, MENU_C_MAX);

            return tft->getCursorY();
        }

        // render the current value at current position
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            const char *txt = this->getter_func() ? label_on : label_off;
            //bool use_small = strlen(txt) <= (max_character_width/2);
            //int textSize = use_small ? 2 : 1;
            int textSize = tft->get_textsize_for_width(txt, max_character_width*tft->characterWidth());
            //if (this->debug) Serial.printf(F("%s:\trenderValue '%s' (len %i) with max_character_width %i got textSize %i\n"), this->label, txt, strlen(txt), max_character_width/2, textSize);
            tft->setTextSize(textSize);
            tft->println(txt);
            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            //if (this->debug) Serial.printf(F("LambdaToggleControl#action_opened on %s\n"), this->label);
            bool value = !this->getter_func();
            this->setter_func(value); 
            return false;   // don't 'open'
        }
};


class LambdaActionItem : public MenuItem {
    public:

    char button_label_false[MAX_LABEL_LENGTH] = "";
    char button_label_true[MAX_LABEL_LENGTH] = "";
    //char button_label[MAX_LABEL_LENGTH]         = "                             ";

    /*using setter_def_2 = void(TargetClass::*)();
    using setter_def = void(TargetClass::*)(bool);
    using getter_def = bool(TargetClass::*)();

    //void(TargetClass::*setter)(bool) = nullptr; 
    //bool(TargetClass::*getter)() = nullptr;
    //get_label_def *get_label = nullptr; //void(TargetClass::*setter)(bool) = nullptr;
    setter_def setter = nullptr;
    setter_def_2 setter2 = nullptr;
    getter_def getter = nullptr;*/
    using setter_2_def = vl::Func<void(void)>;  // callback with no parameters
    using getter_def = vl::Func<bool(void)>;

    //vl::Func<void(bool)>    setter_func;
    setter_2_def setter_func_2;
    getter_def getter_func;
    bool has_getter = false, has_setter = false, has_setter_2 = false;

    /*LambdaActionItem(const char *label, vl::Func<void(bool)> setter_func) : 
        MenuItem(label) {
            this->setter_func = setter_func;
            this->has_setter = true;
        }*/
    LambdaActionItem(const char *label, setter_2_def setter_func_2) : 
        MenuItem(label) {
            this->setter_func_2 = setter_func_2;
            this->has_setter_2 = true;
        }

    LambdaActionItem(const char *label, setter_2_def setter_func_2, getter_def getter_func, const char *button_label_true, const char *button_label_false = nullptr) 
        : LambdaActionItem(label, setter_func_2) {
        this->getter_func = getter_func;
        this->has_getter = true;

        static const char *label_1_format = "<< %s >>";
        static const char *label_2_format = ">> %s <<";

        if (button_label_true!=nullptr) {
            strncpy(this->button_label_true, button_label_true, MAX_LABEL_LENGTH);
        } else
            snprintf(this->button_label_true, MAX_LABEL_LENGTH, label_1_format, label);

        if (button_label_false!=nullptr)
            snprintf(this->button_label_false, MAX_LABEL_LENGTH, label_2_format, button_label_false);
        else
            snprintf(this->button_label_false, MAX_LABEL_LENGTH, label_2_format, label);
    };

    virtual int display(Coord pos, bool selected, bool opened) override {
        this->tft->setCursor(pos.x, pos.y);
        return this->renderValue(selected, opened, MENU_C_MAX);
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        char *button_label = nullptr;
        if (this->has_getter && this->button_label_true[0]) {
            button_label = this->getter_func() ? this->button_label_true : this->button_label_false;
        } else if (button_label_false[0]) {
            //Serial.printf(F("%s: rendering button_label_false '%s'\n"), this->label, this->button_label_false);
            button_label = this->button_label_false;
        } else {
            button_label = label;
        }
        //int y = header(button_label, Coord(this->tft->getCursorX(), this->tft->getCursorY()), selected, opened);
        colours(selected);

        // determine size font to use
        bool use_small = strlen(button_label) <= (max_character_width/2);
        int textSize = use_small ? 2 : 1;
        tft->setTextSize(textSize);

        tft->println(button_label);
        const int y = tft->getCursorY();
        return y;
    }

    virtual void on_open() {
        /*if (this->has_setter)
            this->setter_func(true);*/
        if (this->has_setter_2)
            this->setter_func_2();
    }

    virtual bool action_opened() override {
        //Serial.println(F("LambdaActionItem#action_opened"));
        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }

    virtual bool is_openable () {
        return false;
    }
};


class LambdaActionConfirmItem : public LambdaActionItem {
    public:

    LambdaActionConfirmItem(
        const char *label, 
        setter_2_def setter_func_2
    ) : LambdaActionItem(label, setter_func_2) {
        this->go_back_on_select = true;
    };

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        //Serial.println("LambdaActionConfirmItem#renderValue..");
        const char *button_label = opened ? sure_message : this->label;

        this->colours(selected);

        // determine size font to use
        bool use_small = strlen(button_label) <= (max_character_width/2);
        int textSize = use_small ? 2 : 1;
        this->tft->setTextSize(textSize);

        this->tft->println(button_label);

        const int y = this->tft->getCursorY();
        return y;
    }

    virtual bool action_opened() override {
        //Serial.println(F("ActionConfirmItem#action_opened"));
        //this->on_open();
        return true; 
    }

    /*virtual void on_open() override {
        (this->target_object->*setter)();
    }*/

    virtual bool button_select() override {
        //Serial.println(F("ActionConfirmItem#button_select"));

        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, this->label);
        //Serial.printf("about to set_last_message!");
        //msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return this->go_back_on_select;    // return to menu
    }

};

// you might also call this LambdaMenuItem
class CallbackMenuItem : public MenuItem {
    using label_callback_def = vl::Func<const char*(void)>;
    using colour_callback_def = vl::Func<uint16_t(void)>;

    public:
    label_callback_def label_callback_func;
    colour_callback_def colour_callback_func = [=]() -> uint16_t { return C_WHITE; };

    CallbackMenuItem(const char *label, label_callback_def label_callback_func, bool show_header = true) : 
        MenuItem(label, false, show_header), 
        label_callback_func(label_callback_func) {
        this->selectable = false;
    }

    CallbackMenuItem(const char *label, label_callback_def label_callback_func, colour_callback_def colour_callback_func, bool show_header = true) : 
        MenuItem(label, false, show_header), 
        label_callback_func(label_callback_func), colour_callback_func(colour_callback_func) {
        this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header((const char*)this->label_callback_func(), pos, selected, opened);
        tft->setCursor(pos.x,pos.y);

        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const char *txt = (const char*) this->label_callback_func();
        //bool use_small = strlen(txt) <= (max_character_width/2);
        //int textSize = use_small ? 2 : 1;
        int textSize = tft->get_textsize_for_width(txt, max_character_width*tft->characterWidth());
        //if (this->debug) Serial.printf(F("%s:\trenderValue '%s' (len %i) with max_character_width %i got textSize %i\n"), this->label, txt, strlen(txt), max_character_width/2, textSize);
        this->colours(selected, this->colour_callback_func ? this->colour_callback_func() : this->default_fg, BLACK);
        tft->setTextSize(textSize);
        tft->println(txt);
        return tft->getCursorY();
    }
};

#include "menuitems_lambda_selector.h"


#include <functional-vlpp.h>

#include "menuitems_object_multitoggle.h"
// multi-toggle item that targts getters & setters on an object
class MultiToggleItemLambda : public MultiToggleItemBase {
    public:
        using setter_func_def = vl::Func<void(bool)>;
        using getter_func_def = vl::Func<bool(void)>;

        setter_func_def setter = [this](bool v) -> void {};
        getter_func_def getter = [this]() -> bool { return false; };

        MultiToggleItemLambda(const char *label, setter_func_def setter, getter_func_def getter, bool invert_colours = false) 
            : MultiToggleItemBase(label, invert_colours) 
        {
            this->setter = setter;
            this->getter = getter;
        }

        virtual bool do_getter() override {
            return this->getter();
        }
        virtual void do_setter(bool value) override {
            this->setter(value);
        }
};


#endif
