#include "../fluids.h"
#include "../fprops.h"
#include "../solve_pT.h"
#include "../color.h"
#include "../sat.h"

#include <assert.h>

#define PENGROB_DEBUG
#ifdef PENGROB_DEBUG
# include "../color.h"
# define MSG FPROPS_MSG
#else
# define MSG(ARGS...) ((void)0)
#endif

int main(void){

	PureFluid *fluid;
	FluidState state;
	FpropsError err = 0;
	double T, p, rho, p1;
	double rhof, rhog;
	
#if 1
	// test the (p,T) solver with Toluene, requires cubicroots with a small D.
	MSG("\n\nTesting case of toluene...");

	fluid = fprops_fluid("toluene","pengrob",NULL);
	
	T = 260;
	p = 20e5;
	err = 0;
	
	MSG("Solving T = %g K, p = %g bar",T,p/1e5);
	
	fprops_solve_pT(p, T, &rho, fluid, &err);   assert(err == 0);
	
	MSG("rho = %f kg/m³",rho);
	
	state = fprops_set_Trho(T,rho,fluid,&err);   assert(err == 0);
	MSG("State set");

	p1 = fprops_p(state,&err);   assert(err == 0);
	
	MSG("At (T,rho), got back p = %g bar",p/1e5);
	assert(fabs(p1 - p) < 1e-4);

	fprops_fluid_destroy(fluid);
#endif

#if 0
	// test water near triple point
	
	MSG("\n\nTesting case of water near Tmin")
	fluid = fprops_fluid("water","pengrob",NULL);
	
	T = fluid->data->T_min;
	
	fprops_sat_T(T,&p1,&rhof, &rhog, fluid, &err); assert(err == 0);

	fprops_fluid_destroy(fluid);
#endif

#if 0
	// test case of ethanol near the critical point
	MSG("\n\nTesting case of ethanol near critical point:");
	
	fluid = fprops_fluid("ethanol","pengrob",NULL);
	T = 500.;
	MSG("T = %g (T_crit = %g)",T,fluid->data->T_c);
	
	fprops_sat_T(T,&p1,&rhof, &rhog, fluid, &err); assert(err == 0);
	
	MSG("Saturation: p = %g bar, rho_f = %f kg/m³, rho_g = %f kg/m³",p1/1e5,rhof,rhog);
#endif	
	
	// test case of CO2 near critical point
	MSG("\n\nTesting case of CO₂ near critical point:");
	
	fluid = fprops_fluid("carbondioxide","pengrob",NULL);
	T = 303.;
	MSG("T = %g (T_crit = %g)",T,fluid->data->T_c);
	
	fprops_sat_T(T,&p1,&rhof, &rhog, fluid, &err); assert(err == 0);
	
	MSG("Saturation: p = %g bar, rho_f = %f kg/m³, rho_g = %f kg/m³",p1/1e5,rhof,rhog);
	
	MSG("PASSED");
	
}

