LIBTARGET = libut3k.a
INCLUDE = ../../include
LIBDIR = ../../lib
# libs: just to note incase one is linking...
LIBS = -lpulse -lsndfile 
CC = gcc
#CFLAGS = -g -Wall
CFLAGS = -O2 -Wall

.PHONY: default all clean

default: install
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

