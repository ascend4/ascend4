/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef ASCXX_LIBRARY_H
#define ASCXX_LIBRARY_H

#include "extmethod.h"
#include "type.h"
#include "module.h"
#include "symchar.h"
#include "annotation.h"

#include <vector>
#include <string>

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
	void listModules(const int module_type=0);
	Type &findType(const SymChar &nametofind);
	std::vector<Module> getModules(const int module_type=0);
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

};

#endif
