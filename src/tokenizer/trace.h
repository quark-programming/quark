#ifndef TRACE_H
#define TRACE_H

#include <stdbool.h>
#include <stdio.h>

#include <wrap.h>
#include <helpers.h>

typedef struct Trace {
    str source;
    const char* filename;
    char* line_start;
    u32 row, col;
} Trace;

typedef struct Message {
    Trace trace;
    str content;
    u8 tty_color;
    u8 label;
} Message;

extern const char* global_message_labels[];
extern bool global_stdout_only;

Trace stretch(Trace a, Trace b);

bool print_message(Message message, bool newline);

#define MERROR(trace, content) ((Message) { trace, content, 1, 0 })
#define MHINT(content) ((Message) { { 0 }, content, 6, 1 })
#define MINFO(trace, content) ((Message) { trace, content, 6, 2 })
#define MWARN(trace, content) ((Message) { trace, content, 3, 3 })

#define HERR "\33[31m"
#define HHNT "\33[36m"
#define HINF "\33[36m"
#define HWRN "\33[33m"
#define H "\33[0m"

#endif
