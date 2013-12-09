#include "../ideal.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../color.h"

extern const EosData eos_rpp_methane;

#include <assert.h>
#include <math.h>

#define MSG FPROPS_MSG
#define ERRMSG FPROPS_ERRMSG

#define TOL_T 1e-3
#define TOL_RHO 1e-3

int main(void){
	PureFluid *P;
	FpropsError err = FPROPS_NO_ERROR;
	FluidState S;
	
	MSG("Testing ideal EOS");

	ReferenceState ref = {FPROPS_REF_REF0};
	P = ideal_prepare(&eos_rpp_methane, &ref); assert(P);

	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);
	fprintf(stderr,"\n");
	return 0;
}

