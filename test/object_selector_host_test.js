const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const include = path.resolve(__dirname, '..', 'include');
function read(name) {
    return fs.readFileSync(path.join(include, name), 'utf8');
}
function extract(text, startMarker, endMarker) {
    const start = text.indexOf(startMarker);
    const end = text.indexOf(endMarker, start);
    if (start < 0 || end < 0) throw new Error(`Cannot locate ${startMarker}`);
    return text.slice(start, end);
}
function withoutIncludes(text) {
    return text.replace(/^\s*#(?:include|pragma)[^\n]*\n/gm, '');
}

const source = `
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>
#define F(value) value
#define Serial_flush() ((void)0)
#define constrain(value, minimum, maximum) ((value) < (minimum) ? (minimum) : ((value) > (maximum) ? (maximum) : (value)))
#define MENU_PERF_PARTIAL_UPDATES 1
#define IF_MENU_PERF_PARTIAL_UPDATES(...) __VA_ARGS__
static constexpr int REDRAW_ON_OWN_INPUT = 1, REDRAW_ON_CUSTOM = 2;
static constexpr int MAX_LABEL_LENGTH = 32, MENU_MESSAGE_MAX = 128;
static constexpr uint16_t GREEN = 1, C_WHITE = 2, BLACK = 0;
static const char *label_on = "On", *label_off = "Off";
static uint32_t millis() { return 1000; }
struct SerialMock {
    template<typename... Args> void printf(const char *, Args...) {}
    void println(const char *) {}
} Serial;
struct Coord { int x = 0, y = 0; };
struct DisplayMock {
    int currentCharacterWidth() { return 6; }
    void setCursor(int, int) {}
    template<typename... Args> void printf(const char *, Args...) {}
    void println() {}
    void println(const char *) {}
    int get_c_max() { return 40; }
    int getCursorY() { return 0; }
    bool isTextWrap() { return false; }
    void setTextWrap(bool) {}
    uint8_t get_textsize_for_width(const char *, int) { return 1; }
    void setTextSize(uint8_t) {}
} display_mock;
struct MenuItem {
    const char *label;
    bool debug = false;
    uint16_t default_fg = C_WHITE, default_bg = BLACK;
    struct { bool go_back_on_select = false; } flags;
    DisplayMock *tft = &display_mock;
    explicit MenuItem(const char *label) : label(label) {}
    virtual ~MenuItem() = default;
    virtual void on_add() {}
    virtual int display(Coord, bool, bool) { return 0; }
    virtual int renderValue(bool, bool, uint16_t) { return 0; }
    virtual bool knob_left() { return false; }
    virtual bool knob_right() { return false; }
    virtual bool button_select() { return false; }
    virtual bool action_opened() { return false; }
    virtual bool check_needs_redraw_custom(bool, bool) { return false; }
    void add_redraw_policy(int) {}
    int header(const char *, Coord, bool, bool) { return 0; }
    void colours(bool, uint16_t = 0, uint16_t = 0) {}
};
static char last_message[MENU_MESSAGE_MAX];
static void menu_set_last_message(const char *message, int) {
    std::snprintf(last_message, sizeof(last_message), "%s", message);
}
${extract(read('menuitems.h'), 'template<typename T>\nclass OptionList', '// basic line')}
${withoutIncludes(read('menuitems_numbers.h')).replace('getFormattedValue(int32_t value)', 'getFormattedValue(long value)')}
${extract(read('menuitems_object.h'), 'template<class TargetClass, class DataType>', '\ntemplate<class TargetClass, class DataType = bool>')}
${withoutIncludes(read('menuitems_object_selector.h'))}

enum Rule { Off = 0, Slow = 10, Fast = 40, Unknown = 99 };
enum class ScopedRule { First = -20, Second = 800 };
template<typename Value> struct Target {
    Value value;
    unsigned writes = 0, reads = 0;
    explicit Target(Value value) : value(value) {}
    Value get() { ++reads; return value; }
    void set(Value next) { value = next; ++writes; }
};
using Control = ObjectSelectorControl<Target<Rule>, Rule>;
static_assert(std::is_base_of<NumberControl<int>, Control>::value, "Selector editing state must use integer indices");
static std::vector<Rule> previous_values, next_values;
static void on_change(Rule previous, Rule next) {
    previous_values.push_back(previous);
    next_values.push_back(next);
}
static void populate(Control &control) {
    control.add_available_value(Off, "Off");
    control.add_available_value(Slow, "Slow");
    control.add_available_value(Fast, "Fast");
}
static void check_deferred_edits() {
    Target<Rule> target(Fast);
    Control control("Rule", &target, &Target<Rule>::set, &Target<Rule>::get, on_change, true);
    assert(target.reads == 0);
    assert(control.get_internal_value() == -1);
    OptionList<Control::option> options;
    control.set_available_values(&options);
    populate(control);
    control.on_add();
    assert(control.get_internal_value() == 2);
    assert(control.get_current_value() == 2);
    assert(std::strcmp(control.getFormattedValue(), "Fast") == 0);
    assert(control.action_opened());
    control.knob_left();
    assert(control.get_internal_value() == 1 && target.writes == 0);
    assert(std::strcmp(control.getFormattedInternalValue(), "Slow") == 0);
    assert(std::strcmp(control.getFormattedValue(), "Fast") == 0);
    assert(control.button_select());
    assert(target.value == Slow && target.writes == 1);
    assert(previous_values.back() == Fast && next_values.back() == Slow);
    assert(std::strstr(last_message, "Slow") != nullptr);
    target.value = Off;
    control.action_opened();
    assert(control.get_internal_value() == 0);
    control.knob_left();
    assert(control.get_internal_value() == 0 && target.writes == 1);
    control.setReadOnly();
    assert(!control.action_opened());
    control.knob_right();
    control.increase_value();
    control.change_value(2);
    control.set_current_value(2);
    control.button_select();
    assert(target.writes == 1 && target.value == Off);
}
static void check_direct_edits() {
    Target<Rule> target(Off);
    Control control("Rule", &target, &Target<Rule>::set, &Target<Rule>::get, on_change, false, true);
    OptionList<Control::option> options;
    control.set_available_values(&options);
    populate(control);
    control.action_opened();
    control.knob_right();
    assert(target.value == Slow && target.writes == 1);
    assert(previous_values.back() == Off && next_values.back() == Slow);
    control.knob_right();
    assert(target.value == Fast && target.writes == 2);
    control.knob_right();
    assert(target.writes == 2);
    control.knob_left();
    assert(target.value == Slow && target.writes == 3);
    assert(previous_values.back() == Fast && next_values.back() == Slow);
}
static void check_missing_values_and_options() {
    Target<Rule> target(Unknown);
    Control control("Rule", &target, &Target<Rule>::set, &Target<Rule>::get, on_change);
    assert(control.get_index_for_value(Unknown) == -1);
    assert(std::strcmp(control.get_label_for_index(-1), "N/A") == 0);
    control.action_opened();
    control.increase_value();
    control.decrease_value();
    control.button_select();
    assert(target.writes == 0 && control.get_internal_value() == -1);
    OptionList<Control::option> options;
    control.set_available_values(&options);
    populate(control);
    control.action_opened();
    assert(control.get_internal_value() == -1);
    assert(std::strcmp(control.getFormattedValue(), "N/A") == 0);
    control.button_select();
    control.change_value(10);
    control.set_current_value(-1);
    assert(target.writes == 0);
    control.knob_left();
    assert(control.get_internal_value() == 0);
    control.button_select();
    assert(target.value == Off && target.writes == 1);
    target.value = Unknown;
    control.action_opened();
    control.knob_right();
    assert(control.get_internal_value() == 0);
    control.set_internal_value(2);
    options.clear();
    control.button_select();
    assert(target.writes == 1);
    control.set_available_values(static_cast<OptionList<Control::option> *>(nullptr));
    assert(control.get_internal_value() == -1 && control.getMaximumDataValue() == -1);
    control.change_value(0);
    assert(target.writes == 1);
}
static void check_option_replacement() {
    Target<Rule> target(Fast);
    Control control("Rule", &target, &Target<Rule>::set, &Target<Rule>::get);
    labelled_value_t<Rule> entries[] = {{Fast, "Fast"}, {Off, "Off"}};
    labelled_value_list_t<Rule> labelled(entries);
    control.set_available_values(&labelled);
    assert(control.get_internal_value() == 0 && control.getMaximumDataValue() == 1);
    control.change_value(1);
    assert(target.value == Off);
    labelled_value_t<Rule> replacement[] = {{Slow, "Slow"}};
    labelled_value_list_t<Rule> replaced(replacement);
    control.set_available_values(&replaced);
    assert(control.get_internal_value() == -1 && control.getMaximumDataValue() == 0);
    control.change_value(0);
    assert(target.value == Slow);
    control.set_available_values(static_cast<labelled_value_list_t<Rule> *>(nullptr));
    assert(control.get_internal_value() == -1 && control.getMaximumDataValue() == -1);
    delete control.available_values;
    control.available_values = nullptr;
}
template<typename Value> static void check_value_type(Value first, Value second) {
    using TypedControl = ObjectSelectorControl<Target<Value>, Value>;
    Target<Value> target(second);
    TypedControl control("Typed", &target, &Target<Value>::set, &Target<Value>::get);
    OptionList<typename TypedControl::option> options;
    control.set_available_values(&options);
    control.add_available_value(first, "First");
    control.add_available_value(second, "Second");
    control.action_opened();
    assert(control.get_internal_value() == 1);
    control.knob_left();
    control.button_select();
    assert(target.value == first && target.writes == 1);
}
static void check_null_bindings() {
    Target<Rule> target(Off);
    Control absent("Absent", nullptr, &Target<Rule>::set, &Target<Rule>::get);
    Control no_getter("No getter", &target, &Target<Rule>::set, nullptr);
    Control no_setter("No setter", &target, nullptr, &Target<Rule>::get);
    for (Control *control : {&absent, &no_getter, &no_setter}) {
        OptionList<Control::option> options;
        control->set_available_values(&options);
        populate(*control);
        control->on_add();
        control->action_opened();
        control->knob_right();
        control->button_select();
        control->set_available_values(static_cast<OptionList<Control::option> *>(nullptr));
    }
    assert(target.writes == 0);
}
int main() {
    check_deferred_edits();
    check_direct_edits();
    check_missing_values_and_options();
    check_option_replacement();
    check_value_type<ScopedRule>(ScopedRule::First, ScopedRule::Second);
    check_value_type<int8_t>(-12, 100);
    check_value_type<unsigned>(4000, 9000);
    check_value_type<float>(0.25f, 0.75f);
    check_null_bindings();
    std::cout << "Selector indices, typed values/callbacks, direct/deferred edits, bounds, option replacement and null bindings passed\\n";
}
`;

const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'mymenu-selector-'));
try {
    const sourcePath = path.join(temporary, 'selector.cpp');
    const binary = path.join(temporary, process.platform === 'win32' ? 'selector.exe' : 'selector');
    fs.writeFileSync(sourcePath, source);
    const compiler = spawnSync(process.env.CXX || 'g++', [
        '-std=c++11', '-Wall', '-Wextra', '-Werror=format', '-Werror=return-type',
        '-Wno-unused-parameter', '-Wno-unused-variable', '-fno-permissive', '-fmax-errors=8',
        '-fsanitize=undefined', '-fno-sanitize-recover=all', sourcePath, '-o', binary,
    ], { encoding: 'utf8' });
    if (compiler.error) throw compiler.error;
    process.stdout.write(compiler.stdout || '');
    process.stderr.write(compiler.stderr || '');
    if (compiler.status !== 0) process.exitCode = compiler.status || 1;
    else {
        const result = spawnSync(binary, [], { encoding: 'utf8' });
        if (result.error) throw result.error;
        process.stdout.write(result.stdout || '');
        process.stderr.write(result.stderr || '');
        process.exitCode = result.status || (result.signal ? 1 : 0);
    }
} finally {
    fs.rmSync(temporary, { recursive: true, force: true });
}