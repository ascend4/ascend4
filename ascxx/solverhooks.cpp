#include "config.h"
#include "solverhooks.h"
#include "simulation.h"
#include "solver.h"
#include "solverparameters.h"
#include "solverreporter.h"
#include "registry.h"
#include "value.h"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

extern "C"{
#include <ascend/utilities/error.h>
#include <ascend/compiler/value_type.h>
};

#define SOLVERHOOKS_DEBUG 0

namespace{

struct StoredOption{
	enum ValueType{
		INT_VALUE,
		BOOL_VALUE,
		REAL_VALUE,
		SYMBOL_VALUE
	};

	std::string name;
	ValueType type;
	long int_data;
	bool bool_data;
	double real_data;
	std::string symbol_data;

	StoredOption(const std::string &name, const value_t &value)
		: name(name), type(INT_VALUE), int_data(0), bool_data(false), real_data(0.0){
		switch(ValueKind(value)){
		case integer_value:
			type = INT_VALUE;
			int_data = IntegerValue(value);
			break;
		case boolean_value:
			type = BOOL_VALUE;
			bool_data = BooleanValue(value);
			break;
		case real_value:
			type = REAL_VALUE;
			real_data = RealValue(value);
			break;
		case symbol_value:
			type = SYMBOL_VALUE;
			symbol_data = SCP(SymbolValue(value));
			break;
		default:
			throw std::runtime_error("Unsupported solver option value type");
		}
	}
};

struct StoredSolverConfig{
	bool have_solver = false;
	std::string solver_name;
	std::vector<StoredOption> options;
};

static std::map<Simulation *, StoredSolverConfig> g_solver_configs;

static StoredSolverConfig &get_solver_config(Simulation *S){
	return g_solver_configs[S];
}

static int apply_option_to_system(Simulation *S, const char *optionname, const value_t *val){
	SolverParameters pp = S->getParameters();

	try{
		SolverParameter p = pp.getParameter(optionname);
		try{
			p.setValueValue(Value(val));
		}catch(std::runtime_error &){
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
	}catch(std::runtime_error &){
		return SLVREQ_INVALID_OPTION_NAME;
	}
	S->setParameters(pp);
	return 0;
}

static int apply_option_to_system(Simulation *S, const StoredOption &stored){
	SolverParameters pp = S->getParameters();

	try{
		SolverParameter p = pp.getParameter(stored.name);
		try{
			switch(stored.type){
			case StoredOption::INT_VALUE:
				p.setIntValue((int)stored.int_data);
				break;
			case StoredOption::BOOL_VALUE:
				p.setBoolValue(stored.bool_data);
				break;
			case StoredOption::REAL_VALUE:
				p.setRealValue(stored.real_data);
				break;
			case StoredOption::SYMBOL_VALUE:
				p.setStrValue(stored.symbol_data);
				break;
			}
		}catch(std::runtime_error &){
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
	}catch(std::runtime_error &){
		return SLVREQ_INVALID_OPTION_NAME;
	}
	S->setParameters(pp);
	return 0;
}

static void remember_option(StoredSolverConfig &config, const char *optionname, const value_t *val){
	for(std::vector<StoredOption>::iterator i = config.options.begin(); i != config.options.end(); ++i){
		if(i->name == optionname){
			*i = StoredOption(optionname, *val);
			return;
		}
	}
	config.options.push_back(StoredOption(optionname, *val));
}

static int apply_stored_solver_config(Simulation *S){
	StoredSolverConfig &config = get_solver_config(S);
	if(config.have_solver){
		Solver solver(config.solver_name.c_str());
		S->setSolver(solver);
	}
	for(std::vector<StoredOption>::const_iterator i = config.options.begin(); i != config.options.end(); ++i){
		int res = apply_option_to_system(S, *i);
		if(res != 0){
			return res;
		}
	}
	return 0;
}

}

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
	Registry reg;
	reg.setPointer("slvreq_target", instance);
	int res = S->getSolverHooks()->doSolve(instance, S);
	reg.setPointer("slvreq_target", NULL);
	return res;
}

int ascxx_slvreq_delete_system(void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_DELETE_HOOK_NOT_SET;
	return S->getSolverHooks()->deleteSystem(S);
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
		StoredSolverConfig &config = get_solver_config(S);
		config.have_solver = true;
		config.solver_name = solvername;
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
	try{
		S->build();
	}catch(std::runtime_error &){
		return SLVREQ_OPTIONS_UNAVAILABLE;
	}
	try{
		(void)S->getSolver();
	}catch(std::runtime_error &){
		return SLVREQ_OPTIONS_UNAVAILABLE;
	}
	int res = apply_option_to_system(S, optionname, val.v);
	if(res == 0){
		remember_option(get_solver_config(S), optionname, val.v);
	}
	return res;
}

int
SolverHooks::doSolve(Instance *i, Simulation *S){
	CONSOLE_DEBUG("Solving model...");
	
	try{
		Instanc target(i);
		S->build(target);
		int applyres = apply_stored_solver_config(S);
		if(applyres != 0){
			return applyres;
		}
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

int
SolverHooks::deleteSystem(Simulation *S){
	S->invalidateSystem();
	return 0;
}

void
SolverHooks::assign(Simulation *S){
	S->setSolverHooks(this);
#if SOLVERHOOKS_DEBUG
	CONSOLE_DEBUG("Assigning SolverHooks to Simulation...");
#endif
	slvreq_assign_hooks(S->getInternalType(),&ascxx_slvreq_set_solver, &ascxx_slvreq_set_option, &ascxx_slvreq_do_solve, &ascxx_slvreq_delete_system, (void *)S);
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
