CC     = gcc
CFLAGS = -O2 -Wall
INCDIR = -I/usr/X11R6/include/X11 -I/usr/X11R6/include -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/freetype2 -I/usr/include/libdrm -I/usr/include/libpng16 -I/usr/include/gdk-pixbuf-2.0
DESTDIR= /usr
MANDIR = /usr/share/man
LIBDIR = -L/usr/X11R6/lib

# for Linux
LIBS   = -lXpm -lX11 -lXext -lm -lcairo

# for Solaris
# LIBS   = -lXpm -lX11 -lXext -lsocket -lnsl

OBJS   = wmHWifi.o \
         xutils_cairo.o


.c.o:
	$(CC) $(CFLAGS) -D$(shell echo `uname -s`) -c $< -o $*.o $(INCDIR)


all:	wmHWifi.o wmHWifi

wmHWifi.o: wmHWifi_master.xpm wmHWifi_mask.xbm
wmHWifi:	$(OBJS) 
	$(CC) $(COPTS) $(SYSTEM) -o wmHWifi $^ $(INCDIR) $(LIBDIR) $(LIBS)

clean:
	for i in $(OBJS) ; do \
		rm -f $$i; \
	done
	rm -f wmHWifi

install:: wmHWifi
	install -c -s -m 0755 wmHWifi $(DESTDIR)/bin
	install -c -m 0644 wmHWifi.1 $(MANDIR)/man1 

