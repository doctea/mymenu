#pragma once

#include <Arduino.h>
#include <stdarg.h>

#define MAX_MESSAGES_LOG 20
#define MAX_MESSAGE_LENGTH 96

struct CircularMessageLog {
    char buf[MAX_MESSAGES_LOG][MAX_MESSAGE_LENGTH];
    int head = 0;   // next write slot
    int count = 0;  // number of valid entries (capped at MAX_MESSAGES_LOG)

    void add(const char* msg) {
        strncpy(buf[head], msg, MAX_MESSAGE_LENGTH - 1);
        buf[head][MAX_MESSAGE_LENGTH - 1] = '\0';
        head = (head + 1) % MAX_MESSAGES_LOG;
        if (count < MAX_MESSAGES_LOG) count++;
    }

    void clear() { head = 0; count = 0; }

    // i=0 is oldest, i=count-1 is newest
    const char* get(int i) const {
        int oldest = (head - count + MAX_MESSAGES_LOG * 2) % MAX_MESSAGES_LOG;
        return buf[(oldest + i) % MAX_MESSAGES_LOG];
    }

    int size() const { return count; }
};

extern CircularMessageLog message_log;
void messages_log_add(const char* msg);
void messages_log_add(String msg);          // backward-compat wrapper
void messages_log_add_fmt(const char* fmt, ...);
void messages_log_clear();

void setup_messages_menu();