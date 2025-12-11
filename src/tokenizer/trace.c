#include "trace.h"

Trace stretch(Trace a, const Trace b) {
    a.source.size = b.source.data - a.source.data + b.source.size;
    if(a.source.size >= 256) a.source.size = 16;
    return a;
}

bool print_message(const Message message) {
    if(!message.trace.filename) {
        // TODO: see #1
        printf("\33[1m%s%s:\33[0m %.*s\n", message.highlight, message.label, PRINT(message.content));
        return 0;
    }

    char* line_start = message.trace.line_start;
    size_t offset = message.trace.source.data - message.trace.line_start;
    while(*line_start && *line_start <= ' ') {
        line_start++;
        offset--;
    }

    char underline[offset + message.trace.source.size + 1];
    for(size_t i = 0; i < sizeof(underline) - 1; i++) {
        underline[i] = i >= offset ? '~' : ' ';
    }
    underline[sizeof(underline) - 1] = '\0';

    // TODO: see #1
    printf("\33[1m%s:%u:%u: %s%s:\33[0m %.*s\n%4d | %s\n     : %s%.*s\33[0m\n",
           message.trace.filename, message.trace.row, message.trace.col,
           message.highlight, message.label, PRINT(message.content),
           message.trace.row, line_start,
           message.highlight, (int) sizeof(underline), underline);

    return message.highlight[3] == '1'; // REPORT_ERR
}
