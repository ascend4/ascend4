#include "../fluids.h"
#include "../fprops.h"
#include "../solve_pT.h"
#include "../color.h"

#include <assert.h>

#define PENGROB_DEBUG
#ifdef PENGROB_DEBUG
# include "../color.h"
# define MSG FPROPS_MSG
#else
# define MSG(ARGS...) ((void)0)
#endif

int main(void){
	const PureFluid *fluid = fprops_fluid("toluene","pengrob",NULL);
	
	double T = 260;
	double p = 20e5;
	double rho;
	FpropsError err = 0;
	
	MSG("Solving T = %g K, p = %g bar",T,p/1e5);
	
	fprops_solve_pT(p, T, &rho, fluid, &err);   assert(err == 0);
	
	MSG("rho = %f kg/m³",rho);
	
	FluidState state = fprops_set_Trho(T,rho,fluid,&err);   assert(err == 0);
	
	MSG("State set");

	double p1 = fprops_p(state,&err);   assert(err == 0);
	
	MSG("At (T,rho), got back p = %g bar",p/1e5);
	assert(fabs(p1 - p) < 1e-4);
	
	MSG("PASSED");
}

