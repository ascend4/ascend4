/* This file is created by Hongke Zhu, 06-17-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Ihmels, E.C. and Lemmon, E.W.
"Experimental Densities, Vapor Pressures, and Critical Point, and a Fundamental Equation of State for Dimethyl Ether,"
in press, Fluid Phase Equilibria, 2007.
*/

#include "../helmholtz.h"

#define DIMETHYLETHER_M 46.06844 /* kg/kmol */
#define DIMETHYLETHER_R (8314.472/DIMETHYLETHER_M) /* J/kg/K */
#define DIMETHYLETHER_TSTAR 400.3 /* K */

const IdealData ideal_data_dimethylether = {
    -1.928925 /* constant */
    , 3.150284 /* linear */
    , DIMETHYLETHER_TSTAR /* Tstar */
    , DIMETHYLETHER_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {4.039,	0.0}
    }
    , 4 /* exponential terms */
    , (const IdealExpTerm[]){
        {2.641,    361.0}
        ,{2.123,   974.0}
        ,{8.992,   1916.0}
        ,{6.191,   4150.0}
    }
};

const HelmholtzData helmholtz_data_dimethylether = {
	"dimethylether"
    , /* R */ DIMETHYLETHER_R /* J/kg/K */
    , /* M */ DIMETHYLETHER_M /* kg/kmol */
    , /* rho_star */ 6.013*DIMETHYLETHER_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ DIMETHYLETHER_TSTAR /* K (= T_c for this model) */

    , /* T_c */ DIMETHYLETHER_TSTAR
    , /* rho_c */ 6.013*DIMETHYLETHER_M /* kg/m3 */
    , /* T_t */ 0

    , 0.197 /* acentric factor */
    , &ideal_data_dimethylether
    , 10 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {1.22690,       0.21,   1.0,  	0.0}
        , {-2.47245,    1.0,    1.0,	0.0}
  	, {0.119889,    0.5,    3.0,    0.0}
  	, {0.0000354,   1.0,    8.0,    0.0}
  	, {0.567139,    1.4,    2.0,    1.0}
  	, {0.166649,    3.1,    1.0,    1.0}
 	, {-0.078412,   1.5,    5.0,    1.0}
 	, {-0.289066,   5.0,    1.0,    2.0}
 	, {-0.031272,   5.9,    4.0,    2.0}
 	, {-0.065607,   3.7,    3.0,    2.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py dimethylether
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_dimethylether, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_dimethylether, ntd, td);
    return helm_run_test_cases(&helmholtz_data_dimethylether, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 8.31894601956E+2, -1.63160366855E+2, -1.63040159319E+2, -7.81028795904E-1, 1.52247244099E+0, 2.13569275474E+0, 1.11201315613E+0, -2.7925230844E+1}
    , {-5.0E+1, 9.99999999999E-2, 7.68913537962E+2, -5.55940001819E+1, -5.54639465477E+1, -2.35439663017E-1, 1.484871802E+0, 2.17835580527E+0, 1.23155462508E+0, -3.05563937971E+0}
    , {0.E+0, 1.E-1, 2.08656134474E+0, 4.49411669831E+2, 4.97337411508E+2, 1.99819519387E+0, 1.22789142481E+0, 1.44191416907E+0, 1.35974026423E+0, -9.63953473735E+1}
    , {5.0E+1, 1.E-1, 1.73966304562E+0, 5.13902839914E+2, 5.7138523586E+2, 2.24692224549E+0, 1.33756580742E+0, 1.53215586031E+0, 1.50106878919E+0, -2.12190083717E+2}
    , {1.00E+2, 1.E-1, 1.49786876396E+0, 5.84469076561E+2, 6.51230599604E+2, 2.4764295464E+0, 1.47714397448E+0, 1.66519279146E+0, 1.65071866747E+0, -3.39610608676E+2}
    , {1.50E+2, 1.E-1, 1.31692766958E+0, 6.62153760089E+2, 7.38088074713E+2, 2.69467891084E+0, 1.62451605962E+0, 1.80976233821E+0, 1.80184733302E+0, -4.78099621033E+2}
    , {2.00E+2, 1.E-1, 1.1756772785E+0, 7.47144595057E+2, 8.32201950361E+2, 2.90475222054E+0, 1.77042145445E+0, 1.95423899413E+0, 1.94932593634E+0, -6.27238918093E+2}
    , {2.50E+2, 1.E-1, 1.06209755569E+0, 8.39273358976E+2, 9.33426668589E+2, 3.10800705996E+0, 1.91070361518E+0, 2.09369051776E+0, 2.09032875234E+0, -7.86680534441E+2}
    , {-1.00E+2, 1.E+0, 8.32445415621E+2, -1.63435597281E+2, -1.62234317316E+2, -7.82620868489E-1, 1.52171947757E+0, 2.13468201636E+0, 1.11201315613E+0, -2.79247939026E+1}
    , {-5.0E+1, 1.E+0, 7.69742079677E+2, -5.60344992376E+1, -5.47353627782E+1, -2.37417111357E-1, 1.48438996613E+0, 2.17608371736E+0, 1.23155462508E+0, -3.05487083815E+0}
    , {0.E+0, 1.E+0, 7.01117317546E+2, 5.53070785326E+1, 5.67333733552E+1, 2.12895886623E-1, 1.51091831208E+0, 2.29957869215E+0, 1.35974026423E+0, -2.84543289852E+0}
    , {5.0E+1, 1.E+0, 2.05421692184E+1, 4.90224241602E+2, 5.3890459222E+2, 1.7557887633E+0, 1.52144083915E+0, 1.9565238653E+0, 1.50106878919E+0, -7.71588972587E+1}
    , {1.00E+2, 1.E+0, 1.63571641587E+1, 5.70805679872E+2, 6.31940971436E+2, 2.0236863719E+0, 1.5459473706E+0, 1.82747892257E+0, 1.65071866747E+0, -1.84332889801E+2}
    , {1.50E+2, 1.E+0, 1.39095337393E+1, 6.52687561949E+2, 7.24580697891E+2, 2.2565818756E+0, 1.65479876625E+0, 1.89167967281E+0, 1.80184733302E+0, -3.02185058712E+2}
    , {2.00E+2, 1.E+0, 1.2195490934E+1, 7.39853817676E+2, 8.2185133671E+2, 2.47374074745E+0, 1.78528867331E+0, 2.00287403992E+0, 1.94932593634E+0, -4.30596616979E+2}
    , {2.50E+2, 1.E+0, 1.08958838468E+1, 8.33279022748E+2, 9.25056799941E+2, 2.68098874952E+0, 1.91867721862E+0, 2.12602179921E+0, 2.09032875234E+0, -5.69280241561E+2}
    , {-1.00E+2, 1.0E+1, 8.37742147172E+2, -1.66084505988E+2, -1.54147658792E+2, -7.98157756789E-1, 1.51551907027E+0, 2.1255158921E+0, 1.11201315613E+0, -2.78834903997E+1}
    , {-5.0E+1, 1.E+1, 7.77565751982E+2, -6.01984967384E+1, -4.73378480094E+1, -2.56394013661E-1, 1.48104214171E+0, 2.15617451838E+0, 1.23155462508E+0, -2.98417258991E+0}
    , {0.E+0, 1.E+1, 7.13703331897E+2, 4.85591161789E+1, 6.25705401881E+1, 1.87699568169E-1, 1.50598957464E+0, 2.25245278858E+0, 1.35974026423E+0, -2.71102086653E+0}
    , {5.0E+1, 1.E+1, 6.41871539237E+2, 1.63748185277E+2, 1.79327626627E+2, 5.79799549762E-1, 1.57857289346E+0, 2.43544357511E+0, 1.50106878919E+0, -2.36140392287E+1}
    , {1.00E+2, 1.0E+1, 5.52056316141E+2, 2.90726252572E+2, 3.08840346565E+2, 9.51841622793E-1, 1.69099017759E+0, 2.79253996495E+0, 1.65071866747E+0, -6.44534489734E+1}
    , {1.50E+2, 1.E+1, 4.02261427806E+2, 4.46340276649E+2, 4.7119973199E+2, 1.35864322515E+0, 1.86139274267E+0, 4.00669826042E+0, 1.80184733302E+0, -1.28569604073E+2}
    , {2.00E+2, 1.0E+1, 2.02581503645E+2, 6.30482371337E+2, 6.79845219475E+2, 1.82553691685E+0, 1.96338225562E+0, 3.4670592235E+0, 1.94932593634E+0, -2.33270420869E+2}
    , {2.50E+2, 1.0E+1, 1.42778943912E+2, 7.59719653545E+2, 8.29757992014E+2, 2.12732436647E+0, 2.01059544652E+0, 2.72807974819E+0, 2.09032875234E+0, -3.53190088777E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
