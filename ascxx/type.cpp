/*	ASCEND modelling environment
	Copyright (C) 2006-2010 Carnegie Mellon University

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
#include <Python.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
using namespace std;

extern "C"{
#include <ascend/general/platform.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/dstring.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/fractions.h>

#include <ascend/compiler/dimen.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/bintoken.h>
#include <ascend/compiler/library.h>
#include <ascend/linear/mtx.h>
#include <ascend/system/calc.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/system.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/child.h>
}

#include "type.h"
#include "simulation.h"
#include "library.h"
#include "dimensions.h"
#include "name.h"
#include "compiler.h"

/**
	@TODO FIXME for some reason there are a lot of empty Type objects being created
*/
Type::Type(){
	//cerr << "CREATED EMPTY TYPE" << endl;
	// throw runtime_error("Type::Type: Can't create new Types via C++ interface");
}

Type::Type(const TypeDescription *t) : t(t){
	//cerr << "CREATED TYPE '" << getName() << "'" << endl;
}

const SymChar
Type::getName() const{
	if(t==NULL){
		throw runtime_error("Type::getName: t is NULL");
	}
	switch(GetBaseType(t)){
		case array_type:
			return SymChar("array");
		case relation_type:
			return SymChar("relation");
		case logrel_type:
			return SymChar("logrel");
		case when_type:
			return SymChar("when");		
		case set_type:
			return SymChar("set");
		default:
			symchar *sym = GetName(t);
			if(sym==NULL){
				throw runtime_error("Unnamed type");
			}
			return SymChar(SCP(sym));
	}
}

const int
Type::getParameterCount() const{
	return GetModelParameterCount(t);
}

const TypeDescription *
Type::getInternalType() const{
	return t;
}

const Dimensions
Type::getDimensions() const{
	if( isRefinedConstant() ){
		return Dimensions( GetConstantDimens(getInternalType()) );
	}else if( isRefinedReal() ){
		return Dimensions( GetRealDimens(getInternalType()) );
	}else{
	if( !isRefinedAtom() )throw runtime_error("Type::getDimensions: called with non-atom type");
		throw runtime_error("Type::getDimensions: unrecognised type");
	}
}

const bool
Type::isRefinedAtom() const{
	return BaseTypeIsAtomic(t);
}

const bool
Type::isRefinedReal() const{
	return BaseTypeIsReal(t);
}

const bool
Type::isRefinedConstant() const{
	return BaseTypeIsConstant(t);
}

/**
	Instantiate a type. This *can be* expensive, if you have selected to 
	compile your model into C-code and load a dynamic library with native
	machine-code versions of your equations.

	Once you have an instance of your model, you can start
	to eliminate variables and attempt to solve it, see Instanc.

	Note that there is some kind of dastardly underhand reference to the
	Compiler class implicit here: the model instantiation call refers to
	g_use_copyanon which is sort-of owned by the Compiler class.
*/
Simulation
Type::getSimulation(const SymChar &sym
		, const bool rundefaultmethod
){
	/* notify the compiler of our bintoken options, if nec */
	Compiler::instance()->sendBinaryCompilationOptions();

	/* removing the following line causes a crash on Windows 7 */
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Starting tree...\n");
#if 1
	error_reporter_tree_start();
#endif
	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Started tree\n"); */

	Instance *i = SimsCreateInstance(getInternalType()->name, sym.getInternalType(), e_normal, NULL);
	Simulation sim(i,sym);

	bool has_error = FALSE;
#if 1
	if(error_reporter_tree_has_error()){
		has_error = TRUE;
	}

	error_reporter_tree_end();
#endif

	if(has_error){
		stringstream ss;
		ss << "Error(s) during instantiation of type '" << getName() << "'";
		throw runtime_error(ss.str());
	}else{
		ERROR_REPORTER_HERE(ASC_USER_NOTE,"Instantiated %s",SCP(getInternalType()->name));
	}

	if(i==NULL){
		throw runtime_error("Failed to create instance");
	}

	if(rundefaultmethod){
		//CONSOLE_DEBUG("RUNNING DEFAULT METHOD");
		sim.runDefaultMethod();
	}

	return sim;
}

vector<Method>
Type::getMethods() const{
	vector<Method> v;
	struct gl_list_t *l = GetInitializationList(getInternalType());
	if(l==NULL) return v;
	for(int i=1, end=gl_length(l); i<=end; ++i){
		v.push_back(Method((struct InitProcedure *)gl_fetch(l,i)));
	}
	return v;
}

Method
Type::getMethod(const SymChar &name) const{
	if(GetBaseType(t)!=model_type){
		stringstream ss;
		ss << "Type '" << getName() << "' is not a MODEL";
		throw runtime_error(ss.str());
	}

	struct InitProcedure *m;
	m = FindMethod(t,name.getInternalType());

	if(m==NULL){
		stringstream ss;
		ss << "No method named '" << name << "' in type '" << getName() << "'";
		throw runtime_error(ss.str());
		return NULL;
	}

	return Method(m);
}	

const bool
Type::isRefinedSolverVar() const{
	const TypeDescription *solver_var_type;
	Type t1 = Library().findType(SymChar("solver_var"));
	solver_var_type=t1.getInternalType();

	if(MoreRefined(t, solver_var_type)==t){
		//cerr << getName() << " IS A REFINED SOLVER_VAR" << endl;
		return true;
	}
	//cerr << getName() << "IS *NOT* A REFINED SOLVER_VAR" << endl;
	return false;
}

const bool
Type::isFundamental() const{
	return CheckFundamental(getName().getInternalType());
}

const bool
Type::isModel() const{
	return GetBaseType(t) == model_type;
}

const bool
Type::hasParameters() const{
	return GetModelParameterCount(t) > 0;
}

bool
Type::operator<(const Type &other) const{
	// modelled on the Unit_CmpAtomName function from UnitsProc.c in Tcl code...
	if(!getInternalType() || !other.getInternalType() || this->isFundamental()){
		return false;
	}
	return (this->getName() < other.getName());
}

Module
Type::getModule() const{
	return GetModule(getInternalType());
}

const Type &
Type::findMember(const SymChar &name){
	
	unsigned long pos;
	ChildListPtr CL;

	CL = GetChildList(t);
	pos = ChildPos(CL,name.getInternalType());
	
  unsigned long clsize = ChildListLen(CL);
  
  if((pos<1) || (pos>clsize))
  {
    stringstream ss;
		ss << "Library::findType: type '" << name << "' not found in library";
		throw runtime_error(ss.str());
  }
	
	const TypeDescription *t = ChildBaseTypePtr(CL,pos);

	if(t==NULL){
		stringstream ss;
		ss << "Library::findType: type '" << name << "' not found in library";
		throw runtime_error(ss.str());
	}
	Type *t2=new Type(t);
	return *t2;
}
