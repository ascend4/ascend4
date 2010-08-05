%module fprops

%{
#include "../carbondioxide.h"
#include "../water.h"
#include "../ammonia.h"
#include "../hydrogen.h"
#include "../nitrogen.h"
#include "../methane.h"
#include "../helmholtz.h"
#include "../sat.h"
#include "../solve_ph.h"
%}

%include exception.i

%immutable helmholtz_data_water;
%immutable helmholtz_data_carbondioxide;
%immutable helmholtz_data_ammonia;
%immutable helmholtz_data_nitrogen;
%immutable helmholtz_data_hydrogen;
%immutable helmholtz_data_methane;

%include "../helmholtz.h"

%apply double *OUTPUT {double *p_sat};
%apply double *OUTPUT {double *rho_f};
%apply double *OUTPUT {double *rho_g};
%include "../sat.h"
%clear double *p_sat;
%clear double *rho_f;
%clear double *rho_g;

%apply double *OUTPUT {double *T};
%apply double *OUTPUT {double *rho};
int fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess, const HelmholtzData *D);
%clear double *T;
%clear double *rho;


%exception;

%include "../carbondioxide.h"
%include "../water.h"
%include "../ammonia.h"
%include "../hydrogen.h"
%include "../nitrogen.h"
%include "../methane.h"

/*
If you're not using SCons, then try something like...
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

