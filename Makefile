LIBES= -lbasis
CFLAGS=-O2
LFLAGS=-O2
CC=cc -baout -m386
LIBOBJ=rpmarchnames.o rpmosnames.o rpmsignames.o rpmtagnames.o cpio_wr.o

all:	xrpm makepkg

clean:
	rm -f makepkg xrpm *.o core xrpm.a

distclean seppuku: clean
	rm -f config.h config.log config.sub config.mak config.cmd Makefile

install: xrpm makepkg
	install -d /Mastodon/Core/xrpm/usr/bin
	install -d /Mastodon/Core/xrpm/usr/man/man1
	install -o bin -g bin -m 555 -s -c makepkg xrpm /Mastodon/Core/xrpm/usr/bin
	install -o man -g man -m 444    -c makepkg.1 /Mastodon/Core/xrpm/usr/man/man1

xrpm.a:	$(LIBOBJ)
	ar crv xrpm.a $(LIBOBJ)
	ranlib xrpm.a

makepkg: makepkg.c xrpm.a
	$(CC) -o makepkg $(CFLAGS) $(LFLAGS) makepkg.c xrpm.a $(LIBES)

xrpm:	xrpm.c xrpm.a
	$(CC) -o xrpm $(CFLAGS) $(LFLAGS) xrpm.c xrpm.a $(LIBES)

cpio_wr.o: cpio_wr.c pax.h cpio_wr.h makepkg.h
makepkg.o: makepkg.c makepkg.h xrpm.h mapping.h
rpmarchnames.o: rpmarchnames.c mapping.h
rpmosnames.o: rpmosnames.c mapping.h
rpmsignames.o: rpmsignames.c
rpmtagnames.o: rpmtagnames.c
xrpm.o: xrpm.c xrpm.h mapping.h
