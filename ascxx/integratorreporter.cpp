#include "integrator.h"
#include "integratorreporter.h"

extern "C"{
#include <ascend/utilities/error.h>
#include <ascend/integrator/integrator.h>
}

#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>
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
	long nobs = integrator->getNumObservedItems();
	stringstream ss;
	Variable indep = integrator->getIndependentVariable();
	UnitsM indep_units = indep.getInstance().getDisplayUnits(false);
	string indep_label = indep.getName();
	string indep_units_name = indep_units.getName().toString();
	bool indep_show_units = !indep_units_name.empty() && indep_units_name != "1";
	if(indep_units.getDimensions().isWild() && indep.getInstance().isDimensionless()){
		indep_show_units = false;
	}
	if(indep_show_units){
		indep_label += " [" + indep_units_name + "]";
	}
	f << setw(20) << right << indep_label;
	ss << setw(20)<< right << "--------------------";
	for(long i=0; i<nobs; ++i){
		Instanc inst = integrator->getObservedInstance(i);
		string label = integrator->simulation.getInstanceName(inst);
		if(inst.isReal()){
			UnitsM units = inst.getDisplayUnits(false);
			string units_name = units.getName().toString();
			bool show_units = !units_name.empty() && units_name != "1";
			if(units.getDimensions().isWild() && inst.isDimensionless()){
				show_units = false;
			}
			if(show_units){
				label += " [" + units_name + "]";
			}
		}
		f << "  " << setw(20) << right << label;
		ss<< "  " << setw(20) << right << "--------------------";
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
	Variable indep = integrator->getIndependentVariable();
	UnitsM indep_units = indep.getInstance().getDisplayUnits(false);
	double indep_value = integrator_get_t(sys) / indep_units.getConversion();
	f << setw(20) << indep_value;
	for(long j = 0; j < integrator->getNumObservedItems(); ++j){
		Instanc inst = integrator->getObservedInstance(j);
		string value;
		if(inst.isAssigned()){
			if(inst.isReal()){
				UnitsM units = inst.getDisplayUnits(false);
				double conversion = units.getConversion();
				if(conversion == 0.0){
					conversion = 1.0;
				}
				stringstream ss;
				ss << (inst.getRealValue() / conversion);
				value = ss.str();
			}else if(inst.isBool()){
				value = inst.getBoolValue() ? "TRUE" : "FALSE";
			}else if(inst.isInt()){
				stringstream ss;
				ss << inst.getIntValue();
				value = ss.str();
			}else if(inst.isSymbol()){
				value = string("'") + inst.getSymbolValue().toString() + "'";
			}else{
				value = inst.getValueAsString();
			}
		}else{
			value = "undefined";
		}
		f << "  " << setw(20) << value;
	}
	if(integrator->getNumObservedVars() > 0){
		integrator->saveObservations();
	}
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
	integrator->saveObservations();
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
