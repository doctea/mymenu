#ifndef MENUITEMS_OBJECT_SELECTOR__INCLUDED
#define MENUITEMS_OBJECT_SELECTOR__INCLUDED

template<class TargetClass, class DataType>
class ObjectSelectorControl : public ObjectNumberControl<TargetClass,DataType> {

    struct option {
        DataType value;
        const char *label;
    };

    LinkedList<option> available_values = LinkedList<option>();

    public:

    ObjectSelectorControl(const char* label, 
                        TargetClass *target_object, 
                        void(TargetClass::*setter_func)(DataType), 
                        DataType(TargetClass::*getter_func)(), 
                        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr
    ) : ObjectNumberControl<TargetClass,DataType>(label, target_object, setter_func, getter_func, on_change_handler) {
        this->set_internal_value (this->get_index_for_value( (this->target_object->*this->getter)() ));
        //this->debug = true;
    }

    bool action_opened() override {
        this->internal_value = this->get_index_for_value((this->target_object->*this->getter)());
        return ObjectNumberControl<TargetClass,DataType>::action_opened();
    }

    virtual void add_available_value(DataType value, const char *label) {
        available_values.add(option { .value = value, .label = label });
        this->maximum_value = available_values.size() - 1;
    }

    /*virtual int get_index_for_value(int value) {
        if (value<0 || value>=available_values.size())
            return -1;
        const int size = available_values.size();
        for (int i = 0 ; i < size ; i++) {
            if(available_values.get(i).value==value)
                return i;
        }
        return -1;
    }*/

    virtual int get_index_for_value(DataType value) {
        int size = available_values.size();
        for (int i = 0 ; i < size ; i++) {
            if (available_values.get(i).value==value) {
                //Serial.printf("get_index_for_value(%i) returning %i\n", value, i);
                return i;
            }
        }
        return -1;
    }
    virtual const DataType get_value_for_index(int index) {
        if (index<0 || index>=available_values.size())
            return 0;
        //Serial.printf("get_value_for_index(%i) returning %i\n", index, available_values.get(index).value);
        return available_values.get(index).value;
    }
    virtual const char*get_label_for_value(int value) {
        /*static char value_label[MENU_C_MAX];
        sprintf(value_label, "%i", value);
        return value_label;*/
        int index = get_index_for_value(value);
        return this->get_label_for_index(index);
    }
    virtual const char*get_label_for_index(int index) {
        if (index<0 || index>=available_values.size())
            return "N/A";
        //Serial.printf("get_label_for_index(%i) returning '%s'\n", index, available_values.get(index).label);
        return available_values.get(index).label;
    }

    virtual const char *getFormattedInternalValue() override {
        return this->get_label_for_index(this->get_internal_value());
    }
    virtual const char *getFormattedValue() override {
        return this->get_label_for_value(this->get_current_value());
    }

    // override in subclass if need to do something special eg getter/setter
    /*virtual DataType get_current_value() override {
        if (this->target_object!=nullptr && this->getter!=nullptr) {
            if (this->debug) { Serial.printf("ObjectNumberControl#get_current_value in %s about to call getter\n", this->label); Serial.flush(); }
            DataType v = (this->target_object->*this->getter)();
            Serial.printf("get_current_value from getter got %i, returning index %i\n", v, this->get_index_for_value(v));
            if (this->debug) { Serial.println("Called getter!"); Serial.flush(); }

            return this->get_index_for_value(v);
        }
        
        return 0;
    }*/

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) override { 
        //this->internal_value = value;
        //if (this->debug) { 

        //}
        if (this->target_object!=nullptr && this->setter!=nullptr) {
            //Serial.printf("ObjectSelectorControl#set_current_value() with index %i ", value); Serial.flush(); 
            value = this->get_value_for_index(value);
            //Serial.printf("and value_for_index %i\n", value); Serial.flush(); 
            (this->target_object->*this->setter)(value);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %8s to %s", this->label, get_label_for_value(value));
            //Serial.printf("about to set_last_message!");
            msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg,GREEN);
        }
        if (this->debug) { Serial.println("Done."); Serial.flush(); }
    }

};

#endif