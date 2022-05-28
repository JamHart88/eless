# Makefile for less.

#### Start of system configuration section. ####

srcdir = .


CC = g++
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CFLAGS = 
CFLAGS_COMPILE_ONLY = -c
LDFLAGS = --coverage
CPPFLAGS = -std=c++11 -g -O0 -Wall --coverage
EXEEXT = 
O=o

LIBS =  -lgtest -lgmock

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

OBJ=ch_mocks.${O} ../mocks.o

all: test_ch$(EXEEXT) ../mocks.o

test_ch: ${OBJ} ch.hpp ch.cpp ../mocks.hpp ../mocks.cpp ../mocks.o ch_mocks.cpp ch_mocks.hpp
	${CC} ${LDFLAGS} ch_unittest.cpp -o $@ ${OBJ} ${LIBS}

../mocks.o:
	make -C ../

${OBJ}: less.hpp ch_unittest.cpp ch.cpp ../mocks.cpp ch_mocks.hpp ch_mocks.cpp


clean:
	rm -rf *.${O} core test_ch *.gcda *.gcno ./results my.info lcov.out

moreclean:
	./clean.sh

run: test_ch
	./test_ch; lcov -c -d . -o my.info > lcov.out; genhtml -o results/ my.info >> lcov.out; egrep "lines|functions" lcov.out

