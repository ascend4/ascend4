# Makefile for add-ins in the current directory; for each .c file you will get a separate .so file.

PACKAGES = extfntest
SO_SUF = .so
SO_PREF = lib

ASCEND_BASE_REL = ../../..

ASCEND_BASE = $(shell cd $(ASCEND_BASE_REL) && pwd)

ASCEND_INCDIR = $(ASCEND_BASE)/base/generic

LIBS = $(PACKAGES:%=$(SO_PREF)%$(SO_SUF))

all: ascend makelibs test

makelibs: $(LIBS) 
	@echo "Compiled libraries $(LIBS)"

test: $(LIBS)
	PYTHONPATH=$(ASCEND_BASE)/pygtk/interface/ python extfntest.py

ascend:
	make -j2 -C $(ASCEND_BASE)/pygtk/interface/

CC = gcc

LD = libtool --mode=link $(CC) -module -shared

LDFLAGS = -lm -Wl,-no-undefined

CPPFLAGS = -I$(ASCEND_INCDIR)
CFLAGS = -O -c

$(SO_PREF)%$(SO_SUF): %.o
	$(LD) -o $@ $^ $(LDFLAGS)

%.lo: %.c
	libtool --mode=compile $(CC) $(CPPFLAGS) $(CFLAGS) $^

clean:
	-rm *.o *.so
