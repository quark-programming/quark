#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "vector.c"

typedef Vector(char) str;
typedef Vector(str) strs;

typedef struct {
	str slice;
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
typedef Vector(Message) Messages;

#define str(lit) ((str) { sizeof(lit) - 1, 0, lit })

str strf(str* self, const char* fmt, ...) {
	static char buffer[65536];
	va_list va_args; va_start(va_args, fmt);
	self = self ?: &(str) { 0 };
	size_t bytes = vsnprintf(buffer, sizeof(buffer), fmt, va_args);
	resv(self, bytes);
	memcpy(self->data + self->size, buffer, bytes);
	self->size += bytes;
	return *self;
}

int streq(str a, str b) {
	return a.size == b.size && !strncmp(a.data, b.data, a.size);
}

Trace stretch(Trace a, Trace b) {
	a.slice.size = b.slice.data - a.slice.data + b.slice.size;
	return a;
}

#define Err(trace, content) \
	((Message) { trace, "\33[31m", "error", content })
#define Hint(content) \
	((Message) { { 0 }, "\33[36m", "hint", content })
#define See(trace, content) \
	((Message) { trace, "\33[36m", "see", content })
#define Warn(trace, content) \
	((Message) { trace, "\33[33m", "warning", content })

int print_message(Message message) {
	if(!message.trace.filename) {
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

	char underline[offset + message.trace.slice.size];
	for(size_t i = 0; i < sizeof(underline); i++) {
		underline[i] = i >= offset ? '~' : ' ';
	}

	printf("\33[1m%s:%u:%u: %s%s:\33[0m %.*s\n"
			"%4d | %s\n     : %s%.*s\33[0m\n",
			message.trace.filename, message.trace.row, message.trace.col,
			message.highlight, message.label, (int) message.content.size, message.content.data,
			message.trace.row, line_start,
			message.highlight,
			(int) sizeof(underline), underline);

	return message.highlight[3] == '1';
}
