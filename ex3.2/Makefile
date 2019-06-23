CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=MapReduceFramework.cpp Barrier.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -pthread -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -pthread -g $(INCS)

LIBNAME = libMapReduceFramework.a
TARGETS = $(LIBNAME)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS=$(LIBSRC) Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(LIBNAME) $(OBJ) $(LIBOBJ) *~ *core testo tarant

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS) Barrier.h

testo:
	make
	$(CC) -pthread -g -std=c++11 -o testo SampleClient.cpp libMapReduceFramework.a
	chmod 777 testo
	#./testo

tarant:
	make
	$(CC) -pthread -g -std=c++11 -o tarant Tarantino/WordFrequencies.cpp libMapReduceFramework.a
	chmod 777 tarant
	#./tarant

valgrind:
	valgrind --leak-check=full --show-possibly-lost=no --show-reachable=no --undef-value-errors=no testo

