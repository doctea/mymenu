#ifndef MENUITEMS_LAMBDA_SELECTOR__INCLUDED
#define MENUITEMS_LAMBDA_SELECTOR__INCLUDED

#include "functional-vlpp.h"
#include "menuitems_lambda.h"

template<class DataType=int>
class LambdaSelectorControl : public LambdaNumberControl<DataType> {
    public:
    
    struct option {
        DataType value;
        const char *label;
    };

    LinkedList<option> *available_values = nullptr; //new LinkedList<option>();

    LambdaSelectorControl(
        const char* label, 
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaNumberControl<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select, direct) {
        //if (this->target_object!=nullptr && this->getter!=nullptr)
        //    this->set_internal_value (this->get_index_for_value( (this->target_object->*this->getter)() ));
        this->set_internal_value ( this->getter_func() );
        //this->debug = true;
    }

    virtual bool action_opened() override {
        //Serial.printf("ObjectSelectorControl#action_opened, internal_value is currently %i (%s)\n", this->internal_value, getFormattedValue());
        //Serial.printf("ObjectSelectorControl#action_opened, value from getter is %i\n", (this->target_object->*this->getter)());
        //this->internal_value = this->get_index_for_value((this->target_object->*this->getter)());
        this->internal_value = (DataType)this->get_index_for_value(this->getter_func());
        //Serial.printf("ObjectSelectorControl#action_opened, internal_value setting to %i (%s)\n", this->internal_value, getFormattedValue());
        return !LambdaNumberControl<DataType>::readOnly;
    }

    virtual int get_index_for_value(DataType value) {
        if (this->available_values == nullptr) return 0;

        const uint_fast8_t size = available_values->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            if (available_values->get(i).value==value) {
                //Serial.printf("get_index_for_value(%i) returning %i\n", value, i);
                return i;
            }
        }
        return -1;
    }
    virtual const DataType get_value_for_index(int index) {
        if (this->available_values == nullptr) return (DataType)0;

        if (index<0 || index>=(int)available_values->size())
            return (DataType)0;
        //Serial.printf("get_value_for_index(%i) returning %i/%3.3f\n", index, available_values->get(index).value, available_values->get(index).value);
        return available_values->get(index).value;
    }
    virtual const char*get_label_for_value(DataType value) {
        /*static char value_label[MENU_C_MAX];
        sprintf(value_label, "%i", value);
        return value_label;*/
        int index = get_index_for_value((DataType)value);
        return this->get_label_for_index(index);
    }
    virtual const char*get_label_for_index(int index) {
        if (this->available_values == nullptr) return 0;
        if (index<0 || index>=(int)available_values->size())
            return "N/A";
        //Serial.printf("get_label_for_index(%i) returning '%s'\n", index, available_values.get(index).label);
        return available_values->get(index).label;
    }

    virtual const char *getFormattedInternalValue() override {
        return this->get_label_for_index(this->get_internal_value());
    }
    virtual const char *getFormattedValue() override {
        return this->get_label_for_value(this->get_current_value());
    }

    virtual void increase_value() override {
        int idx = this->get_internal_value();

        if ((DataType)idx>=this->getMaximumDataValue()) 
            return; //idx = this->getMaximumDataValue(); //available_values->size();

        idx++;

        //Serial.printf("%s: increase_value got new idx %i (corresponding to value %s)\n", this->label, idx, this->get_label_for_index(idx));
        this->set_internal_value((DataType)idx);
        if (this->direct) {
            if (this->debug) Serial.printf("%s: increase_value got new idx %i (corresponding to label %s and value %i)\n", this->label, idx, this->get_label_for_index(idx), this->get_value_for_index(idx));
            this->set_current_value((DataType)idx);
        }
    }
    virtual void decrease_value() override {
        int idx = this->get_internal_value();

        if (idx==this->getMinimumDataValue())   // for protection against unsigned values wrapping around
            return;

        idx--;

        if ((DataType)idx<this->getMinimumDataValue()) 
            idx = this->getMinimumDataValue();

        this->set_internal_value((DataType)idx);
        if (this->direct) {
            if (this->debug) Serial.printf("%s: decrease_value got new idx %i (corresponding to label %s and value %i)\n", this->label, idx, this->get_label_for_index(idx), this->get_value_for_index(idx));
            this->set_current_value((DataType)idx);
        }
    }

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType index) override { 
        //this->internal_value = value;
        //if (this->debug) { Serial.printf(F("ObjectSelectorControl#set_current_value() passed value %i "), value); Serial_flush(); }
        if (this->debug) { 
            Serial.printf("LambdaSelectorControl '%s'#set_current_value() passed index argument %i/%3.3f", this->label, index, index); Serial_flush(); 
            Serial.printf("\tconverted to value: %i/%3.3f\n", this->get_value_for_index(index), this->get_value_for_index(index));
        }
        //Serial.printf("ObjectSelectorControl#set_current_value() with index %i ", value); Serial_flush(); 
        // TODO: shouldn't this be get_index_for_value..?
        DataType value = this->get_value_for_index(index);
        //Serial.printf(F("\tConverted to value_for_index %i\n"), value); Serial_flush(); 
        this->setter_func(value);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        // todo: fix compiler warning / type of value, may need to override or just only print FormattedValue to the message and not the real value
        snprintf(msg, MENU_MESSAGE_MAX, "Set %8s to %s (%i)", this->label, get_label_for_value(value), index);
        //Serial.printf("about to set_last_message!");
        //msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);
        if (this->debug) { Serial.println(F("Done.")); Serial_flush(); }
    }

    virtual LinkedList<option> *setup_available_values() {
        if (this->available_values==nullptr)
            this->available_values = new LinkedList<option>();
        return this->available_values;
    }

    virtual void add_available_value(DataType value, const char *label) {
        if (this->available_values == nullptr) 
            this->setup_available_values();
        available_values->add(option { .value = value, .label = label });
        this->minimumDataValue = (DataType)0;
        this->maximumDataValue = (DataType)(available_values->size() - 1);
    }

    virtual LinkedList<option> *get_available_values() {
        if (this->available_values==nullptr)
            this->setup_available_values();
        return this->available_values;
    }
    virtual void set_available_values(LinkedList<option> *available_values) {
        this->available_values = available_values;
        if (available_values!=nullptr) {
            this->maximumDataValue = (DataType)(available_values->size() - 1);
        }
    }

};

#endif