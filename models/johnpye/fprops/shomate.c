#include "shomate.h"

#include <math.h>

static int shomate_exp_matches(double a, double b){
	return fabs(a - b) <= 1e-12 ? 1 : 0;
}

static unsigned char shomate_term_kind(double exponent){
	if(shomate_exp_matches(exponent, 0.0)) return SHOMATE_TERM_CONST;
	if(shomate_exp_matches(exponent, 1.0)) return SHOMATE_TERM_T;
	if(shomate_exp_matches(exponent, 2.0)) return SHOMATE_TERM_T2;
	if(shomate_exp_matches(exponent, 3.0)) return SHOMATE_TERM_T3;
	if(shomate_exp_matches(exponent, -0.5)) return SHOMATE_TERM_INV_SQRT_T;
	if(shomate_exp_matches(exponent, -1.0)) return SHOMATE_TERM_INV_T;
	if(shomate_exp_matches(exponent, -2.0)) return SHOMATE_TERM_INV_T2;
	return SHOMATE_TERM_POW;
}

int shomate_range_contains(const ShomateRange *R, double T){
	if(!R || !(T > 0.0)){
		return 0;
	}
	return (T >= R->T_min && T <= R->T_max) ? 1 : 0;
}

int shomate_prepare_range(ShomateRange *R){
	unsigned i;
	if(!R){
		return 0;
	}
	if(R->prepared){
		return 1;
	}
	if(R->n_terms == 0 || !R->terms){
		return 0;
	}
	for(i = 0; i < R->n_terms; ++i){
		if(R->terms[i].kind == SHOMATE_TERM_UNSET){
			R->terms[i].kind = shomate_term_kind(R->terms[i].exponent);
		}
	}
	R->prepared = 1;
	return 1;
}

double shomate_cp_molar(const ShomateRange *R, double T, FpropsError *err){
	unsigned i;
	double cp = 0.0;
	double invT;
	double invT2;
	double sqrtT;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!R || !(T > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	if(!shomate_prepare_range((ShomateRange *)R)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	invT = 1.0 / T;
	invT2 = invT * invT;
	sqrtT = sqrt(T);
	for(i = 0; i < R->n_terms; ++i){
		const ShomateTerm *term = &R->terms[i];
		switch(term->kind){
		case SHOMATE_TERM_CONST:
			cp += term->coeff;
			break;
		case SHOMATE_TERM_T:
			cp += term->coeff * T;
			break;
		case SHOMATE_TERM_T2:
			cp += term->coeff * T * T;
			break;
		case SHOMATE_TERM_T3:
			cp += term->coeff * T * T * T;
			break;
		case SHOMATE_TERM_INV_SQRT_T:
			cp += term->coeff / sqrtT;
			break;
		case SHOMATE_TERM_INV_T:
			cp += term->coeff * invT;
			break;
		case SHOMATE_TERM_INV_T2:
			cp += term->coeff * invT2;
			break;
		case SHOMATE_TERM_LOG_T:
			cp += term->coeff * log(T);
			break;
		default:
			cp += term->coeff * pow(T, term->exponent);
			break;
		}
	}
	return cp;
}

static double shomate_term_delta_h(const ShomateTerm *term, double T0, double T1){
	double c = term->coeff;
	switch(term->kind){
	case SHOMATE_TERM_CONST:
		return c * (T1 - T0);
	case SHOMATE_TERM_T:
		return 0.5 * c * (T1 * T1 - T0 * T0);
	case SHOMATE_TERM_T2:
		return (c / 3.0) * (T1 * T1 * T1 - T0 * T0 * T0);
	case SHOMATE_TERM_T3:
		return 0.25 * c * (T1 * T1 * T1 * T1 - T0 * T0 * T0 * T0);
	case SHOMATE_TERM_INV_SQRT_T:
		return 2.0 * c * (sqrt(T1) - sqrt(T0));
	case SHOMATE_TERM_INV_T:
		return c * log(T1 / T0);
	case SHOMATE_TERM_INV_T2:
		return -c * (1.0 / T1 - 1.0 / T0);
	case SHOMATE_TERM_LOG_T:
		return c * ((T1 * log(T1) - T1) - (T0 * log(T0) - T0));
	default:
		{
			double n = term->exponent;
			if(shomate_exp_matches(n + 1.0, 0.0)){
				return c * log(T1 / T0);
			}
			return (c / (n + 1.0)) * (pow(T1, n + 1.0) - pow(T0, n + 1.0));
		}
	}
}

static double shomate_term_delta_s(const ShomateTerm *term, double T0, double T1){
	double c = term->coeff;
	switch(term->kind){
	case SHOMATE_TERM_CONST:
		return c * log(T1 / T0);
	case SHOMATE_TERM_T:
		return c * (T1 - T0);
	case SHOMATE_TERM_T2:
		return 0.5 * c * (T1 * T1 - T0 * T0);
	case SHOMATE_TERM_T3:
		return (c / 3.0) * (T1 * T1 * T1 - T0 * T0 * T0);
	case SHOMATE_TERM_INV_SQRT_T:
		return -2.0 * c * (1.0 / sqrt(T1) - 1.0 / sqrt(T0));
	case SHOMATE_TERM_INV_T:
		return -c * (1.0 / T1 - 1.0 / T0);
	case SHOMATE_TERM_INV_T2:
		return -0.5 * c * (1.0 / (T1 * T1) - 1.0 / (T0 * T0));
	case SHOMATE_TERM_LOG_T:
		{
			double l0 = log(T0);
			double l1 = log(T1);
			return 0.5 * c * (l1 * l1 - l0 * l0);
		}
	default:
		{
			double n = term->exponent;
			if(shomate_exp_matches(n, 0.0)){
				return c * log(T1 / T0);
			}
			return (c / n) * (pow(T1, n) - pow(T0, n));
		}
	}
}

double shomate_delta_h_molar(const ShomateRange *R, double T0, double T1, FpropsError *err){
	unsigned i;
	double dh = 0.0;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!R || !(T0 > 0.0) || !(T1 > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	if(!shomate_prepare_range((ShomateRange *)R)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	for(i = 0; i < R->n_terms; ++i){
		dh += shomate_term_delta_h(&R->terms[i], T0, T1);
	}
	return dh;
}

double shomate_delta_s_molar(const ShomateRange *R, double T0, double T1, FpropsError *err){
	unsigned i;
	double ds = 0.0;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!R || !(T0 > 0.0) || !(T1 > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	if(!shomate_prepare_range((ShomateRange *)R)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	for(i = 0; i < R->n_terms; ++i){
		ds += shomate_term_delta_s(&R->terms[i], T0, T1);
	}
	return ds;
}
