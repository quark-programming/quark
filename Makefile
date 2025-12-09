out = qc
SRCS := $(wildcard src/*.c) $(wildcard src/*/*.c)
CFLAGS = -Wall -g -ggdb -Wno-missing-braces -Wno-char-subscripts

# TODO: a release build target
build: $(SRCS)
	$(CC) $(CFLAGS) src/main.c -o $(out)

all: build

test: build
	./qc test/main.qk -o test/main.c
	$(CC) test/main.c -o test/main -Wno-parentheses-equality
	./test/main

clean:
	rm $(out)
