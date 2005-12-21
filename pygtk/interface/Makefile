SOURCE_ROOT = ../..

CXX = g++
CC = gcc

CXXFLAGS = -c -g

LD = libtool --mode=link gcc -export-dynamic 
LD_CXX = libtool --mode=link g++ -export-dynamic

LDFLAGS = -g -lieee -lm -lc -ldl -lstdc++

# Are you on a dual processor machine?
_smp_mflags = -j2 

CLASSES = library type module symchar instance variable method name reporter simulation set units dimensions extfn solver
OBJS = $(CLASSES:%=%.o)
HDRS = $(CLASSES:%=%.h)

all:

#-------------------------
# Linking with ASCEND

ASCEND_CPPFLAGS = -I../../base/generic -I../../base/generic/solver -I../../base/generic/compiler -I../../base/generic/utilities -I../../base/generic/general -I../../base/generic/packages $(PYTHON_CPPFLAGS)

BLAS = /usr/lib/libblas.so.3.0.3

#BUILDDIR = $(SOURCE_ROOT)/base/unixAC213
#ARDIR = $(BUILDDIR)/archive
#ASC_LIBS_STEM = asc packages solver utils general 
#ASC_LIBS = $(ASC_LIBS_STEM:%=$(ARDIR)/lib%.a)
BUILDDIR = $(SOURCE_ROOT)/base/jam/Release/linux
ARDIR = $(BUILDDIR)
ASC_LIBS_STEM = compiler packages solver utilities general 
ASC_LIBS = $(ASC_LIBS_STEM:%=$(ARDIR)/libasc_%.a)

$(ASC_LIBS): libs

.PHONY: libs

libs:
	cd $(SOURCE_ROOT)/base/jam; \
	jam $(_smp_mflags) -f../../jam/Jambase libs; 

#------------------
# PYTHON STUFF

PYTHON_VERSION := $(shell python -V 2>&1 | sed s/Python\ // | sed s/^\\\([0-9][0-9]*\\.[0-9][0-9]*\\\).*/\\1/ )

PYTHON_CPPFLAGS := -I/usr/include/python$(PYTHON_VERSION)/

PYTHON_LDFLAGS := -lpython$(PYTHON_VERSION)

USE_PYTHON := 1

ifdef USE_PYTHON
	ASCXX_CPPFLAGS := $(ASCEND_CPPFLAGS) $(PYTHON_CPPFLAGS) -DASCXX_USE_PYTHON
	ASCXX_PYTHON_LDFLAGS := $(PYTHON_LDFLAGS)
else
	ASCXX_CPPFLAGS := $(ASCEND_CPPFLAGS)
	ASCXX_PYTHON_LDFLAGS :=
endif


again: clean all

clean:
	-rm *.so
	-rm *.o
	-rm *_wrap.cxx
	-rm *_wrap.c
	-rm *_wrap.o
	-rm ascend.py
	-rm main
	-rm *.pyc
	-rmdir .libs

cleanall: clean
	cd $(SOURCE_ROOT)/base/jam; \
	jam -f../../jam/Jambase clean; 
	
#--------------------------
# Pure C++ testing

CPPFLAGS := $(ASCXX_CPPFLAGS)

main: main.o $(OBJS) $(ASC_LIBS)
	$(LD) -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

#------------------------
# SWIG targets

SWIG_MODULE = ascend
SWIG = swig
SWIGFLAGS = -c++ -python
SWIG_CFLAGS = -c -fPIC 
SWIG_CPPFLAGS = $(ASCXX_CPPFLAGS)

SWIG_OBJS = $(SWIG_MODULE:%=%_wrap.o)

LIB = _ascend.so

all: $(LIB)

# FIXME add ' $(ASC_LIBS)' to deps once Jam build is fixed:
%_wrap.cxx %.py: %.i $(HDRS)
	$(SWIG) $(SWIGFLAGS) $<

.precious: $(CLASSES:%=%_wrap.cxx)
.precious: _ascend.so
	
%_wrap.o: %_wrap.cxx
	$(CC) $(SWIG_CFLAGS) -o $@ $^ $(SWIG_CPPFLAGS)

$(LIB): $(OBJS) $(SWIG_OBJS) $(ASC_LIBS)
	$(LD) -module -shared -o $(LIB) $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

#------------------------
# Testing the Python module

test: $(LIB)
#	@echo "Does module include 'do_solve_eval' symbol?"
#	nm $(LIB) | grep "do_solve_eval" | egrep -v "U"
#	@echo "Does module include 'system_build' symbol?"
#	nm $(LIB) | grep "system_build" | egrep -v "U"
	@echo "Can python load the $(LIB) file?"
	python -c "import _ascend"
	python librarytest.py

#--------------------------
# tarfile

tar: clean
	pushd $(SOURCE_ROOT) && \
	tar jcvf pygtk.tar.bz2 pygtk/ && \
	popd
	mv ../../pygtk.tar.bz2 .

#--------------------------
# storing a patch

patch: clean
	pushd $(SOURCE_ROOT)/base && \
	svn diff > ../pygtk/interface/ascend-base.patch && \
	popd
	pushd $(SOURCE_ROOT)/tcltk98 && \
	svn diff > ../pygtk/interface/ascend-tcltk98.patch && \
	popd

#---------------------------
patchapplybase: clean
	pushd $(SOURCE_ROOT)/base && \
	patch -p0 < ../pygtk/interface/ascend-base.patch && \
    popd
 
