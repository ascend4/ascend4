%module fprops

%feature("autodoc");

%{
#include "../helmholtz.h"
#include "../sat.h"
#include "../solve_ph.h"
#include "../fluids.h"
%}

%include exception.i

%include "../helmholtz.h"

%apply double *OUTPUT {double *p_sat};
%apply double *OUTPUT {double *T_sat};
%apply double *OUTPUT {double *rho_f};
%apply double *OUTPUT {double *rho_g};
%include "../sat.h"
%clear double *p_sat;
%clear double *T_sat;
%clear double *rho_f;
%clear double *rho_g;

%apply double *OUTPUT {double *T};
%apply double *OUTPUT {double *rho};
int fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess, const HelmholtzData *D);
%clear double *T;
%clear double *rho;

%exception;

%include "../fluids.h"

/*
If you're not using SCons, then try something like...
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

