/* This file is created by Hongke Zhu, 7-25-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "helmholtz.h"

#define R142B_M 100.49503 /* kg/kmol */
#define R142B_R (8314.472/R142B_M) /* J/kg/K */
#define R142B_TSTAR 410.26 /* K */

const IdealData ideal_data_r142b = {
    -12.6016527149 /* constant */
    , 8.3160183265 /* linear */
    , R142B_TSTAR /* Tstar */
    , R142B_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
    }
    , 4 /* exponential terms */
    , (const IdealExpTerm[]){
        {5.0385,    473.0}
 	, {6.8356,   1256.0}
 	, {4.0591,   2497.0}
 	, {2.8136,   6840.0}
    } 
};

const HelmholtzData helmholtz_data_r142b = {
	"r142b"
    , /* R */ R142B_R /* J/kg/K */
    , /* M */ R142B_M /* kg/kmol */
    , /* rho_star */ 4.438*R142B_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ R142B_TSTAR /* K (= T_c for this model) */

    , /* T_c */ R142B_TSTAR
    , /* rho_c */ 4.438*R142B_M /* kg/m3 */
    , /* T_t */ 0

    , 0.2321 /* acentric factor */
    , &ideal_data_r142b
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {1.0038,          0.25,    1.0,   0}
        , {-2.7662,       1.25,    1.0,   0}
  	, {0.42921,       1.5,     1.0,   0}
  	, {0.081363,      0.25,    3.0,   0}
  	, {0.00024174,    0.875,   7.0,   0}
  	, {0.48246,       2.375,   1.0,   1}
  	, {0.75542,       2.0,     2.0,   1}
 	, {-0.007430,     2.125,   5.0,   1}
 	, {-0.41460,      3.5,     1.0,   2}
 	, {-0.016558,     6.5,     1.0,   2}
 	, {-0.10644,      4.75,    4.0,   2}
 	, {-0.021704,     12.5,    2.0,   3}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py r142b
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_r142b, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_r142b, ntd, td);
    return helm_run_test_cases(&helmholtz_data_r142b, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 1.38613794042E+3, 8.32865717775E+1, 8.33587146696E+1, 4.70935917602E-1, 7.08438443083E-1, 1.10511648545E+0, 5.83782651152E-1, 1.7440176446E+0}
    , {-5.0E+1, 9.99999999999E-2, 1.28181271609E+3, 1.39749186009E+2, 1.39827200527E+2, 7.57146374042E-1, 7.72506540349E-1, 1.16024289885E+0, 6.86830092703E-1, -2.9208027358E+1}
    , {0.E+0, 1.E-1, 4.58765597798E+0, 3.97633468385E+2, 4.19431092375E+2, 1.83252362429E+0, 7.22414193547E-1, 8.23371751503E-1, 7.83452621413E-1, -1.02920359589E+2}
    , {5.0E+1, 1.E-1, 3.81308688113E+0, 4.3585472966E+2, 4.62080200812E+2, 1.9757535502E+0, 7.96432565692E-1, 8.87159254789E-1, 8.72226275011E-1, -2.02610030086E+2}
    , {1.00E+2, 1.E-1, 3.27746306167E+0, 4.77753596405E+2, 5.08265000536E+2, 2.10851427908E+0, 8.7259719144E-1, 9.59940851023E-1, 9.52262697859E-1, -3.09038506832E+2}
    , {1.50E+2, 1.E-1, 2.87847707837E+0, 5.23257411866E+2, 5.57998004644E+2, 2.23349882557E+0, 9.42613987292E-1, 1.02837821854E+0, 1.0237468952E+0, -4.21847616175E+2}
    , {-1.00E+2, 1.E+0, 1.38703184183E+3, 8.31173751559E+1, 8.3838339138E+1, 4.69957274473E-1, 7.088080858E-1, 1.10474627037E+0, 5.83782651152E-1, 1.74427308098E+0}
    , {-5.0E+1, 1.E+0, 1.28321008486E+3, 1.39492673766E+2, 1.40271969387E+2, 7.55994778065E-1, 7.72763360509E-1, 1.15913118976E+0, 6.86830092703E-1, -2.92075609596E+1}
    , {0.E+0, 1.E+0, 1.17392966964E+3, 1.99482462987E+2, 2.00334302775E+2, 9.98553714566E-1, 8.40545360957E-1, 1.24922710001E+0, 7.83452621413E-1, -7.32724841463E+1}
    , {5.0E+1, 1.E+0, 1.04590256333E+3, 2.65030189277E+2, 2.65986301285E+2, 1.21894407827E+0, 9.11218096179E-1, 1.38977237255E+0, 8.72226275011E-1, -1.28871589616E+2}
    , {1.00E+2, 1.E+0, 3.72172211672E+1, 4.69022402011E+2, 4.95891683721E+2, 1.89399990424E+0, 9.09578731132E-1, 1.06497210131E+0, 9.52262697859E-1, -2.37723662257E+2}
    , {1.50E+2, 1.E+0, 3.10713231924E+1, 5.17121401528E+2, 5.49305418726E+2, 2.02830910399E+0, 9.59013171129E-1, 1.08052292234E+0, 1.0237468952E+0, -3.41157595827E+2}
    , {-1.00E+2, 1.E+1, 1.3956576187E+3, 8.14910633603E+1, 8.86561444418E+1, 4.60424705653E-1, 7.12655205342E-1, 1.10156944202E+0, 5.83782651152E-1, 1.7685255764E+0}
    , {-5.0E+1, 1.E+1, 1.29639792434E+3, 1.37076055282E+2, 1.44789736253E+2, 7.44973099377E-1, 7.75644303854E-1, 1.14965479302E+0, 6.86830092703E-1, -2.91646918442E+1}
    , {0.E+0, 1.E+1, 1.19522994206E+3, 1.95721351615E+2, 2.04087942552E+2, 9.84487551527E-1, 8.41884419293E-1, 1.22611868021E+0, 7.83452621413E-1, -7.31914230843E+1}
    , {5.0E+1, 1.0E+1, 1.08491675355E+3, 2.5855258643E+2, 2.67769883488E+2, 1.19834802119E+0, 9.08613675387E-1, 1.32520555452E+0, 8.72226275011E-1, -1.28693576617E+2}
    , {1.00E+2, 1.E+1, 9.54991803877E+2, 3.26693475881E+2, 3.37164769938E+2, 1.39778758979E+0, 9.74657117837E-1, 1.45895188079E+0, 9.52262697859E-1, -1.94890963248E+2}
    , {1.50E+2, 1.0E+1, 7.82560821366E+2, 4.02393961633E+2, 4.1517252111E+2, 1.59367474973E+0, 1.04222888643E+0, 1.68765895925E+0, 1.0237468952E+0, -2.71969508716E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
