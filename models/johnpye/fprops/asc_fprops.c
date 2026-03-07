/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

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
	Wrapper for FPROPS to allow access from ASCEND.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* include the external function API from libascend... */
#include <ascend/compiler/extfunc.h>
#include <ascend/compiler/extcall.h>

/* include error reporting API as well, so we can send messages to user */
#include <ascend/utilities/error.h>

/* for accessing the DATA instance */
#include <ascend/compiler/child.h>
#include <ascend/general/list.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/childinfo.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/slist.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instance_types.h>
#include <ascend/compiler/arrayinst.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/setinstval.h>

/* the code that we're wrapping... */
#include "fprops.h"
#include "sat.h"
#include "solve_ph.h"
#include "thcond.h"
#include "visc.h"
#include "eqm.h"
#include "flash.h"
#include "flash_unifac.h"
#include "mixtures/unifac_data.h"
#include "mixtures/unifac_rundata.h"
#include "name_resolve.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "fluids.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

//#define ASCFPROPS_DEBUG
#ifdef ASCFPROPS_DEBUG
# define MSG(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_PROG_NOTE,MSG "\n",##ARGS);
#else
# define MSG(ARGS...) ((void)0)
#endif

#define ERRMSG(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_USER_ERROR,MSG,##ARGS);
#define ERRMSGP(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_PROG_ERR,MSG,##ARGS);
#define ASCFPROPS_UNIFAC_MAX_GROUP 47

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc asc_fprops_prepare;
ExtBBoxFinalFunc asc_fprops_final;
ExtBBoxFunc fprops_p_Trho_calc;
ExtBBoxFunc fprops_u_Trho_calc;
ExtBBoxFunc fprops_s_Trho_calc;
ExtBBoxFunc fprops_a_Trho_calc;
ExtBBoxFunc fprops_h_Trho_calc;
ExtBBoxFunc fprops_g_Trho_calc;
ExtBBoxFunc fprops_cp_Trho_calc;
ExtBBoxFunc fprops_cv_Trho_calc;
ExtBBoxFunc fprops_w_Trho_calc;
ExtBBoxFunc fprops_mu_Trho_calc;
ExtBBoxFunc fprops_lam_Trho_calc;
ExtBBoxFunc fprops_rho_Tp_calc;
ExtBBoxFunc fprops_cp_Tp_calc;
ExtBBoxFunc fprops_h_Tp_calc;
ExtBBoxFunc fprops_s_Tp_calc;
ExtBBoxFunc fprops_mu_T_incomp_calc;
ExtBBoxFunc fprops_lam_T_incomp_calc;
ExtBBoxFunc fprops_cp_T_incomp_calc;
ExtBBoxFunc fprops_phsx_vT_calc;
ExtBBoxFunc fprops_Tvsx_ph_calc;
ExtBBoxFunc fprops_Tvsx_h_incomp_calc;
ExtBBoxInitFunc asc_fprops_rxn_prepare;
ExtBBoxInitFunc asc_fprops_rxneq_prepare;
ExtBBoxInitFunc asc_fprops_flash_prepare;
ExtBBoxInitFunc asc_fprops_unifac_flash_prepare;
ExtBBoxInitFunc asc_fprops_unifac_gamma_prepare;
ExtBBoxFinalFunc asc_fprops_rxn_final;
ExtBBoxFinalFunc asc_fprops_unifac_flash_final;
ExtBBoxFunc fprops_rxn_h_TPn_calc;
ExtBBoxFunc fprops_rxn_v_TPn_calc;
ExtBBoxFunc fprops_rxn_eqm_TPn_calc;
ExtBBoxFunc fprops_flash_TPz_calc;
ExtBBoxFunc fprops_unifac_flash_TPz_calc;
ExtBBoxFunc fprops_unifac_gamma_Tx_calc;

/* FIXME need incompressible fluid functions that depend only on T or h, to 
	avoid unpivoted external relations...
*/

#define TCRIT(FLUID) (FLUID->data->T_c)
#define TTRIP(FLUID) (FLUID->data->T_t)
#define RHOCRIT(FLUID) (FLUID->data->rho_c)
#define PCRIT(FLUID) (FLUID->data->p_c)
#define FSU_TRHO(T,RHO) (FluidStateUnion){.Trho={T,RHO}}

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *fprops_symbols[3];
#define COMPONENT_SYM fprops_symbols[0]
#define TYPE_SYM fprops_symbols[1]
#define SOURCE_SYM fprops_symbols[2]

static const char *fprops_p_Trho_help = "Calculate pressure from temperature and density, using FPROPS";
static const char *fprops_u_Trho_help = "Calculate specific internal energy from temperature and density, using FPROPS";
static const char *fprops_s_Trho_help = "Calculate specific entropy from temperature and density, using FPROPS";
static const char *fprops_h_Trho_help = "Calculate specific enthalpy from temperature and density, using FPROPS";
static const char *fprops_a_Trho_help = "Calculate specific Helmholtz energy from temperature and density, using FPROPS";
static const char *fprops_g_Trho_help = "Calculate specific Gibbs energy from temperature and density, using FPROPS";
static const char *fprops_cp_Trho_help = "Calculate isobaric specific heat from temperature and density, using FPROPS";
static const char *fprops_cv_Trho_help = "Calculate isochoric specific heat from temperature and density, using FPROPS";
static const char *fprops_w_Trho_help = "Calculate speed of sound from temperature and density, using FPROPS";
static const char *fprops_mu_Trho_help = "Calculate viscosity from temperature and density, using FPROPS";
static const char *fprops_lam_Trho_help = "Calculate thermal conductivity sound from temperature and density, using FPROPS";

static const char *fprops_rho_Tp_help = "rho(T,p) esp. for incompressible substances";
static const char *fprops_cp_Tp_help = "h(T,p) esp. for incompressible substances";
static const char *fprops_h_Tp_help = "h(T,p) esp. for incompressible substances";
static const char *fprops_s_Tp_help = "s(T,p) esp. for incompressible substances";
static const char *fprops_mu_T_incomp_help = "mu(T,p) (dynamic viscosity) esp. for incompressible substances";
static const char *fprops_lam_T_incomp_help = "lam(T) (thermal conductivity) esp. for incompressible substances";
static const char *fprops_cp_T_incomp_help = "cp(T) (specific heat capacity) esp. for incompressible substances";

static const char *fprops_phsx_vT_help = "Calculate p, h, s, x from specific volume and temperature, using FPROPS";

static const char *fprops_Tvsx_ph_help = "Calculate T, v, s, x from pressure and enthalpy, using FPROPS";
static const char *fprops_Tvsx_h_incomp_help = "Calculate T, v, s, x for incompressible fluid from enthalpy, using FPROPS";
static const char *fprops_rxn_h_TPn_help = "Calculate package-based reactive mixture enthalpy from temperature, pressure and species molar vector, using FPROPS";
static const char *fprops_rxn_v_TPn_help = "Calculate package-based reactive mixture volume from temperature, pressure and species molar vector, using FPROPS";
static const char *fprops_rxn_eqm_TPn_help = "Calculate package-based equilibrium outlet species molar vector from temperature, pressure and inlet species molar vector, using FPROPS";
static const char *fprops_flash_TPz_help = "Calculate package-based TPz flash from temperature, pressure and overall composition, using FPROPS";
static const char *fprops_unifac_flash_TPz_help = "Calculate ideal-vapor plus UNIFAC-liquid TPz flash from temperature, pressure and overall composition, using FPROPS";
static const char *fprops_unifac_gamma_Tx_help = "Calculate original-UNIFAC liquid activity coefficients from temperature and liquid composition, using FPROPS";

typedef struct{
	int ns;
	FpropsRxnPackage *pkg;
	char *algorithm;
} AscFpropsRxnData;

typedef struct{
	int nc;
	int nsub;
	FpropsMultiphasePackage mpkg;
	FpropsUNIFACFlashPackage pkg;
	FpropsUNIFACComponentData *components;
	FpropsUNIFACSubgroupData *subgroups;
	int *sub_index_data;
	double *nu_data;
	double *a;
} AscFpropsUNIFACFlashData;
/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT fprops"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int fprops_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"FPROPS is still EXPERIMENTAL. Use with caution.");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, asc_fprops_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, asc_fprops_final \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

#define CALCFNDERIV(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, asc_fprops_prepare \
		, NAME##_calc /* value */ \
		, NAME##_calc /* derivatives */ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, asc_fprops_final \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFNDERIV(fprops_p_Trho,2,1);
	CALCFN(fprops_u_Trho,2,1);
	CALCFN(fprops_s_Trho,2,1);
	CALCFN(fprops_h_Trho,2,1);
	CALCFN(fprops_a_Trho,2,1);
	CALCFN(fprops_g_Trho,2,1);
	CALCFN(fprops_cp_Trho,2,1);
	CALCFN(fprops_cv_Trho,2,1);
	CALCFN(fprops_w_Trho,2,1);
	CALCFN(fprops_mu_Trho,2,1);
	CALCFN(fprops_lam_Trho,2,1);

	CALCFN(fprops_rho_Tp,2,1);
	CALCFN(fprops_cp_Tp,2,1);
	CALCFN(fprops_h_Tp,2,1);
	CALCFN(fprops_s_Tp,2,1);
	CALCFN(fprops_mu_T_incomp,1,1);
	CALCFN(fprops_lam_T_incomp,1,1);
	CALCFN(fprops_cp_T_incomp,1,1);
	CALCFN(fprops_phsx_vT,2,4);
	CALCFN(fprops_Tvsx_ph,2,4);
	CALCFN(fprops_Tvsx_h_incomp,2,4);
	result += CreateUserFunctionBlackBox("fprops_rxn_h_TPn"
		, asc_fprops_rxn_prepare
		, fprops_rxn_h_TPn_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_rxn_final
		, 3,1
		, fprops_rxn_h_TPn_help
		, 0.0
	);
	result += CreateUserFunctionBlackBox("fprops_rxn_v_TPn"
		, asc_fprops_rxn_prepare
		, fprops_rxn_v_TPn_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_rxn_final
		, 3,1
		, fprops_rxn_v_TPn_help
		, 0.0
	);
	result += CreateUserFunctionBlackBox("fprops_rxn_eqm_TPn"
		, asc_fprops_rxneq_prepare
		, fprops_rxn_eqm_TPn_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_rxn_final
		, 3,1
		, fprops_rxn_eqm_TPn_help
		, 0.0
	);
	result += CreateUserFunctionBlackBox("fprops_flash_TPz"
		, asc_fprops_flash_prepare
		, fprops_flash_TPz_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_unifac_flash_final
		, 3,3
		, fprops_flash_TPz_help
		, 0.0
	);
	result += CreateUserFunctionBlackBox("fprops_unifac_flash_TPz"
		, asc_fprops_unifac_flash_prepare
		, fprops_unifac_flash_TPz_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_unifac_flash_final
		, 3,3
		, fprops_unifac_flash_TPz_help
		, 0.0
	);
	result += CreateUserFunctionBlackBox("fprops_unifac_gamma_Tx"
		, asc_fprops_unifac_gamma_prepare
		, fprops_unifac_gamma_Tx_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_unifac_flash_final
		, 2,1
		, fprops_unifac_gamma_Tx_help
		, 0.0
	);

#undef CALCFN

	if(result){
		MSG("CreateUserFunction result = %d.",result);
	}
	return result;
}

/**
   'fprops_prepare' just gets the data member and checks that it's
	valid, and stores it in the blackbox data field.
*/
int asc_fprops_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *compinst, *typeinst, *srcinst;
	const char *comp, *type = NULL, *src = NULL;

	fprops_symbols[0] = AddSymbol("component");
	fprops_symbols[1] = AddSymbol("type");
	fprops_symbols[2] = AddSymbol("source");

	/* get the component name */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERRMSG("Couldn't locate 'component' in DATA, please check usage of FPROPS.")
		return 1;
	}
	if(InstanceKind(compinst)!=SYMBOL_CONSTANT_INST){
		ERRMSG("DATA member 'component' must be a symbol_constant");
		return 1;
	}
	comp = SCP(SYMC_INST(compinst)->value);
	if(comp==NULL || strlen(comp)==0){
		ERRMSG("'component' is NULL or empty");
		return 1;
	}

	/* get the component correlation type (FPROPS doesn't mind if none given) */
	typeinst = ChildByChar(data,TYPE_SYM);
	if(typeinst){
		if(InstanceKind(typeinst)!=SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'type' must be a symbol_constant");
			return 1;
		}
		type = SCP(SYMC_INST(typeinst)->value);
		//CONSOLE_DEBUG("TYPE: %s",type?type:"(null)");
		if(type && strlen(type)==0)type = NULL;
	}

	/* get the source data string (FPROPS doesn't mind if none given) */
	srcinst = ChildByChar(data,SOURCE_SYM);
	if(srcinst){
		if(InstanceKind(srcinst)!=SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'source' must be a symbol_constant");
			return 1;
		}
		src = SCP(SYMC_INST(srcinst)->value);
		CONSOLE_DEBUG("SOURCE: %s",src?src:"(null)");
		if(src && strlen(src)==0)src = NULL;
	}

	bbox->user_data = (void *)fprops_fluid(comp,type,src);
	if(bbox->user_data == NULL){
		ERRMSG("Unsupported component requested (name='%s',type='%s'). Check source-code for supported species.",comp,type);
		return 1;
	}

	MSG("Prepared component '%s'%s%s%s OK.",comp, type?" type '":"", type?type:"" ,type?"'":""
	);
	return 0;
}

void asc_fprops_final(struct BBoxInterp *bbox){
	if(bbox == NULL || bbox->user_data == NULL){
		return;
	}
	fprops_fluid_destroy((PureFluid *)bbox->user_data);
	bbox->user_data = NULL;
}

int asc_fprops_rxn_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *srcinst, *alginst, *components_inst, *species_name_inst;
	const char *source = NULL;
	const char *algorithm = NULL;
	const char **names = NULL;
	AscFpropsRxnData *rxn = NULL;
	unsigned long actual_inputs, actual_outputs, c, ns;
	symchar *components_sym, *species_name_sym, *source_sym, *algorithm_sym;
	const struct set_t *components_set = NULL;

	if(!bbox || !data || !arglist){
		ERRMSG("Reactive FPROPS blackbox received invalid prepare arguments");
		return 1;
	}
	if(gl_length(arglist) != 4){
		ERRMSG("Reactive FPROPS blackbox expects 3 INPUT groups and 1 OUTPUT group");
		return 1;
	}
	actual_inputs = CountNumberOfArgs(arglist,1,3);
	actual_outputs = CountNumberOfArgs(arglist,4,4);
	if(actual_inputs < 3){
		ERRMSG("Reactive FPROPS blackbox requires T, P and a species flow vector");
		return 1;
	}
	if(actual_outputs < 1){
		ERRMSG("Reactive FPROPS blackbox requires at least one output");
		return 1;
	}

	components_sym = AddSymbol("components");
	species_name_sym = AddSymbol("species_name");
	source_sym = AddSymbol("source");
	algorithm_sym = AddSymbol("algorithm");
	species_name_inst = ChildByChar(data, species_name_sym);
	components_inst = ChildByChar(data, components_sym);
	if(!species_name_inst && !components_inst){
		ERRMSG("Couldn't locate 'species_name' or 'components' in reactive package DATA");
		return 1;
	}
	if(species_name_inst){
		if(InstanceKind(species_name_inst) != ARRAY_INT_INST
				&& InstanceKind(species_name_inst) != ARRAY_ENUM_INST){
			ERRMSG("Reactive package species_name must be an array of symbol_constant");
			return 1;
		}
	}
	if(components_inst){
		components_set = SetAtomList(components_inst);
		if(!components_set || SetKind(components_set) != string_set){
			ERRMSG("Reactive package components must be a symbol-valued set");
			return 1;
		}
	}
	ns = species_name_inst ? NumberChildren(species_name_inst) : (components_set ? Cardinality(components_set) : 0);
	if(ns == 0){
		ERRMSG("Reactive package DATA contains no components");
		return 1;
	}
	if(species_name_inst && components_set && Cardinality(components_set) != ns){
		ERRMSG("Reactive package species_name size does not match components set cardinality");
		return 1;
	}
	if(actual_inputs != ns + 2){
		ERRMSG("Reactive package input vector length mismatch: got %lu species inputs, expected %lu",
			actual_inputs - 2, ns);
		return 1;
	}

	names = (const char **)calloc((size_t)ns, sizeof(char *));
	rxn = (AscFpropsRxnData *)calloc(1, sizeof(AscFpropsRxnData));
	if(!names || !rxn){
		ERRMSG("Unable to allocate reactive FPROPS blackbox workspace");
		free(names);
		free(rxn);
		return 1;
	}

	for(c = 1; c <= ns; ++c){
		const char *fallback_name = NULL;
		if(components_set){
			symchar *comp_sym = FetchStrMember(components_set, c);
			fallback_name = comp_sym ? SCP(comp_sym) : NULL;
		}
		names[c - 1] = NULL;
		if(species_name_inst){
			struct Instance *child = InstanceChild(species_name_inst, c);
			if(!child || InstanceKind(child) != SYMBOL_CONSTANT_INST){
				ERRMSG("Reactive package species_name must contain symbol_constant values");
				free(names);
				free(rxn);
				return 1;
			}
			if(AtomAssigned(child)){
				names[c - 1] = SCP(SYMC_INST(child)->value);
			}
		}
		if((!names[c - 1] || strlen(names[c - 1]) == 0) && fallback_name && strlen(fallback_name) > 0){
			names[c - 1] = fallback_name;
		}
		if(!names[c - 1] || strlen(names[c - 1]) == 0){
			ERRMSG("Reactive package DATA contains an empty component/species name");
			free(names);
			free(rxn);
			return 1;
		}
	}

	srcinst = ChildByChar(data, source_sym);
	if(srcinst){
		if(InstanceKind(srcinst) != SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'source' must be a symbol_constant");
			free(names);
			free(rxn);
			return 1;
		}
		source = SCP(SYMC_INST(srcinst)->value);
		if(source && strlen(source) == 0)source = NULL;
	}
	alginst = ChildByChar(data, algorithm_sym);
	if(alginst){
		if(InstanceKind(alginst) != SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'algorithm' must be a symbol_constant");
			free(names);
			free(rxn);
			return 1;
		}
		algorithm = SCP(SYMC_INST(alginst)->value);
		if(algorithm && strlen(algorithm) == 0)algorithm = NULL;
	}

	rxn->pkg = fprops_rxn_package_build(names, (int)ns, source);
	free(names);
	if(!rxn->pkg){
		ERRMSG("Failed to build reactive FPROPS package from DATA");
		free(rxn);
		return 1;
	}
	if(algorithm){
		rxn->algorithm = ASC_NEW_ARRAY(char, strlen(algorithm) + 1);
		if(!rxn->algorithm){
			fprops_rxn_package_free(rxn->pkg);
			free(rxn);
			ERRMSG("Unable to allocate reactive FPROPS algorithm string");
			return 1;
		}
		strcpy(rxn->algorithm, algorithm);
	}
	rxn->ns = (int)ns;
	bbox->user_data = (void *)rxn;
	return 0;
}

int asc_fprops_rxneq_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	int status;
	AscFpropsRxnData *rxn = NULL;
	if(!bbox || !data || !arglist){
		ERRMSG("Reactive FPROPS equilibrium blackbox received invalid prepare arguments");
		return 1;
	}
	status = asc_fprops_rxn_prepare(bbox, data, arglist);
	if(status){
		return status;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(!rxn){
		ERRMSG("Reactive FPROPS equilibrium blackbox prepare returned no package");
		return 1;
	}
	if(CountNumberOfArgs(arglist,4,4) != (unsigned long)rxn->ns){
		ERRMSG("Reactive FPROPS equilibrium blackbox requires one output per package species (got %lu, expected %d)",
			CountNumberOfArgs(arglist,4,4), rxn->ns);
		asc_fprops_rxn_final(bbox);
		return 1;
	}
	return 0;
}

void asc_fprops_rxn_final(struct BBoxInterp *bbox){
	AscFpropsRxnData *rxn;
	if(!bbox || !bbox->user_data){
		return;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(rxn->pkg){
		fprops_rxn_package_free(rxn->pkg);
	}
	free(rxn->algorithm);
	free(rxn);
	bbox->user_data = NULL;
}

static int asc_read_real_child(struct Instance *inst, const char *name, double *value){
	struct Instance *child = ChildByChar(inst, AddSymbol((char *)name));
	if(!child){
		ERRMSG("Couldn't locate '%s' in UNIFAC flash DATA", name);
		return 1;
	}
	*value = RealAtomValue(child);
	return 0;
}

static int asc_read_int_child(struct Instance *inst, const char *name, long *value){
	struct Instance *child = ChildByChar(inst, AddSymbol((char *)name));
	if(!child){
		ERRMSG("Couldn't locate '%s' in UNIFAC flash DATA", name);
		return 1;
	}
	*value = GetIntegerAtomValue(child);
	return 0;
}

static int asc_unifac_find_subgroup(symchar **subs, int nsub, symchar *sub){
	int i;
	for(i = 0; i < nsub; ++i){
		if(subs[i] == sub){
			return i;
		}
	}
	return -1;
}

static unsigned long asc_find_symbol_in_set(const struct set_t *set, symchar *sym){
	unsigned long i;
	if(!set || SetKind(set) != string_set){
		return 0;
	}
	for(i = 1; i <= Cardinality(set); ++i){
		if(FetchStrMember(set, i) == sym){
			return i;
		}
	}
	return 0;
}

static void asc_unifac_flash_free_data(AscFpropsUNIFACFlashData *fp){
	if(!fp){
		return;
	}
	fprops_flash_destroy_package(&fp->mpkg);
	ascfree(fp->components);
	ascfree(fp->subgroups);
	ascfree(fp->sub_index_data);
	ascfree(fp->nu_data);
	ascfree(fp->a);
	ascfree(fp);
}

static int asc_build_unifac_flash_package_from_ascend(struct Instance *cd, AscFpropsUNIFACFlashData **outpkg){
	struct Instance *components_inst, *data_inst;
	struct Instance *uc_inst = NULL;
	const struct set_t *components_set;
	symchar **sub_syms = NULL;
	int *comp_nsub = NULL;
	AscFpropsUNIFACFlashData *fp = NULL;
	unsigned long nc_ul, i_ul, k_ul;
	int nsub = 0;
	int total_nu = 0;
	int offset = 0;

	if(!cd || !outpkg){
		return 1;
	}

	components_inst = ChildByChar(cd, AddSymbol("components"));
	data_inst = ChildByChar(cd, AddSymbol("data"));
	if(!components_inst || !data_inst){
		ERRMSG("UNIFAC flash DATA must be a components_data instance");
		return 1;
	}

	components_set = SetAtomList(components_inst);
	if(!components_set || SetKind(components_set) != string_set){
		ERRMSG("UNIFAC flash DATA requires a string-valued components set");
		return 1;
	}
	nc_ul = Cardinality(components_set);
	if(nc_ul == 0){
		ERRMSG("UNIFAC flash DATA contains no components");
		return 1;
	}

	comp_nsub = ASC_NEW_ARRAY(int, nc_ul);
	if(!comp_nsub){
		ERRMSG("Unable to allocate UNIFAC flash workspace");
		return 1;
	}

	for(i_ul = 1; i_ul <= nc_ul; ++i_ul){
		symchar *comp_sym = FetchStrMember(components_set, i_ul);
		struct Instance *compinst = InstanceChild(data_inst, i_ul);
		const struct set_t *sub_set;
		unsigned long sub_card;
		if(!compinst){
			ERRMSG("UNIFAC flash DATA missing component '%s'", SCP(comp_sym));
			ascfree(comp_nsub);
			return 1;
		}
		if(!uc_inst){
			uc_inst = ChildByChar(compinst, AddSymbol("uc"));
		}
		sub_set = SetAtomList(ChildByChar(compinst, AddSymbol("subgroups")));
		if(!sub_set || SetKind(sub_set) != string_set){
			ERRMSG("Component '%s' has no UNIFAC subgroup set", SCP(comp_sym));
			ascfree(comp_nsub);
			return 1;
		}
		sub_card = Cardinality(sub_set);
		if(sub_card == 0){
			ERRMSG("Component '%s' has no UNIFAC subgroup data", SCP(comp_sym));
			ascfree(comp_nsub);
			return 1;
		}
		comp_nsub[i_ul - 1] = (int)sub_card;
		total_nu += (int)sub_card;
		for(k_ul = 1; k_ul <= sub_card; ++k_ul){
			symchar *sub_sym = FetchStrMember(sub_set, k_ul);
			if(asc_unifac_find_subgroup(sub_syms, nsub, sub_sym) < 0){
				symchar **tmp = ASC_NEW_ARRAY(symchar *, nsub + 1);
				int i;
				if(!tmp){
					ascfree(comp_nsub);
					ascfree(sub_syms);
					ERRMSG("Unable to allocate UNIFAC subgroup list");
					return 1;
				}
				for(i = 0; i < nsub; ++i){
					tmp[i] = sub_syms[i];
				}
				tmp[nsub] = sub_sym;
				ascfree(sub_syms);
				sub_syms = tmp;
				++nsub;
			}
		}
	}

	if(!uc_inst){
		ERRMSG("Unable to locate UNIFAC constants in components DATA");
		ascfree(comp_nsub);
		ascfree(sub_syms);
		return 1;
	}

	fp = ASC_NEW(AscFpropsUNIFACFlashData);
	if(!fp){
		ascfree(comp_nsub);
		ascfree(sub_syms);
		ERRMSG("Unable to allocate UNIFAC flash package");
		return 1;
	}
	memset(fp, 0, sizeof(*fp));
	fp->nc = (int)nc_ul;
	fp->nsub = nsub;
	fp->components = ASC_NEW_ARRAY(FpropsUNIFACComponentData, nc_ul);
	fp->subgroups = ASC_NEW_ARRAY(FpropsUNIFACSubgroupData, nsub);
	fp->sub_index_data = ASC_NEW_ARRAY(int, total_nu);
	fp->nu_data = ASC_NEW_ARRAY(double, total_nu);
	fp->a = ASC_NEW_ARRAY(double, ASCFPROPS_UNIFAC_MAX_GROUP * ASCFPROPS_UNIFAC_MAX_GROUP);
	if(!fp->components || !fp->subgroups || !fp->sub_index_data || !fp->nu_data || !fp->a){
		asc_unifac_flash_free_data(fp);
		ascfree(comp_nsub);
		ascfree(sub_syms);
		ERRMSG("Unable to allocate UNIFAC flash package storage");
		return 1;
	}

	for(i_ul = 0; i_ul < (unsigned long)nsub; ++i_ul){
		const struct set_t *uc_subgroups = SetAtomList(ChildByChar(uc_inst, AddSymbol("subgroups")));
		unsigned long pos;
		struct Instance *group_arr;
		struct Instance *R_arr;
		struct Instance *Q_arr;
		fp->subgroups[i_ul].name = SCP(sub_syms[i_ul]);
		pos = asc_find_symbol_in_set(uc_subgroups, sub_syms[i_ul]);
		group_arr = ChildByChar(uc_inst, AddSymbol("group"));
		R_arr = ChildByChar(uc_inst, AddSymbol("R"));
		Q_arr = ChildByChar(uc_inst, AddSymbol("Q"));
		if(!pos || !group_arr || !R_arr || !Q_arr){
			ERRMSG("Incomplete UNIFAC constants for subgroup '%s'", SCP(sub_syms[i_ul]));
			asc_unifac_flash_free_data(fp);
			ascfree(comp_nsub);
			ascfree(sub_syms);
			return 1;
		}
		fp->subgroups[i_ul].group = (int)GetIntegerAtomValue(InstanceChild(group_arr, pos));
		fp->subgroups[i_ul].R = RealAtomValue(InstanceChild(R_arr, pos));
		fp->subgroups[i_ul].Q = RealAtomValue(InstanceChild(Q_arr, pos));
	}

	for(i_ul = 1; i_ul <= ASCFPROPS_UNIFAC_MAX_GROUP; ++i_ul){
		struct Instance *row = InstanceChild(ChildByChar(uc_inst, AddSymbol("a")), i_ul);
		unsigned long j_ul;
		for(j_ul = 1; j_ul <= ASCFPROPS_UNIFAC_MAX_GROUP; ++j_ul){
			struct Instance *cell = InstanceChild(row, j_ul);
			fp->a[(i_ul - 1) * ASCFPROPS_UNIFAC_MAX_GROUP + (j_ul - 1)] = RealAtomValue(cell);
		}
	}

	for(i_ul = 1; i_ul <= nc_ul; ++i_ul){
		symchar *comp_sym = FetchStrMember(components_set, i_ul);
		struct Instance *compinst = InstanceChild(data_inst, i_ul);
		const struct set_t *sub_set = SetAtomList(ChildByChar(compinst, AddSymbol("subgroups")));
		struct Instance *nu_inst = ChildByChar(compinst, AddSymbol("nu"));
		long vp_corr;
		unsigned long sub_card = Cardinality(sub_set);
		double r = 0.0, q = 0.0;
		FpropsResolvedName resolved_name;
		FpropsNameResolveStatus resolve_status;
		const char *comp_name = SCP(comp_sym);

		resolve_status = fprops_name_resolve(comp_name,
			FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT, "UNIFAC-orig-2003", &resolved_name);
		if(resolve_status == FPROPS_NAME_RESOLVE_OK && resolved_name.canonical
				&& resolved_name.canonical->canonical){
			comp_name = resolved_name.canonical->canonical;
		}
		fp->components[i_ul - 1].name = comp_name;
		if(asc_read_real_child(compinst, "Tc", &fp->components[i_ul - 1].Tc)
				|| asc_read_real_child(compinst, "Pc", &fp->components[i_ul - 1].Pc)
				|| asc_read_real_child(compinst, "vpa", &fp->components[i_ul - 1].vpa)
				|| asc_read_real_child(compinst, "vpb", &fp->components[i_ul - 1].vpb)
				|| asc_read_real_child(compinst, "vpc", &fp->components[i_ul - 1].vpc)
				|| asc_read_real_child(compinst, "vpd", &fp->components[i_ul - 1].vpd)
				|| asc_read_real_child(compinst, "T0", &fp->components[i_ul - 1].T0)
				|| asc_read_real_child(compinst, "P0", &fp->components[i_ul - 1].P0)
				|| asc_read_real_child(compinst, "H0", &fp->components[i_ul - 1].H0)
				|| asc_read_real_child(compinst, "G0", &fp->components[i_ul - 1].G0)
				|| asc_read_real_child(compinst, "cpvapa", &fp->components[i_ul - 1].cpvapa)
				|| asc_read_real_child(compinst, "cpvapb", &fp->components[i_ul - 1].cpvapb)
				|| asc_read_real_child(compinst, "cpvapc", &fp->components[i_ul - 1].cpvapc)
				|| asc_read_real_child(compinst, "cpvapd", &fp->components[i_ul - 1].cpvapd)
				|| asc_read_real_child(compinst, "omega", &fp->components[i_ul - 1].omega)
				|| asc_read_real_child(compinst, "Zc", &fp->components[i_ul - 1].Zc)
				|| asc_read_real_child(compinst, "Vliq", &fp->components[i_ul - 1].Vliq)
				|| asc_read_real_child(compinst, "Tliq", &fp->components[i_ul - 1].Tliq)
				|| asc_read_int_child(compinst, "vp_correlation", &vp_corr)){
			asc_unifac_flash_free_data(fp);
			ascfree(comp_nsub);
			ascfree(sub_syms);
			return 1;
		}
		fp->components[i_ul - 1].vp_correlation = (int)vp_corr;
		fp->components[i_ul - 1].nsub = (int)sub_card;
		fp->components[i_ul - 1].sub_index = &fp->sub_index_data[offset];
		fp->components[i_ul - 1].nu = &fp->nu_data[offset];

		for(k_ul = 1; k_ul <= sub_card; ++k_ul){
			symchar *sub_sym = FetchStrMember(sub_set, k_ul);
			int si = asc_unifac_find_subgroup(sub_syms, nsub, sub_sym);
			double nu;
			if(si < 0 || !nu_inst){
				ERRMSG("Incomplete UNIFAC stoichiometry for component '%s' subgroup '%s'",
					SCP(comp_sym), SCP(sub_sym));
				asc_unifac_flash_free_data(fp);
				ascfree(comp_nsub);
				ascfree(sub_syms);
				return 1;
			}
			nu = (double)GetIntegerAtomValue(InstanceChild(nu_inst, k_ul));
			fp->sub_index_data[offset + (int)k_ul - 1] = si;
			fp->nu_data[offset + (int)k_ul - 1] = nu;
			r += nu * fp->subgroups[si].R;
			q += nu * fp->subgroups[si].Q;
		}
		fp->components[i_ul - 1].r = r;
		fp->components[i_ul - 1].q = q;
		offset += (int)sub_card;
	}

	fp->pkg.nc = fp->nc;
	fp->pkg.nsub = fp->nsub;
	fp->pkg.components = fp->components;
	fp->pkg.subgroups = fp->subgroups;
	fp->pkg.a = fp->a;
	fp->mpkg.kind = FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL;
	fp->mpkg.nc = fp->nc;
	fp->mpkg.data.unifac_ideal_vl.run = NULL;
	fp->mpkg.data.unifac_ideal_vl.pkg = &fp->pkg;
	*outpkg = fp;
	ascfree(comp_nsub);
	ascfree(sub_syms);
	return 0;
}

static int asc_build_unifac_flash_package_native(struct Instance *cd, AscFpropsUNIFACFlashData **outpkg){
	struct Instance *components_inst;
	const struct set_t *components_set;
	const FpropsUNIFACSourceData *src;
	AscFpropsUNIFACFlashData *fp = NULL;
	const char **names = NULL;
	unsigned long nc_ul, i_ul;

	if(!cd || !outpkg){
		return 1;
	}

	src = fprops_unifac_source("UNIFAC-orig-2003");
	if(!src){
		ERRMSG("Unable to locate native UNIFAC source data");
		return 1;
	}

	components_inst = ChildByChar(cd, AddSymbol("components"));
	components_set = components_inst ? SetAtomList(components_inst) : NULL;
	if(!components_set || SetKind(components_set) != string_set){
		ERRMSG("UNIFAC flash DATA requires a string-valued components set");
		return 1;
	}
	nc_ul = Cardinality(components_set);
	if(nc_ul == 0){
		ERRMSG("UNIFAC flash DATA contains no components");
		return 1;
	}
	names = ASC_NEW_ARRAY(const char *, nc_ul);
	if(!names){
		ERRMSG("Unable to allocate native UNIFAC component-name list");
		return 1;
	}

	fp = ASC_NEW(AscFpropsUNIFACFlashData);
	if(!fp){
		ascfree(names);
		ERRMSG("Unable to allocate native UNIFAC flash package");
		return 1;
	}
	memset(fp, 0, sizeof(*fp));

	for(i_ul = 1; i_ul <= nc_ul; ++i_ul){
		symchar *comp_sym = FetchStrMember(components_set, i_ul);
		FpropsResolvedName resolved_name;
		FpropsNameResolveStatus resolve_status;
		const char *comp_name = SCP(comp_sym);

		resolve_status = fprops_name_resolve(comp_name,
			FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT, "UNIFAC-orig-2003", &resolved_name);
		if(resolve_status == FPROPS_NAME_RESOLVE_OK && resolved_name.canonical
				&& resolved_name.canonical->canonical){
			comp_name = resolved_name.canonical->canonical;
		}
		names[i_ul - 1] = comp_name;
	}

	if(fprops_flash_prepare_unifac(&fp->mpkg, "UNIFAC-orig-2003", names, (int)nc_ul)){
		asc_unifac_flash_free_data(fp);
		ascfree(names);
		ERRMSG("Unable to prepare native UNIFAC flash package");
		return 1;
	}
	fp->nc = fp->mpkg.nc;
	fp->nsub = fp->mpkg.data.unifac_ideal_vl.pkg ? fp->mpkg.data.unifac_ideal_vl.pkg->nsub : 0;
	*outpkg = fp;
	ascfree(names);
	return 0;
}

static int asc_build_unifac_flash_package(struct Instance *cd, AscFpropsUNIFACFlashData **outpkg){
	if(!asc_build_unifac_flash_package_native(cd, outpkg)){
		return 0;
	}
	MSG("native UNIFAC package build failed, falling back to ASCEND instance extraction");
	return asc_build_unifac_flash_package_from_ascend(cd, outpkg);
}

int asc_fprops_flash_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	return asc_fprops_unifac_flash_prepare(bbox, data, arglist);
}

int asc_fprops_unifac_flash_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	AscFpropsUNIFACFlashData *fp = NULL;
	unsigned long actual_inputs, actual_outputs;
	struct Instance *components_inst;
	const struct set_t *components_set;
	unsigned long nc;

	if(!bbox || !data || !arglist){
		ERRMSG("UNIFAC flash blackbox received invalid prepare arguments");
		return 1;
	}
	if(gl_length(arglist) != 6){
		ERRMSG("UNIFAC flash blackbox expects 3 INPUT groups and 3 OUTPUT groups");
		return 1;
	}
	actual_inputs = CountNumberOfArgs(arglist, 1, 3);
	actual_outputs = CountNumberOfArgs(arglist, 4, 6);

	components_inst = ChildByChar(data, AddSymbol("components"));
	components_set = components_inst ? SetAtomList(components_inst) : NULL;
	if(!components_set){
		ERRMSG("UNIFAC flash DATA must provide a components set");
		return 1;
	}
	nc = Cardinality(components_set);
	if(actual_inputs != nc + 2){
		ERRMSG("UNIFAC flash input vector length mismatch: got %lu component inputs, expected %lu",
			actual_inputs - 2, nc);
		return 1;
	}
	if(actual_outputs != 1 + 2 * nc){
		ERRMSG("UNIFAC flash output vector length mismatch: got %lu outputs, expected %lu",
			actual_outputs, 1 + 2 * nc);
		return 1;
	}
	if(asc_build_unifac_flash_package(data, &fp)){
		return 1;
	}
	bbox->user_data = fp;
	return 0;
}

int asc_fprops_unifac_gamma_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	AscFpropsUNIFACFlashData *fp = NULL;
	unsigned long actual_inputs, actual_outputs;
	struct Instance *components_inst;
	const struct set_t *components_set;
	unsigned long nc;

	if(!bbox || !data || !arglist){
		ERRMSG("UNIFAC gamma blackbox received invalid prepare arguments");
		return 1;
	}
	if(gl_length(arglist) != 3){
		ERRMSG("UNIFAC gamma blackbox expects 2 INPUT groups and 1 OUTPUT group");
		return 1;
	}
	actual_inputs = CountNumberOfArgs(arglist, 1, 2);
	actual_outputs = CountNumberOfArgs(arglist, 3, 3);

	components_inst = ChildByChar(data, AddSymbol("components"));
	components_set = components_inst ? SetAtomList(components_inst) : NULL;
	if(!components_set){
		ERRMSG("UNIFAC gamma DATA must provide a components set");
		return 1;
	}
	nc = Cardinality(components_set);
	if(actual_inputs != nc + 1){
		ERRMSG("UNIFAC gamma input vector length mismatch: got %lu composition inputs, expected %lu",
			actual_inputs - 1, nc);
		return 1;
	}
	if(actual_outputs != nc){
		ERRMSG("UNIFAC gamma output vector length mismatch: got %lu outputs, expected %lu",
			actual_outputs, nc);
		return 1;
	}
	if(asc_build_unifac_flash_package(data, &fp)){
		return 1;
	}
	bbox->user_data = fp;
	return 0;
}

void asc_fprops_unifac_flash_final(struct BBoxInterp *bbox){
	AscFpropsUNIFACFlashData *fp;
	if(!bbox || !bbox->user_data){
		return;
	}
	fp = (AscFpropsUNIFACFlashData *)bbox->user_data;
	asc_unifac_flash_free_data(fp);
	bbox->user_data = NULL;
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

static const char *ninputs_msg = "Incorrect call: %u inputs received, but expected %u";
static const char *noutputs_msg = "Incorrect call: %u outputs received, but expected %u";
#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN){ERRMSG(ninputs_msg,ninputs,NIN);return -1;} \
	if(noutputs != NOUT){ERRMSG(noutputs_msg,noutputs,NOUT)return -2;} \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	const PureFluid *FLUID = (const PureFluid *)bbox->user_data;\
    FpropsError err=FPROPS_NO_ERROR;

/**
	Evaluation function for 'fprops_p'.
	@param inputs array with values of inputs T and rho.
	@param outputs array with just value of p
	@param jacobian, the partial derivative df/dx, where
		each row is df[i]/dx[j] over each j for the y_out[i] of
		matching index. The jacobian array is 1-D, row major, i.e.
		df[i]/dx[j] -> jacobian[i*ninputs+j].
	@return 0 on success
*/
int fprops_p_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		FluidState2 S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
		outputs[0] = fprops_p(S, &err);
	}else{
		//MSG("JACOBIAN CALCULATION FOR P!\n");
		FluidState2 S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
		jacobian[0*1+0] = fprops_dpdT_rho(S, &err);
		jacobian[0*1+1] = fprops_dpdrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_u'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_u_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		outputs[0] = fprops_u(S, &err);
	}else{
		jacobian[0*1+0] = fprops_dudT_rho(S, &err);
		jacobian[0*1+1] = fprops_dudrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_s'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_s_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_s(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_h'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_h_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		outputs[0] = fprops_h(S, &err);
	}else{
		//MSG("JACOBIAN CALCULATION FOR P!\n");
		jacobian[0*1+0] = fprops_dhdT_rho(S, &err);
		jacobian[0*1+1] = fprops_dhdrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_a'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_a_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_a(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_g'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_g_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_g(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_cp'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_cp_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_cp(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_cv'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_cv_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_cv(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_w'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_w_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_w(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

/**
	Evaluation function for 'fprops_mu'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_mu_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_mu(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

/**
	Evaluation function for 'fprops_lam'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_lam_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_lam(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

#define	CALC_TP_BODY(fn) \
	CALCPREPARE(2,1);\
	FluidState2 S = fprops_set_Tp(inputs[0],inputs[1],FLUID,&err);\
	outputs[0] = fprops_##fn(S,&err);\
	if(err){\
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve %s(T,rho) for '%s' (err '%s'."\
			,#fn,FLUID->name,fprops_error(err));\
		return 1;\
	}\
	return 0;

#define	CALC_T_BODY(fn) \
	CALCPREPARE(1,1); /* the pressure value is completely arbitrary, set to 999 */\
	FluidState2 S = fprops_set_Tp(inputs[0],999,FLUID,&err);\
	outputs[0] = fprops_##fn(S,&err);\
	if(err){\
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve %s(T,rho) for '%s' (err '%s'."\
			,#fn,FLUID->name,fprops_error(err));\
		return 1;\
	}\
	return 0;

int fprops_rho_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(lam);
}
int fprops_cp_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(cp);
}
int fprops_h_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(h);
}
int fprops_s_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(s);
}
int fprops_mu_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(mu);
}
int fprops_lam_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(lam);
}

int fprops_cp_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(cp);
}


/**
	Evaluation function for 'fprops_phsx_vT'
	@return 0 on success
*/
int fprops_phsx_vT_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	/*
		TODO unclear why we don't put this code into the main fprops.c routines...?
	 	there is an attempt here to minimise calls to the slow function fprops_sat_T,
		by making direct calls to s_fn and h_fn. FIXME A better solution would be to move 
		all of this code into solve_ph.c or similar.
	*/
	double rho = 1./inputs[0];
	double T = inputs[1];
	double p_sat, rho_f, rho_g;

	if(T < TCRIT(FLUID)){
		fprops_sat_T(T, &p_sat, &rho_f, &rho_g, FLUID, &err);

		if(rho < rho_f && rho > rho_g){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
			double x = (inputs[0] - vf)  /(vg - vf);
			double sf = FLUID->s_fn(FSU_TRHO(T,rho_f), FLUID->data, &err);
			double hf = FLUID->h_fn(FSU_TRHO(T,rho_f), FLUID->data, &err);
			double sg = FLUID->s_fn(FSU_TRHO(T,rho_g), FLUID->data, &err);
			double hg = FLUID->h_fn(FSU_TRHO(T,rho_g), FLUID->data, &err);
			outputs[0] = p_sat;
			outputs[1] = hf + x * (hg-hf);
			outputs[2] = sf + x * (sg-sf);
			outputs[3] = x;
			/* maybe there was an error solving the saturation state? */
			return err;
		}
	}

	/* non-saturated */
	outputs[0] = FLUID->p_fn(FSU_TRHO(T,rho), FLUID->data, &err);
	outputs[1] = FLUID->h_fn(FSU_TRHO(T,rho), FLUID->data, &err);
	outputs[2] = FLUID->s_fn(FSU_TRHO(T,rho), FLUID->data, &err);
	outputs[3] = rho < RHOCRIT(FLUID) ? 1 : 0;
	return 0;
}


/**
	Evaluation function for 'fprops_Tvsx_ph'
	@return 0 on success
*/
int fprops_Tvsx_ph_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	static const PureFluid *last = NULL;
	static double p,h,T,v,s,x;
	if(last == FLUID && p == inputs[0] && h == inputs[1]){
		outputs[0] = T;
		outputs[1] = v;
		outputs[2] = s;
		outputs[3] = x;
		return 0;
	}

	p = inputs[0];
	h = inputs[1];
	switch(FLUID->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		{
			// the code below aims to avoid calls to fprops_sat_T, which are slow.

	double hft, pt, rhoft,rhogt;
	fprops_triple_point(&pt,&rhoft,&rhogt,FLUID,&err);
	if(err){
				ERRMSGP("Failed to solve triple point for %s.",FLUID->name);
		return 5;
	}
			hft = FLUID->h_fn(FSU_TRHO(TTRIP(FLUID),rhoft),FLUID->data,&err);
	if(h < hft){
				ERRMSGP("Input enthalpy %f kJ/kg is below triple point liquid enthalpy %f kJ/kg"
			,h/1e3,hft/1e3
		);
		return 6;
	}
	
	if(p < pt){
				ERRMSGP("Input pressure %f bar is below triple point pressure %f bar"
			,p/1e5,pt/1e5
		);
		outputs[0] = TTRIP(FLUID);
		outputs[1] = 1./ rhoft;
				outputs[2] = FLUID->s_fn(FSU_TRHO(TTRIP(FLUID),rhoft),FLUID->data, &err);
		outputs[3] = 0;
		return 7;
	}

	if(p < PCRIT(FLUID)){
		double T_sat, rho_f, rho_g;
		
		fprops_sat_p(p, &T_sat, &rho_f, &rho_g, FLUID, &err);
		if(err){
					ERRMSGP("Failed to solve saturation state of %s for p = %f bar < pc (= %f bar)"
				, FLUID->name, p/1e5,PCRIT(FLUID)/1e5
			);
			outputs[0] = TTRIP(FLUID);
			outputs[1] = 1./rhoft;
					outputs[2] = FLUID->s_fn(FSU_TRHO(TTRIP(FLUID), rhoft), FLUID->data, &err);
			outputs[3] = 0;
			return 8;
		}
		
				double hf = FLUID->h_fn(FSU_TRHO(T_sat, rho_f),FLUID->data,&err);
				double hg = FLUID->h_fn(FSU_TRHO(T_sat, rho_g),FLUID->data,&err);

		if(hf < h && h < hg){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
					double sf = FLUID->s_fn(FSU_TRHO(T_sat, rho_f),FLUID->data,&err);
					double sg = FLUID->s_fn(FSU_TRHO(T_sat, rho_g),FLUID->data,&err);
			T = T_sat;
			x = (h - hf)  /(hg - hf);
			v = vf + x * (vg-vf);
			s = sf + x * (sg-sf);
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
					MSG("Saturated state, p=%f bar, h = %f kJ/kg.",p/1e5,h/1e3);
			return 0;
		}
	}

			FluidState2 S = fprops_solve_ph(p,h, FLUID, &err); // prev code was use_guess=0
			double rho = S.vals.Trho.rho;
			T = S.vals.Trho.T;
	if(err){
				ERRMSGP("Failed to solve for (p,h): %s",fprops_error(err));
		return 9;
	}
	/* non-saturated */
	v = 1./rho;
			s = FLUID->s_fn(S.vals, FLUID->data, &err); // straight to EOS, no sat test req.
	x = (v > 1./RHOCRIT(FLUID)) ? 1 : 0;
	last = FLUID;
	outputs[0] = T;
	outputs[1] = v;
	outputs[2] = s;
	outputs[3] = x;
			MSG("Non-saturated state, p = %f bar, h = %f kJ/kg.",p/1e5,h/1e3);
	return 0;
}
	case FPROPS_INCOMP:
		{
			MSG("Solving for p=%f bar, h=%f kJ/kg.",p/1e5, h/1e3);
			FluidState2 S;
			S = fprops_solve_ph(p,h,FLUID,&err);
			double rho = fprops_rho(S,&err);
			T = fprops_T(S,&err);
			s = fprops_s(S,&err);
			v = 1./rho;
			x = 0;
			MSG("Got T = %f, rho = %f, s = %f",T,rho,s);
			if(err){
				ERRMSGP("Failed to solve (p,h): %s (fluid '%s')",fprops_error(err),FLUID->name);
				return 9;
			}
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
			return 0;
		}
	default:
		ERRMSGP("Invalid fluid type (type %u)",FLUID->type);
		return 10;
	}
}




/**
	Evaluation function for 'fprops_Tvsx_h_incomp'
	@return 0 on success
*/
int fprops_Tvsx_h_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(1,4);

	static const PureFluid *last = NULL;
	double p = 1e5; // arbitrary!
	static double h,T,v,s,x;
	if(last == FLUID && h == inputs[1]){
		outputs[0] = T;
		outputs[1] = v;
		outputs[2] = s;
		outputs[3] = x;
		return 0;
	}

	h = inputs[0];

	MSG("hello!");

	switch(FLUID->type){
	case FPROPS_INCOMP:
		{
			MSG("Solving for p=%f bar, h=%f kJ/kg.",p/1e5, h/1e3);
			FluidState2 S;
			S = fprops_solve_ph(p,h,FLUID,&err);
			double rho = fprops_rho(S,&err);
			T = fprops_T(S,&err);
			s = fprops_s(S,&err);
			v = 1./rho;
			x = 0;
			if(err){
				ERRMSGP("Failed to solve (p,h): %s (fluid '%s')",fprops_error(err),FLUID->name);
				return 9;
			}
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
			MSG("...returning T = %f",T);
			return 0;
		}
	default:
		ERRMSGP("Invalid fluid type (type %u)",FLUID->type);
		return 10;
	}
}

int fprops_rxn_h_TPn_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsRxnData *rxn;
	FpropsRxnTPN state;
	double H = 0.0;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(!rxn->pkg){
		ERRMSG("Reactive FPROPS blackbox has no prepared package");
		return -6;
	}
	if(ninputs != rxn->ns + 2){
		ERRMSG("Reactive FPROPS blackbox received %d inputs, expected %d", ninputs, rxn->ns + 2);
		return -1;
	}
	if(noutputs != 1){
		ERRMSG("Reactive FPROPS blackbox received %d outputs, expected 1", noutputs);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	state.T = inputs[0];
	state.P = inputs[1];
	state.n = &inputs[2];
	status = fprops_rxn_mix_h(rxn->pkg, &state, &H);
	if(status){
		ERRMSG("Reactive FPROPS enthalpy evaluation failed with status %d", status);
		return status;
	}
	outputs[0] = H;
	return 0;
}

int fprops_rxn_v_TPn_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsRxnData *rxn;
	FpropsRxnTPN state;
	double V = 0.0;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(!rxn->pkg){
		ERRMSG("Reactive FPROPS volume blackbox has no prepared package");
		return -6;
	}
	if(ninputs != rxn->ns + 2){
		ERRMSG("Reactive FPROPS volume blackbox received %d inputs, expected %d", ninputs, rxn->ns + 2);
		return -1;
	}
	if(noutputs != 1){
		ERRMSG("Reactive FPROPS volume blackbox received %d outputs, expected 1", noutputs);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	state.T = inputs[0];
	state.P = inputs[1];
	state.n = &inputs[2];
	status = fprops_rxn_mix_v(rxn->pkg, &state, &V);
	if(status){
		ERRMSG("Reactive FPROPS volume evaluation failed with status %d", status);
		return status;
	}
	outputs[0] = V;
	return 0;
}

int fprops_rxn_eqm_TPn_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsRxnData *rxn;
	FpropsRxnTPN state;
	FpropsRxnResult out;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(!rxn->pkg){
		ERRMSG("Reactive FPROPS equilibrium blackbox has no prepared package");
		return -6;
	}
	if(ninputs != rxn->ns + 2){
		ERRMSG("Reactive FPROPS equilibrium blackbox received %d inputs, expected %d", ninputs, rxn->ns + 2);
		return -1;
	}
	if(noutputs != rxn->ns){
		ERRMSG("Reactive FPROPS equilibrium blackbox received %d outputs, expected %d", noutputs, rxn->ns);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	state.T = inputs[0];
	state.P = inputs[1];
	state.n = &inputs[2];
	out.status = -99;
	out.H = NAN;
	out.G = NAN;
	out.n_out = outputs;
	status = fprops_rxn_eqm_tpy(rxn->pkg, &state,
		rxn->algorithm ? rxn->algorithm : "reduced",
		NULL, &out);
	if(status != 0 && status != 1 && status != 6){
		ERRMSG("Reactive FPROPS equilibrium evaluation failed with status %d", status);
		return status;
	}
	return 0;
}

int fprops_flash_TPz_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsUNIFACFlashData *fp;
	FpropsFlashTPZ in;
	FpropsFlashVLResult out;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	fp = (AscFpropsUNIFACFlashData *)bbox->user_data;
	if(ninputs != fp->nc + 2){
		ERRMSG("FPROPS flash blackbox received %d inputs, expected %d", ninputs, fp->nc + 2);
		return -1;
	}
	if(noutputs != 1 + 2 * fp->nc){
		ERRMSG("FPROPS flash blackbox received %d outputs, expected %d", noutputs, 1 + 2 * fp->nc);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	in.T = inputs[0];
	in.P = inputs[1];
	in.z = &inputs[2];
	out.status = -99;
	out.beta = NAN;
	out.x = &outputs[1];
	out.y = &outputs[1 + fp->nc];
	status = fprops_flash_tpz(&fp->mpkg, &in, &out);
	if(status){
		ERRMSG("FPROPS flash evaluation failed with status %d", status);
		return status;
	}
	outputs[0] = out.beta;
	return 0;
}

int fprops_unifac_flash_TPz_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsUNIFACFlashData *fp;
	FpropsFlashTPZ in;
	FpropsFlashVLResult out;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	fp = (AscFpropsUNIFACFlashData *)bbox->user_data;
	if(ninputs != fp->nc + 2){
		ERRMSG("UNIFAC flash blackbox received %d inputs, expected %d", ninputs, fp->nc + 2);
		return -1;
	}
	if(noutputs != 1 + 2 * fp->nc){
		ERRMSG("UNIFAC flash blackbox received %d outputs, expected %d", noutputs, 1 + 2 * fp->nc);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	in.T = inputs[0];
	in.P = inputs[1];
	in.z = &inputs[2];
	out.status = -99;
	out.beta = NAN;
	out.x = &outputs[1];
	out.y = &outputs[1 + fp->nc];
	status = fprops_unifac_flash_tpz(&fp->pkg, &in, &out);
	if(status){
		ERRMSG("UNIFAC flash evaluation failed with status %d", status);
		return status;
	}
	outputs[0] = out.beta;
	return 0;
}

int fprops_unifac_gamma_Tx_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsUNIFACFlashData *fp;
	int status;
	(void)jacobian;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	fp = (AscFpropsUNIFACFlashData *)bbox->user_data;
	if(ninputs != fp->nc + 1){
		ERRMSG("UNIFAC gamma blackbox received %d inputs, expected %d", ninputs, fp->nc + 1);
		return -1;
	}
	if(noutputs != fp->nc){
		ERRMSG("UNIFAC gamma blackbox received %d outputs, expected %d", noutputs, fp->nc);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	status = fprops_unifac_gamma(&fp->pkg, inputs[0], &inputs[1], outputs);
	if(status){
		ERRMSG("UNIFAC gamma evaluation failed with status %d", status);
		return status;
	}
	return 0;
}
