/*	ASCEND modelling environment
 Copyright (C) 2011 Carnegie Mellon University

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
This file contains declarations of the data structures used to DECLARE fluid
property data. This may or may not be the same as the data structures passed to
functions that EVALUATE fluid properties. We allow from some preprocessing of
data loaded from input files, if deisred/needed.

Runtime data is in rundata.h
*/

#ifndef FPROPS_FILEDATA_H
#define FPROPS_FILEDATA_H

#include "common.h"
#include <math.h> // needed for NAN

/*---------------------------COMMON-------------------------------*/

#define R_UNIVERSAL 8314.4621

typedef struct CriticalData_struct{
    double T;
    double rho;
    double p; ///< Should be calculated, e.g. =pengrob_p(1, 1, &eos_coeffs_oxygen, &eos_data_oxygen, &err);
} CriticalData;

/*------------------------REFERENCE POINT-------------------------*/
/*
	There are a number of possible ways to specify the reference point that
	defines the absolute scale for enthalpy h and entropy s. The approach of
	IAPWS95 is to state u=0 and s=0 at for saturated liquid. Other correlations
	give h=0, s=0 at certain conditions. It seems that we might need to
	facilitate a few different ways of dealing with this stuff, so we'll
	provide a data structure here that allows a few different options.

	For chemical reactions, we need to use a reference state defined by the
	enthalpy of formation and absolute entropy at defined conditions. Many 
	sources seem to specify ~ambient pressure and ~25 degC, but RPP specifes
	298.2 K and 'ideal gas', which I take to mean zero pressure. I am not sure
	that case is correctly handled by FPROPS_REF_TPHG -- still working on that.
*/

typedef enum{
	FPROPS_REF_UNDEFINED = 0 /**< undefined reference state, should be first to catch cases where ReferenceState is not initialised correctly */
	,FPROPS_REF_PHI0 = 1 /**< phi0 reference point means that 'c' and 'm' in the phi0 expression are provided */
	,FPROPS_REF_IIR = 2  /**< International Institute of Refrigeration reference state: h=200 kJ/kg, s = 1 kJ/kg/K at saturated liquid, 0 deg C */
	,FPROPS_REF_NBP = 3  /**< Set h and s to zero for the saturated liquid at normal atmospheric pressure (101.325 kPa) */
	,FPROPS_REF_TRHS = 4 /**< Reference state specified by T0, p0, h0 and s0 */
	,FPROPS_REF_TPUS = 5 /**< Reference state specified by T0, p0, h0 and s0 */
	,FPROPS_REF_TPHS = 6 /**< Reference state specified by T0, p0, h0 and s0 */
	,FPROPS_REF_TPF = 7  /**< Reference state of h=0 and s=0 for liquid at the triple point */
	,FPROPS_REF_TPFU = 8 /**< Reference state of u=0 and s=0 for liquid at the triple point */
	,FPROPS_REF_TPHG = 9 /**< Reference state specified by T0, p0, h0 and g0 (Gibbs energy) */
/* HACK?: */
	,FPROPS_REF_REF0 /**< Special case: apply the 'ref0' reference state, which should allow calculuation of enthalpy of formation and absolute entropy */
} ReferenceStateType;

/**
	A reference point defined by values of c and m in the phi0 equation.
*/
typedef struct ReferenceStatePhi0_struct{
	double c;
	double m;
} ReferenceStatePhi0;

/**
	Reference point defined by T0, rho0, h0, s0.
s*/
typedef struct ReferenceStateTRHS_struct{
	double T0, rho0, h0, s0;
} ReferenceStateTRHS;

typedef struct ReferenceStateTPUS_struct{
	double T0, p0, u0, s0;
} ReferenceStateTPUS;

typedef struct ReferenceStateTPHS_struct{
	double T0, p0, h0, s0;
} ReferenceStateTPHS;

typedef struct ReferenceStateTPHG_struct{
	double T0, p0, h0, g0;
} ReferenceStateTPHG;

/* TODO add a reference state as defined by coefficients A_6, A_7 of the 
NASA SP-273 polynomials (the constant terms in the H0(T) and S0(T) polynomials),
this would open the way to a fairly easy support for the NASA fluid database, 
although note that it's only giving ideal gas EOS data */

typedef struct ReferenceState_struct{
	ReferenceStateType type;
	union{
		ReferenceStatePhi0 phi0;
		ReferenceStateTRHS trhs;
		ReferenceStateTPUS tpus;
		ReferenceStateTPHS tphs;
		ReferenceStateTPHG tphg;
	} data;
} ReferenceState;

/*----------------------------IDEAL--------------------------------*/

/** Power terms for cp0 (including polynomial) */
typedef struct Cp0PowTerm_struct{
	double c;
	double t;
} Cp0PowTerm;

/** Exponential terms for cp0 */
typedef struct Cp0ExpTerm_struct{
	double b;
	double beta;
} Cp0ExpTerm;

/* FIXME may need to add an alternative form for this data strcture for
source data that gives phi0 instead of cp0 as its ideal data portion. */

/* FIXME need to create a runtime data structure for this function that
factors out the normalisation of T and cp0, so that tau/delta functions can
be used seamlessly to eg for ideal_phi_tautau etc.
*/
/**
	Zero-pressure specific heat capacity data for a fluid
*/
typedef struct 	Cp0Data_struct{
	/* TODO: consider: do we need cp0 and Tstar here? */
	double cp0star; /* reducing parameter used for cp0, usually equals R */
    double Tstar; /* reducing parameter for T, usually equals Tc */

	unsigned np; /* number of power terms */
	const Cp0PowTerm *pt; /* power term data, may be NULL if np == 0 */
	unsigned ne; /* number of 'exponential' terms */
	const Cp0ExpTerm *et; /* exponential term data, maybe NULL if ne == 0 */
} Cp0Data;


/*-----------------------------------*/

/**
	Terms for normalised ideal helmholtz energy

	 a0 * ln(tau),   if p0 = 0
 	 a0 * tau^p0,    otherwise

	where tau = T* / T;
*/
typedef struct Phi0PowTerm_struct{
	double a0; /* coefficient */
	double p0; /* power of tau */
} Phi0PowTerm;

/**
	Terms for exponential or 'Einstein' terms in normalised Helmholtz energy
	n * ln(1 - exp(-gamma*tau)), where tau = T* / T.
*/
typedef struct Phi0ExpTerm_struct{
	/* yet to be implemented */
	double n;
	double gamma;
} Phi0ExpTerm;


/**
	Some publications provide phi0 instead of cp0. We allow that as an input,
	and	we compute cp0 (and h00, s00) as the common internal data structure.
	Conversions are detailed on our wiki.

	phi = ln(del) + c + m*tau + power_terms + Einstein_terms
*/
typedef struct Phi0Data_struct{
	double Tstar;
	unsigned np;
	const Phi0PowTerm *pt;
	unsigned ne;
	const Phi0ExpTerm *et;
} Phi0Data;

typedef enum IdealType_enum{
	IDEAL_CP0
	, IDEAL_PHI0
} IdealType;

/**
	Data structure for CP0 or PHI0 data as required for a wide range of
	correlations. Otherwise this data would have been stored directly in
	IdealFluid.
*/
typedef struct IdealData_struct{
	IdealType type;
	union{
		Cp0Data cp0;
		Phi0Data phi0;
	} data;
} IdealData;

/*--------------------CORRELATION SPECIFC--------------------------*/

/*------------------------ IDEAL GAS ------------------------------*/

typedef struct IdealFluid_struct{
	double M;
	//double T0, p0, h0, s0;
	const IdealData data;
	// reference state information
} IdealFluid;

/*-------------------------HELMHOLTZ-------------------------------*/

/**
 a * tau^t * delta^d
 */
typedef struct HelmholtzPowTerm_struct{
	double a; /* coefficient */
	double t; /* exponent of tau */
	int d; /* exponent of delta */
	unsigned l; /* exponent X in exp(-del^X) */
} HelmholtzPowTerm;

/*
 Data structure for Gaussian terms in the residual expression, for
 improvement of correlation in the critical region. These terms are of the
 form as utilised in the correlations for water (see water.c) and hydrogen
 (see hydrogen.c). According to Leachman, these terms are due to Setzmann
 and Wagner (J Phys Chem Ref Data, 1966).

 Using the nomenclature of IAPWS-95 (see water.c), terms here for the reduced
 helmholtz energy are:

 n * del^d * tau^t * exp[-alpha*(delta-epsilon)^2 - beta*(tau-gamma)^2]

 NOTE the minus signs preceeding 'alpha' and 'beta' and note that this is
 in conflict with the sign convention of Leachman, who assumes a plus sign
 in front of the corresponding parameters in his equation.

 NOTE these terms are also used in Span et al, 1998, as cited in the file
 'nitrogen.c', but in that case, epsilon == 1 for all terms.
 */
typedef struct HelmholtzGausTerm_struct{
	double n; /**< coefficient */
	double t; /**< power of tau */
	double d; /**< power of delta */
	double alpha,beta,gamma,epsilon;
} HelmholtzGausTerm;

/*
 Data structure for 'critical terms' in the residual expression. These
 terms are of the form described in the IAPWS-95 document, as cited in
 the file 'water.c'.

 This structure is for terms are of the form:

 n * DELTA^b * delta * exp(-C (delta-1)^2 - D (tau-1)^2)

 where DELTA = {(1-tau) + A*[(delta-1)^2]^(1/(2*beta)^}^2 + B[(delta-1)^2]^a
 */
typedef struct HelmholtzCritTerm_struct{
	double n; /**< coefficient */
	double a,b,beta,A,B,C,D;
} HelmholtzCritTerm;

/**
 Data structure for fluid-specific data for the Helmholtz free energy EOS.
 See Tillner-Roth 1993 for information about 'atd' and 'a0' data.

 p_c and T_t are removed from this struct because they can more accurately
 be calculated from other data already present.
 */
typedef struct HelmholtzData_struct{
	double R; /**< specific gas constant, set to zero to calculate using M */
	double M; /**< molar mass, kg/kmol */
	double rho_star; /**< normalisation density, kg/m³ */
	double T_star; /**< normalisation temperature, K */

	double T_c; /**< critical temperature */
	//REMOVED: double p_c; /**< critical pressure */
	double rho_c; /**< critical density */
	double T_t; /**< triple-point temperature */
	//REMOVED: double p_t; /**< triple-point pressure */

	ReferenceState ref;

	double omega; /**< Pitzer acentric factor */

	const IdealData *ideal; /* data for ideal component of Helmholtz energy */

	unsigned np; /* number of power terms in residual equation */
	const HelmholtzPowTerm *pt; /* power term data for residual eqn, maybe NULL if np == 0 */
	unsigned ng; /* number of critical terms of the first kind */
	const HelmholtzGausTerm *gt; /* critical terms of the first kind */
	unsigned nc; /* number of critical terms of the second kind */
	const HelmholtzCritTerm *ct; /* critical terms of the second kind */
} HelmholtzData;

/*___________________________CUBIC_________________________________*/

/**
	All cubic EOS use the same critical point properties and acentric
	factor in their equation form. So we can use this structure for
	PR, RK, etc EOSs. Hopefully.
*/
typedef struct CubicData_struct{
	double M;                ///< molar mass (kg/kmol)
	double T_c, p_c, rho_c;  // critical point properties
	double T_t;              ///< triple-point temperature (K)
	double omega;            ///< acentric Factor
	const ReferenceState ref; ///< default reference state, e.g. to reproduce original published data
	const ReferenceState ref0; ///< formation state, for calculation of enthalpy of formation and absolute entropy
	const IdealData *ideal;
} CubicData;

/*____________________________MBWR_________________________________*/

/**
	Modified Benedict-Webb-Rubin EOS.

	TODO XXX FIXME NOT FINISHED!
*/
typedef struct MbwrData_struct{
	double R; /**< ??? we should put this somewhere else */
	double rhob_c; /**< critical molar density in mol/L */
	double beta[32]; /**< constants in MBWR for the fluid in question */
} MbwrData;

/*--------------------------VISCOSITY------------------------------*/

typedef enum ViscosityType_enum{
	FPROPS_VISC_NONE = 0
	,FPROPS_VISC_1 = 1 /**< first viscosity model, as per Lemmon and Jacobsen 2004, "Viscosity and Thermal Conductivity Equations for Nitrogen, Oxygen, Argon, and Air" */
} ViscosityType;

typedef enum ViscCollisionIntegType_enum{
	FPROPS_CI_1 = 1 /**< first collision integral type, as per Lemmon and Jacobsen 2004, "Viscosity and Thermal Conductivity Equations for Nitrogen, Oxygen, Argon, and Air" */
} ViscCollisionIntegType;

typedef struct ViscCI1Term_struct{
	int i; /**< exponent of ln(T/(e/k))*/
	double b; /**< coefficient of ln(T/(e/k)) */
} ViscCI1Term;

typedef struct ViscCI1Data_struct{
	unsigned nt;
	const ViscCI1Term *t;
} ViscCI1Data;

typedef struct ViscCollisionIntegData_struct{
	ViscCollisionIntegType type;
	union {
		ViscCI1Data ci1;
	} data;
} ViscCollisionIntegData;

typedef struct ViscData1Term_struct{
	double N, t;
	int d, l;
} ViscData1Term;

typedef struct ViscosityData1_struct{
	double mu_star; //< normalisation parameter for viscosity (eg use 1e-6 for correlations returning value in µPa·s)
	double T_star; //< normalisation temperature for inverse normalised temperature $\tau = \frac{T^{{}*{}}}{T}$
	double rho_star; //< normalisation tempearture for normalised density $\delta = \frac{\rho}{\rho^{{}*{}}}$
	double sigma;
	double M;
	double eps_over_k;
	ViscCollisionIntegData ci;
	unsigned nt;
	const ViscData1Term *t;
} ViscosityData1;

typedef struct ViscosityData_struct{
	const char *source;
	ViscosityType type;
	union{
		ViscosityData1 v1;
	} data;
} ViscosityData;

/*------------------------DATA WRAPPER-----------------------------*/

/** EOS correlation types */
typedef enum EosType_enum{
	/* note, enum should not allow value of zero, as that return value is needed for errors in fprops_corr_avail */
	FPROPS_IDEAL = 7 /**< we need to be able to flag IDEAL at runtime! */
	,FPROPS_CUBIC = 1 /**< should only exist in source data, not in PureFluid object */
	,FPROPS_PENGROB = 2
	,FPROPS_REDKW = 3
	,FPROPS_SOAVE = 4
	,FPROPS_HELMHOLTZ = 5
	,FPROPS_MBWR = 6//etc.
} EosType;

/** Union of all possible EOS data structures */
typedef union EosUnion_union{
	const HelmholtzData *helm;
	const CubicData *cubic;
	MbwrData *mbwr;
	/* maybe more later */
} EosUnion;


/** Data and metadata for a particular property correlation for a partcular species */
typedef struct EosData_struct{
	const char *name;
	const char *source; /**< source of the data: publication data */
	const char *sourceurl; /**< URL/DOI to source data, if available */
	const double quality; /**< data quality, higher means more accurate */
	const EosType type;
	const EosUnion data;
	const ViscosityData *visc;
} EosData;

#endif
