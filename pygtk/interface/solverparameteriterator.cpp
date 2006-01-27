#include "solverparameteriterator.h"

#include <iostream>
using namespace std;

SolverParameterIterator::SolverParameterIterator(slv_parameter *p) : p(p){
	// create iterator, protected function
}

SolverParameterIterator::SolverParameterIterator(){
	// default ctor
}

void
SolverParameterIterator::operator=(const SolverParameterIterator &old){
	p = old.p;
}

void
SolverParameterIterator::operator++(){
	cerr << "INCREMENT P" << endl;
	p++;
}

const bool
SolverParameterIterator::operator<(const SolverParameterIterator &I2) const{
	return p < I2.p;
}

SolverParameter 
SolverParameterIterator::operator*() const{
	return SolverParameter(p);
}

ostream &operator<<( ostream &outs, const SolverParameterIterator &I){
	outs << I.p;
	return outs;
}
