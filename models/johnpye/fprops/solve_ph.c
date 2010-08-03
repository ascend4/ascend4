/*
ASCEND modelling environment
Copyright (C) 2004-2010 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "solve_ph.h"
#include "sat2.h"

#include <stdio.h>

int fprops_region_ph(double p, double h, const HelmholtzData *D){

	double Tsat, rhof, rhog;

	if(p >= D->p_c)return FPROPS_NON;

	int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);

	double hf = helmholtz_h(Tsat, rhof, D);
	if(h <= hf)return FPROPS_NON;

	double hg = helmholtz_h(Tsat, rhog, D);
	if(h >= hg)return FPROPS_NON;

	return FPROPS_SAT;
}

int fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess, const HelmholtzData *D){
	double Tsat, rhof, rhog, hf, hg;
	if(p < D->p_c){
		int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);
		if(res){
			fprintf(stderr,"Unable to solve saturation state\n");
			return res;
		}
		hf = helmholtz_h(Tsat, rhof, D);
		hg = helmholtz_h(Tsat, rhog, D);
		fprintf(stderr,"hf = %f kJ/kg, hg = %f\n",hf/1e3,hg/1e3);

		if(h > hf && h < hg){
			fprintf(stderr,"SATURATION REGION\n");
			/* saturation region... easy */
			double x = (h - hf)/(hg - hf);
			*rho = x/rhog + (1.-x)/rhof;
			*T = Tsat;
		}

		if(!use_guess){
			*T = Tsat;
			if(h <= hf)*rho = rhof;
			else *rho = rhog;
		}
	}else{
		if(!use_guess){
			*T = D->T_c;
			*rho = D->rho_c;
		}
	}

	int res = fprops_nonsolver('p','h',p,h,T,rho,D);
	fprintf(stderr,"Iteration failed in nonsolver");
	return res;
}

