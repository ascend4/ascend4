/* This file is created by Hongke Zhu, 06-10-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Ihmels, E.C.,
"Thermodynamic Properties of the Butenes.  Part II. Short Fundamental
Equations of State,"
Fluid Phase Equilibria, 228-229C:173-187, 2005.
*/

#include "transbutene.h"

#define TRANSBUTENE_M 56.10632 /* kg/kmol */
#define TRANSBUTENE_R (8314.472/TRANSBUTENE_M) /* J/kg/K */
#define TRANSBUTENE_TSTAR 428.61 /* K */


const IdealData ideal_data_transbutene = {
    0.5917816 /* constant */
    , 2.1427758 /* linear */
    , TRANSBUTENE_TSTAR /* Tstar */
    , TRANSBUTENE_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {3.9988,	0.0}
    }
    , 4 /* exponential terms */
    , (const IdealExpTerm[]){
        {5.3276,    362.0}
        ,{13.290,   1603.0}
        ,{9.6745,   3729.0}
        ,{0.40087,  4527.0}
    } 
};

const HelmholtzData helmholtz_data_transbutene = {
	"transbutene"
    , /* R */ TRANSBUTENE_R /* J/kg/K */
    , /* M */ TRANSBUTENE_M /* kg/kmol */
    , /* rho_star */ 4.213*TRANSBUTENE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ TRANSBUTENE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ TRANSBUTENE_TSTAR
    , /* rho_c */ 4.213*TRANSBUTENE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.21 /* acentric factor */
    , &ideal_data_transbutene
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.81107,         0.12,    1.0,   0}
        , {-2.8846,         1.3,     1.0,   0}
        , {1.0265,          1.74,    1.0,   0}
        , {0.016591,        2.1,     2.0,   0}
        , {0.086511,        0.28,    3.0,   0}
        , {0.00023256,      0.69,    7.0,   0}
        , {0.22654,         0.75,    2.0,   1}
        , {-0.072182,       2.0,     5.0,   1}
        , {-0.24849,        4.4,     1.0,   2}
        , {-0.071374,       4.7,     4.0,   2}
        , {-0.024737,       15.0,    3.0,   3}
        , {0.011843,       14.0,     4.0,   3}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py transbutene
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_transbutene, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_transbutene, ntd, td);
    return helm_run_test_cases(&helmholtz_data_transbutene, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 7.31716558346E+2, -2.08834478282E+2, -2.08697813341E+2, -9.45178269903E-1, 1.35256256477E+0, 1.95401581474E+0, 1.16402634564E+0, -4.51768608481E+1}
    , {-5.0E+1, 9.99999999999E-2, 6.8089771854E+2, -1.0870578649E+2, -1.08558921554E+2, -4.37649278803E-1, 1.42886948552E+0, 2.057596569E+0, 1.30683328675E+0, -1.1044349925E+1}
    , {0.E+0, 1.E-1, 6.27436124525E+2, -2.11425889387E+0, -1.95488012034E+0, -7.13757099641E-3, 1.54189521161E+0, 2.21882704362E+0, 1.46916857421E+0, -1.64631376205E-1}
    , {5.0E+1, 1.E-1, 2.1365400436E+0, 4.37735219807E+2, 4.84539865617E+2, 1.74657825472E+0, 1.51932268936E+0, 1.68502392617E+0, 1.65082327991E+0, -1.26671543205E+2}
    , {1.00E+2, 1.E-1, 1.83365675249E+0, 5.18465038521E+2, 5.73000872376E+2, 2.00080979448E+0, 1.69793549312E+0, 1.85592784731E+0, 1.83854697579E+0, -2.28137136291E+2}
    , {1.50E+2, 1.E-1, 1.60929301474E+0, 6.08073875465E+2, 6.70212963303E+2, 2.2450584338E+0, 1.87776233147E+0, 2.03222422073E+0, 2.02210829607E+0, -3.41922600799E+2}
    , {2.00E+2, 1.E-1, 1.43515948224E+0, 7.06451485057E+2, 7.76130152297E+2, 2.48146949894E+0, 2.05057350697E+0, 2.20315177109E+0, 2.19663494302E+0, -4.67655808367E+2}
    , {2.50E+2, 1.E-1, 1.29563485383E+0, 8.13197631902E+2, 8.90379871716E+2, 2.71087402202E+0, 2.21381054512E+0, 2.36526687518E+0, 2.36073150695E+0, -6.04996112719E+2}
    , {-1.00E+2, 1.E+0, 7.32209811261E+2, -2.09127592191E+2, -2.07761863427E+2, -9.46874023306E-1, 1.35287265492E+0, 1.95325530404E+0, 1.16402634564E+0, -4.51763550553E+1}
    , {-5.0E+1, 1.E+0, 6.81626263039E+2, -1.09149765985E+2, -1.07682686363E+2, -4.39642742155E-1, 1.42917901265E+0, 2.05583482362E+0, 1.30683328675E+0, -1.10434880727E+1}
    , {0.E+0, 1.E+0, 6.28588193655E+2, -2.80874310014E+0, -1.21787644039E+0, -9.6859362762E-3, 1.54203382502E+0, 2.2146245771E+0, 1.46916857421E+0, -1.63029606298E-1}
    , {5.0E+1, 1.E+0, 5.68391193408E+2, 1.13449142911E+2, 1.15208494589E+2, 3.81153456319E-1, 1.68973944774E+0, 2.46131600382E+0, 1.65082327991E+0, -9.72059649855E+0}
    , {1.00E+2, 1.E+0,  2.1446181103E+1, 4.99579381092E+2, 5.46207729291E+2, 1.60730617025E+0, 1.7826845614E+0, 2.09981771663E+0, 1.83854697579E+0, -1.00186916337E+2}
    , {1.50E+2, 1.E+0, 1.76559361847E+1, 5.95193829746E+2, 6.51832004553E+2, 1.87287365114E+0, 1.91624634034E+0, 2.14909075927E+0, 2.02210829607E+0, -1.97312655735E+2}
    , {2.00E+2, 1.E+0, 1.52479560406E+1, 6.96658737801E+2, 7.62241298332E+2, 2.11936786167E+0, 2.07102936892E+0, 2.27227320552E+0, 2.19663494302E+0, -3.06120165946E+2}
    , {2.50E+2, 1.E+0, 1.3507441104E+1, 8.05269543335E+2, 8.79302810799E+2, 2.3544410859E+0, 2.22575437773E+0, 2.41106060234E+0, 2.36073150695E+0, -4.26456310755E+2}
    , {-1.00E+2, 1.0E+1, 7.36969050204E+2, -2.1193971813E+2, -1.98370627274E+2, -9.63392215917E-1, 1.35627352343E+0, 1.94651505246E+0, 1.16402634564E+0, -4.51283559438E+1}
    , {-5.0E+1, 1.E+1, 6.88521301964E+2, -1.13336761544E+2, -9.88128826582E+1, -4.5876216902E-1, 1.43272845476E+0, 2.04068367448E+0, 1.30683328675E+0, -1.09639835271E+1}
    , {0.E+0, 1.E+1, 6.39102887343E+2, -9.15192450073E+0, 6.49500840795E+0, -3.34213975824E-2, 1.54446069304E+0, 2.18086818199E+0, 1.46916857421E+0, -2.28697510877E-2}
    , {5.0E+1, 1.E+1, 5.86107398904E+2, 1.03139759848E+2, 1.20201479285E+2, 3.48389520487E-1, 1.68842237974E+0, 2.37670552506E+0, 1.65082327991E+0, -9.44231369743E+0}
    , {1.00E+2, 1.0E+1, 5.25529091655E+2, 2.26171685264E+2, 2.45200127569E+2, 7.07600196681E-1, 1.85030838239E+0, 2.6359902911E+0, 1.83854697579E+0, -3.78693281274E+1}
    , {1.50E+2, 1.E+1, 4.49296667634E+2, 3.63443185218E+2, 3.85700194273E+2, 1.06045464051E+0, 2.0219563767E+0, 3.01371941405E+0, 2.02210829607E+0, -8.52881959135E+1}
    , {2.00E+2, 1.0E+1, 3.39578661464E+2, 5.21631501619E+2, 5.51079759519E+2, 1.42920540965E+0, 2.20326368575E+0, 3.63700191994E+0, 2.19663494302E+0, -1.54597037958E+2}
    , {2.50E+2, 1.0E+1, 2.20664346937E+2, 6.90384234899E+2, 7.35701931839E+2, 1.80026212645E+0, 2.33931439592E+0, 3.51191198615E+0, 2.36073150695E+0, -2.51422896555E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
