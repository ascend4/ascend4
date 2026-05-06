#include "flash_unifac.h"
#include "common.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define UNIFAC_MAX_GROUP 47
#define UNIFAC_EPS 1e-30

static double clamp_positive(double x){
	return x > UNIFAC_EPS ? x : UNIFAC_EPS;
}

static void normalize_vector(int n, double *x){
	double sum = 0.0;
	int i;
	for(i = 0; i < n; ++i){
		if(x[i] < 0.0){
			x[i] = 0.0;
		}
		sum += x[i];
	}
	if(sum <= 0.0){
		double v = 1.0 / (double)n;
		for(i = 0; i < n; ++i){
			x[i] = v;
		}
		return;
	}
	for(i = 0; i < n; ++i){
		x[i] /= sum;
	}
}

static double fprops_unifac_pure_g_v(const FpropsUNIFACComponentData *comp, double T, double P){
	return comp->G0
		- (comp->H0 - comp->G0) * (T / comp->T0 - 1.0)
		- comp->cpvapa * (T * log(T / comp->T0) - T + comp->T0)
		- comp->cpvapb * (T * T - 2.0 * T * comp->T0 + comp->T0 * comp->T0) / 2.0
		- comp->cpvapc * (T * T * T / 2.0 - 1.5 * T * comp->T0 * comp->T0 + comp->T0 * comp->T0 * comp->T0) / 3.0
		- comp->cpvapd * (T * T * T * T / 3.0 - 4.0 * T * comp->T0 * comp->T0 * comp->T0 / 3.0
			+ comp->T0 * comp->T0 * comp->T0 * comp->T0) / 4.0
		+ FPROPS_R * T * log(P / comp->P0);
}

static double fprops_unifac_liq_volume(const FpropsUNIFACComponentData *comp, double T){
	double num = pow(comp->Zc, pow(fabs(1.0 - T / comp->Tc), 2.0 / 7.0));
	double den = pow(comp->Zc, pow(fabs(1.0 - comp->Tliq / comp->Tc), 2.0 / 7.0));
	return comp->Vliq * num / den;
}

static double fprops_unifac_pure_g_l(const FpropsUNIFACComponentData *comp, double T, double P, double VP){
	return comp->G0
		- (T / comp->T0 - 1.0) * (comp->H0 - comp->G0)
		- (T * log(T / comp->T0) - T + comp->T0) * comp->cpvapa
		- (T * T - 2.0 * T * comp->T0 + comp->T0 * comp->T0) * comp->cpvapb / 2.0
		- (T * T * T / 2.0 - 1.5 * T * comp->T0 * comp->T0 + comp->T0 * comp->T0 * comp->T0) * comp->cpvapc / 3.0
		- (T * T * T * T / 3.0 - 4.0 * T * comp->T0 * comp->T0 * comp->T0 / 3.0
			+ comp->T0 * comp->T0 * comp->T0 * comp->T0) * comp->cpvapd / 4.0
		+ FPROPS_R * T * log(VP / comp->P0)
		+ FPROPS_R * comp->Tc * (VP / comp->Pc)
			* (0.083 - 0.422 * pow(comp->Tc / T, 1.6) + comp->omega * (0.139 - 0.172 * pow(comp->Tc / T, 4.2)))
		+ (P - VP) * fprops_unifac_liq_volume(comp, T);
}

static double solve_rr_bisection(int n, const double *z, const double *K){
	double lo = 0.0, hi = 1.0, mid = 0.5;
	int iter, i;
	for(iter = 0; iter < 100; ++iter){
		double f = 0.0;
		mid = 0.5 * (lo + hi);
		for(i = 0; i < n; ++i){
			double denom = 1.0 + mid * (K[i] - 1.0);
			f += z[i] * (K[i] - 1.0) / denom;
		}
		if(f > 0.0){
			lo = mid;
		}else{
			hi = mid;
		}
		if(fabs(hi - lo) < 1e-12){
			break;
		}
	}
	return mid;
}

int fprops_unifac_psat(const FpropsUNIFACComponentData *comp, double T, double *Psat){
	double lnPsat;
	if(!comp || !Psat || T <= 0.0){
		return -1;
	}
	switch(comp->vp_correlation){
	case 1:
		if(T >= comp->Tc){
			return -2;
		}
		{
			double tau = fabs(1.0 - T / comp->Tc);
			double poly = comp->vpa * tau
				+ comp->vpb * pow(tau, 1.5)
				+ comp->vpc * pow(tau, 3.0)
				+ comp->vpd * pow(tau, 6.0);
			lnPsat = log(comp->Pc) + poly * comp->Tc / T;
		}
		break;
	case 3:
		lnPsat = log(1e5) + comp->vpa - comp->vpb / (comp->vpc + T);
		break;
	default:
		return -3;
	}
	*Psat = exp(lnPsat);
	return isfinite(*Psat) ? 0 : -4;
}

int fprops_unifac_gamma(const FpropsUNIFACFlashPackage *pkg, double T, const double *x, double *gamma){
	double *theta = NULL;
	double *eta = NULL;
	double *theta_group = NULL;
	double *group_i = NULL;
	double rv_mix = 0.0;
	double qs_mix = 0.0;
	int i, k, g;

	if(!pkg || !x || !gamma || pkg->nc <= 0 || pkg->nsub <= 0 || T <= 0.0){
		return -1;
	}

	theta = (double *)calloc((size_t)pkg->nsub, sizeof(double));
	eta = (double *)calloc((size_t)pkg->nsub, sizeof(double));
	theta_group = (double *)calloc((size_t)(UNIFAC_MAX_GROUP + 1), sizeof(double));
	group_i = (double *)calloc((size_t)(UNIFAC_MAX_GROUP + 1), sizeof(double));
	if(!theta || !eta || !theta_group || !group_i){
		free(theta);
		free(eta);
		free(theta_group);
		free(group_i);
		return -2;
	}

	for(i = 0; i < pkg->nc; ++i){
		rv_mix += x[i] * pkg->components[i].r;
		qs_mix += x[i] * pkg->components[i].q;
		for(k = 0; k < pkg->components[i].nsub; ++k){
			int si = pkg->components[i].sub_index[k];
			double contrib = pkg->subgroups[si].Q * pkg->components[i].nu[k] * x[i];
			theta[si] += contrib;
		}
	}

	if(rv_mix <= 0.0 || qs_mix <= 0.0){
		free(theta);
		free(eta);
		free(theta_group);
		free(group_i);
		return -3;
	}

	for(k = 0; k < pkg->nsub; ++k){
		theta_group[pkg->subgroups[k].group] += theta[k];
	}

	for(k = 0; k < pkg->nsub; ++k){
		int gk = pkg->subgroups[k].group;
		eta[k] = theta_group[gk];
		for(g = 1; g <= UNIFAC_MAX_GROUP; ++g){
			if(g == gk || theta_group[g] == 0.0){
				continue;
			}
			eta[k] += theta_group[g] * exp(-pkg->a[(g - 1) * UNIFAC_MAX_GROUP + (gk - 1)] / T);
		}
		eta[k] = clamp_positive(eta[k]);
	}

	for(i = 0; i < pkg->nc; ++i){
		const FpropsUNIFACComponentData *ci = &pkg->components[i];
		double J = clamp_positive(ci->r / rv_mix);
		double L = clamp_positive(ci->q / qs_mix);
		double lngamma = 1.0 - J + log(J)
			- 5.0 * ci->q * (1.0 - J / L + log(J / L))
			+ ci->q * (1.0 - log(L));

		for(g = 1; g <= UNIFAC_MAX_GROUP; ++g){
			group_i[g] = 0.0;
		}
		for(k = 0; k < ci->nsub; ++k){
			int si = ci->sub_index[k];
			group_i[pkg->subgroups[si].group] += ci->nu[k] * pkg->subgroups[si].Q;
		}

		for(k = 0; k < pkg->nsub; ++k){
			int gk = pkg->subgroups[k].group;
			double Aik = group_i[gk];
			for(g = 1; g <= UNIFAC_MAX_GROUP; ++g){
				if(g == gk || group_i[g] == 0.0){
					continue;
				}
				Aik += group_i[g] * exp(-pkg->a[(g - 1) * UNIFAC_MAX_GROUP + (gk - 1)] / T);
			}
			lngamma -= theta[k] * Aik / eta[k];
		}

		for(k = 0; k < ci->nsub; ++k){
			int sk = ci->sub_index[k];
			int gk = pkg->subgroups[sk].group;
			double Aik = group_i[gk];
			double ratio;
			for(g = 1; g <= UNIFAC_MAX_GROUP; ++g){
				if(group_i[g] == 0.0){
					continue;
				}
				if(g != gk){
					Aik += group_i[g] * exp(-pkg->a[(g - 1) * UNIFAC_MAX_GROUP + (gk - 1)] / T);
				}
			}
			ratio = clamp_positive(Aik / eta[sk]);
			lngamma += ci->nu[k] * pkg->subgroups[sk].Q * log(ratio);
		}

		gamma[i] = exp(lngamma);
		if(!isfinite(gamma[i]) || gamma[i] <= 0.0){
			free(theta);
			free(eta);
			free(theta_group);
			free(group_i);
			return -4;
		}
	}

	free(theta);
	free(eta);
	free(theta_group);
	free(group_i);
	return 0;
}

int fprops_unifac_liq_fugacity(const FpropsUNIFACFlashPackage *pkg, double T, double P, const double *x, double *fugacity){
	double *Psat = NULL;
	double *gamma = NULL;
	int i;

	if(!pkg || !x || !fugacity || pkg->nc <= 0 || T <= 0.0 || P <= 0.0){
		return -1;
	}

	Psat = (double *)calloc((size_t)pkg->nc, sizeof(double));
	gamma = (double *)calloc((size_t)pkg->nc, sizeof(double));
	if(!Psat || !gamma){
		free(Psat);
		free(gamma);
		return -2;
	}

	for(i = 0; i < pkg->nc; ++i){
		int status = fprops_unifac_psat(&pkg->components[i], T, &Psat[i]);
		if(status){
			free(Psat);
			free(gamma);
			return -10 + status;
		}
	}

	if(fprops_unifac_gamma(pkg, T, x, gamma)){
		free(Psat);
		free(gamma);
		return -20;
	}

	for(i = 0; i < pkg->nc; ++i){
		double pureK = exp((fprops_unifac_pure_g_l(&pkg->components[i], T, P, Psat[i])
			- fprops_unifac_pure_g_v(&pkg->components[i], T, P)) / (FPROPS_R * T));
		fugacity[i] = x[i] * gamma[i] * P * pureK;
		if(!isfinite(fugacity[i]) || fugacity[i] < 0.0){
			free(Psat);
			free(gamma);
			return -30;
		}
	}

	free(Psat);
	free(gamma);
	return 0;
}

int fprops_unifac_flash_tpz(const FpropsUNIFACFlashPackage *pkg, const FpropsFlashTPZ *in, FpropsFlashVLResult *out){
	double *Psat = NULL;
	double *K = NULL;
	double *Knext = NULL;
	double *gamma = NULL;
	double *x = NULL;
	double *y = NULL;
	double beta = 0.0;
	int i, iter;

	if(!pkg || !in || !out || !out->x || !out->y || !in->z || pkg->nc <= 0){
		return -1;
	}

	Psat = (double *)calloc((size_t)pkg->nc, sizeof(double));
	K = (double *)calloc((size_t)pkg->nc, sizeof(double));
	Knext = (double *)calloc((size_t)pkg->nc, sizeof(double));
	gamma = (double *)calloc((size_t)pkg->nc, sizeof(double));
	x = (double *)calloc((size_t)pkg->nc, sizeof(double));
	y = (double *)calloc((size_t)pkg->nc, sizeof(double));
	if(!Psat || !K || !Knext || !gamma || !x || !y){
		free(Psat);
		free(K);
		free(Knext);
		free(gamma);
		free(x);
		free(y);
		return -2;
	}

	for(i = 0; i < pkg->nc; ++i){
		int status = fprops_unifac_psat(&pkg->components[i], in->T, &Psat[i]);
		if(status){
			free(Psat);
			free(K);
			free(Knext);
			free(gamma);
			free(x);
			free(y);
			return -10 + status;
		}
		K[i] = clamp_positive(exp((fprops_unifac_pure_g_l(&pkg->components[i], in->T, in->P, Psat[i])
			- fprops_unifac_pure_g_v(&pkg->components[i], in->T, in->P)) / (FPROPS_R * in->T)));
	}

	for(iter = 0; iter < 100; ++iter){
		double f0 = 0.0;
		double f1 = 0.0;
		double max_delta = 0.0;

		for(i = 0; i < pkg->nc; ++i){
			f0 += in->z[i] * (K[i] - 1.0);
			f1 += in->z[i] * (K[i] - 1.0) / K[i];
		}

		if(f0 <= 0.0){
			beta = 0.0;
			for(i = 0; i < pkg->nc; ++i){
				x[i] = in->z[i];
				y[i] = K[i] * x[i];
			}
			normalize_vector(pkg->nc, x);
			normalize_vector(pkg->nc, y);
		}else if(f1 >= 0.0){
			beta = 1.0;
			for(i = 0; i < pkg->nc; ++i){
				y[i] = in->z[i];
				x[i] = y[i] / K[i];
			}
			normalize_vector(pkg->nc, y);
			normalize_vector(pkg->nc, x);
		}else{
			beta = solve_rr_bisection(pkg->nc, in->z, K);
			for(i = 0; i < pkg->nc; ++i){
				x[i] = in->z[i] / (1.0 + beta * (K[i] - 1.0));
				y[i] = K[i] * x[i];
			}
			normalize_vector(pkg->nc, x);
			normalize_vector(pkg->nc, y);
		}

		if(fprops_unifac_gamma(pkg, in->T, x, gamma)){
			free(Psat);
			free(K);
			free(Knext);
			free(gamma);
			free(x);
			free(y);
			return -20;
		}

		for(i = 0; i < pkg->nc; ++i){
			double lnK = log(clamp_positive(K[i]));
			double pureK = exp((fprops_unifac_pure_g_l(&pkg->components[i], in->T, in->P, Psat[i])
				- fprops_unifac_pure_g_v(&pkg->components[i], in->T, in->P)) / (FPROPS_R * in->T));
			double lnKraw = log(clamp_positive(gamma[i] * pureK));
			Knext[i] = exp(0.5 * lnK + 0.5 * lnKraw);
			if(fabs(log(Knext[i]) - lnK) > max_delta){
				max_delta = fabs(log(Knext[i]) - lnK);
			}
		}
		for(i = 0; i < pkg->nc; ++i){
			K[i] = Knext[i];
		}
		if(max_delta < 1e-10){
			break;
		}
	}

	out->beta = beta;
	out->status = 0;
	for(i = 0; i < pkg->nc; ++i){
		out->x[i] = x[i];
		out->y[i] = y[i];
	}

	free(Psat);
	free(K);
	free(Knext);
	free(gamma);
	free(x);
	free(y);
	return 0;
}
