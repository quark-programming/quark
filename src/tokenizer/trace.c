#include <tty.h>
#include "trace.h"

const char* global_message_labels[] = { "error", "hint", "info", "warning" };
bool global_stdout_only = false;

Trace stretch(Trace a, const Trace b) {
    a.source.len = b.source.data - a.source.data + b.source.len;
    if(a.source.len >= 256) a.source.len = 16;
    return a;
}

bool print_message(const Message message) {
    FILE* out = !global_stdout_only && message.label == 0 ? stderr : stdout;

    if(!message.trace.filename) {
        fprintf(out, iftty("\33[3%um", "["), message.tty_color);
        fprintf(out, iftty("%s:\33[0m %.*s\n", "%s]: %.*s\n"), global_message_labels[message.label],
                fmtof(message.content));
        return 0;
    }

    char* line_start = message.trace.line_start;
    size_t offset = message.trace.source.data - message.trace.line_start;
    while(*line_start && *line_start <= ' ') {
        line_start++;
        offset--;
    }

    char underline[offset + message.trace.source.len + 1];
    for(size_t i = 0; i < sizeof(underline) - 1; i++) {
        underline[i] = i >= offset ? '~' : ' ';
    }
    underline[sizeof(underline) - 1] = '\0';

    fprintf(out, iftty("\33[1m%s:%u:%u: \33[3%um", "%s:%u:%u: ["),
            message.trace.filename, message.trace.row, message.trace.col,
            message.tty_color);
    fprintf(out, iftty("%s:\33[0m %.*s\n%4d | %s\n     : \33[3%um", "%s]: %.*s\n%4d | %s\n     : "),
            global_message_labels[message.label], fmtof(message.content),
            message.trace.row, line_start, message.tty_color);
    fprintf(out, iftty("%.*s\33[0m\n", "%.*s\n"),
            (int) sizeof(underline), underline);

    return !message.label;
}
