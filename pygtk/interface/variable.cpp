#include <iostream>
#include <stdexcept>
using namespace std;

#include "variable.h"

Variable::Variable(){
	throw runtime_error("Can't create new Variable objects");
}

Variable::Variable(slv_system_t s, struct var_variable *var) : s(s), var(var){
	//cerr << "CREATED VARIABLE" << endl;
	char *n=var_make_name(s,var);
	name=n;
	delete n;
}

Variable::~Variable(){
	//cerr << "DESTROYED VARIABLE" << endl;
}

const string &
Variable::getName(){
	return name;
}
