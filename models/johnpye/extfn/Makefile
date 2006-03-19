# Makefile for add-ins in the current directory; for each .c file you will get a separate .so file.

PACKAGES = extfntest

ASCEND_BASE_REL = ../../..

ASCEND_BASE = $(shell cd $(ASCEND_BASE_REL) && pwd)

ASCEND_INCDIR = $(ASCEND_BASE)/base/generic

LIBS = $(PACKAGES:%=$(SO_PREF)%$(SO_SUF))

ifeq ($(OSTYPE),msys)
	PYTHON_VERSION := 24
	PYTHON_ROOT := /c/Python$(PYTHON_VERSION)
	PYTHON_CPPFLAGS := -I$(PYTHON_ROOT)/include
	PYTHON_LDFLAGS := -L$(PYTHON_ROOT)/libs -lpython$(PYTHON_VERSION)
	PYTHON := $(PYTHON_ROOT)/python
	SO_SUF = .dll
	SO_PREF = 
else
	ifeq ($(TERM),cygwin)
		PYTHON_VERSION := 24
		PYTHON_ROOT := /cygdrive/c/Python$(PYTHON_VERSION)
		PYTHON_CPPFLAGS := -I$(PYTHON_ROOT)/include
		PYTHON_LDFLAGS := -L$(PYTHON_ROOT)/libs -lpython24
		PYTHON := $(PYTHON_ROOT)/python
		SO_SUF = .dll
		SO_PREF = 
	else
		PYTHON_VERSION := $(shell python -V 2>&1 | sed s/Python\ // | sed s/^\\\([0-9][0-9]*\\.[0-9][0-9]*\\\).*/\\1/ )
		PYTHON_CPPFLAGS := -I/usr/include/python$(PYTHON_VERSION)
		PYTHON_LDFLAGS := -lpython$(PYTHON_VERSION)
		PYTHON := python
		SO_SUF = .so
		SO_PREF = lib
	endif
endif

all: ascend makelibs test

makelibs: $(LIBS) 
	@echo "Compiled libraries $(LIBS)"

test: $(LIBS)
	PYTHONPATH=$(ASCEND_BASE)/pygtk/interface/ $(PYTHON) extfntest.py

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
