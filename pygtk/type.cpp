#include <Python.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
using namespace std;

extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <general/dstring.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instantiate.h>
#include <compiler/bintoken.h>
#include <utilities/readln.h>
#include <solver/mtx.h>
#include <solver/slv_types.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/discrete.h>
#include <solver/conditional.h>
#include <solver/logrel.h>
#include <solver/bnd.h>
#include <solver/calc.h>
#include <solver/relman.h>
#include <solver/slv_common.h>
#include <solver/linsol.h>
#include <solver/linsolqr.h>
#include <solver/slv_client.h>
#include <solver/system.h>
#include <solver/slv_interface.h>
#include <compiler/simlist.h>
}

#include "type.h"
#include "simulation.h"
#include "library.h"
#include "dimensions.h"
#include "name.h"

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
	Instantiate a type. This expensive: it will compile your
	model as C-code and load a dynamic library with native
	machine-code versions of your equations.

	Once you have an instance of your model, you can start
	to eliminate variables and attempt to solve it, see Instanc.

	Note that there is some kind of dastardly underhand reference to the
	Compiler class implicit here: the model instantiation call refers to
	g_use_copyanon which is sort-of owned by the Compiler class.
*/
Simulation
Type::getSimulation(const SymChar &sym, const bool &rundefaultmethod){
	static bool have_bintoken_setup;
	static string bin_targetstem;
	static string bin_srcname;
	static string bin_objname;
	static string bin_libname;
	static string bin_cmd;
	static string bin_rm;

	//CONSOLE_DEBUG("Type %s, getSimulation('%s')",getName().toString(), sym.toString());

	// Tell ASCEND file locations and compiler commands:
	if(0 && !have_bintoken_setup){
		CONSOLE_DEBUG("SETUP BINTOKENS...");

/*
	THE FOLLOWING STUFF IS CURRENTLY BROKEN.
	The reason is that we need to add configuration options with SCons that
	pass in the correct values here. These values need to correspond with 
	the *installed* location of things like 'Makefile.bt' (if that is what is
	to be used, that is), where the ASCEND libs are, etc, whatever.
*/
//		bin_targetstem = ASCEND_TMPDIR "/asc_bintoken";
		bin_targetstem = "/tmp/asc_bintoken";
		bin_srcname = bin_targetstem + ".c";
		bin_objname = bin_targetstem + ".o";
		bin_libname = bin_targetstem + ".so";
		bin_rm = "/bin/rm";
#if 0
/*		bin_cmd = "make -C " ASCEND_TMPDIR " -f " ASCEND_MAKEFILEDIR_1 "/Makefile.bt" \
			" SO=" + bin_targetstem + " ASCEND_INCDIR=\"" ASCEND_INCDIR "\" ASCEND_LIBDIR=\"" ASCEND_LIBDIR "\"";

		cerr << "BINTOKEN COMMAND" << endl << "----" << bin_cmd << endl << "----" << endl;
*/
#elif 0
/*
		# define BTINCLUDES "-I" ASCEND_INCDIR
		bin_cmd = "cd " ASCEND_INCDIR " && make BTTARGET=" + bin_targetstem + " BTINCLUDES=" BTINCLUDES \
			" -f " ASCEND_MAKEFILEDIR "/Makefile.bt " + bin_targetstem;
*/
#else
		bin_cmd = "echo \"FIXME: BINTOKEN COMMANDLINE PARAMETERS\" && exit 1";
#endif
		BinTokenSetOptions(bin_srcname.c_str(), bin_objname.c_str(), bin_libname.c_str()
							, bin_cmd.c_str(), bin_rm.c_str(), 1000, 1, 0);

		CONSOLE_DEBUG("...SETUP BINTOKENS");
		have_bintoken_setup = true;
	}

	//cerr << "CREATING INSTANCE..." << endl;
	// Perform the instantiation (C compile etc):
	/*Instance *i = Instantiate(getInternalType()->name, sym.getInternalType(),
								 0, SymChar("on_load").getInternalType()); */
	Instance *i = SimsCreateInstance(getInternalType()->name, sym.getInternalType(), e_normal, NULL);
	Simulation sim(i,sym);

	if(i==NULL){
		throw runtime_error("Failed to create instance");
	}

	if(rundefaultmethod){
		CONSOLE_DEBUG("RUNNING DEFAULT METHOD");
		sim.runDefaultMethod();
	}

	//cerr << "CREATED INSTANCE " << sym << " OF " << getName() << endl;
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
	struct gl_list_t *l = GetInitializationList(getInternalType());
	if(l==NULL){
		stringstream ss;
		ss << "No methods present in type '" << getName() << "'";
		throw runtime_error(ss.str());
	}
	struct InitProcedure *m;
	m = SearchProcList(l,name.getInternalType());
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
	static const TypeDescription *solver_var_type;
	if(!solver_var_type){
		Type t1 = Library().findType(SymChar("solver_var"));
		solver_var_type=t1.getInternalType();
	}
	if(MoreRefined(t, solver_var_type)==t){
		//cerr << getName() << " IS A REFINED SOLVER_VAR" << endl;
		return true;
	}
	//cerr << getName() << "IS *NOT* A REFINED SOLVER_VAR" << endl;
	return false;
}


const bool
Type::hasParameters() const{
	return TypeHasParameterizedInsts(t);
}
