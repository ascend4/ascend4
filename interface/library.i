/*
	SWIG interface routines to read a file into the library
*/

%module ascend

%{

#include "library.h"
#include "type.h"

%}

class Library{
public:
	Library();
	~Library();

	void load(char *filename);
	void listModules(const int &module_type=0);
	void findType(const char *name);
};

class Type{
	const char *getName();
	const int getParameterCount();
};
