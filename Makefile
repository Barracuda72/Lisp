CFLAGS=-std=c18 -pedantic -Wall -Wextra
CC=gcc
LD=gcc
LDFLAGS=-lc -lreadline
TARGET=lisp

ifeq ($(DEBUG),1)
  Y_DBG=-t
  CFLAGS += -DDEBUG -O0 -g
else
  CFLAGS += -O2
endif

$(TARGET): main.o tree.o y.tab.o lex.yy.o
	$(LD) $(LDFLAGS) $^ -o $@

y.tab.c: lisp.y
	yacc ${Y_DBG} -d $^ --verbose

lex.yy.c: lisp.lex
	lex $^

clean:
	-rm $(TARGET) *.o lex.* y.*
