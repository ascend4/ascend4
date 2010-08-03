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

#include <stdio.h>
#include <stdlib.h>

int fprops_region_ph(double p, double h, const HelmholtzData *D){

	double Tsat, rhof, rhof;

	if(p >= D->p_c)return FPROPS_NON;

	int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);

	double hf = helmholtz_h(Tsat, rhof, D)
	if(h <= hf)return FPROPS_NON;

	double hg = helmholtz_h(Tsat, rhog, D);
	if(h >= hg)return FPROPS_NON;

	return FPROPS_SAT;
}

int fprops_solve_ph(double p, double h, double *T, double &rho, const HelmholtzData *D){
	double Tsat, rhof, rhog;
	int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);

	double hf = helmholtz_h(Tsat, rhof, D)
	double hg = helmholtz_h(Tsat, rhog, D);

	if(h > hf && h < hg){
		/* saturation region... easy, once we know saturation conditions */
		double x = (h - hf)/(hg - hf);
		*rho = x/rhog + (1.-x)/rhof;
		*T = Tsat;
	}else{
		

		
	if(h <= hf)h_upper = hf;

	if(h >= hg)h_lower = hg;

	if(h

#endif




