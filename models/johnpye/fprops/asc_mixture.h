/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, August 14-, 2015

	Includes other file headers, defines function headers and some constant 
	char* documentation strings to reduce the clutter in 'asc_mixture.c'
 */

#ifndef MIX_LIBRARY_HEADER
#define MIX_LIBRARY_HEADER

#include <ascend/utilities/error.h>
#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include <ascend/compiler/extfunc.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/instance_types.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/sets.h>
#include <ascend/compiler/arrayinst.h>

#include <stdio.h>

/* Code that is wrapped up in this file */
#include "mixtures/mixture_properties.h"
#include "mixtures/mixture_generics.h"
#include "mixtures/mixture_phases.h"
#include "mixtures/mixture_struct.h"

#ifndef ASC_EXPORT
#error "ASC_EXPORT not found -- where is it?"
#endif

#define NULL_STR(STR) (STR==NULL) ? "NULL" : STR
#define MIN_P 0.001        /* minimum pressure */
#define MIN_T 1.e-6        /* minimum temperature */
#define MIN_S 0.0          /* minimum entropy */
#define MIN_HEAT_CAPAC 0.0 /* minimum heat capacity */

/* ---------------------------------------------------------------------
	Forward Declarations
 */
ExtBBoxInitFunc asc_mixture_prepare;

/*
	Macros to declare external functions (forward declarations)
 */
#define MIX_EXTDECL(NAME) ExtBBoxFunc mixture_##NAME##_calc;
#define MIX_PROP_EXTDECL(PROP) MIX_EXTDECL(PROP) MIX_EXTDECL(phase_##PROP) \
    MIX_EXTDECL(comps_##PROP)

MIX_PROP_EXTDECL(rho); MIX_PROP_EXTDECL(u); MIX_PROP_EXTDECL(h); MIX_PROP_EXTDECL(cp);
MIX_PROP_EXTDECL(cv); MIX_PROP_EXTDECL(s); MIX_PROP_EXTDECL(g); MIX_PROP_EXTDECL(a);

MIX_EXTDECL(count_phases); MIX_EXTDECL(count_components); MIX_EXTDECL(component_frac);
MIX_EXTDECL(component_index); MIX_EXTDECL(dew_p); MIX_EXTDECL(bubble_p);
MIX_EXTDECL(dew_T); MIX_EXTDECL(bubble_T); MIX_EXTDECL(state_T_ph);

/* ---------------------------------------------------------------------
	Global Variables
 */
static symchar *mix_symbols[6];
enum Symbol_Enum {NPURE_SYM, COMP_SYM, X_SYM, TYPE_SYM, SOURCE_SYM};

/* 
	Macros to define help texts for external functions
 */
#define MIX_HELP_TEXT(PROP) "Calculate mixture " PROP ", using ideal-solution assumption."
#define MIX_PHASE_HELP_TEXT(PROP) "Calculate mixture " PROP " for a single phase, using ideal-solution assumption."
#define MIX_COMPS_HELP_TEXT(PROP) "Calculate " PROP " for a single component in one phase of the mixture, using ideal-solution assumption."

#define MIX_HELP_DECL(PROP,MESSAGE) static const char *mixture_##PROP##_help = MESSAGE;
#define MIX_PH_HELP_DECL(PROP,MESSAGE) static const char *mixture_phase_##PROP##_help = MESSAGE;
#define MIX_CMP_HELP_DECL(PROP,MESSAGE) static const char *mixture_comps_##PROP##_help = MESSAGE;

#define MIX_HELP_DOUBLE(PROP,NAME) MIX_HELP_DECL(PROP, MIX_HELP_TEXT(NAME)) \
            MIX_PH_HELP_DECL(PROP, MIX_PHASE_HELP_TEXT(NAME)) \
            MIX_CMP_HELP_DECL(PROP, MIX_COMPS_HELP_TEXT(NAME))

/*
	Define help texts
 */
MIX_HELP_DOUBLE(rho, "density");
MIX_HELP_DOUBLE(u, "internal energy");
MIX_HELP_DOUBLE(h, "enthalpy");
MIX_HELP_DOUBLE(cp, "constant-pressure heat capacity");
MIX_HELP_DOUBLE(cv, "contant-volume heat capacity");
MIX_HELP_DOUBLE(s, "entropy");
MIX_HELP_DOUBLE(g, "Gibbs energy");
MIX_HELP_DOUBLE(a, "Helmholtz energy");

MIX_HELP_DECL(count_phases, "Calculate and return number of phases in the mixture, and "
		"mass fraction of the mixture in each phase.");
MIX_HELP_DECL(count_components, "Return number of components within a phase of the mixture.");
MIX_HELP_DECL(component_frac, "Return mass fractions of a component in all phases throughout the mixture.");
MIX_HELP_DECL(component_index, "Return indexes that give the location of a component in all phases of the mixture");
MIX_HELP_DECL(dew_p, MIX_HELP_TEXT("dew pressure"));
MIX_HELP_DECL(bubble_p, MIX_HELP_TEXT("bubble pressure"));
MIX_HELP_DECL(dew_T, MIX_HELP_TEXT("dew temperature"));
MIX_HELP_DECL(bubble_T, MIX_HELP_TEXT("bubble temperature"));
MIX_HELP_DECL(state_T_ph, "Calculate mixture temperature from the pressure and enthalpy, "
		"using ideal-solution assumption");

/* ---------------------------------------------------------------------
	Macros used in creating mixture-property and other ASCEND functions
 */
#define CALCPREP(NIN,NOUT) \
	if(ninputs!=NIN){ \
		return -1; \
	} \
	if(noutputs!=NOUT){ \
		return -2; \
	} \
	if(inputs==NULL){ \
		return -3; \
	} \
	if(outputs==NULL){ \
		return -4; \
	} \
	if(bbox==NULL){ \
		return -5; \
	} \
	FpropsError err=FPROPS_NO_ERROR; \
	MixtureSpec *MS = (MixtureSpec *)bbox->user_data; \
	unsigned iii, iij; \
	for(iii=0;iii<MS->pures;iii++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "The component number %u has mass fraction %g" \
				, iii, MS->Xs[iii]); \
	}

/*
    Perform switch on the output of a function that seeks some property, 
    outputting different error messages depending on the value of the output.
 */
#define ROOTSOLVE_SWITCH(RESULT,PROP) \
	switch(RESULT){ \
		case 1: \
			ERROR_REPORTER_HERE(ASC_USER_ERROR \
					, "The " PROP " converged on a non-solution point."); \
			break; \
		case 2: \
			ERROR_REPORTER_HERE(ASC_USER_ERROR \
					, "The " PROP " fractions converged on Infinity or NaN."); \
			break; \
		case 3: \
			ERROR_REPORTER_HERE(ASC_USER_ERROR \
					, "The search for the " PROP " failed to converge " \
					"\nin the maximum number of iterations."); \
			break; \
	}

/*
    Perform switch on the output of seeking some property, like ROOTSOLVE_SWITCH 
    with the added possibility that no flash occurred.
 */
#define ROOT_VLE_SWITCH(RESULT,PROP1,PROP2,PROP2_UNIT,PROP2_VAL) \
    switch(RESULT){ \
        case 1: \
            ERROR_REPORTER_HERE(ASC_USER_ERROR \
                    , "The " PROP " converged on a non-solution point."); \
            break; \
        case 2: \
            ERROR_REPORTER_HERE(ASC_USER_ERROR \
                    , "The " PROP " converged on Infinity or NaN."); \
            break; \
        case 3: \
            ERROR_REPORTER_HERE(ASC_USER_ERROR \
                    , "The search for the " PROP " failed to converge" \
                    "\nin the maximum number of iterations."); \
            break; \
        case 4: \
            ERROR_REPORTER_HERE(ASC_USER_ERROR \
                    , "There is no " PROP "; all components are supercritical" \
                    "\nat " PROP2 " %g " PROP2_UNIT, PROP2_VAL); \
            break; \
    }

#define CALCFLASH \
	PhaseSpec *PS = new_PhaseSpec(MS->pures, 3); \
	int fl_result = 0 \
		, mx_result = 0; \
	double T = inputs[0] /* mixture temperature */ \
		, p = inputs[1] /* mixture pressure */ \
		, tol = MIX_XTOL; /* tolerance used when solving for component densities */ \
	fl_result = mixture_flash(PS, MS, T, p, tol, &err); \
	ROOTSOLVE_SWITCH(fl_result,"vapor/liquid phase fractions"); \
	for(iij=0;iij<PS->phases;iij++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "AFTER FLASH: In phase number %u", iij); \
		for(iii=0;iii<PS->PH[iij]->pures;iii++){ \
			ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tcomponent number %u has mass fraction %g" \
					, iii, PS->PH[iij]->Xs[iii]); \
		} \
	} \
	mx_result = mixture_rhos_sat(PS, T, p, tol, &err); \
	ROOTSOLVE_SWITCH(mx_result,"density"); \
	for(iij=0;iij<PS->phases;iij++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "AFTER DENSITY: In phase number %u", iij); \
		for(iii=0;iii<PS->PH[iij]->pures;iii++){ \
			ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tcomponent number %u has mass fraction %g" \
					, iii, PS->PH[iij]->Xs[iii]); \
		} \
	}

#define CALC_PH_FLASH \
	PhaseSpec *PS = new_PhaseSpec(MS->pures, 3); \
	int ph_result = 0     /* result of finding temperature from (p,h) conditions */ \
		, fl_result = 0  /* result of performing flash */ \
		, mx_result = 0; /* result of finding mixture component densities */ \
	double p = inputs[0]  /* pressure */ \
		, h = inputs[1]   /* enthalpy */ \
		, T               /* temperature, determined from 'p' and 'h' */ \
		, tol = MIX_XTOL; /* tolerance used when solving for component densities */ \
	ph_result = mixture_T_ph(&T, MS, p, h, tol, &err); \
    ROOTSOLVE_SWITCH(ph_result, "system temperature"); \
	for(iii=0;iii<MS->pures;iii++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "AFTER TEMP: The component number %u has mass fraction %g" \
				, iii, MS->Xs[iii]); \
	} \
    fl_result = mixture_flash(PS, MS, T, p, tol, &err); \
	ROOTSOLVE_SWITCH(fl_result, "vapor/liquid phase fractions"); \
	for(iij=0;iij<PS->phases;iij++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "AFTER FLASH: In phase number %u", iij); \
		for(iii=0;iii<MS->pures;iii++){ \
			ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tcomponent number %u has mass fraction %g" \
					, iii, PS->PH[iij]->Xs[iii]); \
		} \
	} \
	mx_result = mixture_rhos_sat(PS, T, p, tol, &err); \
	ROOTSOLVE_SWITCH(mx_result,"density"); \
	for(iij=0;iij<PS->phases;iij++){ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "AFTER DENSITY: In phase number %u", iij); \
		for(iii=0;iii<PS->PH[iij]->pures;iii++){ \
			ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tcomponent number %u has mass fraction %g" \
					, iii, PS->PH[iij]->Xs[iii]); \
		} \
	}

#endif
