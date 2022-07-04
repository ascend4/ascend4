/*	FPROPS
	Copyright (C) 2022 John Pye

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
	Some experiments towards calculating properties of mixtures

	Following Example 1 from the following paper:
	
	LE Baker, AC Pierce & KD Luks (1982), "Gibbs Energy Analysis of Phase
	Equilibria", Soc. Pet. Eng. J. 22(5), pp. 731--742.
	https://doi.org/10.2118/9806-PA
	
	John Pye, 29 Jun 2022
*/

#include "rundata.h"

extern EosData eos_carbondioxide;
extern EosData eos_toluene;

#include "pengrob.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define MIX_DEBUG
#ifdef MIX_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

double pengrob_g(double T, double rho, const FluidData *data, FpropsError *err);
double pengrob_p(double T, double rho, const FluidData *data, FpropsError *err);

// solve rho for fixed (p,T)

int main(void){

	PureFluid *CO2 = pengrob_prepare(&eos_carbondioxide,NULL);
	PureFluid *C7H8 = pengrob_prepare(&eos_toluene,NULL);
	
	double T1 = 38.1 + 273.15;
	double v1 = 0.1;
	double rho1 = 1/v1;
	FpropsError err = 0;
	double p1 = pengrob_p(T1,rho1,CO2->data, &err);
	if(err){
		ERRMSG("couldn't calculate p");
		return 1;
	}
	
	MSG("T1 = %f K = %f °C", T1, T1-273.15);
	MSG("p1 = %f bar",p1/1e5);
	MSG("p_c = %f bar",CO2->data->p_c/1e5);
	
	// scan values of p for fixed T, varying v -- for CO₂
	
	double vmin = 0.0008;
	double vmax = 0.5;
	unsigned n = 30;
	for(unsigned i=0;i<n;++i){
		double v=exp(log(vmin)+i*(log(vmax)-log(vmin))/(n-1));
		double p = pengrob_p(T1,1/v,CO2->data, &err);
		assert(!err);
		MSG("v = %f m³/kg (rho %f kg/m³) → p = %f bar",v,1/v, p/1e5);
	}
	
	// for (T,p) pair, what is the gibbs energy of each species?
	
	p1 = 1379e3;
	MSG("Solving p1 = %f kPa...",p1/1e3);
	
	pengrob_solve_pT(p1,T1,&rho1,CO2->data,&err);
	assert(!err);
	
	double p1c = pengrob_p(T1,rho1,CO2->data,&err);
	assert(!err);
	MSG("p1c = %f kPa",p1c/1e3);
	assert(fabs(p1 - p1c)<1e-8);
	
	MSG("solve rho = %f",rho1);
	double G1 = pengrob_g(T1,rho1,CO2->data,&err);
	MSG("G1 = %f",G1);
	
	double rho2;
	pengrob_solve_pT(p1,T1,&rho2,C7H8->data,&err);
	MSG("rho2 = %f",rho2);
	double G2 = pengrob_g(T1,rho2,C7H8->data,&err);
	MSG("G2 = %f",G2);
	
	// now we need an expressions for the mixing enthalpy ΔHmix and ΔSmix
	// so we can plot G(yB)
	

	return 0;
}
