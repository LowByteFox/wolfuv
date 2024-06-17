CC = tcc
AR = tcc -ar
CFLAGS = -g -Wall -Wextra

all: lib/libwolfuv.a bin/echo

bin/echo: lib/libwolfuv.a
	@mkdir -p bin
	$(CC) $(CFLAGS) -Iinclude -Llib example/main.c -lwolfuv -luv -lwolfssl -o bin/echo

lib/libwolfuv.a: _obj/wolfuv.o
	@mkdir -p lib
	$(AR) rcs lib/libwolfuv.a _obj/wolfuv.o

_obj/wolfuv.o:
	@mkdir -p _obj
	$(CC) $(CFLAGS) -Iinclude -c src/wolfuv.c -o _obj/wolfuv.o

clean:
	rm -rf bin lib _obj
