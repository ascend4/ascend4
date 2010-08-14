%module fprops

%{
#include "../ammonia.h"
#include "../nitrogen.h"
#include "../hydrogen.h"
#include "../water.h"
#include "../carbondioxide.h"
#include "../methane.h"
#include "../carbonmonoxide.h"
#include "../ethanol.h"
#include "../acetone.h"
#include "../carbonylsulfide.h"
#include "../decane.h"
#include "../hydrogensulfide.h"
#include "../isohexane.h"
#include "../isopentane.h"
#include "../krypton.h"
#include "../neopentane.h"
#include "../nitrousoxide.h"
#include "../nonane.h"
#include "../sulfurdioxide.h"
#include "../toluene.h"
#include "../xenon.h"
#include "../butane.h"
#include "../butene.h"
#include "../cisbutene.h"
#include "../isobutene.h"
#include "../transbutene.h"
#include "../dimethylether.h"
#include "../ethane.h"
#include "../parahydrogen.h"
#include "../isobutane.h"
#include "../r41.h"
#include "../r116.h"
#include "../r141b.h"
#include "../r142b.h"
#include "../r218.h"
#include "../r245fa.h"

#include "../helmholtz.h"
#include "../sat.h"
#include "../solve_ph.h"
%}

%include exception.i

%immutable helmholtz_data_ammonia;
%immutable helmholtz_data_nitrogen;
%immutable helmholtz_data_hydrogen;
%immutable helmholtz_data_water;
%immutable helmholtz_data_carbondioxide;
%immutable helmholtz_data_methane;
%immutable helmholtz_data_carbonmonoxide;
%immutable helmholtz_data_ethanol;
%immutable helmholtz_data_acetone;
%immutable helmholtz_data_carbonylsulfide;
%immutable helmholtz_data_decane;
%immutable helmholtz_data_hydrogensulfide;
%immutable helmholtz_data_isohexane;
%immutable helmholtz_data_isopentane;
%immutable helmholtz_data_krypton;
%immutable helmholtz_data_neopentane;
%immutable helmholtz_data_nitrousoxide;
%immutable helmholtz_data_nonane;
%immutable helmholtz_data_sulfurdioxide;
%immutable helmholtz_data_toluene;
%immutable helmholtz_data_xenon;
%immutable helmholtz_data_butane;
%immutable helmholtz_data_butene;
%immutable helmholtz_data_cisbutene;
%immutable helmholtz_data_isobutene;
%immutable helmholtz_data_transbutene;
%immutable helmholtz_data_dimethylether;
%immutable helmholtz_data_ethane;
%immutable helmholtz_data_parahydrogen;
%immutable helmholtz_data_isobutane;
%immutable helmholtz_data_r41;
%immutable helmholtz_data_r116;
%immutable helmholtz_data_r141b;
%immutable helmholtz_data_r142b;
%immutable helmholtz_data_r218;
%immutable helmholtz_data_r245fa;

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

%include "../ammonia.h"
%include "../nitrogen.h"
%include "../hydrogen.h"
%include "../water.h"
%include "../carbondioxide.h"
%include "../methane.h"
%include "../carbonmonoxide.h"
%include "../ethanol.h"
%include "../acetone.h"
%include "../carbonylsulfide.h"
%include "../decane.h"
%include "../hydrogensulfide.h"
%include "../isohexane.h"
%include "../isopentane.h"
%include "../krypton.h"
%include "../neopentane.h"
%include "../nitrousoxide.h"
%include "../nonane.h"
%include "../sulfurdioxide.h"
%include "../toluene.h"
%include "../xenon.h"
%include "../butane.h"
%include "../butene.h"
%include "../cisbutene.h"
%include "../isobutene.h"
%include "../transbutene.h"
%include "../dimethylether.h"
%include "../ethane.h"
%include "../parahydrogen.h"
%include "../isobutane.h"
%include "../r41.h"
%include "../r116.h"
%include "../r141b.h"
%include "../r142b.h"
%include "../r218.h"
%include "../r245fa.h"

/*
If you're not using SCons, then try something like...
swig -python fprops.i 
gcc -I /usr/include/python2.5 -shared fprops_wrap.c ../helmholtz.c ../ideal.c ../water.c -o _fprops.so
*/

