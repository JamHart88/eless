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

OBJ= mocks.${O} 

all: mocks.o

mocks.o:
	${CC} -c mocks.cpp

${OBJ}: mocks.cpp

clean:
	rm -rf *.${O} 

mostlyclean: clean

