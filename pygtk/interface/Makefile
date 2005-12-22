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


ASCEND_CPPFLAGS = -I../../base/generic -I../../base/generic/solver -I../../base/generic/compiler -I../../base/generic/utilities -I../../base/generic/general -I../../base/generic/packages

#-------------------------
# Linking with ASCEND

ASC_TOOLS = autotools

ifeq ($(ASCTOOLS),jam)
	# If you build with jam
	ARDIR = $(SOURCE_ROOT)/jam/Release/linux	
	ASC_BUILD_LIBS = cd $(SOURCE_ROOT)/jam; jam libs
else
	ARDIR = $(SOURCE_ROOT)/base/autotools/archive
	ASC_BUILD_LIBS = cd $(SOURCE_ROOT)/base/autotools; make libs
endif

# Libs now are named the same way on both autotools and jam builds...
ASC_LIBS_STEM = compiler packages solver utilities general 
ASC_LIBS = $(ASC_LIBS_STEM:%=$(ARDIR)/libasc%.a)

$(ASC_LIBS): libs

.PHONY: libs

libs:
	$(ASC_BUILD_LIBS)

#------------------
# PYTHON STUFF

USE_PYTHON := 1

ifeq ($(USE_PYTHON),1)
	ifeq ($(OSTYPE),msys)
		PYTHON_VERSION := 24
		PYTHON_ROOT := /c/Python$(PYTHON_VERSION)
		PYTHON_CPPFLAGS := -I$(PYTHON_ROOT)/include
		PYTHON_LDFLAGS := -L$(PYTHON_ROOT)/libs -lpython($PYTHON_VERSION)
	else
		PYTHON_VERSION := $(shell python -V 2>&1 | sed s/Python\ // | sed s/^\\\([0-9][0-9]*\\.[0-9][0-9]*\\\).*/\\1/ )
		PYTHON_CPPFLAGS := -I/usr/include/python$(PYTHON_VERSION)
		PYTHON_LDFLAGS := -lpython$(PYTHON_VERSION)
	endif
endif

ifeq ($(USE_PYTHON),1)
	ASCXX_CPPFLAGS := $(ASCEND_CPPFLAGS) $(PYTHON_CPPFLAGS) -DASCXX_USE_PYTHON
	ASCXX_PYTHON_LDFLAGS := $(PYTHON_LDFLAGS)
else
	ASCXX_CPPFLAGS := $(ASCEND_CPPFLAGS)
	ASCXX_PYTHON_LDFLAGS :=
endif

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
	cd $(SOURCE_ROOT)/jam; \
	jam clean; 
	
#--------------------------
# Pure C++ testing

CPPFLAGS := $(ASCXX_CPPFLAGS)

main: main.o $(OBJS) $(ASC_LIBS)
	$(LD) -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

#------------------------
# SWIG targets

SWIG_MODULE = ascend
SWIG = swig

ifeq ($(OSTYPE),msys)
	SWIG_VERSION = 1.3.27
	SWIG_INCROOT = /usr/local/share/swig/$(SWIG_VERSION)
	SWIG_INCLUDES = -I$(SWIG_INCROOT) -I$(SWIG_INCROOT)/std -I$(SWIG_INCROOT)/python
else
	SWIG_INCLUDES =
endif

SWIGFLAGS = -c++ -python $(SWIG_INCLUDES)
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
	$(LD) -module -shared -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

#------------------------
# Trying to debug this GDPITA '3' problem, bug #178

testsolverinit.o: testsolverinit.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -o $@ $^

testsolverinit: testsolverinit.o $(OBJS) $(ASC_LIBS)
	$(LD) -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)


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
 
