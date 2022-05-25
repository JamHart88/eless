# Makefile for less.

#### Start of system configuration section. ####

srcdir = .


CC = g++
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CFLAGS = 
CFLAGS_COMPILE_ONLY = -c
LDFLAGS = 
CPPFLAGS = -std=c++0x -g -O0 -Wall  -Wl,--demangle -Wunreachable-code -Wlogical-op -Wfloat-equal -Wpedantic
# Removed CPP flags
# Make this -O2 for production
# -std=c++11  - AIX XLC++ only has c++0x 
#-finstrument-functions -Werror -Wpedantic -Wshadow 
#CPPFLAGS = -g  -Wall 
EXEEXT = 
O=o

LIBS =  -ltinfo

prefix = /usr/local
exec_prefix = ${prefix}

# Where the installed binary goes.
bindir = ${exec_prefix}/bin
binprefix = 

sysconfdir = ${prefix}/etc
datarootdir = ${prefix}/share

mandir = ${datarootdir}/man
manext = 1
manprefix = 
DESTDIR =

#### End of system configuration section. ####

SHELL = /bin/sh

# This rule allows us to supply the necessary -D options
# in addition to whatever the user asks for.
.cpp.o:
	${CC} -I. ${CFLAGS_COMPILE_ONLY} -DBINDIR=\"${bindir}\" -DSYSDIR=\"${sysconfdir}\" ${CPPFLAGS} ${CFLAGS} $<

OBJ = \
	main.${O} screen.${O} brac.${O} ch.${O} charset.${O} cmdbuf.${O} \
	command.${O} cvt.${O} decode.${O} edit.${O} filename.${O} forwback.${O} \
	help.${O} ifile.${O} input.${O} jump.${O} line.${O} linenum.${O} \
	lsystem.${O} mark.${O} optfunc.${O} option.${O} opttbl.${O} os.${O} \
	output.${O} pattern.${O} position.${O} prompt.${O} search.${O} signal.${O} \
	tags.${O} ttyin.${O} version.${O} debug.${O} utils.${O}

all: less$(EXEEXT)

less$(EXEEXT): ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

charset.${O}: compose.uni ubin.uni wide.uni

${OBJ}: ${srcdir}/less.hpp ${srcdir}/funcs.hpp defines.hpp 

install: all ${srcdir}/less.nro installdirs
	${INSTALL_PROGRAM} less$(EXEEXT) ${DESTDIR}${bindir}/${binprefix}less$(EXEEXT)
	${INSTALL_DATA} ${srcdir}/less.nro ${DESTDIR}${mandir}/man${manext}/${manprefix}less.${manext}

install-strip:
	${MAKE} INSTALL_PROGRAM='${INSTALL_PROGRAM} -s' install

installdirs: mkinstalldirs
	${srcdir}/mkinstalldirs ${DESTDIR}${bindir} ${DESTDIR}${mandir}/man${manext}

uninstall:
	rm -f ${DESTDIR}${bindir}/${binprefix}less$(EXEEXT)
	rm -f ${DESTDIR}${mandir}/man${manext}/${manprefix}less.${manext}

info:
install-info:
dvi:
check:
installcheck:

TAGS:
	cd ${srcdir} && etags *.cpp *.hpp

# config.status might not change defines.hpp
# Don't rerun config.status if we just configured (so there's no stamp-h).
#defines.hpp: stamp-h
#stamp-h: defines.h.in 
#test ! -f stamp-h || CONFIG_FILES= CONFIG_HEADERS=defines.h ./config.status
#touch stamp-h

#Makefile: ${srcdir}/Makefile.in config.status
#CONFIG_FILES=Makefile CONFIG_HEADERS= ./config.status

clean:
	rm -f *.${O} core less$(EXEEXT)

mostlyclean: clean

realclean: distclean
	rm -f TAGS

