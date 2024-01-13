#pragma once

#include <Arduino.h>
#include "LinkedList.h"

#define MAX_MESSAGES_LOG 20
extern LinkedList<String> *messages_log;
void messages_log_add(String msg);
void messages_log_clear();

void setup_messages_menu();