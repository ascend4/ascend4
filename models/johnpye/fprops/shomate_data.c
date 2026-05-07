#include "shomate_data.h"

#include <string.h>

/* Piecewise Cp(T) correlations represented as generalized terms
   Cp,m = sum_k c_k T^(n_k). Term kinds are classified at data-prepare time. */

typedef struct {
	const char *name;
	const ShomateSpecies *species;
} ShomateEntry;

#define TERM(C, N) {(C), (N), SHOMATE_TERM_UNSET}

static const char *elements_ni[] = {"Ni"};
static const double stoich_ni[] = {1.0};

static const char *elements_nio[] = {"Ni", "O"};
static const double stoich_nio[] = {1.0, 1.0};

static const char *elements_h2[] = {"H"};
static const double stoich_h2[] = {2.0};

static const char *elements_o2[] = {"O"};
static const double stoich_o2[] = {2.0};

static const char *elements_h2o[] = {"H", "O"};
static const double stoich_h2o[] = {2.0, 1.0};

static const char *elements_co[] = {"C", "O"};
static const double stoich_co[] = {1.0, 1.0};

static const char *elements_co2[] = {"C", "O"};
static const double stoich_co2[] = {1.0, 2.0};

static const char *elements_sio2[] = {"Si", "O"};
static const double stoich_sio2[] = {1.0, 2.0};

static const char *elements_al2o3[] = {"Al", "O"};
static const double stoich_al2o3[] = {2.0, 3.0};

static const char *elements_gibbsite[] = {"Al", "O", "H"};
static const double stoich_gibbsite[] = {2.0, 6.0, 6.0};

static const char *elements_boehmite[] = {"Al", "O", "H"};
static const double stoich_boehmite[] = {2.0, 4.0, 2.0};

static const char *elements_fe2sio4[] = {"Fe", "Si", "O"};
static const double stoich_fe2sio4[] = {2.0, 1.0, 4.0};

static const char *elements_feal2o4[] = {"Fe", "Al", "O"};
static const double stoich_feal2o4[] = {1.0, 2.0, 4.0};

/* ------------------------------------------------------------------------- */
/* OECD/NEA Ni/NiO source (from shomate.py)                                 */
/* ------------------------------------------------------------------------- */

static ShomateTerm terms_ni_298_450[] = {
	TERM(3.36472e+01, 0.0),
	TERM(-2.47530e-02, 1.0),
	TERM(4.35458e-05, 2.0),
	TERM(-3.61955e+05, -2.0)
};

static ShomateTerm terms_ni_450_600[] = {
	TERM(-4.93235e+01, 0.0),
	TERM(1.676480e-01, 1.0),
	TERM(-7.48518e-05, 2.0),
	TERM(3.76214e+06, -2.0)
};

static ShomateTerm terms_ni_600_631[] = {
	TERM(1.05023e+05, 0.0),
	TERM(-2.29221e+02, 1.0),
	TERM(1.40772e-01, 2.0),
	TERM(-6.52809e+09, -2.0)
};

static ShomateTerm terms_ni_631_640[] = {
	TERM(5.29406e+04, 0.0),
	TERM(-1.65828e+02, 1.0),
	TERM(1.29936e-01, 2.0)
};

static ShomateTerm terms_ni_640_690[] = {
	TERM(1.82638e+02, 0.0),
	TERM(-4.20407e-01, 1.0),
	TERM(2.90521e-04, 2.0)
};

static ShomateTerm terms_ni_690_1728[] = {
	TERM(1.23677e+01, 0.0),
	TERM(2.22800e-02, 1.0),
	TERM(4.45300e-06, 2.0),
	TERM(2.46766e+06, -2.0)
};

static ShomateRange ranges_ni_oecd[] = {
	{298.0, 450.0, 4, terms_ni_298_450, 0},
	{450.0, 600.0, 4, terms_ni_450_600, 0},
	{600.0, 631.0, 4, terms_ni_600_631, 0},
	{631.0, 640.0, 3, terms_ni_631_640, 0},
	{640.0, 690.0, 3, terms_ni_640_690, 0},
	{690.0, 1728.0, 4, terms_ni_690_1728, 0}
};

static ShomateTerm terms_nio_298_519[] = {
	TERM(4110.64, 0.0),
	TERM(-5.302412, 1.0),
	TERM(3.52061e-3, 2.0),
	TERM(-53039.297, -0.5),
	TERM(2.43067e+07, -2.0)
};

static ShomateTerm terms_nio_519_1800[] = {
	TERM(-8.776, 0.0),
	TERM(4.2232e-2, 1.0),
	TERM(-7.5267e-6, 2.0),
	TERM(787.25, -0.5),
	TERM(3.6067e+06, -2.0)
};

static ShomateRange ranges_nio_oecd[] = {
	{298.15, 519.0, 5, terms_nio_298_519, 0},
	{519.0, 1800.0, 5, terms_nio_519_1800, 0}
};

static const ShomateSpecies species_ni_oecd = {
	"Ni",
	"oecd_nea_tdb_vol6_nickel",
	58.6934,
	FPROPS_PHASE_SOLID,
	298.15,
	101325.0,
	0.0,
	29.87,
	8908.0,
	1,
	elements_ni,
	stoich_ni,
	(unsigned)(sizeof(ranges_ni_oecd) / sizeof(ranges_ni_oecd[0])),
	ranges_ni_oecd
};

static const ShomateSpecies species_nio_oecd = {
	"NiO",
	"oecd_nea_tdb_vol6_nickel",
	74.6924,
	FPROPS_PHASE_SOLID,
	298.15,
	101325.0,
	-239700.0,
	38.4,
	6670.0,
	2,
	elements_nio,
	stoich_nio,
	(unsigned)(sizeof(ranges_nio_oecd) / sizeof(ranges_nio_oecd[0])),
	ranges_nio_oecd
};

/* ------------------------------------------------------------------------- */
/* Reaktoro/SUPCRT98 clone source                                            */
/* ------------------------------------------------------------------------- */

/* Clone fits target Reaktoro G0(T,1 bar) directly over 298.15..1473.15 K.
   Basis exponents: 0, 1, 2, 3, -0.5, -1, -2. */

static ShomateTerm terms_ni_clone_298_1800[] = {
	TERM(-3.619194026497e+01, 0.0),
	TERM( 1.241293180093e-01, 1.0),
	TERM(-1.216044158751e-04, 2.0),
	TERM( 4.441382623836e-08, 3.0),
	TERM( 6.711517327106e+02,-0.5),
	TERM( 9.852689883015e+01,-1.0),
	TERM( 8.206710463358e-01,-2.0)
};

static ShomateTerm terms_nio_clone_298_1800[] = {
	TERM( 1.360284121488e+03, 0.0),
	TERM(-1.795161933881e+00, 1.0),
	TERM( 1.379159651796e-03, 2.0),
	TERM(-3.951238497373e-07, 3.0),
	TERM(-1.554398257140e+04,-0.5),
	TERM(-2.285198010674e+03,-1.0),
	TERM(-1.904793345042e+01,-2.0)
};

static ShomateTerm terms_h2_clone_298_1800[] = {
	TERM(-2.215833589272e+02, 0.0),
	TERM( 3.391787296453e-01, 1.0),
	TERM(-2.574450536187e-04, 2.0),
	TERM( 7.455618368642e-08, 3.0),
	TERM( 3.013873636648e+03,-0.5),
	TERM( 4.428180598697e+02,-1.0),
	TERM( 3.689949935253e+00,-2.0)
};

static ShomateTerm terms_o2_clone_298_1800[] = {
	TERM(-3.656827987000e+02, 0.0),
	TERM( 5.452130392397e-01, 1.0),
	TERM(-4.172656002975e-04, 2.0),
	TERM( 1.212840625870e-07, 3.0),
	TERM( 4.727688796114e+03,-0.5),
	TERM( 6.947373770964e+02,-1.0),
	TERM( 5.789633845367e+00,-2.0)
};

static ShomateTerm terms_h2o_clone_298_1800[] = {
	TERM( 1.156956899016e+03, 0.0),
	TERM(-1.540126605509e+00, 1.0),
	TERM( 1.149410513394e-03, 2.0),
	TERM(-3.332796318440e-07, 3.0),
	TERM(-1.331076498921e+04,-0.5),
	TERM(-1.956831721504e+03,-1.0),
	TERM(-1.631067917681e+01,-2.0)
};

static ShomateTerm terms_co_clone_298_1800[] = {
	TERM( 2.736756418767e+01, 0.0),
	TERM( 6.412050284521e-03, 1.0),
	TERM(-1.916161363026e-06, 2.0),
	TERM( 5.354976707880e-10, 3.0),
	TERM( 2.215232949171e+00,-0.5),
	TERM( 1.527818013139e-01,-1.0),
	TERM( 5.754735288256e-04,-2.0)
};

static ShomateTerm terms_co2_clone_298_1800[] = {
	TERM( 2.782651083522e+01, 0.0),
	TERM( 4.825848931758e-02, 1.0),
	TERM(-3.327628074165e-05, 2.0),
	TERM( 9.366720464621e-09, 3.0),
	TERM( 2.255820378492e+00,-0.5),
	TERM( 1.558306985268e-01,-1.0),
	TERM( 5.889468060225e-04,-2.0)
};

static ShomateRange ranges_ni_clone[] = {
	{298.15, 1800.0, 7, terms_ni_clone_298_1800, 0}
};

static ShomateRange ranges_nio_clone[] = {
	{298.15, 1800.0, 7, terms_nio_clone_298_1800, 0}
};

static ShomateRange ranges_h2_clone[] = {
	{298.15, 1800.0, 7, terms_h2_clone_298_1800, 0}
};

static ShomateRange ranges_o2_clone[] = {
	{298.15, 1800.0, 7, terms_o2_clone_298_1800, 0}
};

static ShomateRange ranges_h2o_clone[] = {
	{298.15, 1800.0, 7, terms_h2o_clone_298_1800, 0}
};

static ShomateRange ranges_co_clone[] = {
	{298.15, 1800.0, 7, terms_co_clone_298_1800, 0}
};

static ShomateRange ranges_co2_clone[] = {
	{298.15, 1800.0, 7, terms_co2_clone_298_1800, 0}
};

static const ShomateSpecies species_ni_clone = {
	"Ni",
	"reaktoro_clone_supcrt98",
	58.6934,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	8610.20295247878,
	28.958110111384237,
	8909.13782635,
	1,
	elements_ni,
	stoich_ni,
	(unsigned)(sizeof(ranges_ni_clone) / sizeof(ranges_ni_clone[0])),
	ranges_ni_clone
};

static const ShomateSpecies species_nio_clone = {
	"NiO",
	"reaktoro_clone_supcrt98",
	74.6928,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-199963.32247670897,
	38.981778595801316,
	6808.82406563,
	2,
	elements_nio,
	stoich_nio,
	(unsigned)(sizeof(ranges_nio_clone) / sizeof(ranges_nio_clone[0])),
	ranges_nio_clone
};

static const ShomateSpecies species_o2_clone = {
	"oxygen",
	"reaktoro_clone_supcrt98",
	31.9988,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	60767.61453857804,
	203.89671925355952,
	0.0,
	1,
	elements_o2,
	stoich_o2,
	(unsigned)(sizeof(ranges_o2_clone) / sizeof(ranges_o2_clone[0])),
	ranges_o2_clone
};

static const ShomateSpecies species_h2_clone = {
	"hydrogen",
	"reaktoro_clone_supcrt98",
	2.01588,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	38726.897934772845,
	129.93944076364238,
	0.0,
	1,
	elements_h2,
	stoich_h2,
	(unsigned)(sizeof(ranges_h2_clone) / sizeof(ranges_h2_clone[0])),
	ranges_h2_clone
};

static const ShomateSpecies species_h2o_clone = {
	"water",
	"reaktoro_clone_supcrt98",
	18.01528,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	-171226.63495885613,
	190.64104147064737,
	0.0,
	2,
	elements_h2o,
	stoich_h2o,
	(unsigned)(sizeof(ranges_h2o_clone) / sizeof(ranges_h2o_clone[0])),
	ranges_h2o_clone
};

static const ShomateSpecies species_co_clone = {
	"carbonmonoxide",
	"reaktoro_clone_supcrt98",
	28.0101,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	-78236.4140672601,
	197.65837978447058,
	0.0,
	2,
	elements_co,
	stoich_co,
	(unsigned)(sizeof(ranges_co_clone) / sizeof(ranges_co_clone[0])),
	ranges_co_clone
};

static const ShomateSpecies species_co2_clone = {
	"carbondioxide",
	"reaktoro_clone_supcrt98",
	44.0098,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	-330711.06876907416,
	213.47533533766835,
	0.0,
	2,
	elements_co2,
	stoich_co2,
	(unsigned)(sizeof(ranges_co2_clone) / sizeof(ranges_co2_clone[0])),
	ranges_co2_clone
};

/* ------------------------------------------------------------------------- */
/* Pragmatic slag/ore source                                                 */
/* ------------------------------------------------------------------------- */

/*
 * Source summary for the first Fe-O-H-Si-Al extension:
 *
 * - SiO2 (quartz): NIST WebBook / JANAF Shomate data, transformed from
 *   the standard t = T / 1000 form into direct T exponents.
 * - Al2O3 (alpha-corundum): same approach using the alpha-phase NIST
 *   Shomate coefficients.
 * - Fe2SiO4 (fayalite): anchored to Robie, Finch, and Hemingway (1982)
 *   at 298.15 K, with piecewise-linear Cp(T) segments through the
 *   Benisek, Kroll, and Dachs (2012) table.
 * - FeAl2O4 (hercynite): anchored to Sack and Ghiorso (1991) standard
 *   state at 298.15 K, with piecewise-linear Cp(T) segments through the
 *   0 GPa Table 3 values in Verma et al. (2024) and a room-temperature
 *   cross-check from Klemme and van Miltenburg (2003).
 */

static ShomateTerm terms_sio2_quartz_298_847[] = {
	TERM(-6.076591, 0.0),
	TERM(0.2516755, 1.0),
	TERM(-3.247964e-4, 2.0),
	TERM(1.685604e-7, 3.0),
	TERM(2548.0, -2.0)
};

static ShomateTerm terms_sio2_quartz_847_1996[] = {
	TERM(58.75340, 0.0),
	TERM(0.01027925, 1.0),
	TERM(-1.31384e-7, 2.0),
	TERM(2.5210e-11, 3.0),
	TERM(25601.0, -2.0)
};

static ShomateRange ranges_sio2_slag[] = {
	{298.0, 847.0, 5, terms_sio2_quartz_298_847, 0},
	{847.0, 1996.0, 5, terms_sio2_quartz_847_1996, 0}
};

static ShomateTerm terms_al2o3_alpha_298_2327[] = {
	TERM(102.4290, 0.0),
	TERM(0.03874980, 1.0),
	TERM(-1.59109e-5, 2.0),
	TERM(2.628181e-9, 3.0),
	TERM(-3.007551e6, -2.0)
};

static ShomateRange ranges_al2o3_slag[] = {
	{298.0, 2327.0, 5, terms_al2o3_alpha_298_2327, 0}
};

/* ------------------------------------------------------------------------- */
/* USGS Bulletin 1452 gamma alumina source                                    */
/* ------------------------------------------------------------------------- */

/*
 * Robie, Hemingway, and Fisher, USGS Bulletin 1452, "Thermodynamic
 * Properties of Minerals and Related Substances" lists gamma-Al2O3 as
 * crystalline from 298.15 to 1800 K. The molar-volume field is blank in the
 * report, so no condensed pressure correction is applied for this source.
 */

static ShomateTerm terms_gamma_al2o3_usgs_298_1800[] = {
	TERM(1.5343e2, 0.0),
	TERM(1.9681e-3, 1.0),
	TERM(-9.0063e2, -0.5),
	TERM(-2.0307e6, -2.0)
};

static ShomateRange ranges_gamma_al2o3_usgs[] = {
	{298.15, 1800.0, 4, terms_gamma_al2o3_usgs_298_1800, 0}
};

static const ShomateSpecies species_gamma_al2o3_usgs = {
	"gamma-Al2O3",
	"usgs_bull_1452_1978",
	101.962,
	FPROPS_PHASE_SOLID,
	298.15,
	101325.0,
	-1653517.0,
	59.83,
	0.0,
	2,
	elements_al2o3,
	stoich_al2o3,
	(unsigned)(sizeof(ranges_gamma_al2o3_usgs) / sizeof(ranges_gamma_al2o3_usgs[0])),
	ranges_gamma_al2o3_usgs
};

/* ------------------------------------------------------------------------- */
/* Serena et al. (2009) Al2O3-H2O source                                     */
/* ------------------------------------------------------------------------- */

/*
 * Serena, Raso, Rodriguez, Caballero, and Leo (2009), Ceramics International
 * 35, 3081-3090, assessed the Al2O3-H2O binary up to 30 MPa. The hydrate
 * entries below use the optimized Cp(T), Delta_f H_298, and S_298 values in
 * their Table 6. Basis is the double formula used by the paper:
 *
 *   gibbsite: Al2O3.3H2O = Al2H6O6
 *   boehmite: Al2O3.H2O  = Al2H2O4
 *
 * Molar volumes in Serena Table 1 are reported on the Al(OH)3 / AlOOH basis,
 * so the densities below double those volumes to stay on the same double
 * formula basis as the thermodynamic functions.
 */

static ShomateTerm terms_gibbsite_serena_298_600[] = {
	TERM(-37.714, 0.0),
	TERM(0.91337, 1.0),
	TERM(-5.8953e-4, 2.0),
	TERM(6.7386e3, -2.0)
};

static ShomateRange ranges_gibbsite_serena[] = {
	{298.15, 600.0, 4, terms_gibbsite_serena_298_600, 0}
};

static ShomateTerm terms_boehmite_serena_298_900[] = {
	TERM(-24.038, 0.0),
	TERM(0.56165, 1.0),
	TERM(-4.2275e-4, 2.0),
	TERM(2.0153e3, -2.0)
};

static ShomateRange ranges_boehmite_serena[] = {
	{298.15, 900.0, 4, terms_boehmite_serena_298_900, 0}
};

static const ShomateSpecies species_gibbsite_serena = {
	"gibbsite",
	"serena_2009",
	156.00708,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-2594300.0,
	139.4,
	2420.661765565485,
	3,
	elements_gibbsite,
	stoich_gibbsite,
	(unsigned)(sizeof(ranges_gibbsite_serena) / sizeof(ranges_gibbsite_serena[0])),
	ranges_gibbsite_serena
};

static const ShomateSpecies species_boehmite_serena = {
	"boehmite",
	"serena_2009",
	119.97768,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-1981100.0,
	96.86,
	3070.046281237297,
	3,
	elements_boehmite,
	stoich_boehmite,
	(unsigned)(sizeof(ranges_boehmite_serena) / sizeof(ranges_boehmite_serena[0])),
	ranges_boehmite_serena
};

static const ShomateSpecies species_al2o3_serena = {
	"Al2O3",
	"serena_2009",
	101.9613,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-1675690.0,
	50.92,
	3987.0,
	2,
	elements_al2o3,
	stoich_al2o3,
	(unsigned)(sizeof(ranges_al2o3_slag) / sizeof(ranges_al2o3_slag[0])),
	ranges_al2o3_slag
};

static ShomateTerm terms_fe2sio4_298_400[] = {
	TERM(71.65525773195883, 0.0),
	TERM(0.2020618556701029, 1.0)
};

static ShomateTerm terms_fe2sio4_400_500[] = {
	TERM(102.11999999999998, 0.0),
	TERM(0.12590000000000004, 1.0)
};

static ShomateTerm terms_fe2sio4_500_600[] = {
	TERM(124.96999999999994, 0.0),
	TERM(0.0802000000000001, 1.0)
};

static ShomateTerm terms_fe2sio4_600_700[] = {
	TERM(138.89000000000007, 0.0),
	TERM(0.056999999999999884, 1.0)
};

static ShomateTerm terms_fe2sio4_700_800[] = {
	TERM(147.29, 0.0),
	TERM(0.045, 1.0)
};

static ShomateTerm terms_fe2sio4_800_900[] = {
	TERM(152.49000000000004, 0.0),
	TERM(0.038499999999999944, 1.0)
};

static ShomateTerm terms_fe2sio4_900_1000[] = {
	TERM(155.64, 0.0),
	TERM(0.035, 1.0)
};

static ShomateTerm terms_fe2sio4_1000_1100[] = {
	TERM(157.93999999999988, 0.0),
	TERM(0.032700000000000104, 1.0)
};

static ShomateTerm terms_fe2sio4_1100_1200[] = {
	TERM(159.48000000000005, 0.0),
	TERM(0.03129999999999995, 1.0)
};

static ShomateTerm terms_fe2sio4_1200_1300[] = {
	TERM(160.67999999999998, 0.0),
	TERM(0.03030000000000001, 1.0)
};

static ShomateTerm terms_fe2sio4_1300_1400[] = {
	TERM(161.71999999999977, 0.0),
	TERM(0.029500000000000172, 1.0)
};

static ShomateTerm terms_fe2sio4_1400_1500[] = {
	TERM(162.70000000000007, 0.0),
	TERM(0.028799999999999954, 1.0)
};

static ShomateTerm terms_fe2sio4_1500_1600[] = {
	TERM(163.6000000000001, 0.0),
	TERM(0.028199999999999933, 1.0)
};

static ShomateTerm terms_fe2sio4_1600_1700[] = {
	TERM(164.56000000000014, 0.0),
	TERM(0.02759999999999991, 1.0)
};

static ShomateTerm terms_fe2sio4_1700_1800[] = {
	TERM(165.5799999999997, 0.0),
	TERM(0.02700000000000017, 1.0)
};

static ShomateTerm terms_fe2sio4_1800_1900[] = {
	TERM(166.4799999999999, 0.0),
	TERM(0.026500000000000058, 1.0)
};

static ShomateTerm terms_fe2sio4_1900_2000[] = {
	TERM(167.43000000000012, 0.0),
	TERM(0.025999999999999943, 1.0)
};

static ShomateRange ranges_fe2sio4_slag[] = {
	{298.15, 400.0, 2, terms_fe2sio4_298_400, 0},
	{400.0, 500.0, 2, terms_fe2sio4_400_500, 0},
	{500.0, 600.0, 2, terms_fe2sio4_500_600, 0},
	{600.0, 700.0, 2, terms_fe2sio4_600_700, 0},
	{700.0, 800.0, 2, terms_fe2sio4_700_800, 0},
	{800.0, 900.0, 2, terms_fe2sio4_800_900, 0},
	{900.0, 1000.0, 2, terms_fe2sio4_900_1000, 0},
	{1000.0, 1100.0, 2, terms_fe2sio4_1000_1100, 0},
	{1100.0, 1200.0, 2, terms_fe2sio4_1100_1200, 0},
	{1200.0, 1300.0, 2, terms_fe2sio4_1200_1300, 0},
	{1300.0, 1400.0, 2, terms_fe2sio4_1300_1400, 0},
	{1400.0, 1500.0, 2, terms_fe2sio4_1400_1500, 0},
	{1500.0, 1600.0, 2, terms_fe2sio4_1500_1600, 0},
	{1600.0, 1700.0, 2, terms_fe2sio4_1600_1700, 0},
	{1700.0, 1800.0, 2, terms_fe2sio4_1700_1800, 0},
	{1800.0, 1900.0, 2, terms_fe2sio4_1800_1900, 0},
	{1900.0, 2000.0, 2, terms_fe2sio4_1900_2000, 0}
};

static ShomateTerm terms_feal2o4_200_298[] = {
	TERM(-146.82081253183904, 0.0),
	TERM(0.9096790626591953, 1.0)
};

static ShomateTerm terms_feal2o4_298_400[] = {
	TERM(4.984850760922939, 0.0),
	TERM(0.4005203730976927, 1.0)
};

static ShomateTerm terms_feal2o4_400_600[] = {
	TERM(-29.816999999999922, 0.0),
	TERM(0.4875249999999998, 1.0)
};

static ShomateTerm terms_feal2o4_600_800[] = {
	TERM(119.98199999999997, 0.0),
	TERM(0.23786000000000002, 1.0)
};

static ShomateTerm terms_feal2o4_800_1000[] = {
	TERM(146.798, 0.0),
	TERM(0.20433999999999997, 1.0)
};

static ShomateTerm terms_feal2o4_1000_1600[] = {
	TERM(219.79133333333323, 0.0),
	TERM(0.13134666666666675, 1.0)
};

static ShomateTerm terms_feal2o4_1600_1800[] = {
	TERM(198.77800000000033, 0.0),
	TERM(0.1444799999999998, 1.0)
};

static ShomateRange ranges_feal2o4_slag[] = {
	{200.0, 298.15, 2, terms_feal2o4_200_298, 0},
	{298.15, 400.0, 2, terms_feal2o4_298_400, 0},
	{400.0, 600.0, 2, terms_feal2o4_400_600, 0},
	{600.0, 800.0, 2, terms_feal2o4_600_800, 0},
	{800.0, 1000.0, 2, terms_feal2o4_800_1000, 0},
	{1000.0, 1600.0, 2, terms_feal2o4_1000_1600, 0},
	{1600.0, 1800.0, 2, terms_feal2o4_1600_1800, 0}
};

static const ShomateSpecies species_sio2_slag = {
	"SiO2",
	"slag_pragmatic_2026",
	60.0843,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-910856.8,
	41.44,
	2650.0,
	2,
	elements_sio2,
	stoich_sio2,
	(unsigned)(sizeof(ranges_sio2_slag) / sizeof(ranges_sio2_slag[0])),
	ranges_sio2_slag
};

static const ShomateSpecies species_al2o3_slag = {
	"Al2O3",
	"slag_pragmatic_2026",
	101.9613,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-1675690.0,
	50.92,
	3987.0,
	2,
	elements_al2o3,
	stoich_al2o3,
	(unsigned)(sizeof(ranges_al2o3_slag) / sizeof(ranges_al2o3_slag[0])),
	ranges_al2o3_slag
};

static const ShomateSpecies species_fe2sio4_slag = {
	"Fe2SiO4",
	"slag_pragmatic_2026",
	203.778,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-1478170.0,
	151.00,
	4390.0,
	3,
	elements_fe2sio4,
	stoich_fe2sio4,
	(unsigned)(sizeof(ranges_fe2sio4_slag) / sizeof(ranges_fe2sio4_slag[0])),
	ranges_fe2sio4_slag
};

static const ShomateSpecies species_feal2o4_slag = {
	"FeAl2O4",
	"slag_pragmatic_2026",
	173.8077,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-1947681.0,
	115.362,
	3950.0,
	3,
	elements_feal2o4,
	stoich_feal2o4,
	(unsigned)(sizeof(ranges_feal2o4_slag) / sizeof(ranges_feal2o4_slag[0])),
	ranges_feal2o4_slag
};

static const ShomateEntry entries[] = {
	/* OECD source */
	{"Ni", &species_ni_oecd},
	{"Ni(cr)", &species_ni_oecd},
	{"NiO", &species_nio_oecd},
	{"NiO(cr)", &species_nio_oecd},

	/* Reaktoro clone source */
	{"Ni", &species_ni_clone},
	{"Ni(cr)", &species_ni_clone},
	{"NiO", &species_nio_clone},
	{"NiO(cr)", &species_nio_clone},
	{"oxygen", &species_o2_clone},
	{"hydrogen", &species_h2_clone},
	{"water", &species_h2o_clone},
	{"carbonmonoxide", &species_co_clone},
	{"CO", &species_co_clone},
	{"carbondioxide", &species_co2_clone},
	{"CO2", &species_co2_clone},

	/* Pragmatic slag/ore source */
	{"SiO2", &species_sio2_slag},
	{"quartz", &species_sio2_slag},
	{"Al2O3", &species_al2o3_slag},
	{"corundum", &species_al2o3_slag},
	{"Fe2SiO4", &species_fe2sio4_slag},
	{"fayalite", &species_fe2sio4_slag},
	{"FeAl2O4", &species_feal2o4_slag},
	{"hercynite", &species_feal2o4_slag},

	/* USGS Bulletin 1452 gamma alumina */
	{"gamma-Al2O3", &species_gamma_al2o3_usgs},
	{"gamma_Al2O3", &species_gamma_al2o3_usgs},
	{"gamma_alumina", &species_gamma_al2o3_usgs},
	{"Al2O3_gamma", &species_gamma_al2o3_usgs},

	/* Serena Al2O3-H2O source */
	{"gibbsite", &species_gibbsite_serena},
	{"Al2O3.3H2O", &species_gibbsite_serena},
	{"Al2H6O6", &species_gibbsite_serena},
	{"boehmite", &species_boehmite_serena},
	{"Al2O3.H2O", &species_boehmite_serena},
	{"Al2H2O4", &species_boehmite_serena},
	{"Al2O3", &species_al2o3_serena},
	{"corundum", &species_al2o3_serena},
	{"alpha_Al2O3", &species_al2o3_serena},
	{"alpha-Al2O3", &species_al2o3_serena}
};

static int entry_count(void){
	return (int)(sizeof(entries) / sizeof(entries[0]));
}

static void shomate_prepare_species_ranges(const ShomateSpecies *S){
	unsigned i;
	for(i = 0; i < S->nranges; ++i){
		shomate_prepare_range((ShomateRange *)&S->ranges[i]);
	}
}

const ShomateSpecies *shomate_data_lookup(const char *name, const char *source){
	int i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < entry_count(); ++i){
		const ShomateSpecies *S = entries[i].species;
		if(0 != strcmp(entries[i].name, name)){
			continue;
		}
		if(source){
			if(!(S->source && NULL != strstr(S->source, source))){
				continue;
			}
		}
		shomate_prepare_species_ranges(S);
		return S;
	}
	return NULL;
}
