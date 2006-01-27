#ifndef ASCXX_SOLVERPARAMETERITERATOR_H
#define ASCXX_SOLVERPARAMETERITERATOR_H

#include "solverparameters.h"

#include <iostream>

class SolverParameterIterator{
private:
	slv_parameter *p;
protected:	
	friend class SolverParameters;
	explicit SolverParameterIterator(slv_parameter *p);
public:
	explicit SolverParameterIterator();

	void operator++();
	void operator=(const SolverParameterIterator &);

	const bool operator<(const SolverParameterIterator &) const;
	SolverParameter operator*() const;

	friend std::ostream &operator<<( std::ostream &outs, const SolverParameterIterator &I);
};


#endif // ASCXX_SOLVERPARAMETERITERATOR_H
