# Makefile for add-ins in the current directory; for each .c file you will get a separate .so file.

PACKAGES = extfntest
SO_SUF = .so
SO_PREF = lib

ASCEND_INCDIR = ~/src/ascend/trunk/base/generic/

LIBS = $(PACKAGES:%=$(SO_PREF)%$(SO_SUF))

all: ascend makelibs test

makelibs: $(LIBS) 
	@echo "Compiled libraries $(LIBS)"

test: $(LIBS)
	PYTHONPATH=~/src/ascend/trunk/pygtk/interface/ python extfntest.py

ascend:
	make -j2 -C ~/src/ascend/trunk/pygtk/interface/

CC = gcc

LD = libtool --mode=link gcc -module -shared -fPIC

LDFLAGS = -lm

CPPFLAGS = -I $(ASCEND_INCDIR)
CFLAGS = -O -c

$(SO_PREF)%$(SO_SUF): %.o
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

clean:
	-rm *.o *.so
