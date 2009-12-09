%module fprops

%{
#include "../carbondioxide.h"
#include "../water.h"
#include "../ammonia.h"
#include "../hydrogen.h"
#include "../nitrogen.h"
#include "../helmholtz.h"
%}

%immutable helmholtz_data_water;
%immutable helmholtz_data_carbondioxide;
%immutable helmholtz_data_ammonia;
%immutable helmholtz_data_nitrogen;
%immutable helmholtz_data_hydrogen;

%include "../helmholtz.h"

%include "../carbondioxide.h"
%include "../water.h"
%include "../ammonia.h"
%include "../hydrogen.h"
%include "../nitrogen.h"

/*
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

