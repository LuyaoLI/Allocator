DEBUG ?= 0
CFLAGS=-c -g -Wall -Wextra -fPIC -DDEBUG=$(DEBUG)
APP=/bin/ls

lib: bin/libmyalloc.so

run: bin/libmyalloc.so
	LD_PRELOAD=$< $(APP)

gdb: bin/libmyalloc.so
	gdb -ex "set exec-wrapper env LD_PRELOAD=$<" --args $(APP)

clean:
	rm ./bin/*

bin/libmyalloc.so: bin/alloc.o
	$(CC) -shared -g $< -o $@

bin/alloc.o: allocator.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: lib run gdb clean
