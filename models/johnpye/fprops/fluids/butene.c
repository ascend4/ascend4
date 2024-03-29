/* This file is created by Hongke Zhu, 06-10-2010.
Chemical & Materials Engineering Department,
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Ihmels, E.C.,
"Thermodynamic Properties of the Butenes.  Part II. Short Fundamental Equations of State,"
Fluid Phase Equilibria, 228-229C:173-187, 2005.
*/

#include "../helmholtz.h"
#ifndef CUNIT_TEST

#define BUTENE_M 56.10632 /* kg/kmol */
#define BUTENE_R (8314.472/BUTENE_M) /* J/kg/K */
#define BUTENE_TC 419.29 /* K */

static const IdealData ideal_data_butene = {
	IDEAL_CP0, {.cp0={
		BUTENE_R /* cp0star */
		, 1. /* Tstar */
		, 1 /* power terms */
		, (const Cp0PowTerm[]){
			{3.9197,	0.0}
		}
		, 4 /* exponential terms */
		, (const Cp0ExpTerm[]){
			{2.9406,    274.0}
			,{6.5395,   951.0}
			,{14.535,   2127.0}
			,{5.8971,   5752.0}
		}
    }}
};

static const HelmholtzData helmholtz_data_butene = {
	/* R */ BUTENE_R /* J/kg/K */
    , /* M */ BUTENE_M /* kg/kmol */
    , /* rho_star */ 4.24*BUTENE_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ BUTENE_TC /* K (= T_c for this model) */

    , /* T_c */ BUTENE_TC
    , /* rho_c */ 4.24*BUTENE_M /* kg/m3 */
    , /* T_t */ 87.8

	,{FPROPS_REF_NBP}
    , 0.192 /* acentric factor */
    , &ideal_data_butene
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.78084,         0.12,    1.0,   0}
        , {-2.8258,         1.3,     1.0,   0}
        , {0.99403,         1.74,    1.0,   0}
        , {0.017951,        2.1,     2.0,   0}
        , {0.088889,        0.28,    3.0,   0}
        , {0.00024673,      0.69,    7.0,   0}
        , {0.22846,         0.75,    2.0,   1}
        , {-0.074009,       2.0,     5.0,   1}
        , {-0.22913,        4.4,     1.0,   2}
        , {-0.062334,       4.7,     4.0,   2}
        , {-0.025385,       15.0,    3.0,   3}
        , {0.011040,       14.0,     4.0,   3}
    }
    // no other terms
};

const EosData eos_butene = {
	"butene"
	,"E W Lemmon and E C Ihmels, 2005. 'Thermodynamic Properties of "
	"the Butenes.  Part II. Short Fundamental Equations of State', "
	"Fluid Phase Equilibria, 228-229C:173-187."
	,"http://dx.doi.org/10.1016/j.fluid.2004.09.004"
	,100
	,FPROPS_HELMHOLTZ
	,.data = {.helm = &helmholtz_data_butene}
};

#else
extern const EosData eos_butene;
# include "../test.h"
# include <math.h>
# include <assert.h>
# include <stdio.h>
/*
A small set of data points calculated using REFPROP 8.0, for validation.
*/

static const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.50E+2, 1.E-1, 7.79340672611E+2, -2.84455838449E+2, -2.84327524858E+2, -1.51709405807E+0, 1.29872067535E+0, 1.89384310917E+0, 8.99563543536E-1, -9.76257051977E+1}
    , {-1.00E+2, 1.E-1, 7.26376719092E+2, -1.89582007647E+2, -1.89444338037E+2, -8.70674175384E-1, 1.29686141965E+0, 1.91712345912E+0, 1.05893758449E+0, -3.88247741794E+1}
    , {-5.0E+1, 9.99999999998E-2, 6.73830411092E+2, -9.14322087226E+1, -9.1283803429E+1, -3.73188479204E-1, 1.37832788093E+0, 2.02072373984E+0, 1.23526147018E+0, -8.15519958835E+0}
    , {0.E+0, 1.E-1, 2.56811177208E+0, 3.6271425505E+2, 4.01653370196E+2, 1.50665983701E+0, 1.30819445035E+0, 1.4895325781E+0, 1.42073805662E+0, -4.88298794295E+1}
    , {5.0E+1, 1.E-1, 2.13236224783E+0, 4.32860627733E+2, 4.79756974778E+2, 1.76887329518E+0, 1.47972143807E+0, 1.64329139393E+0, 1.61407195813E+0, -1.38750777604E+2}
    , {1.00E+2, 1.E-1, 1.8317723125E+0, 5.11875651397E+2, 5.66467589116E+2, 2.01804546404E+0, 1.67067258542E+0, 1.82766013952E+0, 1.81255959862E+0, -2.41158013511E+2}
    , {1.50E+2, 1.E-1, 1.60831435247E+0, 6.00463142628E+2, 6.62640042216E+2, 2.25965919757E+0, 1.86545072004E+0, 2.01936143349E+0, 2.0104319498E+0, -3.55711646824E+2}
    , {2.00E+2, 1.E-1, 1.43460025869E+0, 6.98635929419E+2, 7.68341758199E+2, 2.49557102937E+0, 2.0552527716E+0, 2.20749351262E+0, 2.20165677535E+0, -4.82143503128E+2}
    , {2.50E+2, 1.E-1, 1.29529348111E+0, 8.06024362406E+2, 8.83226943488E+2, 2.72623847178E+0, 2.2347484161E+0, 2.38597983435E+0, 2.38186616816E+0, -6.20207294103E+2}
    , {-1.50E+2, 1.E+0, 7.79698123525E+2, -2.84652449202E+2, -2.83369901547E+2, -1.51869319767E+0, 1.29893753083E+0, 1.89350110779E+0, 8.99563543536E-1, -9.76253819093E+1}
    , {-1.00E+2, 1.E+0, 7.26895319275E+2, -1.89889434396E+2, -1.88513720488E+2, -8.72452785421E-1, 1.29727092672E+0, 1.91630133637E+0, 1.05893758449E+0, -3.88242345998E+1}
    , {-5.0E+1, 1.E+0, 6.74610357275E+2, -9.19013231353E+1, -9.04189859769E+1, -3.75294938191E-1, 1.37872620036E+0, 2.01875899051E+0, 1.23526147018E+0, -8.154257678E+0}
    , {0.E+0, 1.E+0, 6.1956541114E+2, 1.28351444722E+1, 1.44491790552E+1, 4.81713981098E-2, 1.50126570117E+0, 2.1876475903E+0, 1.42073805662E+0, -3.22872921479E-1}
    , {5.0E+1, 1.E+0, 5.56284823758E+2, 1.28058438088E+2, 1.29856078356E+2, 4.3554688342E-1, 1.65370752475E+0, 2.45071246795E+0, 1.61407195813E+0, -1.26885372895E+1}
    , {1.00E+2, 1.E+0, 2.10990939739E+1, 4.9518004186E+2, 5.42575441929E+2, 1.63074977961E+0, 1.73945450556E+0, 2.02985670864E+0, 1.81255959862E+0, -1.13334238403E+2}
    , {1.50E+2, 1.E+0, 1.7516987031E+1, 5.887928052E+2, 6.45880248276E+2, 1.89043083434E+0, 1.89728050824E+0, 2.1205512072E+0, 2.0104319498E+0, -2.11143002351E+2}
    , {2.00E+2, 1.E+0, 1.5177949167E+1, 6.89630461549E+2, 7.55515515518E+2, 2.13517019708E+0, 2.07233564757E+0, 2.26872750753E+0, 2.20165677535E+0, -3.206253172E+2}
    , {2.50E+2, 1.E+0, 1.34677364468E+1, 7.98660037135E+2, 8.72911564401E+2, 2.37089896686E+0, 2.24481266082E+0, 2.42721309296E+0, 2.38186616816E+0, -4.41675757378E+2}
    , {-1.50E+2, 1.E+1, 7.83183355514E+2, -2.8655608655E+2, -2.73787684452E+2, -1.5344040835E+0, 1.3014123592E+0, 1.89045596541E+0, 8.99563543536E-1, -9.75942236675E+1}
    , {-1.00E+2, 1.E+1, 7.31890458308E+2, -1.92833426005E+2, -1.79170179154E+2, -8.89750293449E-1, 1.30163524039E+0, 1.90907300309E+0, 1.05893758449E+0, -3.87731626947E+1}
    , {-5.0E+1, 1.E+1, 6.8196382862E+2, -9.63084282203E+1, -8.1644893909E+1, -3.95431631735E-1, 1.38313953684E+0, 2.00203654639E+0, 1.23526147018E+0, -8.06785959865E+0}
    , {0.E+0, 1.0E+1, 6.3099679718E+2, 6.08187358372E+0, 2.19298145633E+1, 2.28759063196E-2, 1.50453184195E+0, 2.14948591168E+0, 1.42073805662E+0, -1.66680227465E-1}
    , {5.0E+1, 1.0E+1, 5.76129460694E+2, 1.16833084713E+2, 1.3419029465E+2, 3.99813573475E-1, 1.65297585542E+0, 2.35059778141E+0, 1.61407195813E+0, -1.23666715553E+1}
    , {1.00E+2, 1.E+1, 5.12924320378E+2, 2.38653467197E+2, 2.58149520713E+2, 7.5601189229E-1, 1.82144588486E+0, 2.62270285743E+0, 1.81255959862E+0, -4.34523704114E+1}
    , {1.50E+2, 1.E+1, 4.32186297213E+2, 3.75570454566E+2, 3.98708624528E+2, 1.10897050301E+0, 2.00526973166E+0, 3.03454093198E+0, 2.0104319498E+0, -9.36904137842E+1}
    , {2.00E+2, 1.0E+1, 3.15864668698E+2, 5.34193075846E+2, 5.65852203919E+2, 1.48165933971E+0, 2.19905638797E+0, 3.64139693792E+0, 2.20165677535E+0, -1.66854040737E+2}
    , {2.50E+2, 1.0E+1, 2.08058439889E+2, 6.96219831955E+2, 7.44283251085E+2, 1.84043648878E+0, 2.34108290593E+0, 3.35080555993E+0, 2.38186616816E+0, -2.66604517149E+2}
};

static const unsigned ntd = sizeof(td)/sizeof(TestData);

void test_fluid_butene(){
	PureFluid *P = helmholtz_prepare(&eos_butene, NULL);
    helm_run_test_cases(P, ntd, td, 'C');
}

#endif
