/*
ASCEND modelling environment
Copyright (C) 2004-2011 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file Small demonstration of the 'solve_ph' routines for FPROPS.

To build this file, run

gcc -o test_ph test_ph.c solve_ph.c helmholtz.c ideal.c fluids.c derivs.c sat.c fluids/*.c -lm
*/
#include "solve_ph.h"
#include "fluids.h"
#include "helmholtz.h"
#include <stdio.h>

int main(void){
	double p = 1e5;
	double h = 3500e3;
	const char *fluid = "water";

	const HelmholtzData *D = fprops_fluid(fluid);

	printf("Solving for (p = %f bar, h = %f kJ/kg) for fluid '%s'\n",p/1e5,h/1e3,fluid);

	int reg = fprops_region_ph(p,h, D);

	printf("Region = %d\n",reg);

	double T = 0,rho = 0;
	int res = fprops_solve_ph(p,h,&T, &rho, 0, D);

	if(res)printf("ERROR %d! (check solve_ph.c for meaning)\n",res);
	else printf("Success, solved state.\n");

	printf("Result: T = %f K, rho = %f kg/m3\n",T,rho);

	double p1, h1;
	p1 = helmholtz_p(T,rho, D);
	h1 = helmholtz_h(T,rho, D);
	printf("Double check: p = %f bar\n",p1/1e5);
	printf("Double check: h = %f kJ/kg\n",h1/1e3);

	return 0;
}
