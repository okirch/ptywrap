LIBDIR	= /usr/lib64
INCDIR	= /usr/include

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC -pthread -Iinclude -D_GNU_SOURCE
LDFLAGS = -shared -pthread

# Library versioning
MAJOR = 0
MINOR = 1
VERSION = $(MAJOR).$(MINOR)

SRC_DIR = src
SOURCES = $(SRC_DIR)/pty.c $(SRC_DIR)/container.c $(SRC_DIR)/terminal.c \
          $(SRC_DIR)/reader.c $(SRC_DIR)/api.c $(SRC_DIR)/parser.c \
          $(SRC_DIR)/screenshot.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = include/ptywrap.h $(SRC_DIR)/internal.h $(SRC_DIR)/pty.h \
          $(SRC_DIR)/container.h $(SRC_DIR)/terminal.h $(SRC_DIR)/reader.h \
          $(SRC_DIR)/parser.h $(SRC_DIR)/screenshot.h

# Library targets
TARGET = libptywrap.so
SONAME = $(TARGET).$(MAJOR)
REALNAME = $(TARGET).$(VERSION)

all: $(REALNAME)

$(REALNAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -Wl,-soname,$(SONAME) -o $@ $^
	ln -sf $(REALNAME) $(SONAME)
	ln -sf $(SONAME) $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(REALNAME) $(SONAME) $(TARGET)
	rm -f tests/test_basic tests/test_integration
	rm -f examples/simple examples/interactive

install: $(REALNAME)
	mkdir -p $(DESTROOT)$(LIBDIR)
	install -m 555 $(REALNAME) $(DESTROOT)$(LIBDIR)/$(REALNAME)
	ln -sf $(REALNAME) $(DESTROOT)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTROOT)$(LIBDIR)/$(TARGET)
	mkdir -p $(DESTROOT)$(INCDIR)
	install -m 444 include/ptywrap.h $(DESTROOT)$(INCDIR)
	if [ -z "$(DESTROOT)" ]; then ldconfig; fi

.PHONY: all clean install
