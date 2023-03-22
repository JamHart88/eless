# Makefile for eless.

#### Start of system configuration section. ####

srcdir = .


CC = g++
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CFLAGS = 
CFLAGS_COMPILE_ONLY = -c
LDFLAGS = 
CPPFLAGS = -std=c++17 -g -ggdb -O0 -Wall -Wshadow -Wnon-virtual-dtor -pedantic -Wl,--demangle -Wunreachable-code -Wlogical-op \
	-Wfloat-equal -Wduplicated-branches -Wduplicated-cond -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion \
	-Wimplicit-fallthrough -Wpedantic
# -Wsign-conversion -Wextra -Wconversion 
# TODO: Add these : -Wold-style-cast
# -pedantic - Warn on language extensions
# -Wall -Wextra reasonable and standard
# -Wshadow warn the user if a variable declaration shadows one from a parent context
# -Wnon-virtual-dtor warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
# -Wold-style-cast warn for c-style casts
# -Wcast-align warn for potential performance problem casts
# -Wunused warn on anything being unused
# -Woverloaded-virtual warn if you overload (not override) a virtual function
# -Wpedantic (all versions of GCC, Clang >= 3.2) warn if non-standard C++ is used
# -Wconversion warn on type conversions that may lose data
# -Wsign-conversion (Clang all versions, GCC >= 4.3) warn on sign conversions
# -Wmisleading-indentation (only in GCC >= 6.0) warn if indentation implies blocks where blocks do not exist
# -Wduplicated-cond (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
# -Wduplicated-branches (only in GCC >= 7.0) warn if if / else branches have duplicated code
# -Wlogical-op (only in GCC) warn about logical operations being used where bitwise were probably wanted
# -Wnull-dereference (only in GCC >= 6.0) warn if a null dereference is detected
# -Wuseless-cast (only in GCC >= 4.8) warn if you perform a cast to the same type
# -Wdouble-promotion (GCC >= 4.6, Clang >= 3.8) warn if float is implicitly promoted to double
# -Wformat=2 warn on security issues around functions that format output (i.e., printf)
# -Wlifetime (only special branch of Clang currently) shows object lifetime issues
# -Wimplicit-fallthrough Warns when case statements fall-through. (Included with -Wextra in GCC, not in clang)

# Removed CPP flags
# Make this -O2 for production
# -std=c++11  - AIX XLC++ only has c++0x 
# -finstrument-functions -Werror -Wpedantic -Wshadow 
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
	ifile.${O} input.${O} jump.${O} line.${O} linenum.${O} \
	lsystem.${O} mark.${O} optfunc.${O} option.${O} opttbl.${O} os.${O} \
	output.${O} pattern.${O} position.${O} prompt.${O} search.${O} signal.${O} \
	tags.${O} ttyin.${O} version.${O} debug.${O} utils.${O}

all: eless$(EXEEXT)

eless$(EXEEXT): ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

charset.${O}: compose.uni ubin.uni wide.uni

${OBJ}: ${srcdir}/less.hpp ${srcdir}/defines.hpp ${srcdir}/brac.hpp ${srcdir}/cvt.hpp ${srcdir}/forwback.hpp \
	${srcdir}/lesskey.hpp ${srcdir}/option.hpp ${srcdir}/position.hpp ${srcdir}/tags.hpp ${srcdir}/charset.hpp \
	${srcdir}/debug.hpp ${srcdir}/help.hpp ${srcdir}/line.hpp ${srcdir}/opttbl.hpp ${srcdir}/prompt.hpp ${srcdir}/ttyin.hpp \
	${srcdir}/ch.hpp ${srcdir}/decode.hpp ${srcdir}/ifile.hpp ${srcdir}/linenum.hpp ${srcdir}/os.hpp \
	${srcdir}/utils.hpp ${srcdir}/cmdbuf.hpp ${srcdir}/input.hpp ${srcdir}/lsystem.hpp ${srcdir}/output.hpp ${srcdir}/screen.hpp \
	${srcdir}/cmd.hpp ${srcdir}/edit.hpp ${srcdir}/jump.hpp ${srcdir}/mark.hpp ${srcdir}/pattern.hpp ${srcdir}/search.hpp \
	${srcdir}/command.hpp ${srcdir}/filename.hpp ${srcdir}/less.hpp ${srcdir}/optfunc.hpp ${srcdir}/pckeys.hpp ${srcdir}/signal.hpp


install: all ${srcdir}/less.nro installdirs
	${INSTALL_PROGRAM} eless$(EXEEXT) ${DESTDIR}${bindir}/${binprefix}less$(EXEEXT)
	${INSTALL_DATA} ${srcdir}/less.nro ${DESTDIR}${mandir}/man${manext}/${manprefix}less.${manext}

install-strip:
	${MAKE} INSTALL_PROGRAM='${INSTALL_PROGRAM} -s' install

installdirs: mkinstalldirs
	${srcdir}/mkinstalldirs ${DESTDIR}${bindir} ${DESTDIR}${mandir}/man${manext}

uninstall:
	rm -f ${DESTDIR}${bindir}/${binprefix}eless$(EXEEXT)
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
	rm -f *.${O} core eless$(EXEEXT)

mostlyclean: clean

realclean: distclean
	rm -f TAGS

