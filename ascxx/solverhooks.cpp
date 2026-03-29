#include "config.h"
#include "solverhooks.h"
#include "simulation.h"
#include "solver.h"
#include "solverparameters.h"
#include "solverreporter.h"
#include "integrator.h"
#include "integratorreporter.h"
#include "registry.h"
#include "value.h"
#include <ascend/compiler/simstatus.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>

extern "C"{
#include <ascend/utilities/error.h>
#include <ascend/compiler/value_type.h>
};

//#define SOLVERHOOKS_DEBUG
#ifdef SIMULATION_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

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

struct StoredIntegratorConfig{
	bool have_integrator = false;
	std::string integrator_name;
	std::vector<StoredOption> options;
};

enum StoredFocus{
	FOCUS_NONE = 0,
	FOCUS_SOLVER,
	FOCUS_INTEGRATOR
};

struct StoredStudyConfig{
	std::vector<Instanc> print_vars;
	bool suppress_print = false;
	std::vector<Instanc> default_observed;
	std::map<std::string, std::vector<Instanc> > named_observed;
};

struct StudyColumn{
	enum Kind{
		OBS_REAL,
		OBS_BOOL,
		OBS_INT,
		OBS_SYMBOL
	};
	Instance *inst;
	std::string name;
	std::string units;
	double conversion;
	Kind kind;
};

static StudyColumn get_study_column(Instance *inst, Simulation *S){
	StudyColumn column;
	Instanc wrapped(inst);

	column.inst = inst;
	column.name = S->getInstanceName(wrapped);
	column.units.clear();
	column.conversion = 1.0;
	if(wrapped.isReal()){
		column.kind = StudyColumn::OBS_REAL;
		try{
			UnitsM display_units = wrapped.getDisplayUnits(false);
			column.units = display_units.getName().toString();
			column.conversion = display_units.getConversion();
			if(column.conversion == 0.0){
				column.conversion = 1.0;
			}
		}catch(std::runtime_error &){
			column.units = wrapped.isDimensionless() ? "1" : "?";
			column.conversion = 1.0;
		}
	}else if(wrapped.isBool()){
		column.kind = StudyColumn::OBS_BOOL;
	}else if(wrapped.isInt()){
		column.kind = StudyColumn::OBS_INT;
	}else{
		column.kind = StudyColumn::OBS_SYMBOL;
	}
	return column;
}

static void write_study_headers(FILE *fp, const std::vector<StudyColumn> &columns){
	for(std::vector<StudyColumn>::size_type i = 0; i < columns.size(); ++i){
		if(columns[i].kind == StudyColumn::OBS_REAL){
			fprintf(fp, "%s [%s]%s", columns[i].name.c_str(), columns[i].units.c_str(),
				(i + 1 < columns.size()) ? "\t" : "");
		}else{
			fprintf(fp, "%s%s", columns[i].name.c_str(),
				(i + 1 < columns.size()) ? "\t" : "");
		}
	}
	fprintf(fp, "\n");
}

static int write_study_row(FILE *fp, const std::vector<StudyColumn> &columns){
	for(std::vector<StudyColumn>::size_type i = 0; i < columns.size(); ++i){
		Instanc obs(columns[i].inst);
		switch(columns[i].kind){
		case StudyColumn::OBS_REAL:
			fprintf(fp, "%.15g", obs.getRealValue() / columns[i].conversion);
			break;
		case StudyColumn::OBS_BOOL:
			fprintf(fp, "%s", obs.getBoolValue() ? "TRUE" : "FALSE");
			break;
		case StudyColumn::OBS_INT:
			fprintf(fp, "%ld", obs.getIntValue());
			break;
		case StudyColumn::OBS_SYMBOL:
			fprintf(fp, "'%s'", obs.getSymbolValue().toString());
			break;
		}
		fprintf(fp, "%s", (i + 1 < columns.size()) ? "\t" : "");
	}
	return fprintf(fp, "\n");
}

static std::map<Instance *, StoredSolverConfig> g_solver_configs;
static std::map<Instance *, StoredIntegratorConfig> g_integrator_configs;
static std::map<Instance *, StoredStudyConfig> g_study_configs;
static std::map<Instance *, StoredFocus> g_focus_configs;

static StoredSolverConfig &get_solver_config(Simulation *S){
	return g_solver_configs[S->getInternalType()];
}

static StoredStudyConfig &get_study_config(Simulation *S){
	return g_study_configs[S->getInternalType()];
}

static StoredIntegratorConfig &get_integrator_config(Simulation *S){
	return g_integrator_configs[S->getInternalType()];
}

static StoredFocus &get_focus_config(Simulation *S){
	return g_focus_configs[S->getInternalType()];
}

static bool has_instance(const std::vector<Instanc> &vars, const Instanc &inst){
	for(std::vector<Instanc>::const_iterator it = vars.begin(); it != vars.end(); ++it){
		if(it->getInternalType() == inst.getInternalType()){
			return true;
		}
	}
	return false;
}

static int apply_option_to_parameters(SolverParameters &pp, const char *optionname, const value_t *val){
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
	return 0;
}

static int apply_option_to_parameters(SolverParameters &pp, const StoredOption &stored){
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
	return 0;
}

static int apply_option_to_system(Simulation *S, const char *optionname, const value_t *val){
	SolverParameters pp = S->getParameters();
	int res = apply_option_to_parameters(pp, optionname, val);
	if(res != 0){
		return res;
	}
	S->setParameters(pp);
	return 0;
}

static int apply_option_to_system(Simulation *S, const StoredOption &stored){
	SolverParameters pp = S->getParameters();
	int res = apply_option_to_parameters(pp, stored);
	if(res != 0){
		return res;
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

static void remember_option(StoredIntegratorConfig &config, const char *optionname, const value_t *val){
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

static int apply_stored_integrator_config(Integrator &I, const StoredIntegratorConfig &config){
	SolverParameters pp = I.getParameters();
	for(std::vector<StoredOption>::const_iterator i = config.options.begin(); i != config.options.end(); ++i){
		int res = apply_option_to_parameters(pp, *i);
		if(res != 0){
			return res;
		}
	}
	I.setParameters(pp);
	return 0;
}

}

//------------------------------------------------------------------------------
// C-level functions that SolverHooks can pass back to libascend

int ascxx_slvreq_set_solver(const char *solvername, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_SOLVER_HOOK_NOT_SET;
	MSG("Got solver hooks at %p from Simulation at %p",S->getSolverHooks(),S);
	return S->getSolverHooks()->setSolver(solvername, S);
}

int ascxx_slvreq_set_integrator(const char *integratorname, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_INTEGRATOR_HOOK_NOT_SET;
	return S->getSolverHooks()->setIntegrator(integratorname, S);
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

int ascxx_slvreq_do_observe(const SlvReqObserveRequest *request, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_OBSERVE_HOOK_NOT_SET;
	ObserveRequest observe_request(request);
	return S->getSolverHooks()->doObserve(observe_request, S);
}

int ascxx_slvreq_do_study(const SlvReqStudyRequest *request, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_STUDY_HOOK_NOT_SET;
	StudyRequest study_request(request);
	return S->getSolverHooks()->doStudy(study_request, S);
}

int ascxx_slvreq_do_integrate(const SlvReqIntegrateRequest *request, void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_INTEGRATE_HOOK_NOT_SET;
	IntegrateRequest integrate_request(request);
	return S->getSolverHooks()->doIntegrate(integrate_request, S);
}

int ascxx_slvreq_delete_system(void *user_data){
	Simulation *S = (Simulation *)user_data;
	if(NULL==S->getSolverHooks())return SLVREQ_DELETE_HOOK_NOT_SET;
	return S->getSolverHooks()->deleteSystem(S);
}

ObserveRequest::ObserveRequest() : observed(), name(){
}

ObserveRequest::ObserveRequest(const SlvReqObserveRequest *request) : observed(), name(){
	unsigned long i;
	if(request == NULL){
		return;
	}
	if(request->observed != NULL){
		for(i = 0; i < request->n_observed; ++i){
			if(request->observed[i] != NULL){
				observed.push_back(Instanc(request->observed[i]));
			}
		}
	}
	if(request->name != NULL){
		name = request->name;
	}
}

std::vector<Instanc>
ObserveRequest::getObserved() const{
	return observed;
}

bool
ObserveRequest::hasName() const{
	return !name.empty();
}

std::string
ObserveRequest::getName() const{
	return name;
}


//------------------------------------------------------------------------------
// SOLVER HOOKS (C++ layer implementation)

SolverHooks::SolverHooks(SolverReporter *R) : R(R){
	MSG("Creating SolverHooks at %p",this);
	// nothing else to do
}

SolverHooks::~SolverHooks(){
	/* nothing that we own that we need to destroy? */
}

SolverHooks::SolverHooks(SolverHooks &old) : R(old.R){
	MSG("Creating new SolverHooks at %p (copy of old at %p",this,&old);
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
	get_focus_config(S) = FOCUS_SOLVER;
	MSG("Solver set to '%s'",solvername);
	asc_simstatus_mark_dirty(S->getInternalType());
	return 0;
}

int
SolverHooks::setIntegrator(const char *integratorname, Simulation *S){
	std::vector<std::string> engines = Integrator::getEngines();
	for(std::vector<std::string>::const_iterator i = engines.begin(); i != engines.end(); ++i){
		if(*i == integratorname){
			StoredIntegratorConfig &config = get_integrator_config(S);
			config.have_integrator = true;
			config.integrator_name = integratorname;
			get_focus_config(S) = FOCUS_INTEGRATOR;
			return 0;
		}
	}
	return SLVREQ_UNKNOWN_INTEGRATOR;
}

int
SolverHooks::doObserve(const ObserveRequest &request, Simulation *S){
	StoredStudyConfig &study_config = get_study_config(S);
	std::vector<Instanc> observed = request.getObserved();

	if(observed.empty()){
		return SLVREQ_OBSERVE_INVALID_REQUEST;
	}

	if(request.hasName()){
		study_config.named_observed[request.getName()] = observed;
	}else{
		study_config.default_observed = observed;
	}
	return 0;
}

int
SolverHooks::setOption(const char *optionname, Value val, Simulation *S){
	StoredFocus focus = get_focus_config(S);
	if(focus == FOCUS_SOLVER){
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
			asc_simstatus_mark_dirty(S->getInternalType());
		}
		return res;
	}else if(focus == FOCUS_INTEGRATOR){
		StoredIntegratorConfig &config = get_integrator_config(S);
		if(!config.have_integrator){
			return SLVREQ_OPTIONS_UNAVAILABLE;
		}
		try{
			S->build();
			Integrator I(*S);
			I.setEngine(config.integrator_name);
			SolverParameters pp = I.getParameters();
			int res = apply_option_to_parameters(pp, optionname, val.v);
			if(res != 0){
				return res;
			}
			I.setParameters(pp);
			remember_option(config, optionname, val.v);
			asc_simstatus_mark_dirty(S->getInternalType());
			return 0;
		}catch(std::runtime_error &){
			return SLVREQ_OPTIONS_UNAVAILABLE;
		}
	}
	return SLVREQ_OPTIONS_UNAVAILABLE;
}

int
SolverHooks::doSolve(Instance *i, Simulation *S){
	MSG("Solving model...");
	
	try{
		Instanc target(i);
		S->build(target);
		int applyres = apply_stored_solver_config(S);
		if(applyres != 0){
			return applyres;
		}
		if(!getSolverReporter()){
			MSG("Creating default SolverReporter");
			SolverReporter R;
			S->solve(S->getSolver(), R);
		}else{
			MSG("Using SolverReporter at %p",getSolverReporter());
			S->solve(S->getSolver(), *getSolverReporter());
		}
	}catch(std::runtime_error &E){
		return SLVREQ_SOLVE_FAIL;
	}

	/* solver succeeded */
	asc_simstatus_mark_clean(S->getInternalType(), i);
	return 0;
}

StudyRequest::StudyRequest()
	: observed(), have_vary(false), vary(), lower(0.0), upper(0.0), value(0.0)
	, steps(0), mode(SLVREQ_STUDY_NONE), distribution(SLVREQ_STUDY_DIST_DEFAULT)
	, run_method(), now(false), filename(){
}

StudyRequest::StudyRequest(const SlvReqStudyRequest *request)
	: observed(), have_vary(false), vary(), lower(0.0), upper(0.0), value(0.0)
	, steps(0), mode(SLVREQ_STUDY_NONE), distribution(SLVREQ_STUDY_DIST_DEFAULT)
	, run_method(), now(false), filename(){
	unsigned long i;
	if(request == NULL){
		return;
	}
	if(request->observed != NULL){
		for(i = 0; i < request->n_observed; ++i){
			if(request->observed[i] != NULL){
				observed.push_back(Instanc(request->observed[i]));
			}
		}
	}
	if(request->vary != NULL){
		have_vary = true;
		vary = Instanc(request->vary);
	}
	if(ValueKind(request->lower) == real_value){
		lower = RealValue(request->lower);
	}
	if(ValueKind(request->upper) == real_value){
		upper = RealValue(request->upper);
	}
	if(ValueKind(request->value) == real_value){
		value = RealValue(request->value);
	}
	steps = request->steps;
	mode = request->mode;
	distribution = request->distribution;
	if(request->run_method != NULL){
		run_method = request->run_method;
	}
	now = request->now ? true : false;
	if(request->filename != NULL){
		filename = request->filename;
	}
}

std::vector<Instanc>
StudyRequest::getObserved() const{
	return observed;
}

bool
StudyRequest::hasVary() const{
	return have_vary;
}

Instanc
StudyRequest::getVary() const{
	return vary;
}

double
StudyRequest::getLower() const{
	return lower;
}

double
StudyRequest::getUpper() const{
	return upper;
}

double
StudyRequest::getValue() const{
	return value;
}

long
StudyRequest::getSteps() const{
	return steps;
}

int
StudyRequest::getMode() const{
	return mode;
}

int
StudyRequest::getDistribution() const{
	return distribution;
}

bool
StudyRequest::hasRunMethod() const{
	return !run_method.empty();
}

std::string
StudyRequest::getRunMethod() const{
	return run_method;
}

bool
StudyRequest::getNow() const{
	return now;
}

bool
StudyRequest::hasFilename() const{
	return !filename.empty();
}

std::string
StudyRequest::getFilename() const{
	return filename;
}

IntegrateRequest::IntegrateRequest()
	: start(0.0), stop(0.0), steps(0){
}

IntegrateRequest::IntegrateRequest(const SlvReqIntegrateRequest *request)
	: start(0.0), stop(0.0), steps(0){
	if(request == NULL){
		return;
	}
	if(ValueKind(request->start) == real_value){
		start = RealValue(request->start);
	}
	if(ValueKind(request->stop) == real_value){
		stop = RealValue(request->stop);
	}
	steps = request->steps;
}

double
IntegrateRequest::getStart() const{
	return start;
}

double
IntegrateRequest::getStop() const{
	return stop;
}

long
IntegrateRequest::getSteps() const{
	return steps;
}

int
SolverHooks::doStudy(const StudyRequest &request, Simulation *S){
	FILE *fp = stdout;
	bool close_fp = false;
	std::vector<StudyColumn> columns;
	unsigned long i;
	int res = 0;
	std::vector<Instanc> observed = request.getObserved();
	StoredStudyConfig &study_config = get_study_config(S);

	if(observed.empty()){
		observed = study_config.default_observed;
	}

	if(observed.empty()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"STUDY requires observed variables, either explicitly or via a prior OBSERVE statement."
		);
		return SLVREQ_STUDY_INVALID_REQUEST;
	}

	if(!request.hasVary() || request.getMode() == SLVREQ_STUDY_NONE){
		if(!study_config.suppress_print){
			for(i = 0; i < observed.size(); ++i){
				if(!has_instance(study_config.print_vars, observed[i])){
					study_config.print_vars.push_back(observed[i]);
				}
			}
		}
		return 0;
	}

	study_config.print_vars.clear();
	study_config.suppress_print = true;

	if(request.hasFilename()){
		fp = fopen(request.getFilename().c_str(), "w");
		if(fp == NULL){
			return SLVREQ_STUDY_IO_ERROR;
		}
		close_fp = true;
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Writing STUDY output to '%s'.",request.getFilename().c_str());
	}

	try{
		if(request.hasVary()){
			Instanc vary = request.getVary();
			columns.push_back(get_study_column(vary.getInternalType(), S));
		}
		for(i = 0; i < observed.size(); ++i){
			if(request.hasVary() && observed[i].getInternalType() == request.getVary().getInternalType()){
				continue;
			}
			columns.push_back(get_study_column(observed[i].getInternalType(), S));
		}
		write_study_headers(fp, columns);

		{
			Instanc vary = request.getVary();
			Method run_method;
			bool have_run_method = false;

			if(request.hasRunMethod()){
				run_method = S->getType().getMethod(SymChar(request.getRunMethod().c_str()));
				have_run_method = true;
			}

			if(vary.getType().isRefinedSolverVar()){
				vary.setFixed(true);
			}

			if(request.getMode() == SLVREQ_STUDY_STEPS){
				long steps = request.getSteps();
				double lower = request.getLower();
				double upper = request.getUpper();
				for(long step = 0; step <= steps; ++step){
					double value;
					if(request.getDistribution() == SLVREQ_STUDY_DIST_LOG){
						double ratio = pow(upper / lower, 1.0 / (double)steps);
						value = lower * pow(ratio, (double)step);
					}else{
						value = lower + (upper - lower) * ((double)step / (double)steps);
					}
					if(have_run_method){
						S->run(run_method);
					}
					vary.setRealValue(value);
					res = doSolve(S->getModel().getInternalType(), S);
					if(res != 0){
						goto cleanup;
					}
					write_study_row(fp, columns);
				}
			}else if(request.getMode() == SLVREQ_STUDY_STEP){
				double value = request.getLower();
				double upper = request.getUpper();
				double delta = request.getValue();
				for(;;){
					if(have_run_method){
						S->run(run_method);
					}
					vary.setRealValue(value);
					res = doSolve(S->getModel().getInternalType(), S);
					if(res != 0){
						goto cleanup;
					}
					write_study_row(fp, columns);
					value += delta;
					if((delta > 0.0 && value > upper) || (delta < 0.0 && value < upper)){
						break;
					}
				}
			}else if(request.getMode() == SLVREQ_STUDY_RATIO){
				double value = request.getLower();
				double upper = request.getUpper();
				double ratio = request.getValue();
				for(;;){
					if(have_run_method){
						S->run(run_method);
					}
					vary.setRealValue(value);
					res = doSolve(S->getModel().getInternalType(), S);
					if(res != 0){
						goto cleanup;
					}
					write_study_row(fp, columns);
					value *= ratio;
					if((ratio > 1.0 && value > upper) || (ratio < 1.0 && value < upper)){
						break;
					}
				}
			}else{
				res = SLVREQ_STUDY_INVALID_REQUEST;
			}
		}
	}catch(std::runtime_error &){
		res = SLVREQ_STUDY_INVALID_REQUEST;
	}

cleanup:
	if(close_fp && fp != NULL){
		fclose(fp);
	}
	if(res == 0 && request.hasVary() && request.getMode() != SLVREQ_STUDY_NONE){
		asc_simstatus_mark_clean(S->getInternalType(), S->getModel().getInternalType());
	}
	return res;
}

int
SolverHooks::doIntegrate(const IntegrateRequest &request, Simulation *S){
	StoredStudyConfig &study_config = get_study_config(S);
	StoredIntegratorConfig &integrator_config = get_integrator_config(S);
	std::vector<Instanc> observed = study_config.default_observed;
	int res = 0;

	if(!integrator_config.have_integrator){
		return SLVREQ_NO_INTEGRATOR_SELECTED;
	}
	if(observed.empty()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"INTEGRATE requires observed variables via a prior OBSERVE statement."
		);
		return SLVREQ_INTEGRATE_INVALID_REQUEST;
	}
	if(request.getSteps() <= 0 || request.getStop() < request.getStart()){
		return SLVREQ_INTEGRATE_INVALID_REQUEST;
	}

	try{
		S->build();
		{
			Integrator I(*S);
			IntegratorReporterConsole reporter(&I);
			I.setEngine(integrator_config.integrator_name);
			I.clearObservedInstances();
			for(std::vector<Instanc>::const_iterator i = observed.begin(); i != observed.end(); ++i){
				I.addObservedInstance(*i);
			}
			res = apply_stored_integrator_config(I, integrator_config);
			if(res != 0){
				return res;
			}
			I.findIndependentVar();

			Instanc indep_inst = I.getIndependentVariable().getInstance();
			UnitsM bounds_units = indep_inst.getDisplayUnits(false);
			double conversion = bounds_units.getConversion();
			if(conversion == 0.0){
				conversion = 1.0;
			}

			I.setLinearTimesteps(
				bounds_units,
				request.getStart() / conversion,
				request.getStop() / conversion,
				(unsigned long)request.getSteps()
			);
			I.analyse();
			I.setReporter(&reporter);
			I.solve();
		}

		asc_simstatus_mark_clean(S->getInternalType(), S->getModel().getInternalType());
	}catch(std::runtime_error &){
		res = SLVREQ_INTEGRATE_FAIL;
	}
	return res;
}

int
SolverHooks::deleteSystem(Simulation *S){
	S->invalidateSystem();
	asc_simstatus_mark_dirty(S->getInternalType());
	return 0;
}

std::vector<Instanc>
SolverHooks::getStudyPrintVars(Simulation *S) const{
	const StoredStudyConfig &study_config = get_study_config(S);
	if(study_config.suppress_print){
		return std::vector<Instanc>();
	}
	return study_config.print_vars;
}

std::vector<Instanc>
SolverHooks::getObservedVars(Simulation *S) const{
	const StoredStudyConfig &study_config = get_study_config(S);
	return study_config.default_observed;
}

int
SolverHooks::applyIntegratorConfig(Integrator *I, Simulation *S) const{
	if(I == NULL || S == NULL){
		return SLVREQ_INTEGRATE_INVALID_REQUEST;
	}
	return apply_stored_integrator_config(*I, get_integrator_config(S));
}

void
SolverHooks::assign(Simulation *S){
	S->setSolverHooks(this);
	MSG("Assigning SolverHooks to Simulation...");
	get_study_config(S) = StoredStudyConfig();
	get_integrator_config(S) = StoredIntegratorConfig();
	get_focus_config(S) = FOCUS_NONE;
	SlvReqHooks hooks = SLVREQ_HOOKS_EMPTY;
	hooks.set_solver_fn = &ascxx_slvreq_set_solver;
	hooks.set_integrator_fn = &ascxx_slvreq_set_integrator;
	hooks.set_option_fn = &ascxx_slvreq_set_option;
	hooks.do_solve_fn = &ascxx_slvreq_do_solve;
	hooks.do_observe_fn = &ascxx_slvreq_do_observe;
	hooks.do_study_fn = &ascxx_slvreq_do_study;
	hooks.do_integrate_fn = &ascxx_slvreq_do_integrate;
	hooks.delete_system_fn = &ascxx_slvreq_delete_system;
	hooks.user_data = (void *)S;
	slvreq_assign_hooks(S->getInternalType(), &hooks);
}

SolverReporter *
SolverHooks::getSolverReporter(){
	MSG("SolverReporter is at %p", R);
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
	MSG("Creating SolverHooksManager with NULL hooks");
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
		MSG("Delete owned hooks");
		delete hooks;
	}
}

void
SolverHooksManager::setHooks(SolverHooks *H){
	MSG("Using hooks at %p",H);
	if(hooks && own_hooks){
		MSG("Deleting previous owned hooks");
		delete(hooks);
	}
	this->hooks = H;
	this->own_hooks = 0;
}

SolverHooks *
SolverHooksManager::getHooks(){
	if(this->hooks == NULL){
		MSG("Creating new default SolverHooks...");
		this->hooks = new SolverHooks();
		this->own_hooks = 1;
	}
	return this->hooks;
}
