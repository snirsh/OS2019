CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=FileApp.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS) -o
CXXFLAGS = -Wall -std=c++11 -g $(INCS) -o

OSMLIB = FileApp
TARGETS = $(OSMLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=$(LIBSRC) Makefile README

all:
	$(CXX) $(CFLAGS) $(OSMLIB) $(LIBSRC)

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

testo:
	make clean
	make
	chmod 777 FileApp
	./FileApp arg1 arg2 arg3 arg4 arg5