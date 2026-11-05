#pragma once

#include "menu.h"

template<class TargetClass, class DataType>
class ObjectSelectorControl : public NumberControl<int> {

    public:

    struct option {
        DataType value;
        const char *label;
    };

    TargetClass *target_object = nullptr;
    void(TargetClass::*setter)(DataType) = nullptr;
    DataType(TargetClass::*getter)() = nullptr;
    void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;
    OptionList<option> *available_values = nullptr;

    ObjectSelectorControl(
        const char* label, 
        TargetClass *target_object, 
        void(TargetClass::*setter_func)(DataType), 
        DataType(TargetClass::*getter_func)(), 
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : NumberControl<int>(label, go_back_on_select, direct),
        target_object(target_object), setter(setter_func), getter(getter_func), on_change_handler(on_change_handler) {
        this->internal_value = -1;
        this->minimumDataValue = 0;
        this->maximumDataValue = -1;
    }

    virtual void on_add() override {
        NumberControl<int>::on_add();
        this->set_internal_value(this->get_current_value());
    }

    virtual bool action_opened() override {
        if (this->readOnly || this->target_object == nullptr || this->getter == nullptr || this->setter == nullptr)
            return false;
        this->set_internal_value(this->get_current_value());
        return true;
    }

    virtual int get_index_for_value(DataType value) {
        if (this->available_values == nullptr) return -1;

        int index = 0;
        for (auto& entry : *available_values) {
            if (entry.value == value)
                return index;
            ++index;
        }
        return -1;
    }
    virtual const DataType get_value_for_index(int index) {
        if (!this->is_valid_index(index))
            return DataType{};
        return available_values->get(index).value;
    }
    virtual const char *get_label_for_value(DataType value) {
        return this->get_label_for_index(this->get_index_for_value(value));
    }
    virtual const char *get_label_for_index(int index) {
        if (!this->is_valid_index(index))
            return "N/A";
        return available_values->get(index).label;
    }

    virtual const char *getFormattedInternalValue() override {
        return this->get_label_for_index(this->get_internal_value());
    }
    virtual const char *getFormattedValue() override {
        return this->get_label_for_index(this->get_current_value());
    }

    virtual void set_internal_value(int index) override {
        this->internal_value = this->is_valid_index(index) ? index : -1;
    }

    virtual void increase_value() override {
        if (this->readOnly || this->available_values == nullptr || this->available_values->empty())
            return;

        int index = this->get_internal_value();
        if (index >= this->getMaximumDataValue())
            return;

        this->set_internal_value(index + 1);
        if (this->direct)
            this->change_value(this->get_internal_value());
    }
    virtual void decrease_value() override {
        if (this->readOnly || this->available_values == nullptr || this->available_values->empty())
            return;

        int index = this->get_internal_value();
        if (index == this->getMinimumDataValue())
            return;

        this->set_internal_value(index < 0 ? 0 : index - 1);
        if (this->direct)
            this->change_value(this->get_internal_value());
    }

    virtual int get_current_value() override {
        if (this->target_object == nullptr || this->getter == nullptr)
            return -1;
        return this->get_index_for_value((this->target_object->*this->getter)());
    }

    virtual void change_value(int index) override {
        if (this->readOnly || this->target_object == nullptr || this->getter == nullptr || this->setter == nullptr || !this->is_valid_index(index))
            return;

        DataType last_value = (this->target_object->*this->getter)();
        this->set_internal_value(index);
        this->set_current_value(index);
        if (this->on_change_handler != nullptr)
            this->on_change_handler(last_value, (this->target_object->*this->getter)());
    }

    virtual void set_current_value(int index) override {
        if (this->readOnly || this->target_object == nullptr || this->getter == nullptr || this->setter == nullptr || !this->is_valid_index(index))
            return;

        DataType value = this->get_value_for_index(index);
        (this->target_object->*this->setter)(value);

        char message[MENU_MESSAGE_MAX];
        snprintf(message, MENU_MESSAGE_MAX, "Set %8s to %s", this->label, this->get_label_for_value(value));
        menu_set_last_message(message, GREEN);
    }

    virtual OptionList<option> *setup_available_values() {
        if (this->available_values==nullptr)
            this->available_values = new OptionList<option>();
        return this->available_values;
    }

    virtual void add_available_value(DataType value, const char *label) {
        if (this->available_values == nullptr)
            this->setup_available_values();
        available_values->add(option { value, label });
        this->update_index_range();
    }

    virtual OptionList<option> *get_available_values() {
        if (this->available_values==nullptr)
            this->setup_available_values();
        return this->available_values;
    }
    virtual void set_available_values(OptionList<option> *available_values) {
        this->available_values = available_values;
        this->update_index_range();
        this->set_internal_value(this->get_current_value());
    }
    virtual void set_available_values(labelled_value_list_t<DataType> *available_values) {
        if (this->available_values==nullptr)
            this->setup_available_values();
        this->available_values->clear();
        if (available_values != nullptr) {
            for (auto& entry : *available_values)
                this->add_available_value(entry.value, entry.label);
        }
        this->update_index_range();
        this->set_internal_value(this->get_current_value());
    }

    protected:
    bool is_valid_index(int index) const {
        return this->available_values != nullptr && index >= 0 && index < static_cast<int>(this->available_values->size());
    }

    void update_index_range() {
        this->minimumDataValue = 0;
        this->maximumDataValue = this->available_values == nullptr ? -1 : static_cast<int>(this->available_values->size()) - 1;
    }
};
