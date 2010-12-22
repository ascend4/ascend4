#include <iostream>
#include <stdexcept>
using namespace std;

#include "set.h"

template<>
const bool
ASCXX_Set<long>::isCorrectType() const{
	return (SetKind(s)==integer_set);
}

template<>
const bool
ASCXX_Set<SymChar>::isCorrectType() const{
	return (SetKind(s)==string_set);
}


template<>
const bool
ASCXX_Set<Empty>::isCorrectType() const{
	return (SetKind(s)==string_set);
}


template<>
const long
ASCXX_Set<long>::operator[](const unsigned long &i) const{
	return FetchIntMember(s,i);
}

template<>
const SymChar
ASCXX_Set<SymChar>::operator[](const unsigned long &i) const{
	return SymChar(SCP(FetchStrMember(s,i)));
}

template<>
const Empty
ASCXX_Set<Empty>::operator[](const unsigned long &i) const{
	throw runtime_error("Invalid attempt to retreive item from empty set");
}


