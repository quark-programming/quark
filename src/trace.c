#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "vector.h"

typedef Vector(char) str;
typedef Vector(str) strs;

typedef struct {
	str slice; // TODO: rename wtf means slice?
	char* filename;
	char* line_start;
	unsigned row, col;
} Trace;

typedef struct {
	Trace trace;
	char* highlight;
	char* label;
	str content;
} Message;
typedef Vector(Message) Message_Vector;


#define str(lit) ((str) { sizeof(lit) - 1, 0, lit })

str strf(str* self, const char* fmt, ...) {
	static char buffer[65536]; // TODO: maybe extract static size to a config file??
	va_list va_args;
    va_start(va_args, fmt);

    self = self ?: &(str) { 0 };

    size_t bytes = vsnprintf(buffer, sizeof(buffer), fmt, va_args);
	resv(self, bytes);
	memcpy(self->data + self->size, buffer, bytes);
	self->size += bytes;
	return *self;
}

int streq(str a, str b) {
	return a.size == b.size && !memcmp(a.data, b.data, a.size);
}

Trace stretch(Trace a, Trace b) {

	a.slice.size = b.slice.data - a.slice.data + b.slice.size;
	if(a.slice.size >= 256) a.slice.size = 16;
	return a;
}

// TODO(#1): color is normally not a bad practice but at least check if the output is a tty not a file (a user can put > at the end of the command and the escape codes go into files without being escaped so implement a helper function that checks if tty)
#define REPORT_ERR(trace, content) ((Message) { trace, "\33[31m", "error", content })
#define REPORT_HINT(content) ((Message) { { 0 }, "\33[36m", "hint", content })
#define REPORT_INFO(trace, content) ((Message) { trace, "\33[36m", "info", content })
#define REPORT_WARNING(trace, content) ((Message) { trace, "\33[33m", "warning", content })


int print_message(Message message) {
	if(!message.trace.filename) {
        // TODO: see #1
	    printf("\33[1m%s%s:\33[0m %.*s\n",
				message.highlight, message.label,
				(int) message.content.size,
				message.content.data);
		return 0;
	}

	char* line_start = message.trace.line_start;
	size_t offset = message.trace.slice.data - message.trace.line_start;
	while(*line_start && *line_start <= ' ') {
		line_start++;
		offset--;
	}

	char underline[offset + message.trace.slice.size + 1];
	for(size_t i = 0; i < sizeof(underline); i++) {
		underline[i] = i >= offset ? '~' : ' ';
	}
    underline[sizeof(underline) - 1] = '\0';

    // TODO: see #1
	printf("\33[1m%s:%u:%u: %s%s:\33[0m %.*s\n"
			"%4d | %s\n     : %s%.*s\33[0m\n",
			message.trace.filename, message.trace.row, message.trace.col,
			message.highlight, message.label, (int) message.content.size, message.content.data,
			message.trace.row, line_start,
			message.highlight,
			(int) sizeof(underline), underline);

	return message.highlight[3] == '1';
}
