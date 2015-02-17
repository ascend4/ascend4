#include "ascxxset.h"

SetInt::SetInt()
{}
SetInt::~SetInt()
{}
SetString::SetString()
{}
SetString::~SetString()
{}

unsigned long SetInt::length() const
{
	return ASCXX_Set<long>::length();
}
inline const long SetInt::at(const unsigned long &index) const
{
	return ASCXX_Set<long>::at(index);
}
long SetInt::__getitem__(int i)
{
	return at(i);
}

unsigned long SetString::length() const
{
	return ASCXX_Set<SymChar>::length();
}
inline const SymChar SetString::at(const unsigned long &index) const
{
	return ASCXX_Set<SymChar>::at(index);
}
SymChar SetString::__getitem__(int i)
{
	return at(i);
}
