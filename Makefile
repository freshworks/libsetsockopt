TARGET = libsetsockopt.so
LIBS = -ldl -lpthread
CC = gcc
CFLAGS = -O3 -g -fPIC -Wall
LDFLAGS = -fPIC -Wall -W -shared
LIBDIR = /usr/local/lib
INSTALL = install

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@

install: $(TARGET)
	$(INSTALL) -m 755 -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 $< $(DESTDIR)$(LIBDIR)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
