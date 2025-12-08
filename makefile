out = qc
SRCS := $(wildcard src/*.c) $(wildcard src/*/*.c)

build: $(SRCS)
	$(CC) src/main.c -o $(out)

build-debug: $(SRCS)
	$(CC) src/main.c -o $(out) -g

all: build-debug

test: build
	./qc test/main.qk -o test/main.c
	$(CC) test/main.c -o test/main -Wno-parentheses-equality
	./test/main

clear:
	rm $(out)
