#ifndef MENUITEMS_OBJECT__INCLUDED
#define MENUITEMS_OBJECT__INCLUDED

//#include "menuitems.h"

template<class TargetClass, class DataType>
class ObjectNumberControl : public NumberControl<DataType> {
    public:
    bool debug = false;
    DataType internal_value;

    void(TargetClass::*setter)(DataType) = nullptr;
    DataType(TargetClass::*getter)() = nullptr;
    void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;
    TargetClass *target_object = nullptr;

    ObjectNumberControl(const char* label, 
                        TargetClass *target_object, 
                        void(TargetClass::*setter_func)(DataType), 
                        DataType(TargetClass::*getter_func)(), 
                        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr
                ) : NumberControl<DataType>(label) {
        this->target_object = target_object;
        this->getter = getter_func;
        this->setter = setter_func;
        this->on_change_handler = on_change_handler;
        this->minimum_value = 0;
        this->maximum_value = 100;

        if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( (this->target_object->*getter)() );
    }
    ObjectNumberControl(const char* label, 
                        TargetClass *target_object, 
                        void(TargetClass::*setter_func)(DataType), 
                        DataType(TargetClass::*getter_func)(), 
                        void (*on_change_handler)(DataType last_value, DataType new_value),
                        DataType minimum_value,
                        DataType maximum_value
                ) : ObjectNumberControl<TargetClass,DataType>(label, target_object, setter_func, getter_func, on_change_handler) {
        this->minimum_value = minimum_value;
        this->maximum_value = maximum_value;
    }

    virtual void on_add() override {
        NumberControl<DataType>::on_add();
        if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( (this->target_object->*getter)() );
    }

    virtual DataType get_internal_value() override {
        return this->internal_value;
    }

    virtual void set_internal_value(DataType value) {
        if (this->debug) { Serial.printf("ObjectNumberControl.set_internal_value(%i)..\n", value); }
        this->internal_value = constrain(value, this->minimum_value, this->maximum_value);
    }

    virtual void change_value(DataType new_value) override {
        if (this->readOnly) return;
        if (this->debug) { Serial.printf("ObjectNumberControl#change_value(%i)..\n", new_value); Serial.flush(); }
        DataType last_value = get_current_value();
        //Serial.println("ObjectNumberControl#change_value about to call set_current_value");
        this->set_current_value(new_value);
        if (on_change_handler!=nullptr) {
            if (this->debug)  { Serial.println("ObjectNumberControl calling on_change_handler"); Serial.flush(); }
            on_change_handler(last_value, internal_value);
        }
    }

    // override in subclass if need to do something special eg getter/setter
    virtual DataType get_current_value() override {
        if (this->target_object!=nullptr && this->getter!=nullptr) {
            if (this->debug) { Serial.printf("ObjectNumberControl#get_current_value in %s about to call getter\n", this->label); Serial.flush(); }
            DataType v = (this->target_object->*getter)();
            if (this->debug) { Serial.printf("Called getter and got value %i!", v); Serial.flush(); }
            return v;
        }
        
        return 0;
    }

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) override { 
        //this->internal_value = value;
        if (this->debug) { Serial.printf("ObjectNumberControl#set_current_value(%i)\n", value); Serial.flush(); }
        if (this->target_object!=nullptr && this->setter!=nullptr) {
            (this->target_object->*setter)(value);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %8s to %i", this->label, value);
            //Serial.printf("about to set_last_message!");
            msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg,GREEN);
        }
        if (this->debug) { Serial.println("Done."); Serial.flush(); }
    }
};


template<class TargetClass>
class ObjectToggleControl : public MenuItem {
    public:
        TargetClass *target_object;

        void(TargetClass::*setter)(bool) = nullptr;
        bool(TargetClass::*getter)() = nullptr;
        void (*on_change_handler)(int last_value, int new_value) = nullptr;

        ObjectToggleControl(
            const char *label, 
            TargetClass *target_object, 
            void(TargetClass::*setter)(bool), 
            bool(TargetClass::*getter)(), 
            void (*on_change_handler)(int last_value, int new_value)
        ) : MenuItem(label) {
            this->target_object = target_object;
            this->setter = setter;
            this->getter = getter;
            this->on_change_handler = on_change_handler;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? GREEN : C_WHITE, BLACK);
            //tft->setTextSize(2);        // was 2 ?
            //char tmp[20] = "";
            tft->setTextSize(2);

            this->renderValue(selected, opened, MENU_C_MAX);

            return tft->getCursorY();
        }

        // render the current value at current position
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            const char *txt = (this->target_object->*getter)() ? "On" : "Off";
            tft->setTextSize((strlen(txt) < max_character_width/2) ? 2 : 1);
            tft->println(txt);
            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            if (this->debug) Serial.printf("ObjectToggleControl#action_opened on %s\n", this->label);
            bool value = !(this->target_object->*getter)();
            //this->internal_value = !this->internal_value;

            (this->target_object->*setter)(value); //(bool)this->internal_value);
            return false;   // don't 'open'
        }
};


template<class TargetClass>
class ObjectActionItem : public MenuItem {
    public:

    TargetClass *target_object;
    char button_label_false[MAX_LABEL_LENGTH];//   = "                               ";
    char button_label_true[MAX_LABEL_LENGTH];//    = "                               ";
    //char button_label[MAX_LABEL_LENGTH]         = "                             ";

    using setter_def = void(TargetClass::*)(bool);
    //using get_label_def = char*(TargetClass::*)(bool); //, get_label_def *get_label
    using getter_def = bool(TargetClass::*)();

    //void(TargetClass::*setter)(bool) = nullptr; 
    //bool(TargetClass::*getter)() = nullptr;
    //get_label_def *get_label = nullptr; //void(TargetClass::*setter)(bool) = nullptr;
    setter_def setter;
    getter_def getter;

    ObjectActionItem(const char *label, TargetClass *target_object, setter_def setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : MenuItem(label) 
    {
        this->target_object = target_object;
        this->setter = setter;
        this->getter = getter;

        //while(1) {

        if (button_label_true!=nullptr) {
            //while(1) {
            //    Serial.printf("Setting button_label_true to '%s'\n", button_label_true);
            //}
            
            strcpy(this->button_label_true, button_label_true);
        } else
            //strcpy(this->button_label_true, label);
            sprintf(this->button_label_true, "<< %s >>", label);

        //while(1) {
        //Serial.printf("before button_label_false in ObjectActionItem constructor, button_label_true is '%s' from '%s'\n", this->button_label_true, button_label_true);
        //}

        if (button_label_false!=nullptr)
            sprintf(this->button_label_false, ">> %s <<", button_label_false);
        else
            sprintf(this->button_label_false, ">> %s <<", label);

        //Serial.printf("after button_label_false in ObjectActionItem constructor, button_label_true is '%s' from '%s'\n", this->button_label_true, button_label_true);

        //}

    };
    //ObjectActionItem(const char *label, TargetClass *target_object, setter_def *setter, getter_def *getter, const char *button_label_true) 
    //    : ObjectActionItem(label, target_object, setter, getter, button_label_true, label) {}; 

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.printf("button_label_true is %s\n", this->button_label_true);
        //char *button_label = this->target_object->*get_label( this->target_object->*getter() );
        char *button_label = (this->target_object->*getter)() ? this->button_label_true : this->button_label_false;
        pos.y = header(button_label, pos, selected, opened);

        tft->setCursor(pos.x,pos.y);
        tft->setTextSize(1);

        //colours(opened, opened ? GREEN : C_WHITE, BLACK);

        return tft->getCursorY();
    }

    void on_open() {
        (this->target_object->*setter)(true);
    }

    virtual bool action_opened() override {
        Serial.println("ObjectActionItem#action_opened");
        this->on_open();

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Fired %8s", label);
        //Serial.printf("about to set_last_message!");
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }

};


#include "menuitems_object_selector.h"

#endif
