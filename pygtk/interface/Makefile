SOURCE_ROOT = ../..

CXX = g++
CC = gcc 

SO_SUFF = .dll
PLATFORM_CPPFLAGS = 

COVERAGE_FLAGS = 

CXXFLAGS = -c -g $(COVERAGE_FLAGS)

LD = libtool --mode=link gcc
LD_CXX = libtool --mode=link g++ 

# -lieee -ldl
LDFLAGS = -g -lm -lstdc++ $(COVERAGE_FLAGS)

# Are you on a dual processor machine?
_smp_mflags = -j2 

CLASSES = library type module symchar \
	instance instanceinterfacedata \
	method name \
	reporter simulation set units dimensions extmethod \
	variable relation \
	solver curve plot \
	solverparameters solverparameter solverparameteriterator \
	solverstatus solverreporter \
	incidencematrix

OBJS = $(CLASSES:%=%.o)
HDRS = $(CLASSES:%=%.h)

all:
	@echo "SO SUFF: $(SO_SUFF)"
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
		PYTHON_LDFLAGS := -L$(PYTHON_ROOT)/libs -lpython$(PYTHON_VERSION)
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
	-rm *$(SO_SUFF) *.so *.dll
	-rm *.o
	-rm *_wrap.cxx
	-rm *_wrap.c
	-rm *_wrap.o
	-rm ascend.py
	-rm main
	-rm *.pyc
	-rm *.gcno
	-rm *.gcda
	-rmdir .libs

cleanall: clean
	cd $(SOURCE_ROOT)/jam; \
	jam clean; 


#------------------------
# SWIG targets

SWIG_MODULE = ascend

ifeq ($(OSTYPE),msys)
	SWIG=~/swigwin-1.3.28/swig
	SWIG_LIB = $(shell $(SWIG) -swiglib)
	SWIG_VERSION = $(shell $(SWIG) -version | grep "^SWIG Version " | sed s/SWIG\ Version\ \\\([0-9][0-9]*\\\(\\.[0-9][0-9]*\\\)*\\\).*/\\1/)
	SWIG_INCLUDES = -I$(SWIG_LIB)
else
	SWIG=swig
	SWIG_VERSION = $(shell $(SWIG) -version | grep "^SWIG Version " | sed s/SWIG\ Version\ \\\([0-9][0-9]*\\\(\\.[0-9][0-9]*\\\)*\\\).*/\\1/)
	SWIG_INCLUDES =
endif

# optimize for SWIG 1.3.28 and newer
SWIG_MAJOR = $(shell echo $(SWIG_VERSION) | sed s/^\\\([0-9]*\\\).*/\\1/)
SWIG_MINOR = $(shell echo $(SWIG_VERSION) | sed s/^[0-9]*\\.\\\([0-9]*\\\).*/\\1/)
SWIG_PATCH = $(shell echo $(SWIG_VERSION) | sed s/^[0-9]*\\.[0-9]*\\.\\\([0-9]*\\\).*/\\1/)

SWIG_FEATURES =
ifeq ($(SWIG_MAJOR),1)
	ifeq ($(SWIG_MINOR),3)
		ifeq ($(shell test $(SWIG_PATCH) -ge 28 && echo 1),1)
			SWIG_FEATURES="-O"
		endif
	endif
endif

SWIGFLAGS = -c++ -python $(SWIG_INCLUDES)
SWIG_CFLAGS = -c -fPIC 
SWIG_CPPFLAGS = $(ASCXX_CPPFLAGS) $(PLATFORM_CPPFLAGS)

SWIG_OBJS = $(SWIG_MODULE:%=%_wrap.o)
SWIG_PY = $(SWIG_MODULE:%=%.py)

LIB = _ascend$(SO_SUFF)

all: $(LIB)

SWIGS = plot.i solver.i

# FIXME add ' $(ASC_LIBS)' to deps once Jam build is fixed:
%_wrap.cxx %.py: %.i $(SWIGS) $(HDRS)
	@echo "SWIG = $(SWIG), VERSION = $(SWIG_VERSION)"
	SWIG_FEATURES=$(SWIG_FEATURES) $(SWIG) $(SWIGFLAGS) $<

.precious: $(SWIG_MODULE:%=%_wrap.cxx)
.precious: _ascend$(SO_SUFF)
	
%_wrap.o: %_wrap.cxx
	$(CC) $(SWIG_CFLAGS) -o $@ $^ $(SWIG_CPPFLAGS)

$(LIB): $(ASC_LIBS) $(OBJS) $(SWIG_OBJS) $(SWIG_PY) 
	@echo "OS TYPE = $(OSTYPE)"
	@echo "SO_SUFF = $(SO_SUFF)"
	@echo "CREATING LIBRARY FILE '$(LIB)'"
	$(LD) -module -shared -o $@ $(ASC_LIBS) $(OBJS) $(SWIG_OBJS) $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)


#--------------------------
# Pure C++ tests (no swigging required)

CPPFLAGS := $(ASCXX_CPPFLAGS) $(PLATFORM_CPPFLAGS)

main: main.o $(OBJS) $(ASC_LIBS)
	$(LD) -o $@ $^ $(LDFLAGS) $(ASCXX_PYTHON_LDFLAGS)

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
 
