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
	$(RM) $(TARGETS) $(LIBNAME) $(OBJ) $(LIBOBJ) *~ *core testo
	
cleaneu:
	$(RM) $(TARGETS) $(LIBNAME) $(OBJ) $(LIBOBJ) *~ *core eu

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

testo:
	make
	$(CC) -pthread -g -std=c++11 -o testo SampleClient.cpp libMapReduceFramework.a
	chmod 777 testo
	#./testo

eu:
	make
	$(CC) -pthread -g -std=c++11 -o eu eurovisionClient.cpp libMapReduceFramework.a
	chmod 777 eu
	#./eu

valgrind:
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --undef-value-errors=no testo

valgrindt:
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --undef-value-errors=no eu

