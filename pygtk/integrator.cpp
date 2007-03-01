#include "integrator.h"
#include "integratorreporter.h"
#include "solverparameters.h"
#include <stdexcept>
#include <sstream>
#include <cmath>
using namespace std;

// #define DESTROY_DEBUG

/**
	'creating' an integrator in the context of the GUI just means an object
	we can store the parameters that will be later sent to the underlying
	C-code API.

	@TODO at present the integrator requires a slv_system_t. This is the wrong
	way around.
*/
Integrator::Integrator(Simulation &simulation)
		: simulation(simulation)
{
	// create the C-level object
	this->blsys = integrator_new(simulation.getSystem(),simulation.getModel().getInternalType());

	samplelist = NULL;

	// set default steps
	setMinSubStep(0);
	setMaxSubStep(0);
	setMaxSubSteps(0);
	setInitialSubStep(0);
}

Integrator::~Integrator(){
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("DESTROYING Integrator (C++) at %p",this);
	CONSOLE_DEBUG("DESTROYING IntegratorSystem at %p",blsys);
#endif
	integrator_free(blsys);
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("done");
	CONSOLE_DEBUG("DESTROYING samplelist at %p",samplelist);
#endif
	if(samplelist)samplelist_free(samplelist);
}

SolverParameters
Integrator::getParameters() const{
	SolverParameters params;
	int res = integrator_params_get(blsys,&(params.getInternalType() ) );
	if(res)throw runtime_error("Failed to get integrator parameters");
	return params;
}

void
Integrator::setParameters(const SolverParameters &params){
	int res = integrator_params_set(blsys,&(params.getInternalTypeConst() ) );
	if(res)throw runtime_error("Failed to set integrator parameters");
}

void
Integrator::setReporter(IntegratorReporterCxx *reporter){
	this->blsys->clientdata = reporter;
	integrator_set_reporter(blsys,reporter->getInternalType());
	//CONSOLE_DEBUG("REPORTER HAS BEEN SET");
	(*(this->blsys->reporter->init))(blsys);
	//CONSOLE_DEBUG("DONE TESTING OUTPUT_INIT");
}

double
Integrator::getCurrentTime(){
	return integrator_get_t(blsys);
}

long
Integrator::getCurrentStep(){
	return integrator_getcurrentstep(blsys);
}

long
Integrator::getNumSteps(){
	return integrator_getnsamples(blsys);
}

/**
	Find the independent variable in the system, or throw an exception if not found.
*/
void
Integrator::findIndependentVar(){
	int res = integrator_find_indep_var(blsys);

	if(res){
		stringstream ss;
		ss << "Independent variable not found (" << res << ")";
		throw runtime_error(ss.str());
	}
}

void
Integrator::analyse(){

	int res;
	/*
		Note, we never need to call analyze_make_system in any of the Integrator
		code, as it gets called by Simulation::build.
	*/
	res = integrator_analyse(blsys);

	if(res){
		stringstream ss; ss << "Failed system analysis (error " << res << ")";
		throw runtime_error(ss.str());
	}
}

/**
	@TODO what about root detection?

	Integrate the function for the timesteps specified.

	Method will throw a runtime_error if integrator_solve returns error (non zero)

	@TODO does simulation.processVarStatus work for integrators like IDA???
*/
void
Integrator::solve(){

	// check the integration limits
	// trigger of the solution process
	// report errors?

	assert(samplelist!=NULL);
	assert(samplelist->ns>0);
	assert(blsys->reporter!=NULL);
	assert(blsys->clientdata!=NULL);

	int res;
	res = integrator_solve(blsys, 0, samplelist_length(samplelist)-1);

	if(res){
		stringstream ss;
		ss << "Failed integration (integrator_solve returned " << res << ")";
		throw runtime_error(ss.str());
	}

	// communicate solver variable status back to the instance tree via 'interface_ptr'
	simulation.processVarStatus();
}

void
Integrator::writeMatrix(FILE *fp) const{
	if(integrator_write_matrix(this->blsys, fp)){
		throw runtime_error("Failed to write matrix");
	}
}

void
Integrator::writeDebug(FILE *fp) const{
	if(integrator_debug(this->blsys, fp)){
		throw runtime_error("Failed to write debug output");
	}
}

void
Integrator::setEngine(IntegratorEngine engine){
	int res = integrator_set_engine(this->blsys, engine);
	if(!res)return;
	if(res==1)throw range_error("Unknown integrator");
	if(res==2)throw range_error("Invalid integrator");
	stringstream ss;
	ss << "Unknown error in setEngine (res = " << res << ")";
	throw runtime_error(ss.str());
}

void
Integrator::setEngine(int engine){
	setEngine((IntegratorEngine)engine);
}

void
Integrator::setEngine(const string &name){
	CONSOLE_DEBUG("Setting integration engine to '%s'",name.c_str());
	IntegratorEngine engine = INTEG_UNKNOWN;
#ifdef ASC_WITH_LSODE
	if(name=="LSODE")engine = INTEG_LSODE;
#endif
#ifdef ASC_WITH_IDA
	if(name=="IDA")engine = INTEG_IDA;
#endif
	if(engine==INTEG_UNKNOWN){
		throw runtime_error("Unkown integrator name");
	}
	setEngine(engine);
}

/**
	Ideally this list would be dynamically generated based on what solvers
	are available or are in memory.
*/
map<int,string>
Integrator::getEngines(){
	map<int,string> m;
	const IntegratorLookup *list = integrator_get_engines();
	while(list->id != INTEG_UNKNOWN){
		if(list->name==NULL)throw runtime_error("list->name is NULL");
		m.insert(pair<int,string>(list->id,list->name));
		++list;
	}
	return m;
}

string
Integrator::getName() const{
	map<int,string> m=getEngines();
	map<int,string>::iterator f = m.find(integrator_get_engine(blsys));
	if(f==m.end()){
		throw runtime_error("No engine selected");
	}
	return f->second;
}

/**
	@TODO what about conversion factors? Is an allowance being made?
*/
void
Integrator::setLinearTimesteps(UnitsM units, double start, double end, unsigned long num){
	if(samplelist!=NULL){
		ASC_FREE(samplelist);
	}
	const dim_type *d = units.getDimensions().getInternalType();
	samplelist = samplelist_new(num+1, d);
	double val = start;
	double inc = (end-start)/(num);
	for(unsigned long i=0;i<=num;++i){
		samplelist_set(samplelist,i,val);
		val += inc;
	}
	integrator_set_samples(blsys,samplelist);
}

/**
	@TODO what about conversion factors? Is an allowance being made?
*/
void
Integrator::setLogTimesteps(UnitsM units, double start, double end, unsigned long num){
	if(samplelist!=NULL){
		ASC_FREE(samplelist);
	}
	const dim_type *d = units.getDimensions().getInternalType();

	if(start<=0)throw runtime_error("starting timestep needs to be > 0");
	if(end<=0)throw runtime_error("end timestep needs to be > 0");
	if(end <= start)throw runtime_error("end timestep needs to be > starting timestep");

	samplelist = samplelist_new(num+1, d);
	double val = start;
	double inc = exp((log(end)-log(start))/num);
	for(unsigned long i=0;i<=num;++i){
		samplelist_set(samplelist,i,val);
		CONSOLE_DEBUG("samplelist[%lu] = %f",i,val);
		val *= inc;
	}
	integrator_set_samples(blsys,samplelist);
}

vector<double>
Integrator::getCurrentObservations(){
	double *d = ASC_NEW_ARRAY(double,getNumObservedVars());
	integrator_get_observations(blsys,d);
	vector<double> v=vector<double>(d,d+getNumObservedVars());
	// do I need to free d?
	// can I do this in such a way as I avoid all this memory-copying?
	return v;
}

Variable
Integrator::getObservedVariable(const long &i){
	var_variable *v = integrator_get_observed_var(blsys,i);
	return Variable(&simulation,v);
}

Variable
Integrator::getIndependentVariable(){
	var_variable *v = integrator_get_independent_var(blsys);
	if(v==NULL){
		throw runtime_error("independent variable is null");
	}
	return Variable(&simulation,v);
}

int
Integrator::getNumVars(){
	return blsys->n_y;
}

int
Integrator::getNumObservedVars(){
	return blsys->n_obs;
}

void
Integrator::setMinSubStep(double n){
	integrator_set_minstep(blsys,n);
}

void
Integrator::setMaxSubStep(double n){
	integrator_set_maxstep(blsys,n);
}

void
Integrator::setInitialSubStep(double n){
	integrator_set_stepzero(blsys,n);
}

void
Integrator::setMaxSubSteps(int n){
	integrator_set_maxsubsteps(blsys,n);
}

IntegratorSystem *
Integrator::getInternalType(){
	return blsys;
}
