#ifdef ENABLE_SCREEN

#include "menu_messages.h"

LinkedList<String> *messages_log = new LinkedList<String>();

void messages_log_add(String msg) {
  messages_log->add(msg);
  if (messages_log->size() >= MAX_MESSAGES_LOG) {
    messages_log->unlink(0);
  }
}

void messages_log_clear() {
  messages_log->clear();
}

#include "menuitems_action.h"
#include "menuitems_listviewer.h"

void setup_messages_menu() {
    menu->add_page("Messages");
    menu->add(new ActionConfirmItem("Clear", messages_log_clear));
    menu->add(new ListViewerMenuItem("Message history", messages_log));
}

#endif