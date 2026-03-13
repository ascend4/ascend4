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
#include <stdio.h>

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
ExtBBoxInitFunc asc_fprops_unifac_liq_fugacity_prepare;
ExtBBoxFinalFunc asc_fprops_rxn_final;
ExtBBoxFinalFunc asc_fprops_unifac_flash_final;
ExtBBoxFunc fprops_rxn_h_TPn_calc;
ExtBBoxFunc fprops_rxn_v_TPn_calc;
ExtBBoxFunc fprops_rxn_eqm_TPn_calc;
ExtBBoxFunc fprops_rxn_eqm_TPn_deriv;
ExtBBoxFunc fprops_flash_TPz_calc;
ExtBBoxFunc fprops_unifac_flash_TPz_calc;
ExtBBoxFunc fprops_unifac_gamma_Tx_calc;
ExtBBoxFunc fprops_unifac_liq_fugacity_TPx_calc;

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
static const char *fprops_unifac_liq_fugacity_TPx_help = "Calculate ideal-vapor-reference UNIFAC liquid component fugacities from temperature, pressure and liquid composition, using FPROPS";

typedef struct{
	int ns;
	FpropsRxnPackage *pkg;
	char *algorithm;
	char *source;
	char **names;
	/* Keep the equilibrium blackbox stateless by default; opt in only when
	   seed reuse is explicitly being studied. */
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
	double *last_n;
	int have_last_n;
#endif
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

static int asc_fprops_rxn_eqm_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("ASC_FPROPS_RXN_EQM_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static int asc_fprops_rxn_state_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("ASC_FPROPS_RXN_STATE_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static void asc_fprops_rxn_state_trace(const char *event, const struct BBoxInterp *bbox,
		const AscFpropsRxnData *rxn, double T, double P, const double *inputs_n,
		const double *outputs_n, int status){
	static long seq = 0;
	double inlet_sum = 0.0;
	double outlet_sum = 0.0;
	int i;
	if(!asc_fprops_rxn_state_trace_enabled()){
		return;
	}
	++seq;
	if(rxn && inputs_n){
		for(i = 0; i < rxn->ns; ++i){
			if(isfinite(inputs_n[i])){
				inlet_sum += inputs_n[i];
			}
		}
	}
	if(rxn && outputs_n){
		for(i = 0; i < rxn->ns; ++i){
			if(isfinite(outputs_n[i])){
				outlet_sum += outputs_n[i];
			}
		}
	}
	fprintf(stderr,
		"ASC_FPROPS_RXN_STATE_TRACE seq=%ld event=%s bbox=%p user_data=%p pkg=%p task=%d ns=%d"
		" T=%.17g P=%.17g inlet_sum=%.17g outlet_sum=%.17g status=%d alg=%s\n",
		seq, event ? event : "(null)", (void *)bbox, bbox ? bbox->user_data : NULL,
		(void *)(rxn ? rxn->pkg : NULL), bbox ? (int)bbox->task : -1, rxn ? rxn->ns : -1,
		T, P, inlet_sum, outlet_sum, status, (rxn && rxn->algorithm) ? rxn->algorithm : "(null)");
	fflush(stderr);
}

static int asc_fprops_rxn_eqm_status_ok(int status){
	return status == 0 || status == 1 || status == 6;
}

static int asc_fprops_rxn_find_name_index(const AscFpropsRxnData *rxn, const char *name){
	int i;
	if(!rxn || !rxn->names || !name){
		return -1;
	}
	for(i = 0; i < rxn->ns; ++i){
		if(rxn->names[i] && 0 == strcmp(rxn->names[i], name)){
			return i;
		}
	}
	return -1;
}

static void asc_fprops_rxn_eqm_trace_report(const AscFpropsRxnData *rxn, double T, double P,
		const double *inputs_n, int status_pkg, const double *out_pkg, int bbox_task){
	static long seq = 0;
	int i_n2, i_o2, i_ar, i_h2o, i_co2, i_no, i_no2, i_co, i_h2;
	double inlet_sum = 0.0;
	int i;
	if(!asc_fprops_rxn_eqm_trace_enabled() || !rxn){
		return;
	}
	++seq;
	i_n2 = asc_fprops_rxn_find_name_index(rxn, "nitrogen");
	i_o2 = asc_fprops_rxn_find_name_index(rxn, "oxygen");
	i_ar = asc_fprops_rxn_find_name_index(rxn, "argon");
	i_h2o = asc_fprops_rxn_find_name_index(rxn, "water");
	i_co2 = asc_fprops_rxn_find_name_index(rxn, "carbondioxide");
	i_no = asc_fprops_rxn_find_name_index(rxn, "nitric_oxide");
	i_no2 = asc_fprops_rxn_find_name_index(rxn, "nitrogen_dioxide");
	i_co = asc_fprops_rxn_find_name_index(rxn, "carbonmonoxide");
	i_h2 = asc_fprops_rxn_find_name_index(rxn, "hydrogen");
	for(i = 0; i < rxn->ns; ++i){
		if(inputs_n && isfinite(inputs_n[i])){
			inlet_sum += inputs_n[i];
		}
	}
	fprintf(stderr,
		"ASC_FPROPS_RXN_EQM_TRACE seq=%ld task=%d alg=%s T=%.17g P=%.17g inlet_sum=%.17g"
		" in[N2]=%.17g in[O2]=%.17g in[Ar]=%.17g in[H2O]=%.17g in[CO2]=%.17g"
		" pkg_status=%d"
		" pkg[NO]=%.17g pkg[NO2]=%.17g pkg[CO]=%.17g pkg[H2]=%.17g"
		"\n",
		seq, bbox_task, rxn->algorithm ? rxn->algorithm : "(null)", T, P, inlet_sum,
		(inputs_n && i_n2 >= 0) ? inputs_n[i_n2] : NAN,
		(inputs_n && i_o2 >= 0) ? inputs_n[i_o2] : NAN,
		(inputs_n && i_ar >= 0) ? inputs_n[i_ar] : NAN,
		(inputs_n && i_h2o >= 0) ? inputs_n[i_h2o] : NAN,
		(inputs_n && i_co2 >= 0) ? inputs_n[i_co2] : NAN,
		status_pkg,
		(out_pkg && i_no >= 0) ? out_pkg[i_no] : NAN,
		(out_pkg && i_no2 >= 0) ? out_pkg[i_no2] : NAN,
		(out_pkg && i_co >= 0) ? out_pkg[i_co] : NAN,
		(out_pkg && i_h2 >= 0) ? out_pkg[i_h2] : NAN
	);
	fflush(stderr);
}

static const char *asc_name_domain_label(unsigned domains){
	switch(domains){
	case FPROPS_NAME_DOMAIN_PURE_FLUID:
		return "pure fluid";
	case FPROPS_NAME_DOMAIN_EQM_SPECIES:
		return "equilibrium species";
	case FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT:
		return "mixture component";
	default:
		return "FPROPS name";
	}
}

static int asc_resolve_name_or_error(const char *token, unsigned domains,
		const char *source, const char *context, const char **canonical_out){
	FpropsResolvedName resolved;
	FpropsNameResolveStatus status;
	const char *label = asc_name_domain_label(domains);

	if(!canonical_out){
		return 1;
	}
	*canonical_out = NULL;
	status = fprops_name_resolve(token, domains, source, &resolved);
	if(status == FPROPS_NAME_RESOLVE_OK && resolved.canonical
			&& resolved.canonical->canonical && resolved.canonical->canonical[0]){
		*canonical_out = resolved.canonical->canonical;
		return 0;
	}
	switch(status){
	case FPROPS_NAME_RESOLVE_NOT_FOUND:
		if(source && source[0]){
			ERRMSG("%s '%s' is not a registered %s for source '%s'%s%s",
				label, token ? token : "(null)", label, source,
				context ? " in " : "", context ? context : "");
		}else{
			ERRMSG("%s '%s' is not a registered %s%s%s",
				label, token ? token : "(null)", label,
				context ? " in " : "", context ? context : "");
		}
		break;
	case FPROPS_NAME_RESOLVE_AMBIGUOUS:
		if(source && source[0]){
			ERRMSG("%s '%s' is ambiguous for source '%s'%s%s",
				label, token ? token : "(null)", source,
				context ? " in " : "", context ? context : "");
		}else{
			ERRMSG("%s '%s' is ambiguous; specify a source%s%s",
				label, token ? token : "(null)",
				context ? " in " : "", context ? context : "");
		}
		break;
	case FPROPS_NAME_RESOLVE_INVALID:
		ERRMSG("Invalid %s '%s'%s%s",
			label, token ? token : "(null)",
			context ? " in " : "", context ? context : "");
		break;
	default:
		ERRMSG("Unable to resolve %s '%s'%s%s",
			label, token ? token : "(null)",
			context ? " in " : "", context ? context : "");
	}
	return 1;
}

static int asc_check_unique_canonical_names(const char **tokens, const char **canonicals,
		unsigned long n, const char *context){
	unsigned long i, j;
	if(!canonicals){
		return 1;
	}
	for(i = 0; i < n; ++i){
		if(!canonicals[i] || !canonicals[i][0]){
			ERRMSG("Empty canonical name at position %lu%s%s", i + 1,
				context ? " in " : "", context ? context : "");
			return 1;
		}
		for(j = i + 1; j < n; ++j){
			if(canonicals[j] && 0 == strcmp(canonicals[i], canonicals[j])){
				ERRMSG("%s '%s' resolves to canonical '%s', which duplicates %s '%s'%s%s",
					tokens && tokens[j] ? "Name" : "Entry",
					tokens && tokens[j] ? tokens[j] : "(unknown)",
					canonicals[j],
					tokens && tokens[i] ? "name" : "entry",
					tokens && tokens[i] ? tokens[i] : "(unknown)",
					context ? " in " : "", context ? context : "");
				return 1;
			}
		}
	}
	return 0;
}
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
		, fprops_rxn_eqm_TPn_deriv
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
	result += CreateUserFunctionBlackBox("fprops_unifac_liq_fugacity_TPx"
		, asc_fprops_unifac_liq_fugacity_prepare
		, fprops_unifac_liq_fugacity_TPx_calc
		, (ExtBBoxFunc*)NULL
		, (ExtBBoxFunc*)NULL
		, asc_fprops_unifac_flash_final
		, 3,1
		, fprops_unifac_liq_fugacity_TPx_help
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
	const char *comp, *resolved_comp = NULL, *type = NULL, *src = NULL;

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

	if(asc_resolve_name_or_error(comp, FPROPS_NAME_DOMAIN_PURE_FLUID, src,
			"FPROPS DATA", &resolved_comp)){
		return 1;
	}

	bbox->user_data = (void *)fprops_fluid(resolved_comp,type,src);
	if(bbox->user_data == NULL){
		ERRMSG("Unsupported component requested (name='%s', canonical='%s', type='%s', source='%s'). Check generated FPROPS data.",
			comp, resolved_comp, type ? type : "", src ? src : "");
		return 1;
	}

	MSG("Prepared component '%s' as '%s'%s%s%s OK.",comp, resolved_comp,
		type?" type '":"", type?type:"" ,type?"'":""
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
	/* Reactive-package source selectors are resolved in the C-side FPROPS layer. */
	struct Instance *srcinst, *alginst, *components_inst, *species_name_inst;
	const char *source = NULL;
	const char *algorithm = NULL;
	const char **names = NULL;
	const char **resolved_names = NULL;
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
	resolved_names = (const char **)calloc((size_t)ns, sizeof(char *));
	rxn = (AscFpropsRxnData *)calloc(1, sizeof(AscFpropsRxnData));
	if(!names || !resolved_names || !rxn){
		ERRMSG("Unable to allocate reactive FPROPS blackbox workspace");
		free(names);
		free(resolved_names);
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
					free(resolved_names);
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
				free(resolved_names);
				free(rxn);
				return 1;
			}
		}

	srcinst = ChildByChar(data, source_sym);
	if(srcinst){
		if(InstanceKind(srcinst) != SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'source' must be a symbol_constant");
			free(names);
			free(resolved_names);
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
			free(resolved_names);
			free(rxn);
			return 1;
		}
			algorithm = SCP(SYMC_INST(alginst)->value);
			if(algorithm && strlen(algorithm) == 0)algorithm = NULL;
		}

	for(c = 0; c < ns; ++c){
		FpropsResolvedName resolved;
		FpropsNameResolveStatus status = fprops_name_resolve(names[c],
			FPROPS_NAME_DOMAIN_PURE_FLUID | FPROPS_NAME_DOMAIN_EQM_SPECIES,
			source, &resolved);
		resolved_names[c] = (status == FPROPS_NAME_RESOLVE_OK && resolved.canonical
			&& resolved.canonical->canonical && resolved.canonical->canonical[0])
			? resolved.canonical->canonical : names[c];
	}
	if(asc_check_unique_canonical_names(names, resolved_names, ns, "reactive package DATA")){
		free(names);
		free(resolved_names);
		free(rxn);
		return 1;
	}

	rxn->pkg = fprops_rxn_package_build(names, (int)ns, source);
	if(!rxn->pkg){
		ERRMSG("Failed to build reactive FPROPS package from DATA");
		free(names);
		free(resolved_names);
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
	rxn->names = ASC_NEW_ARRAY(char *, ns);
	if(!rxn->names){
		fprops_rxn_package_free(rxn->pkg);
		ascfree(rxn->algorithm);
		free(rxn);
		ERRMSG("Unable to allocate reactive FPROPS species-name cache");
		return 1;
	}
	for(c = 0; c < ns; ++c){
		rxn->names[c] = ASC_NEW_ARRAY(char, strlen(names[c]) + 1);
		if(!rxn->names[c]){
			while(c > 0){
				--c;
				ascfree(rxn->names[c]);
			}
			ascfree(rxn->names);
			fprops_rxn_package_free(rxn->pkg);
			ascfree(rxn->algorithm);
			free(rxn);
			ERRMSG("Unable to copy reactive FPROPS species name");
			return 1;
		}
		strcpy(rxn->names[c], names[c]);
	}
	if(source){
		rxn->source = ASC_NEW_ARRAY(char, strlen(source) + 1);
		if(!rxn->source){
			for(c = 0; c < ns; ++c){
				ascfree(rxn->names[c]);
			}
			ascfree(rxn->names);
			fprops_rxn_package_free(rxn->pkg);
			ascfree(rxn->algorithm);
			free(rxn);
			ERRMSG("Unable to copy reactive FPROPS source string");
			return 1;
		}
		strcpy(rxn->source, source);
	}
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
	rxn->last_n = ASC_NEW_ARRAY(double, ns);
	if(!rxn->last_n){
		for(c = 0; c < ns; ++c){
			ascfree(rxn->names[c]);
		}
		ascfree(rxn->names);
		ascfree(rxn->source);
		fprops_rxn_package_free(rxn->pkg);
		ascfree(rxn->algorithm);
		free(rxn);
		ERRMSG("Unable to allocate reactive FPROPS cached seed vector");
		return 1;
	}
	rxn->have_last_n = 0;
#endif
	free(names);
	free(resolved_names);
	bbox->user_data = (void *)rxn;
	asc_fprops_rxn_state_trace("prepare", bbox, rxn, NAN, NAN, NULL, NULL, 0);
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
	asc_fprops_rxn_state_trace("prepare_eqm", bbox, rxn, NAN, NAN, NULL, NULL, 0);
	return 0;
}

void asc_fprops_rxn_final(struct BBoxInterp *bbox){
	AscFpropsRxnData *rxn;
	if(!bbox || !bbox->user_data){
		return;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	asc_fprops_rxn_state_trace("final", bbox, rxn, NAN, NAN, NULL, NULL, 0);
	if(rxn->pkg){
		fprops_rxn_package_free(rxn->pkg);
	}
	if(rxn->names){
		for(int i = 0; i < rxn->ns; ++i){
			ascfree(rxn->names[i]);
		}
		ascfree(rxn->names);
	}
	ascfree(rxn->source);
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
	ascfree(rxn->last_n);
#endif
	ascfree(rxn->algorithm);
	free(rxn);
	bbox->user_data = NULL;
}

#if defined(__GNUC__)
__attribute__((visibility("default")))
#endif
int asc_fprops_rxn_eqm_debug_fresh_compare(const void *user_data, double T, double P,
		const double *n_in, double *n_out){
	const AscFpropsRxnData *rxn = (const AscFpropsRxnData *)user_data;
	FpropsRxnPackage *pkg = NULL;
	FpropsRxnTPN state;
	FpropsRxnResult out;
	const char *algorithm;
	const char *source;
	int status;

	if(!rxn || !rxn->names || !n_in || !n_out || rxn->ns <= 0){
		return -11;
	}
	source = (rxn->source && rxn->source[0]) ? rxn->source : NULL;
	algorithm = (rxn->algorithm && rxn->algorithm[0]) ? rxn->algorithm : "auto_reduced";
	pkg = fprops_rxn_package_build((const char **)rxn->names, rxn->ns, source);
	if(!pkg){
		return -12;
	}
	state.T = T;
	state.P = P;
	state.n = n_in;
	out.status = -99;
	out.H = NAN;
	out.G = NAN;
	out.n_out = n_out;
	status = fprops_rxn_eqm_tpy(pkg, &state, algorithm, NULL, &out);
	fprops_rxn_package_free(pkg);
	return status;
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

static int asc_build_unifac_flash_package_native(struct Instance *cd, AscFpropsUNIFACFlashData **outpkg){
	struct Instance *components_inst;
	const struct set_t *components_set;
	const FpropsUNIFACSourceData *src;
	AscFpropsUNIFACFlashData *fp = NULL;
	const char **names = NULL;
	const char **tokens = NULL;
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
	tokens = ASC_NEW_ARRAY(const char *, nc_ul);
	if(!names || !tokens){
		ERRMSG("Unable to allocate native UNIFAC component-name list");
		ascfree(names);
		ascfree(tokens);
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
		const char *comp_name = SCP(comp_sym);
		tokens[i_ul - 1] = comp_name;
		if(asc_resolve_name_or_error(comp_name, FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT,
				"UNIFAC-orig-2003", "UNIFAC flash DATA", &comp_name)){
			asc_unifac_flash_free_data(fp);
			ascfree(names);
			ascfree(tokens);
			return 1;
		}
		names[i_ul - 1] = comp_name;
	}
	if(asc_check_unique_canonical_names(tokens, names, nc_ul, "UNIFAC flash DATA")){
		asc_unifac_flash_free_data(fp);
		ascfree(names);
		ascfree(tokens);
		return 1;
	}

	if(fprops_flash_prepare_unifac(&fp->mpkg, "UNIFAC-orig-2003", names, (int)nc_ul)){
		asc_unifac_flash_free_data(fp);
		ascfree(names);
		ascfree(tokens);
		ERRMSG("Unable to prepare native UNIFAC flash package");
		return 1;
	}
	fp->nc = fp->mpkg.nc;
	fp->nsub = fp->mpkg.data.unifac_ideal_vl.pkg ? fp->mpkg.data.unifac_ideal_vl.pkg->nsub : 0;
	if(fp->mpkg.data.unifac_ideal_vl.pkg){
		fp->pkg = *fp->mpkg.data.unifac_ideal_vl.pkg;
	}
	*outpkg = fp;
	ascfree(names);
	ascfree(tokens);
	return 0;
}

static int asc_build_unifac_flash_package(struct Instance *cd, AscFpropsUNIFACFlashData **outpkg){
	return asc_build_unifac_flash_package_native(cd, outpkg);
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

int asc_fprops_unifac_liq_fugacity_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	AscFpropsUNIFACFlashData *fp = NULL;
	unsigned long actual_inputs, actual_outputs;
	struct Instance *components_inst;
	const struct set_t *components_set;
	unsigned long nc;

	if(!bbox || !data || !arglist){
		ERRMSG("UNIFAC liquid fugacity blackbox received invalid prepare arguments");
		return 1;
	}
	if(gl_length(arglist) != 4){
		ERRMSG("UNIFAC liquid fugacity blackbox expects 3 INPUT groups and 1 OUTPUT group");
		return 1;
	}
	actual_inputs = CountNumberOfArgs(arglist, 1, 3);
	actual_outputs = CountNumberOfArgs(arglist, 4, 4);

	components_inst = ChildByChar(data, AddSymbol("components"));
	components_set = components_inst ? SetAtomList(components_inst) : NULL;
	if(!components_set){
		ERRMSG("UNIFAC liquid fugacity DATA must provide a components set");
		return 1;
	}
	nc = Cardinality(components_set);
	if(actual_inputs != nc + 2){
		ERRMSG("UNIFAC liquid fugacity input vector length mismatch: got %lu composition inputs, expected %lu",
			actual_inputs - 2, nc);
		return 1;
	}
	if(actual_outputs != nc){
		ERRMSG("UNIFAC liquid fugacity output vector length mismatch: got %lu outputs, expected %lu",
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

static int asc_fprops_rxn_eqm_eval_core(struct BBoxInterp *bbox, AscFpropsRxnData *rxn,
		int ninputs, int noutputs, double *inputs, double *outputs, int trace_state){
	FpropsRxnTPN state;
	FpropsRxnResult out;
	double *n_guess = NULL;
	const double *n_init = NULL;
	int status;
	int do_trace = 0;
	int i;
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
#endif

	if(!bbox || !rxn || !rxn->pkg){
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
	do_trace = asc_fprops_rxn_eqm_trace_enabled() && state.T <= 800.0;
	if(trace_state){
		asc_fprops_rxn_state_trace("eval_enter", bbox, rxn, state.T, state.P, state.n, NULL, 0);
	}
	n_guess = ASC_NEW_ARRAY(double, (size_t)rxn->ns);
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
	if(n_guess && rxn->have_last_n && rxn->last_n){
		for(i = 0; i < rxn->ns; ++i){
			double ni = rxn->last_n[i];
			n_guess[i] = (isfinite(ni) && ni > 1e-30) ? ni : 1e-30;
		}
		n_init = n_guess;
	}
#endif
	if(n_init == NULL && n_guess && bbox && bbox->task == bb_func_eval){
		int ok_init = 1;
		for(i = 0; i < rxn->ns; ++i){
			if(!isfinite(outputs[i]) || outputs[i] < 0.0){
				ok_init = 0;
				break;
			}
			n_guess[i] = outputs[i] > 1e-30 ? outputs[i] : 1e-30;
		}
		if(ok_init){
			n_init = n_guess;
		}
	}
	status = fprops_rxn_eqm_tpy(rxn->pkg, &state,
		rxn->algorithm ? rxn->algorithm : "auto_reduced",
		n_init, &out);
	if(!asc_fprops_rxn_eqm_status_ok(status) && n_init != NULL){
		status = fprops_rxn_eqm_tpy(rxn->pkg, &state,
			rxn->algorithm ? rxn->algorithm : "auto_reduced",
			NULL, &out);
	}
	if(do_trace || (asc_fprops_rxn_eqm_trace_enabled() && !asc_fprops_rxn_eqm_status_ok(status))){
		asc_fprops_rxn_eqm_trace_report(rxn, state.T, state.P, state.n, status,
			outputs, bbox ? (int)bbox->task : -1);
	}
	if(trace_state){
		asc_fprops_rxn_state_trace("eval_exit", bbox, rxn, state.T, state.P, state.n, outputs, status);
	}
#ifdef ASC_FPROPS_RXN_EQM_REUSE_SEEDS
	if(asc_fprops_rxn_eqm_status_ok(status) && rxn->last_n){
		for(i = 0; i < rxn->ns; ++i){
			rxn->last_n[i] = (isfinite(outputs[i]) && outputs[i] > 0.0) ? outputs[i] : 1e-30;
		}
		rxn->have_last_n = 1;
	}
#endif
	ASC_FREE(n_guess);
	return status;
}

static int asc_fprops_rxn_eqm_fd_jacobian(struct BBoxInterp *bbox, AscFpropsRxnData *rxn,
		int ninputs, int noutputs, const double *inputs, const double *outputs, double *jacobian){
	double *inputs_work = NULL;
	double *outputs_work = NULL;
	int j;

	if(!bbox || !rxn || !inputs || !outputs || !jacobian){
		return -11;
	}
	inputs_work = ASC_NEW_ARRAY(double, (size_t)ninputs);
	outputs_work = ASC_NEW_ARRAY(double, (size_t)noutputs);
	if(!inputs_work || !outputs_work){
		ASC_FREE(inputs_work);
		ASC_FREE(outputs_work);
		return -12;
	}
	for(j = 0; j < ninputs; ++j){
		double x = inputs[j];
		double step = 1e-7 * fmax(fabs(x), 1.0);
		int status;
		if(j == 0 || j == 1){
			if(x + step <= 0.0){
				step = fmax(1e-7, 0.5 * fmax(x, 1e-7));
			}
		}
		memcpy(inputs_work, inputs, sizeof(double) * (size_t)ninputs);
		inputs_work[j] = x + step;
		if((j == 0 || j == 1) && !(inputs_work[j] > 0.0)){
			inputs_work[j] = fmax(1e-7, x + fabs(step));
		}
		status = asc_fprops_rxn_eqm_eval_core(bbox, rxn, ninputs, noutputs,
			inputs_work, outputs_work, 0);
		if(!asc_fprops_rxn_eqm_status_ok(status)){
			ASC_FREE(inputs_work);
			ASC_FREE(outputs_work);
			return status;
		}
		for(int i = 0; i < noutputs; ++i){
			jacobian[i * ninputs + j] = (outputs_work[i] - outputs[i]) / (inputs_work[j] - x);
		}
	}
	ASC_FREE(inputs_work);
	ASC_FREE(outputs_work);
	return 0;
}

int fprops_rxn_eqm_TPn_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsRxnData *rxn;
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
	status = asc_fprops_rxn_eqm_eval_core(bbox, rxn, ninputs, noutputs, inputs, outputs, 1);
	if(!asc_fprops_rxn_eqm_status_ok(status)){
		ERRMSG("Reactive FPROPS equilibrium evaluation failed with status %d", status);
		return status;
	}
	return 0;
}

int fprops_rxn_eqm_TPn_deriv(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	AscFpropsRxnData *rxn;
	FpropsRxnTPN state;
	double *dn_dT = NULL;
	double *dn_dP = NULL;
	double *dn_db = NULL;
	const double *A = NULL;
	int ne = 0;
	int status;

	if(!bbox || !bbox->user_data){
		return -5;
	}
	rxn = (AscFpropsRxnData *)bbox->user_data;
	if(!rxn || !rxn->pkg){
		return -6;
	}
	if(!jacobian){
		return -3;
	}
	status = asc_fprops_rxn_eqm_eval_core(bbox, rxn, ninputs, noutputs, inputs, outputs, 0);
	if(!asc_fprops_rxn_eqm_status_ok(status)){
		return status;
	}

	ne = fprops_rxn_package_num_elements(rxn->pkg);
	A = fprops_rxn_package_element_matrix(rxn->pkg);
	if(ne <= 0 || !A){
		return asc_fprops_rxn_eqm_fd_jacobian(bbox, rxn, ninputs, noutputs, inputs, outputs, jacobian);
	}

	dn_dT = ASC_NEW_ARRAY(double, (size_t)rxn->ns);
	dn_dP = ASC_NEW_ARRAY(double, (size_t)rxn->ns);
	dn_db = ASC_NEW_ARRAY(double, (size_t)(rxn->ns * ne));
	if(!dn_dT || !dn_dP || !dn_db){
		ASC_FREE(dn_dT);
		ASC_FREE(dn_dP);
		ASC_FREE(dn_db);
		return -12;
	}

	state.T = inputs[0];
	state.P = inputs[1];
	state.n = &inputs[2];
	status = fprops_rxn_eqm_sensitivities(rxn->pkg, &state, outputs, dn_dT, dn_dP, dn_db);
	if(status == 0){
		for(int i = 0; i < rxn->ns; ++i){
			jacobian[i * ninputs + 0] = dn_dT[i];
			jacobian[i * ninputs + 1] = dn_dP[i];
			for(int j = 0; j < rxn->ns; ++j){
				double s = 0.0;
				for(int e = 0; e < ne; ++e){
					s += dn_db[i * ne + e] * A[e * rxn->ns + j];
				}
				jacobian[i * ninputs + (2 + j)] = s;
			}
		}
	}else{
		status = asc_fprops_rxn_eqm_fd_jacobian(bbox, rxn, ninputs, noutputs, inputs, outputs, jacobian);
	}

	ASC_FREE(dn_dT);
	ASC_FREE(dn_dP);
	ASC_FREE(dn_db);
	return status;
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

int fprops_unifac_liq_fugacity_TPx_calc(struct BBoxInterp *bbox,
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
	if(ninputs != fp->nc + 2){
		ERRMSG("UNIFAC liquid fugacity blackbox received %d inputs, expected %d", ninputs, fp->nc + 2);
		return -1;
	}
	if(noutputs != fp->nc){
		ERRMSG("UNIFAC liquid fugacity blackbox received %d outputs, expected %d", noutputs, fp->nc);
		return -2;
	}
	if(!inputs || !outputs){
		return -3;
	}

	status = fprops_unifac_liq_fugacity(&fp->pkg, inputs[0], inputs[1], &inputs[2], outputs);
	if(status){
		ERRMSG("UNIFAC liquid fugacity evaluation failed with status %d", status);
		return status;
	}
	return 0;
}
