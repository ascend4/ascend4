SOURCE_ROOT = ../..

CXX = g++
CC = gcc 


ifeq ($(OSTYPE),"cygwin")
	PLATFORM_CPPFLAGS = -DPY_LONG_LONG="long long"
    SO_SUFF = .dll
else
	PLATFORM_CPPFLAGS =
	SO_SUFF = .so
endif

CXXFLAGS = -c -g

LD = libtool --mode=link gcc
LD_CXX = libtool --mode=link g++

# -lieee -ldl
LDFLAGS = -g -lm -lc -lstdc++

# Are you on a dual processor machine?
_smp_mflags = -j2 

CLASSES = library type module symchar instance variable method name \
	reporter simulation set units dimensions extmethod \
	solver curve plot \
	solverparameters solverparameter solverparameteriterator \
	incidencematrix

OBJS = $(CLASSES:%=%.o)
HDRS = $(CLASSES:%=%.h)

all:
	@echo "OS TYPE: $(OSTYPE)"

ASCEND_CPPFLAGS = -I../../base/generic -I../../base/generic/solver -I../../base/generic/compiler -I../../base/generic/utilities -I../../base/generic/general -I../../base/generic/packages

#-------------------------
# Linking with ASCEND

USE_JAM_LIBS = 1

ARDIR = $(SOURCE_ROOT)/base/autotools/archive
ASC_BUILD_LIBS = cd $(SOURCE_ROOT)/base/autotools; make libs

# Libs now are named the same way on both autotools and jam builds...
ASC_LIBS_STEM = compiler packages solver utilities general 
ASC_LIBS = $(ASC_LIBS_STEM:%=$(ARDIR)/libasc%.a)

$(ASC_LIBS): libs

.PHONY: libs

libs:
	@echo $(ASC_LIBS)
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
        ifeq ($(TERM),cygwin)
            PYTHON_VERSION := 24
            PYTHON_ROOT := /cygdrive/c/Python$(PYTHON_VERSION)
            PYTHON_CPPFLAGS := -I$(PYTHON_ROOT)/include
            PYTHON_LDFLAGS := -L$(PYTHON_ROOT)/libs -lpython24
        else
			PYTHON_VERSION := $(shell python -V 2>&1 | sed s/Python\ // | sed s/^\\\([0-9][0-9]*\\.[0-9][0-9]*\\\).*/\\1/ )
            PYTHON_CPPFLAGS := -I/usr/include/python$(PYTHON_VERSION)
            PYTHON_LDFLAGS := -lpython$(PYTHON_VERSION)
        endif
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
	-rm *$(SO_SUFF)
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

CPPFLAGS := $(ASCXX_CPPFLAGS) $(PLATFORM_CPPFLAGS)

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
SWIG_CPPFLAGS = $(ASCXX_CPPFLAGS) $(PLATFORM_CPPFLAGS)

SWIG_OBJS = $(SWIG_MODULE:%=%_wrap.o)

LIB = _ascend$(SO_SUFF)

all: $(LIB)

SWIGS = plot.i solver.i

# FIXME add ' $(ASC_LIBS)' to deps once Jam build is fixed:
%_wrap.cxx %.py: %.i $(SWIGS) $(HDRS)
	$(SWIG) $(SWIGFLAGS) $<

.precious: $(SWIG_MODULE:%=%_wrap.cxx)
.precious: _ascend.so
	
%_wrap.o: %_wrap.cxx
	$(CC) $(SWIG_CFLAGS) -o $@ $^ $(SWIG_CPPFLAGS)

$(LIB): $(ASC_LIBS) $(OBJS) $(SWIG_OBJS)
	$(LD) -module -shared -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

#------------------------
# Trying to debug this GDPITA '3' problem, bug #178

testsolverinit.o: testsolverinit.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -o $@ $^

testsolverinit: testsolverinit.o $(OBJS) $(ASC_LIBS)
	$(LD) -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)


#------------------------
# Trying to fix implementation of external functions

testextfn.o: testextfn.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -o $@ $^

testextfn: testextfn.o $(OBJS) $(ASC_LIBS)
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
 
