/* This file is created by Hongke Zhu, 7-25-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "r41.h"

#define R41_M 34.03292 /* kg/kmol */
#define R41_R (8314.472/R41_M) /* J/kg/K */
#define R41_TSTAR 317.28 /* K */


const IdealData ideal_data_r41 = {
    -4.8676441160 /* constant */
    , 4.2527951258 /* linear */
    , R41_TSTAR /* Tstar */
    , R41_R /* cp0star */
    , 2 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
	, {0.00016937,	1.0}
    }
    , 2 /* exponential terms */
    , (const IdealExpTerm[]){
        {5.6936,     1841.0}
        , {2.9351,     4232.0}
    } 
};

const HelmholtzData helmholtz_data_r41 = {
	"r41"
    , /* R */ R41_R /* J/kg/K */
    , /* M */ R41_M /* kg/kmol */
    , /* rho_star */ 9.3*R41_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ R41_TSTAR /* K (= T_c for this model) */

    , /* T_c */ R41_TSTAR
    , /* rho_c */ 9.3*R41_M /* kg/m3 */
    , /* T_t */ 0

    , 0.2004 /* acentric factor */
    , &ideal_data_r41
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {1.6264,          0.52,    1.0,   0}
        , {-2.8337,         1.12,    1.0,   0}
  	, {0.0010932,       4.0,     1.0,   0}
 	, {0.037136,        0.03,    3.0,   0}
  	, {0.00018724,      0.63,    7.0,   0}
 	, {-0.22189,        3.4,     1.0,   1}
  	, {0.55021,         2.2,     2.0,   1}
  	, {0.046100,        1.5,     5.0,   1}
 	, {-0.056405,       0.1,     1.0,   2}
 	, {-0.17005,        4.8,     1.0,   2}
 	, {-0.032409,       3.5,     4.0,   2}
 	, {-0.012276,       15.0,    2.0,   3}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py r41
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_r41, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_r41, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_r41, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_r41, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_r41, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_r41, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_r41, ntd, td);
    return helm_run_test_cases(&helmholtz_data_r41, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 9.24980088556E+2, -1.99303694221E+1, -1.98222589869E+1, 1.6029011057E-2, 1.30940733353E+0, 2.06326405214E+0, 9.88184909757E-1, -2.27057926867E+1}
    , {-5.0E+1, 1.E-1, 1.87849066134E+0, 4.91412081433E+2, 5.44646309347E+2, 2.91941311977E+0, 8.05659380714E-1, 1.08378525422E+0, 1.01120828777E+0, -1.60054956243E+2}
    , {0.E+0, 1.E-1, 1.51562785032E+0, 5.32582349703E+2, 5.98561607065E+2, 3.13739666416E+0, 8.31253995863E-1, 1.08939305009E+0, 1.06347509421E+0, -3.24397549112E+2}
    , {5.0E+1, 1.E-1, 1.27476690481E+0, 5.7609855393E+2, 6.54544267906E+2, 3.32544797075E+0, 9.04281166858E-1, 1.15600640953E+0, 1.14337775187E+0, -4.98519957819E+2}
    , {1.00E+2, 1.E-1, 1.10124766361E+0, 6.23777907884E+2, 7.1458400292E+2, 3.49803946058E+0, 9.99451950233E-1, 1.24834298998E+0, 1.24109000437E+0, -6.81515516832E+2}
    , {1.50E+2, 1.E-1, 9.69787990358E-1, 6.76434482867E+2, 7.79549803941E+2, 3.6612879683E+0, 1.10375467766E+0, 1.35115932339E+0, 1.34651126469E+0, -8.72839520921E+2}
    , {-1.00E+2, 1.E+0, 9.25947274924E+2, -2.02937185905E+1, -1.92137434914E+1, 1.39269650927E-2, 1.3092377534E+0, 2.0609679334E+0, 9.88184909757E-1, -2.27051725963E+1}
    , {-5.0E+1, 1.E+0, 8.18601882889E+2, 8.35028472243E+1, 8.47244422644E+1, 5.40924823566E-1, 1.21189322049E+0, 2.13191641089E+0, 1.01120828777E+0, -3.72045271545E+1}
    , {0.E+0, 1.E+0, 1.70656169564E+1, 5.13898477586E+2, 5.72495831702E+2, 2.50517696416E+0, 9.67654410261E-1, 1.4160561752E+0, 1.06347509421E+0, -1.70390610175E+2}
    , {5.0E+1, 1.E+0, 1.35510085069E+1, 5.64922367302E+2, 6.38717612836E+2, 2.72809551738E+0, 9.5398246117E-1, 1.28578376241E+0, 1.14337775187E+0, -3.16661699138E+2}
    , {1.00E+2, 1.E+0, 1.14197334128E+1, 6.159534043E+2, 7.03521122727E+2, 2.91449545222E+0, 1.02393139471E+0, 1.31824235904E+0, 1.24109000437E+0, -4.71590573696E+2}
    , {1.50E+2, 1.E+0, 9.92526990571E+0, 6.70490704963E+2, 7.71243632541E+2, 3.08471141936E+0, 1.11776489943E+0, 1.39462561108E+0, 1.34651126469E+0, -6.34804932138E+2}
    , {-1.00E+2, 1.0E+1, 9.35141037806E+2, -2.37396059925E+1, -1.3046031873E+1, -6.30639886112E-3, 1.30798547289E+0, 2.04071698197E+0, 9.88184909757E-1, -2.26476530297E+1}
    , {-5.0E+1, 1.E+1, 8.35307660891E+2, 7.70410371188E+1, 8.90126739996E+1, 5.1138340454E-1, 1.21075570262E+0, 2.06694489916E+0, 1.01120828777E+0, -3.70741696043E+1}
    , {0.E+0, 1.E+1, 7.14978440371E+2, 1.83113509321E+2, 1.97099945044E+2, 9.4764148325E-1, 1.18919806525E+0, 2.30892751101E+0, 1.06347509421E+0, -7.5734761829E+1}
    , {5.0E+1, 1.0E+1, 5.22729605608E+2, 3.14298254163E+2, 3.33428603568E+2, 1.40332399113E+0, 1.26328523711E+0, 3.580167677E+0, 1.14337775187E+0, -1.39185893572E+2}
    , {1.00E+2, 1.0E+1, 1.89482571675E+2, 5.09688928852E+2, 5.62464231149E+2, 2.06413152473E+0, 1.29603321023E+0, 3.07868085845E+0, 1.24109000437E+0, -2.60541749599E+2}
    , {1.50E+2, 1.0E+1, 1.27395304477E+2, 6.0355537795E+2, 6.82051206671E+2, 2.36616436094E+0, 1.25413475871E+0, 2.03231400813E+0, 1.34651126469E+0, -3.9768707138E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
