wolfuv - simple library that adds TLS support into libuv using wolfSSL
Copyright (C) 2024  LowByteFox

wolfuv is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
