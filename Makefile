CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=uthreads.cpp sleeping_threads_list.cpp sleeping_threads_list.h thread.cpp thread.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

LIBNAME = libuthreads.a
TARGETS = $(LIBNAME)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(LIBNAME) $(OBJ) $(LIBOBJ) *~ *core th_test

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

testo:
	make
	$(CC) -g -o testo t1.cpp libuthreads.a
	chmod 777 testo
	./testo
