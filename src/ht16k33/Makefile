LIBTARGET = libht16k33.a
INCLUDE = ../../include
LIBDIR = ../../lib
LIBS = -li2c -lpigpio
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(LIBTARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) -I$(INCLUDE) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(LIBTARGET) $(OBJECTS)

$(LIBTARGET): $(OBJECTS)
	$(AR) rcv $(LIBTARGET) $?
	ranlib $(LIBTARGET)

install: $(LIBTARGET)
	cp $(LIBTARGET) $(LIBDIR)
	cp $(HEADERS) $(INCLUDE)

clean:
	-rm -f *.o
	-rm -f $(LIBTARGET)

