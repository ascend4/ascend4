#include "config.h"
#include "solverhooks.h"
#include "simulation.h"
#include "solver.h"
#include "solverparameters.h"
#include "solverreporter.h"
#include "value.h"

#include <stdexcept>
#include <string>

extern "C"{
#include <ascend/utilities/error.h>
};

#define SOLVERHOOKS_DEBUG 0

//------------------------------------------------------------------------------
// C-level functions that SolverHooks can pass back to libascend

int ascxx_slvreq_set_solver(const char *solvername, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_SOLVER_HOOK_NOT_SET;
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Got solver hooks at %p from Simulation at %p",S->getSolverHooks(),S);
#endif
	return S->getSolverHooks()->setSolver(solvername, S);
}

int ascxx_slvreq_set_option(const char *optionname, value_t *val, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_OPTION_HOOK_NOT_SET;
	return S->getSolverHooks()->setOption(optionname, Value(val), S);
}

int ascxx_slvreq_do_solve(struct Instance *instance, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_SOLVE_HOOK_NOT_SET;
	return S->getSolverHooks()->doSolve(instance, S);
}


//------------------------------------------------------------------------------
// SOLVER HOOKS (C++ layer implementation)

SolverHooks::SolverHooks(SolverReporter *R) : R(R){
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Creating SolverHooks at %p",this);
#endif
	// nothing else to do
}

SolverHooks::~SolverHooks(){
	/* nothing that we own that we need to destroy? */
}

SolverHooks::SolverHooks(SolverHooks &old) : R(old.R){
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Creating new SolverHooks at %p (copy of old at %p",this,&old);
#endif
}

int
SolverHooks::setSolver(const char *solvername, Simulation *S){
	/* note desired return codes from slvreq.h */
	try{
		Solver solver(solvername);
		S->build();
		S->setSolver(solver);
	}catch(std::runtime_error &E){
		return SLVREQ_UNKNOWN_SOLVER;
	}
	CONSOLE_DEBUG("Solver set to '%s'",solvername);
	return 0;
}

int
SolverHooks::setOption(const char *optionname, Value val, Simulation *S){
	/* FIXME need to check if the system is built? */
	/* FIXME check if we have got a solver assigned? */
	SolverParameters pp = S->getParameters();

	try{
		SolverParameter p = pp.getParameter(optionname);
		try{
			p.setValueValue(val);
		}catch(std::runtime_error &E){
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
	}catch(std::runtime_error &E){
		return SLVREQ_INVALID_OPTION_NAME;
	}
	return 0;
}

int
SolverHooks::doSolve(Instance *i, Simulation *S){
	CONSOLE_DEBUG("Solving model...");
	
	try{
		/* FIXME do solving of a particular instance? */
		if(!getSolverReporter()){
			CONSOLE_DEBUG("Creating default SolverReporter");
			SolverReporter R;
			S->solve(S->getSolver(), R);
		}else{
			CONSOLE_DEBUG("Using SolverReporter at %p",getSolverReporter());
			S->solve(S->getSolver(), *getSolverReporter());
		}
	}catch(std::runtime_error &E){
		return SLVREQ_SOLVE_FAIL;
	}

	/* solver succeeded */
	return 0;
}

void
SolverHooks::assign(Simulation *S){
	S->setSolverHooks(this);
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Assigning SolverHooks to Simulation...");
#endif
	slvreq_assign_hooks(S->getInternalType(),&ascxx_slvreq_set_solver, &ascxx_slvreq_set_option, &ascxx_slvreq_do_solve, (void *)S);
}

SolverReporter *
SolverHooks::getSolverReporter(){
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("SolverReporter is at %p", R);
#endif
	return R;
}

//------------------------------------------------------------------------------
// SOLVER HOOKS (Python layer implementation)

#if 0
class SolverHooksPython{
private:
	PyObject *set_solver_py;
	PyObject *set_param_py;
	PyObject *do_solve_py;
	PyObject *context_py;
public:
	SolverHooksPython(PyObject *set_solver_fn, PyObject *set_param_fn, PyObject *do_solve_fn, PyObject *context);
	virtual int setSolver(const char *solvername, Simulation *S);
	virtual int setOption(const char *optionname, const char *val, Simulation *S);
	virtual int doSolve(Instance *i, Simulation *S);
};
#endif

//------------------------------------------------------------------------------
// SOLVER HOOKS MANAGER (singleton)

SolverHooksManager::SolverHooksManager(){
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Creating SolverHooksManager with NULL hooks");
#endif
	this->hooks = NULL;
	this->own_hooks = 0;
}

SolverHooksManager *SolverHooksManager::_instance;

SolverHooksManager *
SolverHooksManager::Instance(){
	if(_instance==0){
		_instance = new SolverHooksManager();
	}
	return _instance;
}

SolverHooksManager::~SolverHooksManager(){
	if(own_hooks){
#if SOLVERHOOKS_DEBUG
		CONSOLE_DEBUG("Delete owned hooks");
#endif
		delete hooks;
	}
}

void
SolverHooksManager::setHooks(SolverHooks *H){
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Using hooks at %p",H);
#endif
	if(hooks && own_hooks){
#if SOLVERHOOKS_DEBUG
		CONSOLE_DEBUG("Deleting previous owned hooks");
#endif
		delete(hooks);
	}
	this->hooks = H;
	this->own_hooks = 0;
}

SolverHooks *
SolverHooksManager::getHooks(){
	if(this->hooks == NULL){
#if SOLVERHOOKS_DEBUG
		CONSOLE_DEBUG("Creating new default SolverHooks...");
#endif
		this->hooks = new SolverHooks();
		this->own_hooks = 1;
	}
	return this->hooks;
}



