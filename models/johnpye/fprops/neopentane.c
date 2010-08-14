/* This file is created by Hongke Zhu, 05-30-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "neopentane.h"

#define NEOPENTANE_M 72.14878 /* kg/kmol */
#define NEOPENTANE_R (8314.472/NEOPENTANE_M) /* J/kg/K */
#define NEOPENTANE_TSTAR 433.74 /* K */


const IdealData ideal_data_neopentane = {
    0.8702452614 /* constant */
    , 1.6071746358 /* linear */
    , NEOPENTANE_TSTAR /* Tstar */
    , NEOPENTANE_R /* cp0star */
    , 1 /* power terms */
    , (const IdealPowTerm[]){
        {4.0,	0.0}
    }
    , 4 /* exponential terms */
    , (const IdealExpTerm[]){
        {14.422,	710.0}
        ,{12.868,	1725.0}
        ,{17.247,	3280.0}
        ,{12.663,	7787.0}
    }
};

const HelmholtzData helmholtz_data_neopentane = {
    "neopentane"
	, /* R */ NEOPENTANE_R /* J/kg/K */
    , /* M */ NEOPENTANE_M /* kg/kmol */
    , /* rho_star */ 3.27*NEOPENTANE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ NEOPENTANE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ NEOPENTANE_TSTAR
    , /* rho_c */ 3.27*NEOPENTANE_M /* kg/m3 */
    , /* T_t */ 0

    , 0.1961 /* acentric factor */
    , &ideal_data_neopentane
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {1.1136,	0.25,	1.0,	0.0}
        , {-3.1792,	1.125,	1.0,	0.0}
        , {1.1411,	1.5,	1.0,	0.0}
        , {-0.10467,	1.375,	2.0,	0.0}
        , {0.11754,	0.25,	3.0,	0.0}
        , {0.00034058,	0.875,	7.0,	0.0}
        , {0.29553,	0.625,	2.0,	1.0}
        , {-0.074765,	1.75,	5.0,	1.0}
        , {-0.31474,	3.625,	1.0,	2.0}
        , {-0.099401,	3.625,	4.0,	2.0}
        , {-0.039569,	14.5,	3.0,	3.0}
        , {0.023177,	12.0,	4.0,	3.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py neopentane
*/

#ifdef TEST

#include "test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_neopentane, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_neopentane, ntd, td);
    return helm_run_test_cases(&helmholtz_data_neopentane, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {0.E+0, 1.E-1, 6.1112484306E+2, -2.10036299949E+1, -2.08399973081E+1, -7.498458314E-2, 1.61109921434E+0, 2.16595869764E+0, 1.54407341408E+0, -5.2159111017E-1}
    , {5.0E+1, 1.E-1, 2.76174328726E+0, 3.49851027003E+2, 3.86060040513E+2, 1.3507288737E+0, 1.69831158852E+0, 1.82897873709E+0, 1.80235887256E+0, -8.66370085328E+1}
    , {1.00E+2, 1.E-1, 2.36625242923E+0, 4.40960937654E+2, 4.83221855705E+2, 1.62989805181E+0, 1.93207401765E+0, 2.05638896563E+0, 2.04171012665E+0, -1.67235520378E+2}
    , {1.50E+2, 1.E-1, 2.07449403825E+0, 5.43352867651E+2, 5.91557392781E+2, 1.90206509893E+0, 2.15427746E+0, 2.27548585369E+0, 2.26641836784E+0, -2.61505978962E+2}
    , {2.00E+2, 1.E-1, 1.84873689304E+0, 6.56522728281E+2, 7.10613713524E+2, 2.16778497364E+0, 2.36573024758E+0, 2.48520142797E+0, 2.47912458216E+0, -3.69164731999E+2}
    , {2.50E+2, 1.E-1, 1.66822876424E+0, 7.79977737712E+2, 8.39921554857E+2, 2.4274108576E+0, 2.56714733589E+0, 2.68555224249E+0, 2.6812193798E+0, -4.8992225244E+2}
    , {0.E+0, 1.E+0, 6.12380409463E+2, -2.16768657578E+1, -2.0043893859E+1, -7.74560265948E-2, 1.61166617418E+0, 2.16194584954E+0, 1.54407341408E+0, -5.19752093386E-1}
    , {5.0E+1, 1.E+0, 5.5696543817E+2, 9.34569186469E+1, 9.52523621905E+1, 3.09461052179E-1, 1.82902280044E+0, 2.46017584672E+0, 1.80235887256E+0, -6.54542036472E+0}
    , {1.00E+2, 1.E+0, 2.93749994197E+1, 4.22154236861E+2, 4.56196790725E+2, 1.3115998306E+0, 2.00980538859E+0, 2.32926920665E+0, 2.04171012665E+0, -6.72692399283E+1}
    , {1.50E+2, 1.E+0, 2.34632375736E+1, 5.3070120315E+2, 5.73321067389E+2, 1.60606505928E+0, 2.18720705098E+0, 2.3924949781E+0, 2.26641836784E+0, -1.48905226685E+2}
    , {2.00E+2, 1.E+0, 2.00185586246E+1, 6.46895807296E+2, 6.96849453747E+2, 1.88182375644E+0, 2.38395764776E+0, 2.55389045562E+0, 2.47912458216E+0, -2.43489103065E+2}
    , {2.50E+2, 1.E+0, 1.7616892159E+1, 7.72191837902E+2, 8.28955538955E+2, 2.14709086763E+0, 2.57833061796E+0, 2.73120706129E+0, 2.6812193798E+0, -3.51058749498E+2}
    , {0.E+0, 1.E+1, 6.23722460086E+2, -2.77402923055E+1, -1.17075202956E+1, -1.00234632874E-1, 1.61759037502E+0, 2.13067282946E+0, 1.54407341408E+0, -3.61202336016E-1}
    , {5.0E+1, 1.E+1, 5.75775499975E+2, 8.37577719181E+1, 1.01125652282E+2, 2.78501183398E-1, 1.83268063745E+0, 2.3853802576E+0, 1.80235887256E+0, -6.23988549694E+0}
    , {1.00E+2, 1.E+1, 5.21751061986E+2, 2.07978437122E+2, 2.2714466548E+2, 6.40621160435E-1, 2.0448953655E+0, 2.65925301225E+0, 2.04171012665E+0, -3.10693488945E+1}
    , {1.50E+2, 1.E+1, 4.57387932879E+2, 3.45666362698E+2, 3.67529641725E+2, 9.93280803509E-1, 2.25468622981E+0, 2.96156486347E+0, 2.26641836784E+0, -7.4640409307E+1}
    , {2.00E+2, 1.E+1, 3.77964541869E+2, 4.97257111725E+2, 5.23714620016E+2, 1.34181516067E+0, 2.46167311294E+0, 3.28546377014E+0, 2.47912458216E+0, -1.37622731544E+2}
    , {2.50E+2, 1.0E+1, 2.88770452002E+2, 6.59788672044E+2, 6.94418253882E+2, 1.68459234288E+0, 2.65491719216E+0, 3.50027753389E+0, 2.6812193798E+0, -2.21505812136E+2}
    , {1.50E+2, 1.E+2, 6.08776482726E+2, 2.73938346504E+2, 4.38202247686E+2, 7.75723533886E-1, 2.29077157334E+0, 2.65065806283E+0, 2.26641836784E+0, -5.43090668595E+1}
    , {2.00E+2, 1.00E+2, 5.84560179633E+2, 4.04326457806E+2, 5.75395243338E+2, 1.08198119133E+0, 2.48528604258E+0, 2.83571416245E+0, 2.47912458216E+0, -1.07612942871E+2}
    , {2.50E+2, 1.00E+2, 5.61596136654E+2, 5.43572384884E+2, 7.2163628788E+2, 1.37564762115E+0, 2.67312915663E+0, 3.01253111597E+0, 2.6812193798E+0, -1.76097668121E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
