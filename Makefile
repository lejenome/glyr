# glyr's Makefile

#source dir
SOURCEDIR=src
SUBDIR_COVER=src/cover
SUBDIR_LYRICS=src/lyrics

#MANDIR
MANDIR=/usr/share/man/man1

#install
INSTALLPATH=/usr/bin

#Programs
ECHO=echo
RM=rm -rf
CAT=cat
CP=cp
STRIP=strip -s

#compiling stuff
CC=gcc

#Heavy Warnlevel
WARN=-Wall

#Quite heavy optimization
OPTI=-march=native -Os -finline-functions -fomit-frame-pointer -s
#OPTI=-ggdb3

#also use -pipe
CFLAGS=-c -pipe $(OPTI) $(WARN)

#Link flags 
LDFLAGS=-lcurl

SOURCES= \
    $(SOURCEDIR)/core.c \
    $(SOURCEDIR)/stringop.c \
    $(SOURCEDIR)/lyrics.c \
    $(SOURCEDIR)/cover.c \
    $(SOURCEDIR)/main.c \
    $(SUBDIR_COVER)/google.c \
    $(SUBDIR_COVER)/last_fm.c \
    $(SUBDIR_COVER)/lyricswiki.c \
    $(SUBDIR_COVER)/discogs.c \
    $(SUBDIR_COVER)/amazon.c \
    $(SUBDIR_LYRICS)/lyrdb.c \
    $(SUBDIR_LYRICS)/magistrix.c \
    $(SUBDIR_LYRICS)/lyrix_at.c \
    $(SUBDIR_LYRICS)/darklyrics.c \
    $(SUBDIR_LYRICS)/metrolyrics.c \
    $(SUBDIR_LYRICS)/lyricswiki.c


OBJECTS=$(SOURCES:.c=.o )
INCLUDE=-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
EXECUTABLE=glyr

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE):  $(OBJECTS)
	     @$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXECUTABLE)
	     @$(STRIP) $(EXECUTABLE)
	     @$(ECHO) "=> Stripping program done."
	     @$(ECHO) "=> Linking program done."

.c.o:
	@$(ECHO) "-> Compiling $<"
	@$(CC) $(INCLUDE) $(CFLAGS) $< -o $@

run:
	@./$(EXECUTABLE)

clean:
	@$(ECHO) "<> Making clean."
	@$(RM) $(SOURCEDIR)/*.o $(EXECUTABLE) $(SUBDIR_COVER)/*.o  $(SUBDIR_LYRICS)/*.o  2> /dev/null

install:
	@$(ECHO) "++ Copying executable to /usr/bin."
	@$(CP) $(EXECUTABLE) $(INSTALLPATH)