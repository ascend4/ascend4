#include "integrator.h"
#include "integratorreporter.h"

extern "C"{
#include <utilities/error.h>
#include <solver/integrator.h>
}

#include <stdexcept>
using namespace std;

IntegratorReporterCxx::IntegratorReporterCxx(Integrator *integrator){
	// Initialise the C-API structure with flat function pointers
	reporter.init = &ascxx_integratorreporter_init;
	reporter.write = &ascxx_integratorreporter_write;
	reporter.write_obs = &ascxx_integratorreporter_write_obs;
	reporter.close = &ascxx_integratorreporter_close;
	this->integrator=integrator;
}

IntegratorReporterCxx::~IntegratorReporterCxx(){
	// nothing, just virtual destructor
	CONSOLE_DEBUG("DESTROYING INTEGRATOR REPORTER CXX");
}

IntegratorReporter *
IntegratorReporterCxx::getInternalType(){
	return &reporter;
}

int
IntegratorReporterCxx::initOutput(){
	return ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Starting integration reporting...");
}

int
IntegratorReporterCxx::closeOutput(){
	return ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Closing integration reporting...");
}

int
IntegratorReporterCxx::updateStatus(){
	double t = integrator->getCurrentTime();
	return ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"t = %f",t);
	
}

int
IntegratorReporterCxx::recordObservedValues(){
	CONSOLE_DEBUG("...");
	double *data = ASC_NEW_ARRAY(double,integrator->getNumObservedVars());
	integrator_get_observations(integrator->getInternalType(),data);
	return 0;
}

Integrator *
IntegratorReporterCxx::getIntegrator(){
	return integrator;
}

int ascxx_integratorreporter_init(IntegratorSystem *blsys){
	CONSOLE_DEBUG("...");
	IntegratorReporterCxx *r = (IntegratorReporterCxx *)blsys->clientdata;
	if(r==NULL){
		throw runtime_error("blsys->clientdata was null");
	}
	return r->initOutput();
}

int ascxx_integratorreporter_write(IntegratorSystem *blsys){
	IntegratorReporterCxx *r = (IntegratorReporterCxx *)blsys->clientdata;
	return r->updateStatus();
}

int ascxx_integratorreporter_write_obs(IntegratorSystem *blsys){
	IntegratorReporterCxx *r = (IntegratorReporterCxx *)blsys->clientdata;
	return r->recordObservedValues();
}

int ascxx_integratorreporter_close(IntegratorSystem *blsys){
	IntegratorReporterCxx *r = (IntegratorReporterCxx *)blsys->clientdata;
	return r->closeOutput();
}
