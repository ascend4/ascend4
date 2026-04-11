#include "spinel_fe_degterov.h"

#include <math.h>
#include <string.h>

enum {
	SPINEL_FE_TET_FE2 = 0,
	SPINEL_FE_TET_FE3 = 1,
	SPINEL_FE_OCT_FE2 = 2,
	SPINEL_FE_OCT_FE3 = 3,
	SPINEL_FE_OCT_VA = 4,
	SPINEL_FE_MEMBER_COUNT = 5
};

typedef struct {
	const char *phase_source;
	double dg_ae_affine_a;
	double dg_ae_affine_b;
	int mag_variant;
} SpinelFeParams;

enum {
	SPINEL_MAG_CURRENT = 0,
	SPINEL_MAG_MMC1_TC_BETA_PLUS_EXCESS = 1,
	SPINEL_MAG_MMC1_SELECTIVE_BEST = 2
};

static const SpinelFeParams spinel_params_degterov = {
	"degterov_2001",
	0.0,
	0.0,
	SPINEL_MAG_CURRENT
};

static const SpinelFeParams spinel_params_feoxide_recon = {
	"feoxide_recon_baseline_2026",
	0.0,
	0.0,
	SPINEL_MAG_CURRENT
};

static const SpinelFeParams spinel_params_bg_tuned = {
	"fe_spinel_bg_tuned_2026",
	-16984.331545915302,
	18.769347422265838,
	SPINEL_MAG_CURRENT
};

static const SpinelFeParams spinel_params_mmc1_guess = {
	"fe_spinel_mmc1_guess_2026",
	0.0,
	0.0,
	SPINEL_MAG_MMC1_TC_BETA_PLUS_EXCESS
};

static const SpinelFeParams spinel_params_hidayat_adj1 = {
	"hidayat_adj1",
	0.0,
	0.0,
	SPINEL_MAG_MMC1_SELECTIVE_BEST
};

static double spinel_R(void){
	return 8.31446261815324;
}

static double safe_ylogy(double y){
	if(!(y > 0.0)){
		return 0.0;
	}
	return y * log(y);
}

static double hillert_jarl_A(double p){
	return 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0);
}

static double hillert_jarl_gmag(double T, double Tord, double beta, double p){
	double tau = T / Tord;
	double A = hillert_jarl_A(p);
	double f;
	if(tau <= 1.0){
		double poly = tau * tau * tau / 6.0
			+ pow(tau, 9.0) / 135.0
			+ pow(tau, 15.0) / 600.0;
		f = 1.0 - (((79.0 / (140.0 * p)) / tau)
			+ (474.0 / 497.0) * (1.0 / p - 1.0) * poly) / A;
	}else{
		f = -(pow(tau, -5.0) / 10.0
			+ pow(tau, -15.0) / 315.0
			+ pow(tau, -25.0) / 1500.0) / A;
	}
	return f * spinel_R() * T * log(beta + 1.0);
}

static double spinel_g_ae_base(double T){
	/*
	 * Hidayat 2015 Table 1: the Fe3O4 endmember of spinel was adjusted
	 * slightly relative to Degterov 2001 to reproduce the wustite-spinel
	 * and spinel-Fe2O3 boundaries in the Fe-O system.
	 */
	return -1140237.0
		+ 1015.067 * T
		- 0.008149197 * T * T
		- 174.832 * T * log(T)
		+ 1445276.0 / T;
}

static double spinel_g_ae(const SpinelFeParams *params, double T){
	double g = spinel_g_ae_base(T);
	if(params){
		g += params->dg_ae_affine_a + params->dg_ae_affine_b * T;
	}
	return g;
}

static double spinel_i_ae(double T){
	return -31229.0 + 22.063 * T;
}

static double spinel_v_e(double T){
	return 29932.0 + 28.547 * T;
}

static double spinel_delta_ae(void){
	return 15781.0;
}

static double spinel_delta_eav(void){
	/*
	 * First-pass Fe-only Degterov slice:
	 * use one vacancy parameter for the oxidized side, consistent with the
	 * text discussion that this can be sufficient for (A,E)[A,E,V]2O4.
	 */
	return 0.0;
}

static double spinel_gmag(const SpinelFeParams *params, double T,
		double y_t_fe2, double y_t_fe3, double y_o_fe2, double y_o_fe3,
		double y_o_va){
	int mag_variant = params ? params->mag_variant : SPINEL_MAG_CURRENT;
	if(mag_variant == SPINEL_MAG_MMC1_TC_BETA_PLUS_EXCESS){
		double w_ae = y_t_fe2 * y_o_fe3;
		double w_ea = y_t_fe3 * y_o_fe2;
		double w_125 = y_t_fe2 * y_t_fe3 * y_o_va;
		double w_134 = y_t_fe2 * y_o_fe2 * y_o_fe3;
		double w_145 = y_t_fe2 * y_o_fe3 * y_o_va;
		double w_234 = y_t_fe3 * y_o_fe2 * y_o_fe3;
		double w_235 = y_t_fe3 * y_o_fe2 * y_o_va;
		double tord = 848.0 * w_ae
			+ 424.0 * w_ea
			+ 141.33333 * w_125
			+ 2544.0 * w_134
			+ 848.0 * w_145
			+ 2544.0 * w_234
			- 5088.0 * w_235;
		double beta = 44.54 * w_ae
			+ 22.27 * w_ea
			+ 7.4233333 * w_125
			+ 133.62 * w_134
			+ 44.54 * w_145
			+ 133.62 * w_234
			- 267.24 * w_235;
		if(!(tord > 1e-12) || !(beta > 1e-12)){
			return 0.0;
		}
		return hillert_jarl_gmag(T, tord, beta, 0.28);
	}
	if(mag_variant == SPINEL_MAG_MMC1_SELECTIVE_BEST){
		double s_ea = 1.30;
		double s_red = 0.95;
		double w_ae = y_t_fe2 * y_o_fe3;
		double w_ea = y_t_fe3 * y_o_fe2;
		double w_125 = y_t_fe2 * y_t_fe3 * y_o_va;
		double w_134 = y_t_fe2 * y_o_fe2 * y_o_fe3;
		double w_145 = y_t_fe2 * y_o_fe3 * y_o_va;
		double w_234 = y_t_fe3 * y_o_fe2 * y_o_fe3;
		double w_235 = y_t_fe3 * y_o_fe2 * y_o_va;
		double tord = 848.0 * w_ae
			+ s_ea * 424.0 * w_ea
			+ 141.33333 * w_125
			+ 2544.0 * w_134
			+ 848.0 * w_145
			+ s_red * 2544.0 * w_234
			- s_red * 5088.0 * w_235;
		double beta = 44.54 * w_ae
			+ s_ea * 22.27 * w_ea
			+ 7.4233333 * w_125
			+ 133.62 * w_134
			+ 44.54 * w_145
			+ s_red * 133.62 * w_234
			- s_red * 267.24 * w_235;
		if(!(tord > 1e-12) || !(beta > 1e-12)){
			return 0.0;
		}
		return hillert_jarl_gmag(T, tord, beta, 0.28);
	}
	return hillert_jarl_gmag(T, 848.0, 44.54, 0.28);
}

static int spinel_fe_degterov_g_only_params(const SpinelFeParams *params,
		const double *n_members, double T, double *g_out){
	double nt, no, nphase;
	double y_t_fe2, y_t_fe3, y_o_fe2, y_o_fe3, y_o_va;
	double G_AE, G_EA, G_EE, G_AA, G_EV, G_AV;
	double Gmix, Sconf, Gmag;
	if(!n_members || !g_out || !(T > 0.0)){
		return 0;
	}
	nt = n_members[SPINEL_FE_TET_FE2] + n_members[SPINEL_FE_TET_FE3];
	no = n_members[SPINEL_FE_OCT_FE2] + n_members[SPINEL_FE_OCT_FE3] + n_members[SPINEL_FE_OCT_VA];
	nphase = nt;
	if(!(nt > 0.0) || !(no > 0.0)){
		return 0;
	}

	y_t_fe2 = n_members[SPINEL_FE_TET_FE2] / nt;
	y_t_fe3 = n_members[SPINEL_FE_TET_FE3] / nt;
	y_o_fe2 = n_members[SPINEL_FE_OCT_FE2] / no;
	y_o_fe3 = n_members[SPINEL_FE_OCT_FE3] / no;
	y_o_va = n_members[SPINEL_FE_OCT_VA] / no;

	G_AE = spinel_g_ae(params, T);
	G_EA = G_AE;
	G_EE = G_AE + spinel_i_ae(T);
	G_AA = G_AE - spinel_i_ae(T) + spinel_delta_ae();
	G_EV = 5.0 / 7.0 * G_AE + spinel_v_e(T);
	G_AV = 5.0 / 7.0 * G_AE + spinel_v_e(T) - spinel_i_ae(T)
		+ spinel_delta_ae() - spinel_delta_eav();

	Gmix =
		y_t_fe2 * y_o_fe2 * G_AA +
		y_t_fe2 * y_o_fe3 * G_AE +
		y_t_fe2 * y_o_va * G_AV +
		y_t_fe3 * y_o_fe2 * G_EA +
		y_t_fe3 * y_o_fe3 * G_EE +
		y_t_fe3 * y_o_va * G_EV;

	Sconf = -spinel_R() * (
		safe_ylogy(y_t_fe2) + safe_ylogy(y_t_fe3)
		+ 2.0 * (safe_ylogy(y_o_fe2) + safe_ylogy(y_o_fe3) + safe_ylogy(y_o_va))
	);

	Gmag = spinel_gmag(params, T, y_t_fe2, y_t_fe3, y_o_fe2, y_o_fe3, y_o_va);

	*g_out = nphase * (Gmix - T * Sconf + Gmag);
	return isfinite(*g_out);
}

static int spinel_fe_degterov_eval_params(const SpinelFeParams *params,
		const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	double g0;
	size_t i;
	(void)p;
	if(!spinel_fe_degterov_g_only_params(params, n_members, T, &g0)){
		return 0;
	}
	if(g_out){
		*g_out = g0;
	}
	if(mu_out){
		for(i = 0; i < SPINEL_FE_MEMBER_COUNT; ++i){
			double nwork[SPINEL_FE_MEMBER_COUNT];
			double gp, gm;
			double eps = 1e-8 * fmax(1.0, fabs(n_members[i]));
			memcpy(nwork, n_members, sizeof(nwork));
			if(nwork[i] > eps){
				nwork[i] += eps;
				if(!spinel_fe_degterov_g_only_params(params, nwork, T, &gp)){
					return 0;
				}
				nwork[i] = n_members[i] - eps;
				if(!spinel_fe_degterov_g_only_params(params, nwork, T, &gm)){
					return 0;
				}
				mu_out[i] = (gp - gm) / (2.0 * eps);
			}else{
				nwork[i] += eps;
				if(!spinel_fe_degterov_g_only_params(params, nwork, T, &gp)){
					return 0;
				}
				mu_out[i] = (gp - g0) / eps;
			}
			if(!isfinite(mu_out[i])){
				return 0;
			}
		}
	}
	return 1;
}

static int spinel_fe_degterov_eval(const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	return spinel_fe_degterov_eval_params(&spinel_params_degterov, n_members, T, p, g_out, mu_out);
}

static int spinel_fe_bg_tuned_eval(const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	return spinel_fe_degterov_eval_params(&spinel_params_bg_tuned, n_members, T, p, g_out, mu_out);
}

static int spinel_fe_mmc1_guess_eval(const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	return spinel_fe_degterov_eval_params(&spinel_params_mmc1_guess, n_members, T, p, g_out, mu_out);
}

static int spinel_fe_hidayat_adj1_eval(const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	return spinel_fe_degterov_eval_params(&spinel_params_hidayat_adj1, n_members, T, p, g_out, mu_out);
}

static const char *elements_tet[] = {"Fe", "O"};
static const double stoich_tet[] = {1.0, 4.0};
static const char *elements_oct_fe[] = {"Fe"};
static const double stoich_oct_fe[] = {1.0};

static const FeSpinelPhaseDef spinel_phase = {
	"spinel_fe",
	"degterov_2001",
	{
		"Sp_Fe2_tet",
		"Sp_Fe3_tet",
		"Sp_Fe2_oct",
		"Sp_Fe3_oct",
		"Sp_Va_oct"
	},
	{2, 2, 1, 1, 0},
	{elements_tet, elements_tet, elements_oct_fe, elements_oct_fe, NULL},
	{stoich_tet, stoich_tet, stoich_oct_fe, stoich_oct_fe, NULL},
	&spinel_fe_degterov_eval
};

static int spinel_feoxide_recon_eval(const double *n_members, double T, double p, double *g_out,
		double *mu_out){
	return spinel_fe_degterov_eval_params(&spinel_params_feoxide_recon, n_members, T, p, g_out, mu_out);
}

static const FeSpinelPhaseDef spinel_phase_feoxide_recon = {
	"spinel_fe",
	"feoxide_recon_baseline_2026",
	{
		"Sp_Fe2_tet",
		"Sp_Fe3_tet",
		"Sp_Fe2_oct",
		"Sp_Fe3_oct",
		"Sp_Va_oct"
	},
	{2, 2, 1, 1, 0},
	{elements_tet, elements_tet, elements_oct_fe, elements_oct_fe, NULL},
	{stoich_tet, stoich_tet, stoich_oct_fe, stoich_oct_fe, NULL},
	&spinel_feoxide_recon_eval
};

static const FeSpinelPhaseDef spinel_phase_bg_tuned = {
	"spinel_fe",
	"fe_spinel_bg_tuned_2026",
	{
		"Sp_Fe2_tet",
		"Sp_Fe3_tet",
		"Sp_Fe2_oct",
		"Sp_Fe3_oct",
		"Sp_Va_oct"
	},
	{2, 2, 1, 1, 0},
	{elements_tet, elements_tet, elements_oct_fe, elements_oct_fe, NULL},
	{stoich_tet, stoich_tet, stoich_oct_fe, stoich_oct_fe, NULL},
	&spinel_fe_bg_tuned_eval
};

static const FeSpinelPhaseDef spinel_phase_mmc1_guess = {
	"spinel_fe",
	"fe_spinel_mmc1_guess_2026",
	{
		"Sp_Fe2_tet",
		"Sp_Fe3_tet",
		"Sp_Fe2_oct",
		"Sp_Fe3_oct",
		"Sp_Va_oct"
	},
	{2, 2, 1, 1, 0},
	{elements_tet, elements_tet, elements_oct_fe, elements_oct_fe, NULL},
	{stoich_tet, stoich_tet, stoich_oct_fe, stoich_oct_fe, NULL},
	&spinel_fe_mmc1_guess_eval
};

static const FeSpinelPhaseDef spinel_phase_hidayat_adj1 = {
	"spinel_fe",
	"hidayat_adj1",
	{
		"Sp_Fe2_tet",
		"Sp_Fe3_tet",
		"Sp_Fe2_oct",
		"Sp_Fe3_oct",
		"Sp_Va_oct"
	},
	{2, 2, 1, 1, 0},
	{elements_tet, elements_tet, elements_oct_fe, elements_oct_fe, NULL},
	{stoich_tet, stoich_tet, stoich_oct_fe, stoich_oct_fe, NULL},
	&spinel_fe_hidayat_adj1_eval
};

const FeSpinelPhaseDef *spinel_fe_degterov_phase(void){
	return &spinel_phase;
}

const FeSpinelPhaseDef *spinel_feoxide_recon_phase(void){
	return &spinel_phase_feoxide_recon;
}

const FeSpinelPhaseDef *spinel_fe_bg_tuned_phase(void){
	return &spinel_phase_bg_tuned;
}

const FeSpinelPhaseDef *spinel_fe_mmc1_guess_phase(void){
	return &spinel_phase_mmc1_guess;
}

const FeSpinelPhaseDef *spinel_fe_hidayat_adj1_phase(void){
	return &spinel_phase_hidayat_adj1;
}
