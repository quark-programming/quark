out = qc

build:
	$(CC) src/main.c -o $(out)

test: build
	./qc test/main.qk -o test/main.c
	$(CC) test/main.c -o test/main
	./test/main
