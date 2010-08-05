/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/
#include "sat3.h"
#include "sat.h"
#include <math.h>
#include <stdio.h>
#define SQ(X) ((X)*(X))

#define _GNU_SOURCE
#include <fenv.h>

int fprops_sat_T_akasaka(double T, double *psat_out, double *rhof_out, double * rhog_out, const HelmholtzData *d){
	double tau = d->T_c / T;
	double delf = 1.1 * fprops_rhof_T_rackett(T,d) / d->rho_c;
	double delg = 0.9 * fprops_rhog_T_chouaieb(T,d) / d->rho_c;
	feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

	int i = 0;
	while(i < 20){
		double phirf = helm_resid(delf,tau,d);
		double phirf_d = helm_resid_del(delf,tau,d);
		double phirf_dd = helm_resid_deldel(delf,tau,d);
		double phirg = helm_resid(delg,tau,d);
		double phirg_d = helm_resid_del(delg,tau,d);
		double phirg_dd = helm_resid_deldel(delg,tau,d);

#define J(FG) (del##FG * (1. + del##FG * phir##FG##_d))
#define K(FG) (del##FG * phir##FG##_d + phir##FG + log(del##FG))
#define J_del(FG) (1 + 2 * del##FG * phir##FG##_d + SQ(del##FG) * phir##FG##_dd)
#define K_del(FG) (2 * phir##FG##_d + del##FG * phir##FG##_dd + 1./del##FG)
		double Jf = J(f);
		double Jg = J(g);
		double Kf = K(f);
		double Kg = K(g);
		double Jf_del = J_del(f);
		double Jg_del = J_del(g);
		double Kf_del = K_del(f);
		double Kg_del = K_del(g);

		double DELTA = Jg_del * Kf_del - Jf_del * Kg_del;

#define gamma 1.
		delf += gamma/DELTA * ((Kg - Kf) * Jg_del - (Jg - Jf) * Kg_del);
		delg += gamma/DELTA * ((Kg - Kf) * Jf_del - (Jg - Jf) * Kf_del);

		if(fabs(Kg - Kf) + fabs(Jg - Jf) < 1e-8){
			fprintf(stderr,"%s: CONVERGED\n",__func__);
			*rhof_out = delf * d->rho_c;
			*rhog_out = delg * d->rho_c;
			*psat_out = helmholtz_p_raw(T, *rhog_out, d);
			return 0;
		}
	}
	fprintf(stderr,"%s: NOT CONVERGED\n",__func__);
	*rhof_out = delf * d->rho_c;
	*rhog_out = delg * d->rho_c;
	*psat_out = helmholtz_p_raw(T, *rhog_out, d);
	return 1;

}

