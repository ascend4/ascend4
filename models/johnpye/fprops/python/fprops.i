%module fprops

%feature("autodoc");

%{
#include "../helmholtz.h"
#include "../sat.h"
#include "../solve_ph.h"
#include "../solve_Tx.h"
#include "../fluids.h"
%}

%include exception.i

// strip 'fprops' prefix from function names, since module name is already 'fprops'.
//%rename(solve_Tx) fprops_solve_Tx;
//%rename(solve_ph) fprops_solve_ph;
//%rename(sat_T) fprops_sat_T;
//%rename(sat_p) fprops_sat_p;
//%rename(fluid) fprops_fluid;
//%rename(num_fluids) fprops_num_fluids;
//%rename(get_fluid) fprops_get_fluid;

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

%apply double *OUTPUT {double *rho};
int fprops_solve_Tx(double T, double x, double *rho, const HelmholtzData *D);
%clear double *rho;

%exception;

%include "../fluids.h"

/*
If you're not using SCons, then try something like...
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

