/* This file is created by Hongke Zhu, 05-30-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"
#ifndef CUNIT_TEST

#define KRYPTON_M 83.798 /* kg/kmol */
#define KRYPTON_R (8314.472/KRYPTON_M) /* J/kg/K */
#define KRYPTON_TC 209.48 /* K */

static const IdealData ideal_data_krypton = {
	IDEAL_CP0,{.cp0={
		KRYPTON_R /* cp0star */
		, 1. /* Tstar */
		, 1 /* power terms */
		, (const Cp0PowTerm[]){
			{2.5,	0.0}
		}
	}}
};

static const HelmholtzData helmholtz_data_krypton = {
    /* R */ KRYPTON_R /* J/kg/K */
    , /* M */ KRYPTON_M /* kg/kmol */
    , /* rho_star */ 10.85*KRYPTON_M /* kg/m3(= rho_c for this model) */
    , /* T_star */ KRYPTON_TC /* K (= T_c for this model) */

    , /* T_c */ KRYPTON_TC
    , /* rho_c */ 10.85*KRYPTON_M /* kg/m3 */
    , /* T_t */ 115.77

	,{FPROPS_REF_PHI0,{.phi0={
		.c = -3.7506412806 /* constant */
		, .m = 3.7798018435 /* linear */
	}}}
    , -0.00089 /* acentric factor */
    , &ideal_data_krypton
    , 12 /* power terms */
    , (const HelmholtzPowTerm[]){
        /* a_i, 	t_i, 	d_i, 	l_i */
        {0.83561,	0.25,	1.0,	0.0}
        , {-2.3725,	1.125,	1.0,	0.0}
        , {0.54567,	1.5,	1.0,	0.0}
        , {0.014361,	1.375,	2.0,	0.0}
        , {0.066502,	0.25,	3.0,	0.0}
        , {0.00019310,	0.875,	7.0,	0.0}
        , {0.16818,	0.625,	2.0,	1.0}
        , {-0.033133,	1.75,	5.0,	1.0}
        , {-0.15008,	3.625,	1.0,	2.0}
        , {-0.022897,	3.625,	4.0,	2.0}
        , {-0.021454,	14.5,	3.0,	3.0}
        , {0.0069397,	12.0,	4.0,	3.0}
    }
};

const EosData eos_krypton = {
	"krypton"
	,"Lemmon, E.W. and Span, R., Short Fundamental Equations of State for "
	" 20 Industrial Fluids, J. Chem. Eng. Data, 51:785-850, 2006."
	,NULL
	,100
	,FPROPS_HELMHOLTZ
	,.data = {.helm = &helmholtz_data_krypton}
};

#else
# include "../test.h"
extern const EosData eos_krypton;

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

static const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.50E+2, 1.E-1, 8.43306771336E+0, 9.6144022849E+1, 1.08002103846E+2, 9.0316475661E-1, 1.58469027933E-1, 2.70934234337E-1, 2.48051027471E-1, -1.50807169274E+1}
    , {-1.00E+2, 1.E-1, 5.88290081866E+0, 1.03989329175E+2, 1.20987745957E+2, 9.91816292723E-1, 1.50682439478E-1, 2.53953158352E-1, 2.48051027471E-1, -6.77436619098E+1}
    , {-5.0E+1, 1.E-1, 4.53935490929E+0, 1.11556045659E+2, 1.33585607567E+2, 1.05575171758E+0, 1.49439192734E-1, 2.50629794242E-1, 2.48051027471E-1, -1.24034950119E+2}
    , {0.E+0, 1.E-1, 3.6998108134E+0, 1.19056024581E+2, 1.46084433612E+2, 1.10629502976E+0, 1.49099144946E-1, 2.49490798552E-1, 2.48051027471E-1, -1.83128462797E+2}
    , {5.0E+1, 1.E-1, 3.12370856299E+0, 1.26531379549E+2, 1.58544609332E+2, 1.1481862298E+0, 1.48976732218E-1, 2.4897475739E-1, 2.48051027471E-1, -2.4450500061E+2}
    , {1.00E+2, 1.E-1, 2.70338810928E+0, 1.3399514401E+2, 1.70985763173E+2, 1.18398335831E+0, 1.48923231216E-1, 2.48696775629E-1, 2.48051027471E-1, -3.07808246142E+2}
    , {-1.50E+2, 1.E+0, 2.39441685293E+3, 1.58791890111E+0, 2.00555712429E+0, 1.34391248225E-2, 2.46746804134E-1, 5.21058738095E-1, 2.48051027471E-1, -6.71093207828E-2}
    , {-1.00E+2, 1.E+0, 6.58872843334E+1, 1.0048038898E+2, 1.15657824355E+2, 7.42457925586E-1, 1.71945197839E-1, 3.30921057378E-1, 2.48051027471E-1, -2.8076200835E+1}
    , {-5.0E+1, 1.E+0, 4.76405200089E+1, 1.09497930638E+2, 1.30488465582E+2, 8.17975486907E-1, 1.55232457968E-1, 2.77275571954E-1, 2.48051027471E-1, -7.30332992654E+1}
    , {0.E+0, 1.E+0, 3.79365869193E+1, 1.17588631317E+2, 1.43948409073E+2, 8.72447734641E-1, 1.51585029768E-1, 2.63411610751E-1, 2.48051027471E-1, -1.207204674E+2}
    , {5.0E+1, 1.E+0, 3.1678667086E+1, 1.2538968101E+2, 1.56956665735E+2, 9.16193834783E-1, 1.50313980321E-1, 2.57625506004E-1, 2.48051027471E-1, -1.706783567E+2}
    , {1.00E+2, 1.E+0, 2.72524736189E+1, 1.33062137412E+2, 1.69756054228E+2, 9.5302635206E-1, 1.49765685995E-1, 2.54638111577E-1, 2.48051027471E-1, -2.22559645859E+2}
    , {-1.50E+2, 1.E+1, 2.43359029694E+3, 2.18026599578E-1, 4.32718170781E+0, 2.02219770042E-3, 2.49720927037E-1, 5.06978544925E-1, 2.48051027471E-1, -3.10070472288E-2}
    , {-1.00E+2, 1.E+1, 2.03427262966E+3, 2.60345582829E+1, 3.09503202383E+1, 1.82818245008E-1, 2.17191911964E-1, 5.77174216552E-1, 2.48051027471E-1, -5.62042084033E+0}
    , {-5.0E+1, 1.0E+1, 1.32735496488E+3, 6.08466958952E+1, 6.83804756787E+1, 3.70078329253E-1, 2.15721583436E-1, 1.1685843885E+0, 2.48051027471E-1, -2.17362832777E+1}
    , {0.E+0, 1.E+1, 5.14191286143E+2, 9.84912080853E+1, 1.17939223385E+2, 5.73947476637E-1, 1.80133198843E-1, 5.56767152493E-1, 2.48051027471E-1, -5.8282545158E+1}
    , {5.0E+1, 1.0E+1, 3.62999897081E+2, 1.12652975146E+2, 1.40201192323E+2, 6.49191860608E-1, 1.64545821041E-1, 3.77918326643E-1, 2.48051027471E-1, -9.71333746095E+1}
    , {1.00E+2, 1.0E+1, 2.91883479032E+2, 1.23299597137E+2, 1.57559843839E+2, 6.99228535429E-1, 1.58515186539E-1, 3.24344549736E-1, 2.48051027471E-1, -1.37617530859E+2}
    , {-1.00E+2, 1.00E+2, 2.45324120719E+3, 1.18226963545E+1, 5.25850966056E+1, 7.99922957388E-2, 2.41044601055E-1, 4.4345554343E-1, 2.48051027471E-1, -2.02796965273E+0}
    , {-5.0E+1, 1.00E+2, 2.24172462905E+3, 2.98444656714E+1, 7.44529776644E+1, 1.91006616607E-1, 2.2070686832E-1, 4.31609705076E-1, 2.48051027471E-1, -1.27786608245E+1}
    , {0.E+0, 1.00E+2, 2.04335037415E+3, 4.67941423032E+1, 9.57333752731E+1, 2.77096779376E-1, 2.0680207525E-1, 4.19207724927E-1, 2.48051027471E-1, -2.88948429833E+1}
    , {5.0E+1, 1.00E+2, 1.86186509287E+3, 6.26196731052E+1, 1.16329257319E+2, 3.46373543214E-1, 1.96776730509E-1, 4.04237720135E-1, 2.48051027471E-1, -4.93109373844E+1}
    , {1.00E+2, 1.00E+2, 1.70005787414E+3, 7.73104857916E+1, 1.36132012706E+2, 4.03379996159E-1, 1.89306232224E-1, 3.87775344597E-1, 2.48051027471E-1, -7.32107597752E+1}
};

static const unsigned ntd = sizeof(td)/sizeof(TestData);

void test_fluid_krypton(void){
	PureFluid *P = helmholtz_prepare(&eos_krypton,NULL);
	helm_run_test_cases(P, ntd, td, 'C');
}

#endif
