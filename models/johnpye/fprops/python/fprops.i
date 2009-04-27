%module fprops

%{
#include "../helmholtz.h"
#include "../water.h"
%}

%immutable helmholtz_data_water;

%include "../water.h"
%include "../helmholtz.h"

/*
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

