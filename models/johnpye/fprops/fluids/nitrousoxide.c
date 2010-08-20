/* This file is created by Hongke Zhu, 05-30-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"

#define NITROUSOXIDE_M 44.0128 /* kg/kmol */
#define NITROUSOXIDE_R (8314.472/NITROUSOXIDE_M) /* J/kg/K */
#define NITROUSOXIDE_TSTAR 309.52 /* K */

const IdealData ideal_data_nitrousoxide = {
    -4.4262736272 /* constant */
    , 4.3120475243 /* linear */
    , NITROUSOXIDE_TSTAR /* Tstar */
    , NITROUSOXIDE_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {3.5,	0.0}
    }
    , 3 /* exponential terms */
    , (const IdealExpTerm[]){
        {2.1769,	879.0}
        ,{1.6145,	2372.0}
        ,{0.48393,	5447.0}
    }
};

const HelmholtzData helmholtz_data_nitrousoxide = {
	"nitrousoxide"
    , /* R */ NITROUSOXIDE_R /* J/kg/K */
    , /* M */ NITROUSOXIDE_M /* kg/kmol */
    , /* rho_star */ 10.27*NITROUSOXIDE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ NITROUSOXIDE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ NITROUSOXIDE_TSTAR
    , /* rho_c */ 10.27*NITROUSOXIDE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.1613 /* acentric factor */
    , &ideal_data_nitrousoxide
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.88045,	0.25,	1.0,	0.0}
        , {-2.4235,	1.25,	1.0,	0.0}
        , {0.38237,	1.5,	1.0,	0.0}
        , {0.068917,	0.25,	3.0,	0.0}
        , {0.00020367,	0.875,	7.0,	0.0}
        , {0.13122,	2.375,	1.0,	1.0}
        , {0.46032,	2.0,	2.0,	1.0}
        , {-0.0036985,	2.125,	5.0,	1.0}
        , {-0.23263,	3.5,	1.0,	2.0}
        , {-0.00042859,	6.5,	1.0,	2.0}
        , {-0.042810,	4.75,	4.0,	2.0}
        , {-0.023038,	12.5,	2.0,	3.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py nitrousoxide
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_nitrousoxide, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_nitrousoxide, ntd, td);
    return helm_run_test_cases(&helmholtz_data_nitrousoxide, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-5.0E+1, 1.E-1, 2.40448110259E+0, 3.6350913306E+2, 4.05098147793E+2, 2.18059407864E+0, 6.09832573957E-1, 8.10075603955E-1, 7.91214139249E-1, -1.23090435588E+2}
    , {0.E+0, 1.E-1, 1.95182354872E+0, 3.95557863112E+2, 4.46792002625E+2, 2.34901818376E+0, 6.63942172292E-1, 8.58614957604E-1, 8.50090445226E-1, -2.46076453781E+2}
    , {5.0E+1, 1.E-1, 1.6449640892E+0, 4.30143459284E+2, 4.90935059938E+2, 2.49731052586E+0, 7.14004125771E-1, 9.06430208339E-1, 9.01544458655E-1, -3.76862437148E+2}
    , {1.00E+2, 1.E-1, 1.42229413657E+0, 4.67053030553E+2, 5.37361975397E+2, 2.63081861476E+0, 7.58660896827E-1, 9.49934582323E-1, 9.4678183394E-1, -5.14636935546E+2}
    , {1.50E+2, 1.E-1, 1.25306458983E+0, 5.06057759281E+2, 5.85862105211E+2, 2.75274072946E+0, 7.98849090798E-1, 9.89451878592E-1, 9.87254361842E-1, -6.5876448039E+2}
    , {2.00E+2, 1.E-1, 1.11999121192E+0, 5.46962396913E+2, 6.36248811785E+2, 2.86525282737E+0, 8.35283739822E-1, 1.02546144211E+0, 1.02384430769E+0, -8.08731978359E+2}
    , {2.50E+2, 1.E-1, 1.01255802862E+0, 5.89595154061E+2, 6.88354926019E+2, 2.96991243015E+0, 8.68377712478E-1, 1.05826909234E+0, 1.05703057273E+0, -9.64114533773E+2}

    , {-5.0E+1, 1.E+0, 1.11052099523E+3, 6.66834644765E+1, 6.75839427243E+1, 3.28432681805E-1, 9.21642323812E-1, 1.79706623262E+0, 7.91214139249E-1, -6.60628846841E+0}
    , {0.E+0, 1.E+0, 2.09536522356E+1, 3.87963536744E+2, 4.356879138E+2, 1.8857971489E+0, 6.96383057117E-1, 9.59292677113E-1, 8.50090445226E-1, -1.27141954478E+2}
    , {5.0E+1, 1.E+0, 1.71111871764E+1, 4.24880435283E+2, 4.83321733931E+2, 2.04594253543E+0, 7.27234721003E-1, 9.55694396193E-1, 9.01544458655E-1, -2.36265895042E+2}
    , {1.00E+2, 1.E+0, 1.45670502051E+1, 4.63025754939E+2, 5.31673833028E+2, 2.1850224788E+0, 7.65946006317E-1, 9.80113361087E-1, 9.4678183394E-1, -3.52315383027E+2}
    , {1.50E+2, 1.E+0, 1.27214879707E+1, 5.02810401643E+2, 5.81417558472E+2, 2.31008480665E+0, 8.03440270858E-1, 1.00996489674E+0, 9.87254361842E-1, -4.74701984292E+2}
    , {2.00E+2, 1.E+0, 1.13092402006E+1, 5.44254356175E+2, 6.32677626197E+2, 2.42455358028E+0, 8.38441664241E-1, 1.04033903303E+0, 1.02384430769E+0, -6.02923170336E+2}
    , {2.50E+2, 1.E+0, 1.01886356579E+1, 5.87283183535E+2, 6.85431751565E+2, 2.53051812046E+0, 8.70693936596E-1, 1.06955987951E+0, 1.05703057273E+0, -7.36557371186E+2}

    , {-5.0E+1, 1.E+1, 1.13084365695E+3, 6.15351335248E+1, 7.03780889047E+1, 3.04973408141E-1, 9.25478698601E-1, 1.74040810902E+0, 7.91214139249E-1, -6.51968250179E+0}
    , {0.E+0, 1.E+1, 9.50831461617E+2, 1.5175189489E+2, 1.62269005865E+2, 6.75719646059E-1, 9.07490313537E-1, 1.9924735799E+0, 8.50090445226E-1, -3.28209264311E+1}
    , {5.0E+1, 1.0E+1, 5.30731251965E+2, 2.88390594064E+2, 3.07232521995E+2, 1.15568392687E+0, 1.01326959605E+0, 6.53897771627E+0, 9.01544458655E-1, -8.50686669047E+1}
    , {1.00E+2, 1.0E+1, 1.98285561482E+2, 4.11508822311E+2, 4.61941137833E+2, 1.60963901764E+0, 8.58465820405E-1, 1.63480144063E+0, 9.4678183394E-1, -1.89127977123E+2}
    , {1.50E+2, 1.0E+1, 1.49495433746E+2, 4.66504949857E+2, 5.3339662507E+2, 1.78975565599E+0, 8.52246371713E-1, 1.30703053633E+0, 9.87254361842E-1, -2.90830155976E+2}
    , {2.00E+2, 1.0E+1, 1.2425096007E+2, 5.15738582324E+2, 5.96220857828E+2, 1.93017384229E+0, 8.70075717041E-1, 1.2219028326E+0, 1.02384430769E+0, -3.97523171154E+2}
    , {2.50E+2, 1.0E+1, 1.07771948588E+2, 5.63724057822E+2, 6.56512581469E+2, 2.05132902086E+0, 8.93299840772E-1, 1.19516104689E+0, 1.05703057273E+0, -5.09428719438E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
