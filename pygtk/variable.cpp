#include <iostream>
#include <stdexcept>

#include "variable.h"
#include "relation.h"
#include "simulation.h"

extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>

#include <general/dstring.h>

#include <compiler/symtab.h>
#include <compiler/instance_enum.h>
#include <compiler/instance_io.h>
}

using namespace std;

Variable::Variable(){
	sim=NULL;
	var=NULL;

	// default ctor
}

Variable::Variable(const Variable &old) : sim(old.sim), var(old.var){
	// copy ctor
}

Variable::Variable(Simulation *sim, struct var_variable *var) : sim(sim), var(var){
	if(var==NULL)throw runtime_error("Variable::Variable: var is NULL");
}

const string
Variable::getName() const{
	if(var==NULL)throw runtime_error("Variable::Variable: var is NULL");
	char *n = WriteInstanceNameString((struct Instance *)var_instance(var),sim->getModel().getInternalType());
	if(n==NULL)throw runtime_error("Variable::Variable: n is NULL");
	string name = n;
	ascfree(n);

	return name;
}

const double
Variable::getValue() const{
	return var_value(var);
}

const double
Variable::getNominal() const{
	return var_nominal(var);
}

const double
Variable::getLowerBound() const{
	return var_lower_bound(var);
}

const double
Variable::getUpperBound() const{
	return var_upper_bound(var);
}

/**
	Get the var_incidence_list for the variable in question.
	Note that this is from the solver's point of view; all sorts
	of important things might occur due to 'WHEN' sections in the model,
	and perhaps other things.

	Not clear what happens here with 'inactive' vars, need to check the
	solver-side implementation of this.
*/
const vector<Relation> 
Variable::getIncidentRelations() const{
	struct rel_relation **incid = var_incidence_list_to_modify(var);
	int n = var_n_incidences(var);
	vector<Relation> v;
	for(int i=0; i<n; ++i){
		v.push_back(Relation(sim,incid[i]));
	}
	return v;
}

const int 
Variable::getNumIncidentRelations() const{
	return var_n_incidences(var);
}

Instanc
Variable::getInstance(){
	return Instanc((struct Instance *)var_instance(var),"variablefromsolver");
}
