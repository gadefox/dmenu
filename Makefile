# dmenu - dynamic menu
# See LICENSE file for copyright and license details.

include config.mk

SRC = args.c dmenu.c drw.c input.c font.c menuitem.c \
	  strutil.c thunk.c util.c verbose.c

OBJ = $(SRC:.c=.o)

all: options dmenu

options:
	@echo dmenu build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

config.h:
	cp config.def.h $@

$(OBJ): config.mk

dmenu: args.o dmenu.o drw.o input.o font.o menuitem.o \
	   strutil.o thunk.o util.o verbose.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f dmenu *.o

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f dmenu $(DESTDIR)$(PREFIX)/bin/dmenu
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu
	strip -s $(DESTDIR)$(PREFIX)/bin/dmenu
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -f dmenu.1 $(DESTDIR)$(MANPREFIX)/man1/dmenu.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/dmenu.1
	gzip -f $(DESTDIR)$(MANPREFIX)/man1/dmenu.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dmenu\
		$(DESTDIR)$(MANPREFIX)/man1/dmenu.1.gz\

.PHONY: all options clean install uninstall
