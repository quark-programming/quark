#ifndef TRACE_H
#define TRACE_H

#include <stdbool.h>
#include <stdio.h>

#include <vector.h>
#include <vector-string.h>

typedef struct Trace {
    String source;
    const char* filename;
    char* line_start;
    unsigned row, col;
} Trace;

typedef struct Message {
    Trace trace;
    const char* highlight;
    const char* label;
    String content;
} Message;

typedef Vector(Message) MessageVector;

Trace stretch(Trace a, Trace b);

bool print_message(Message message);

// TODO(#1): color is normally not a bad practice but at least check if the output is a tty not a file
//  (a user can put > at the end of the command and the escape codes go into files without being escaped so implement
//  a helper function that checks if tty)
#define REPORT_ERR(trace, content) ((Message) { trace, "\33[31m", "error", content })
#define REPORT_HINT(content) ((Message) { { 0 }, "\33[36m", "hint", content })
#define REPORT_INFO(trace, content) ((Message) { trace, "\33[36m", "info", content })
#define REPORT_WARNING(trace, content) ((Message) { trace, "\33[33m", "warning", content })

#endif
