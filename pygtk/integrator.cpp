#include "integrator.h"
#include "integratorreporter.h"
#include <stdexcept>
using namespace std;

/**
	'creating' an integrator in the context of the GUI just means an object
	we can store the parameters that will be later sent to the underlying
	C-code API.
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
	integrator_free(blsys);
	samplelist_free(samplelist);
}


void
Integrator::setReporter(IntegratorReporterCxx *reporter){
	this->blsys->clientdata = reporter;
	integrator_set_reporter(blsys,reporter->getInternalType());
	CONSOLE_DEBUG("REPORTER HAS BEEN SET");	
	(*(this->blsys->reporter->init))(blsys);
	CONSOLE_DEBUG("DONE TESTING OUTPUT_INIT");
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

int 
Integrator::findIndependentVar(){
	return integrator_find_indep_var(blsys);
}

int
Integrator::analyse(){

	int res;
	res = integrator_analyse(blsys);

	if(!res){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Failed system analysis");
		return 0;
	}

	return 1;
}

/**
	@TODO what about root detection?
*/
int
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
	if(!res){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Failed integration");
		return 0;
	}

	return 1;
}

int
Integrator::setEngine(IntegratorEngine engine){
	return integrator_set_engine(this->blsys, engine);
}

int
Integrator::setEngine(int engine){
	return integrator_set_engine(this->blsys, (IntegratorEngine)engine);
}

/**
	Ideally this list would be dynamically generated based on what solvers
	are available or are in memory.
*/
map<int,string>
Integrator::getEngines() const{
	map<int,string> m;
#ifdef ASC_WITH_LSODE
	m.insert(pair<int,string>(INTEG_LSODE,"LSODE"));
#endif
#ifdef ASC_WITH_IDA
	m.insert(pair<int,string>(INTEG_IDA,"IDA"));
#endif
	return m;
}

string
Integrator::getEngineName() const{
	map<int,string> m=getEngines();
	map<int,string>::iterator f = m.find(integrator_get_engine(blsys));
	if(f==m.end()){
		throw runtime_error("No engine selected");
	}
	return f->second;
}		

void
Integrator::setLinearTimesteps(UnitsM units, double start, double end, unsigned long num){
	if(samplelist!=NULL){
		ASC_FREE(samplelist);
	}
	const dim_type *d = units.getDimensions().getInternalType();
	samplelist = samplelist_new(num+1, d);
	double val = start;
	double inc = (end-start)/(num);
	for(int i=0;i<=num;++i){
		samplelist_set(samplelist,i,val);
		val += inc;
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
