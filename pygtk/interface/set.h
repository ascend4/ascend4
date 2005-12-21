#ifndef ASCXX_SET_H
#define ASCXX_SET_H

#include <iostream>
#include <stdexcept>

#include "symchar.h"

extern "C"{
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "compiler/setinstval.h"
}

/**
	This C++ template defines ASCXX_Set<long> and ASCXX_Set<SymChar>
	which can hold instance-variable sets (struct set_t) (as opposed
	to 'struct Set' which is different!).

	It has to be a template because the element type is different,
	so the element accessor functions are different.

	In python, this class can be wrapped in a more elegant way, see
	ascend.i for details.
*/

class Empty{
	// empty class for use in ASCXX_Set<Empty>
};

template<class T>
class ASCXX_Set{
private:
	const struct set_t *s;
public:

	ASCXX_Set(){
		throw std::runtime_error("Can't create new ASCXX_Set objects");
	}
	
	ASCXX_Set(const struct set_t *s) : s(s){
		if(!isCorrectType()){
			throw std::runtime_error("Invalid set creation");
		}
	}

	const bool isCorrectType() const;

	unsigned long length() const{
		return Cardinality(s);
	}

	const T operator[](const unsigned long &index) const;
	inline const T at(const unsigned long &index) const{
		return (*this)[index];
	}
};

template<> 
extern const bool ASCXX_Set<long>::isCorrectType() const;

template<> 
extern const bool ASCXX_Set<SymChar>::isCorrectType() const;

template<> 
extern const bool ASCXX_Set<void>::isCorrectType() const;

template<class T>
std::ostream& operator<< (std::ostream& os, const ASCXX_Set<T>& s){
	os << "[ ";
	bool first=true;
	for(int i=1; i<= s.length(); ++i){
		if(!first)os << ", ";
		else first=false;
		os << T(s[i]);
	}
	os << "]";
}

template<>
const long ASCXX_Set<long>::operator[](const unsigned long &i) const;

template<>
const SymChar ASCXX_Set<SymChar>::operator[](const unsigned long &i) const;

template<>
const Empty ASCXX_Set<Empty>::operator[](const unsigned long &i) const;

#endif
