#include "integrator.h"
#include "integratorreporter.h"

extern "C"{
#include <utilities/error.h>
#include <integrator/integrator.h>
}

#include <vector>
#include <stdexcept>
#include <iostream>
#include <iterator>
#include <sstream>
using namespace std;

//---------------------------------------------
// NULL INTEGRATOR REPORTER (makes no output at all)

IntegratorReporterNull::IntegratorReporterNull(Integrator *integrator) : IntegratorReporterCxx(integrator){
	// nothing else
}

IntegratorReporterNull::~IntegratorReporterNull(){
	// nothing else
}

int
IntegratorReporterNull::initOutput(){
	return 1;
}

int IntegratorReporterNull::closeOutput(){
	return 1;
}

int IntegratorReporterNull::updateStatus(){
	return 1;
}

int IntegratorReporterNull::recordObservedValues(){
	return 1;
}

//------------------------------------------------------------------------------
// SIMPLE CONSOLE INTEGRATOR REPORTER

IntegratorReporterConsole::IntegratorReporterConsole(Integrator *integrator)
		 : IntegratorReporterCxx(integrator), f(cout){
	// nothing else
}

IntegratorReporterConsole::~IntegratorReporterConsole(){
	// nothing else
}

int
IntegratorReporterConsole::initOutput(){
	long nobs = integrator->getNumObservedVars();
	stringstream ss;
	f << "t";
	ss<< "-------";
	for(long i=0; i<nobs; ++i){
		Variable v = integrator->getObservedVariable(i);
		f << "\t" << v.getName();
		ss<< "\t" << "-------";
	}
	f << endl;
	f << ss.str() << endl;
	return 1;
}

int IntegratorReporterConsole::closeOutput(){
	return 1;
}

int IntegratorReporterConsole::updateStatus(){
	return 1;
}

int IntegratorReporterConsole::recordObservedValues(){
	IntegratorSystem *sys = integrator->getInternalType();
	f << integrator_get_t(sys) << "\t";
	vector<double> data(integrator->getNumObservedVars());
	integrator_get_observations(sys,&data[0]);
	copy(data.begin(),data.end(),ostream_iterator<double>(f,"\t"));
	f << endl;
	return 1;
}

//----------------------------------------------------
// DEFAULT INTEGRATOR REPORTER (reporter start and end, outputs time at each step)

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
	// CONSOLE_DEBUG("...");
	double *data = ASC_NEW_ARRAY(double,integrator->getNumObservedVars());
	integrator_get_observations(integrator->getInternalType(),data);
	return 0;
}

Integrator *
IntegratorReporterCxx::getIntegrator(){
	return integrator;
}

int ascxx_integratorreporter_init(IntegratorSystem *blsys){
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
