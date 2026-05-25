#ifdef ENABLE_SCREEN

#include "menu_messages.h"

CircularMessageLog message_log;  // static allocation in BSS, zero-initialised

void messages_log_add(const char* msg) {
    if (Serial) {
        Serial.print("messages_log_add: ");
        Serial.println(msg);
    }
    message_log.add(msg);
}

void messages_log_add(String msg) {
    messages_log_add(msg.c_str());
}

void messages_log_add_fmt(const char* fmt, ...) {
    char buf[MAX_MESSAGE_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, MAX_MESSAGE_LENGTH, fmt, args);
    va_end(args);
    messages_log_add(buf);
}

void messages_log_clear() {
    message_log.clear();
}

#include "menuitems_action.h"
#include "menuitems_listviewer.h"

void setup_messages_menu() {
    menu->add_page("Messages");
    menu->add(new ActionConfirmItem("Clear", messages_log_clear));
    menu->add(new CircularListViewerMenuItem("Message history", &message_log));
}

#endif