#ifndef ASCXX_LIBRARY_H
#define ASCXX_LIBRARY_H

#include <vector>
#include <string>

#include "type.h"
#include "module.h"
#include "symchar.h"
#include "extmethod.h"
#include "annotation.h"

/**
	Handles the loading of ASCEND a4c files into memory, then the
	listing of the contents of those loaded files. Creates output
	when loaded files contain errors, although a standardised
	method for reporting errors is desired for reporting back
	via SWIG.
*/
class Library{
public:
	Library(const char *defaultpath=NULL);
	~Library();
	void load(const char *filename);
	void listModules(const int &module_type=0) const;
	Type &findType(const SymChar &nametofind);
	std::vector<Module> getModules();
	std::vector<Type> getModuleTypes(const Module&);

	// External Function library
	std::vector<ExtMethod> getExtMethods();
	static void extMethodTraverse(void *,void *);
	void appendToExtMethodVector(void *);

	// Destroy types from the library
	void clear();

	AnnotationDatabase getAnnotationDatabase();

private:
	std::vector<ExtMethod> extmethod_vector;

	static void displayModule(void *m);
};

#endif
