# Makefile for Zip, ZipNote and ZipSplit for
# Atari ST with Turbo C 2.0.

# To use, do "make -f makefile.st"

# ------------- Turbo C -------------
TCHOME=\TC

CFLAGS=-A -G -I$(TCHOME)\INCLUDE -DATARI_ST
UTILFLAGS=-DUTIL $(CFLAGS)
CC=tcc
LD=tlink
LDFLAGS= -S=8192

# variables
OBJZ = zip.o zipfile.o zipup.o fileio.o util.o globals.o atari.o
OBJI = deflate.o trees.o bits.o

OBJU = zipfile_.o zipup_.o fileio_.o util_.o globals.o
OBJN = zipnote.o  $(OBJU)
OBJS = zipsplit.o $(OBJU)

zips:	zip.prg zipnote.prg zipsplit.prg
	echo done

atari.o: atari.c
	$(CC) -c $(CFLAGS) atari.c

zip.o:	zip.h ziperr.h tailor.h revision.h zip.c
	$(CC) -c $(CFLAGS) zip.c

zipfile.o:	zip.h ziperr.h tailor.h zipfile.c
	$(CC) -c $(CFLAGS) zipfile.c

zipup.o:	zip.h ziperr.h tailor.h revision.h zipup.c
	$(CC) -c $(CFLAGS) zipup.c

fileio.o:	zip.h ziperr.h tailor.h fileio.c
	$(CC) -c $(CFLAGS) fileio.c

util.o:	zip.h ziperr.h tailor.h util.c
	$(CC) -c $(CFLAGS) util.c

globals.o:	zip.h ziperr.h tailor.h globals.c
	$(CC) -c $(CFLAGS) globals.c

zipnote.o:	zip.h ziperr.h tailor.h revision.h zipnote.c
	$(CC) -c $(CFLAGS) zipnote.c

zipsplit.o:	zipsplit.c zip.h ziperr.h tailor.h revision.h
	$(CC) -c $(CFLAGS) zipsplit.c

deflate.o:	zip.h ziperr.h tailor.h deflate.c
	$(CC) -c $(CFLAGS) deflate.c

trees.o:	zip.h ziperr.h tailor.h trees.c
	$(CC) -c $(CFLAGS) trees.c

bits.o:		zip.h ziperr.h tailor.h bits.c
	$(CC) -c $(CFLAGS) bits.c

zipfile_.o:	zipfile.c zip.h ziperr.h tailor.h
	$(CC) -c $(UTILFLAGS) -Ozipfile_.o zipfile.c

zipup_.o:	zipup.c zip.h ziperr.h tailor.h
	$(CC) -c $(UTILFLAGS) -Ozipup_.o zipup.c

fileio_.o:	fileio.c zip.h ziperr.h tailor.h
	$(CC) -c $(UTILFLAGS) -Ofileio_.o fileio.c

util_.o:	util.c zip.h ziperr.h tailor.h
	$(CC) -c $(UTILFLAGS) -Ofileio_.o fileio.c

zip.prg: $(OBJZ) $(OBJI)
	$(LD) $(LDFLAGS) -C=zip.lnk -O=$@

zipnote.prg: $(OBJN)
	$(LD) $(LDFLAGS) -C=zipn.lnk -O=$@

zipsplit.prg: $(OBJS)
	$(LD) $(LDFLAGS) -C=zips.lnk -O=$@
