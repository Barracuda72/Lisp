CFLAGS=-std=c18 -pedantic -Wall -Wextra -O2 -g
CC=gcc
LD=gcc
LDFLAGS=-lc
TARGET=lisp

$(TARGET): main.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	-rm $(TARGET) *.o
