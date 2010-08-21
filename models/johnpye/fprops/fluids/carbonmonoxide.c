/* This file is created by Hongke Zhu, 01-27-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE \
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"

#define CARBONMONOXIDE_M 28.0101 /* kg/kmol */
#define CARBONMONOXIDE_R (8314.472/CARBONMONOXIDE_M) /* J/kg/K */
#define CARBONMONOXIDE_TSTAR 132.86 /* K */

const IdealData ideal_data_carbonmonoxide = {
    -3.3728318564 /* constant, a_1, adjust to solver s */
    , 3.3683460039 /* linear, a_2, adjust to solver h */
    , CARBONMONOXIDE_TSTAR /* Tstar */
    , CARBONMONOXIDE_R /* cp0star */
    , 2 /* power terms */
    , (const IdealPowTerm[]){
        {3.5,	0.0}
        ,{0.22311e-6,	1.5}
    }
    , 1 /* exponential terms */
    , (const IdealExpTerm[]){
        {1.0128,3089.0}
    }
};

const HelmholtzData helmholtz_data_carbonmonoxide = {
	"carbonmonoxide"
    , /* R */ CARBONMONOXIDE_R /* J/kg/K */
    , /* M */ CARBONMONOXIDE_M /* kg/kmol */
    , /* rho_star */ 10.85*CARBONMONOXIDE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ CARBONMONOXIDE_TSTAR /* K (= T_c for this model) */

    , /* T_c */ CARBONMONOXIDE_TSTAR
    , /* rho_c */ 10.85*CARBONMONOXIDE_M /* kg/m3 */
    , /* T_t */ 68.16

    , 0.0497 /* acentric factor */
    , &ideal_data_carbonmonoxide
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.90554,	0.25,	1.0,	0.0}
        , {-2.4515,	1.125,	1.0,	0.0}
        , {0.53149,	1.5,	1.0,	0.0}
        , {0.024173,	1.375,	2.0,	0.0}
        , {0.072156,	0.25,	3.0,	0.0}
        , {0.00018818,	0.875,	7.0,	0.0}
        , {0.19405,	0.625,	2.0,	1.0}
        , {-0.043268,	1.75,	5.0,	1.0}
        , {-0.12778,	3.625,	1.0,	2.0}
        , {-0.027896,	3.625,	4.0,	2.0}
        , {-0.034154,	14.5,	3.0,	3.0}
        , {0.016329,	12.0,	4.0,	3.0}
    }
    , 0 /* gaussian terms */
    , 0
    , 0 /* critical terms */
    , 0
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py carbonmonoxide
*/

#ifdef TEST

#include "../test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
    //return helm_check_u(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dpdT_rho(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dpdrho_T(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dhdT_rho(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dhdrho_T(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dudT_rho(&helmholtz_data_carbonmonoxide, ntd, td);
    //return helm_check_dudrho_T(&helmholtz_data_carbonmonoxide, ntd, td);
    return helm_run_test_cases(&helmholtz_data_carbonmonoxide, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-2.00E+2, 1.E-1, 8.29328555035E+2, -1.8290292793E+1, -1.81697133202E+1, -2.34979295116E-1, 1.2119454996E+0, 2.14153917477E+0, 1.03897567593E+0, -1.10155735531E+0}
    , {-1.50E+2, 1.E-1, 2.76874115186E+0, 2.23405562257E+2, 2.59523059168E+2, 3.07850383224E+0, 7.46796245568E-1, 1.0574377805E+0, 1.03902475277E+0, -1.55712184683E+2}
    , {-1.00E+2, 1.E-1, 1.95392948958E+0, 2.60857241171E+2, 3.12036160643E+2, 3.43649346164E+0, 7.43417572052E-1, 1.04593884434E+0, 1.03908684592E+0, -3.34171601711E+2}
    , {-5.0E+1, 1.E-1, 1.51228016621E+0, 2.98115816861E+2, 3.64241130302E+2, 3.70138331192E+0, 7.42880987463E-1, 1.04281410032E+0, 1.03921107158E+0, -5.27847869194E+2}
    , {0.E+0, 1.E-1, 1.23413765995E+0, 3.35324780203E+2, 4.16353018174E+2, 3.91210272705E+0, 7.43166674335E-1, 1.04193604776E+0, 1.03970477998E+0, -7.33266079692E+2}
    , {-2.00E+2, 1.E+0, 8.31072255319E+2, -1.86815349254E+1, -1.74782701147E+1, -2.4034674961E-1, 1.21384211601E+0, 2.13541693713E+0, 1.03897567593E+0, -1.10017019146E+0}
    , {-1.50E+2, 1.E+0, 3.15192242831E+1, 2.14821500604E+2, 2.46548169732E+2, 2.32317826027E+0, 8.00653211592E-1, 1.30866162554E+0, 1.03902475277E+0, -7.12779021481E+1}
    , {-1.00E+2, 1.E+0, 2.03266855152E+1, 2.56298155595E+2, 3.05494567831E+2, 2.7266210416E+0, 7.54415960617E-1, 1.11497470398E+0, 1.03908684592E+0, -2.15816277759E+2}
    , {-5.0E+1, 1.E+0, 1.53550056449E+1, 2.94981658346E+2, 3.60107000734E+2, 3.00391080856E+0, 7.47544177798E-1, 1.07661342833E+0, 1.03921107158E+0, -3.75341038583E+2}
    , {0.E+0, 1.E+0, 1.24102874687E+1, 3.3295016662E+2, 4.13528477357E+2, 3.21997101165E+0, 7.45894434653E-1, 1.06227419038E+0, 1.03970477998E+0, -5.46584915212E+2}
    , {-2.00E+2, 1.E+1, 8.46976747929E+2, -2.21914952161E+1, -1.03847956527E+1, -2.89982477768E-1, 1.23205777921E+0, 2.08723578937E+0, 1.03897567593E+0, -9.7927696739E-1}
    , {-1.50E+2, 1.E+1, 6.33015776744E+2, 7.93839124778E+1, 9.51813070569E+1, 8.06052933334E-1, 9.68366965064E-1, 2.27403584685E+0, 1.03902475277E+0, -1.98815062623E+1}
    , {-1.00E+2, 1.0E+1, 2.97599842806E+2, 1.94798945671E+2, 2.28401113957E+2, 1.70870238067E+0, 8.62292602302E-1, 2.48221917192E+0, 1.03908684592E+0, -1.01062871542E+2}
    , {-5.0E+1, 1.E+1, 1.70258877184E+2, 2.61830688998E+2, 3.20564777731E+2, 2.18144240967E+0, 7.94196100349E-1, 1.48721097248E+0, 1.03921107158E+0, -2.24958184719E+2}
    , {0.E+0, 1.0E+1, 1.26883698829E+2, 3.09631442692E+2, 3.88443773136E+2, 2.45664303017E+0, 7.73136316622E-1, 1.26831468771E+0, 1.03970477998E+0, -3.61400601E+2}
    , {-1.50E+2, 1.00E+2, 8.33990263416E+2, 3.76942335904E+1, 1.57599710173E+2, 3.47546148332E-1, 1.08979809065E+0, 1.72135078323E+0, 1.03902475277E+0, -5.10607457667E+0}
    , {-1.00E+2, 1.00E+2, 7.41259758873E+2, 1.0559194585E+2, 2.40497421027E+2, 9.13653621509E-1, 9.79801320404E-1, 1.60143520423E+0, 1.03908684592E+0, -5.2607178714E+1}
    , {-5.0E+1, 1.00E+2, 6.6220769714E+2, 1.67080780623E+2, 3.18090804263E+2, 1.30785975111E+0, 9.20056641118E-1, 1.50461036402E+0, 1.03921107158E+0, -1.24768122837E+2}
    , {0.E+0, 1.00E+2, 5.96334255498E+2, 2.23508819016E+2, 3.91200007436E+2, 1.60375988438E+0, 8.8347021931E-1, 1.42242457881E+0, 1.03970477998E+0, -2.14558193401E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
