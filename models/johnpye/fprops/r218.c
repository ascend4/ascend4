/* This file is created by Hongke Zhu, 7-26-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "helmholtz.h"

#define R218_M 188.01933 /* kg/kmol */
#define R218_R (8314.472/R218_M) /* J/kg/K */
#define R218_TSTAR 345.02 /* K */

const IdealData ideal_data_r218 = {
    -15.6587335175 /* constant */
    , 11.4531412796 /* linear */
    , R218_TSTAR /* Tstar */
    , R218_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
    }
    , 3 /* exponential terms */
    , (const IdealExpTerm[]){
        {7.2198,    326.0}
        ,{7.2692,    595.0}
        ,{11.599,   1489.0}
    } 
};

const HelmholtzData helmholtz_data_r218 = {
    "r218"
	, /* R */ R218_R /* J/kg/K */
    , /* M */ R218_M /* kg/kmol */
    , /* rho_star */ 3.34*R218_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ R218_TSTAR /* K (= T_c for this model) */

    , /* T_c */ R218_TSTAR
    , /* rho_c */ 3.34*R218_M /* kg/m3 */
    , /* T_t */ 0

    , 0.3172 /* acentric factor */
    , &ideal_data_r218
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {1.3270,          0.25,    1.0,   0}
        , {-3.8433,         1.25,    1.0,   0}
 	, {0.92200,         1.5,     1.0,   0}
  	, {0.11360,         0.25,    3.0,   0}
  	, {0.00036195,      0.875,   7.0,   0}
  	, {1.1001,          2.375,   1.0,   1}
  	, {1.1896,          2.0,     2.0,   1}
 	, {-0.025147,       2.125,   5.0,   1}
 	, {-0.65923,        3.5,     1.0,   2}
 	, {-0.027969,       6.5,     1.0,   2}
 	, {-0.18330,        4.75,    4.0,   2}
 	, {-0.021630,       12.5,    2.0,   3}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py r218
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_r218, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_r218, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_r218, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_r218, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_r218, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_r218, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_r218, ntd, td);
    return helm_run_test_cases(&helmholtz_data_r218, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 1.84091047427E+3, 1.04721330925E+2, 1.04775651872E+2, 5.70398798424E-1, 5.68203557539E-1, 8.54126698839E-1, 5.53878209743E-1, 5.95677897758E+0}
    , {-5.0E+1, 1.E-1, 1.6623114573E+3, 1.49641465669E+2, 1.49701622868E+2, 7.97847861264E-1, 6.62742604818E-1, 9.46425360174E-1, 6.57377860316E-1, -2.83982845717E+1}
    , {0.E+0, 1.E-1, 8.52734888498E+0, 2.83042011708E+2, 2.94768985865E+2, 1.40550465537E+0, 7.10051805828E-1, 7.61763424972E-1, 7.46421688908E-1, -1.00871584905E+2}
    , {5.0E+1, 1.E-1, 7.10981071873E+0, 3.2053760084E+2, 3.34602672887E+2, 1.53925646462E+0, 7.83923354584E-1, 8.31788175666E-1, 8.25402804462E-1, -1.76873125702E+2}
    , {1.00E+2, 1.E-1, 6.11775882212E+0, 3.6153041725E+2, 3.77876272474E+2, 1.66365235906E+0, 8.51544796293E-1, 8.97969981282E-1, 8.94585287726E-1, -2.59261460534E+2}
    , {1.50E+2, 1.E-1, 5.37577897398E+0, 4.05662381868E+2, 4.24264337135E+2, 1.78023818889E+0, 9.10403575559E-1, 9.56129624135E-1, 9.54041414983E-1, -3.47645407761E+2}
    , {-1.00E+2, 1.E+0, 1.84284039278E+3, 1.04562556643E+2, 1.05105197234E+2, 5.69480019445E-1, 5.68452059225E-1, 8.53503213703E-1, 5.53878209743E-1, 5.95709127613E+0}
    , {-5.0E+1, 1.E+0, 1.66588529311E+3, 1.49371495272E+2, 1.49971776698E+2, 7.96634876785E-1, 6.62730756137E-1, 9.4441885525E-1, 6.57377860316E-1, -2.83975774825E+1}
    , {0.E+0, 1.E+0, 1.46007167296E+3, 1.99355885632E+2, 2.00040783517E+2, 9.98678486701E-1, 7.5012517614E-1, 1.06727072786E+0, 7.46421688908E-1, -7.34331430104E+1}
    , {5.0E+1, 1.E+0, 8.6162306303E+1, 3.14097632723E+2, 3.25703635892E+2, 1.41670188912E+0, 8.20057354091E-1, 9.40626237344E-1, 8.25402804462E-1, -1.43709582746E+2}
    , {1.00E+2, 1.E+0, 6.74422387677E+1, 3.57484668883E+2, 3.72312171918E+2, 1.55080361141E+0, 8.64133883876E-1, 9.39597944035E-1, 8.94585287726E-1, -2.21197698716E+2}
    , {1.50E+2, 1.E+0, 5.68953484132E+1, 4.02656313081E+2, 4.20232442377E+2, 1.67126737603E+0, 9.16201158362E-1, 9.78801500495E-1, 9.54041414983E-1, -3.04540477085E+2}
    , {-1.00E+2, 1.E+1, 1.8610238582E+3, 1.03067737633E+2, 1.08441123878E+2, 5.60681392081E-1, 5.71263547284E-1, 8.48365630237E-1, 5.53878209743E-1, 5.98575459454E+0}
    , {-5.0E+1, 1.E+1, 1.69773562046E+3, 1.46946796321E+2, 1.52836994936E+2, 7.85500518915E-1, 6.63636782959E-1, 9.29205409183E-1, 6.57377860316E-1, -2.8337644475E+1}
    , {0.E+0, 1.E+1, 1.52425675332E+3, 1.94959547116E+2, 2.01520121625E+2, 9.82044753551E-1, 7.46239699493E-1, 1.01972545894E+0, 7.46421688908E-1, -7.3285977316E+1}
    , {5.0E+1, 1.E+1, 1.3247230493E+3, 2.47435936471E+2, 2.54984684117E+2, 1.16155021166E+0, 8.230527383E-1, 1.1207629242E+0, 8.25402804462E-1, -1.27919014428E+2}
    , {1.00E+2, 1.0E+1, 1.08076691964E+3, 3.04477989215E+2, 3.13730678041E+2, 1.33039214742E+0, 8.93324112019E-1, 1.22799640654E+0, 8.94585287726E-1, -1.91957840594E+2}
    , {1.50E+2, 1.0E+1, 8.08686137126E+2, 3.64409426697E+2, 3.7677516359E+2, 1.48888349374E+0, 9.48407636582E-1, 1.2714976472E+0, 9.54041414983E-1, -2.65611623678E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
