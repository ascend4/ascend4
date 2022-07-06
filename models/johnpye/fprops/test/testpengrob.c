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

	// test the (p,T) solver with Toluene, requires cubicroots with a small D.

	PureFluid *fluid;
	FluidState state;
	FpropsError err = 0;
	double T, p, rho, p1;
#if 0
	fluid = fprops_fluid("toluene","pengrob",NULL);
	
	T = 260;
	p = 20e5;
	rho;
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

	// test case of saturation state of bromobenzene (error from test/sat.c)
	
	fluid = fprops_fluid("water","pengrob",NULL);
	
	T = 290.;
	
	double rhof, rhog;
	fprops_sat_T(T,&p1,&rhof, &rhog, fluid, &err); assert(err == 0);
	
	MSG("PASSED");
}

