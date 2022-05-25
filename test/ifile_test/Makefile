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


OBJ= ifile_mocks.${O} ../mocks.${O}

all: test_ifile$(EXEEXT)

test_ifile: ${OBJ} ifile.hpp ifile.cpp ../mocks.hpp ../mocks.cpp
	${CC} ${LDFLAGS} ifile_unittest.cpp -o $@ ${OBJ} ${LIBS}

mocks.o:
	../make

${OBJ}: less.hpp ifile_unittest.cpp ifile.cpp ../mocks.cpp


clean:
	rm -rf *.${O} core test_ifile *.gcda *.gcno ./results my.info lcov.out; ./clean.sh

mostlyclean: clean

run: test_ifile
	./test_ifile; lcov -c -d . -o my.info > lcov.out; genhtml -o results/ my.info >> lcov.out; egrep "lines|functions" lcov.out

