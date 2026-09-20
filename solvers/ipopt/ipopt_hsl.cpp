#include "ipopt_hsl.h"
#include <IpLinearSolvers.h>
#include <IpLibraryLoader.hpp>

/* Match the symbols loaded by IPOPT's HSL interfaces. Checking the capability
   mask alone would advertise every HSL solver, even with no HSL installed. */
#ifdef IPOPT_SINGLE
# define HSL_F ""
# define HSL_C "_s"
#else
# define HSL_F "d"
# define HSL_C "_d"
#endif

static const char *ma27[] = {"ma27a" HSL_F, "ma27b" HSL_F,
	"ma27c" HSL_F, "ma27i" HSL_F, NULL};
static const char *ma57[] = {"ma57a" HSL_F, "ma57b" HSL_F,
	"ma57c" HSL_F, "ma57e" HSL_F, "ma57i" HSL_F, NULL};
static const char *ma77[] = {
	"ma77_default_control" HSL_C, "ma77_open_nelt" HSL_C, "ma77_open" HSL_C,
	"ma77_input_vars" HSL_C, "ma77_input_reals" HSL_C, "ma77_analyse" HSL_C,
	"ma77_factor" HSL_C, "ma77_factor_solve" HSL_C, "ma77_solve" HSL_C,
	"ma77_resid" HSL_C, "ma77_scale" HSL_C, "ma77_enquire_posdef" HSL_C,
	"ma77_enquire_indef" HSL_C, "ma77_alter" HSL_C, "ma77_restart" HSL_C,
	"ma77_finalise" HSL_C, "mc68_default_control_i", "mc68_order_i", NULL};
static const char *ma86[] = {
	"ma86_default_control" HSL_C, "ma86_analyse" HSL_C, "ma86_factor" HSL_C,
	"ma86_factor_solve" HSL_C, "ma86_solve" HSL_C, "ma86_finalise" HSL_C,
	"mc68_default_control_i", "mc68_order_i", NULL};
static const char *ma97[] = {
	"ma97_default_control" HSL_C, "ma97_analyse" HSL_C, "ma97_factor" HSL_C,
	"ma97_factor_solve" HSL_C, "ma97_solve" HSL_C, "ma97_finalise" HSL_C,
	"ma97_free_akeep" HSL_C, NULL};

extern "C" unsigned int asc_ipopt_hsl_available(const char *library){
	const unsigned int flags[] = {IPOPTLINEARSOLVER_MA27, IPOPTLINEARSOLVER_MA57,
		IPOPTLINEARSOLVER_MA77, IPOPTLINEARSOLVER_MA86, IPOPTLINEARSOLVER_MA97};
	const char **symbols[] = {ma27, ma57, ma77, ma86, ma97};
	unsigned int linked = IpoptGetAvailableLinearSolvers(1);
	unsigned int loadable = IpoptGetAvailableLinearSolvers(0) & ~linked;
	unsigned int found = linked & IPOPTLINEARSOLVER_ALLHSL;
	if(!(loadable & IPOPTLINEARSOLVER_ALLHSL)) return found;
	try{
		/* This private loader does not disturb any active IPOPT solve. Its
		   destructor releases only our reference to the shared library. */
		Ipopt::LibraryLoader loader(library ? library : "libhsl." IPOPT_SHAREDLIBEXT);
		loader.loadLibrary();
		for(size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i){
			if(!(loadable & flags[i])) continue;
			try{
				for(const char **symbol = symbols[i]; *symbol; ++symbol){
					loader.loadSymbol(*symbol);
				}
				found |= flags[i];
			}catch(const Ipopt::DYNAMIC_LIBRARY_FAILURE &){
				/* A partial HSL installation can still provide other solvers. */
			}
		}
	}catch(const Ipopt::DYNAMIC_LIBRARY_FAILURE &){
		/* Missing library or one of its dependencies: retain linked solvers. */
	}
	return found;
}
