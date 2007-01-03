#include <Python.h>

#include <iostream>
#include <stdexcept>
#include <sstream>
using namespace std;

#undef NDEBUG

#include "config.h"

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <general/list.h>
#include <compiler/slist.h>
#include <compiler/ascCompiler.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
/* #include <compiler/redirectFile.h> */
#include <compiler/module.h>
#include <compiler/prototype.h>
#include <compiler/dump.h>
#include <compiler/dimen.h>
#include <compiler/child.h>
#include <compiler/childio.h>
#include <compiler/type_desc.h>
#include <compiler/typedef.h>
#include <compiler/library.h>
#include <compiler/childinfo.h>
#include <solver/slv_types.h>
#include <solver/system.h>
#include <utilities/ascEnvVar.h>
#include <compiler/symtab.h>
#include <general/table.h>
#include <compiler/instance_enum.h>
#include <compiler/notate.h>
#include <compiler/simlist.h>
#include <compiler/parser.h>
#include <utilities/error.h>
#include <general/env.h>
#include <compiler/importhandler.h>
}

#include "library.h"
#include "simulation.h"
#include "solver.h"

Library::Library(const char *defaultpath){
	static int have_init;
	if(!have_init){
		cerr << "Initialising ASCEND library..." << endl;

#ifdef REIMPLEMENT_STREAMS
		Asc_RedirectCompilerDefault(); // Ensure that error message reach stderr
#endif

		Asc_CompilerInit(1);
		env_import(PATHENVIRONMENTVAR,getenv,Asc_PutEnv);
		char *x = Asc_GetEnv(PATHENVIRONMENTVAR);
		if(x==NULL || strcmp(x,"")==0){
			if(defaultpath==NULL){
				ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Using default "
					PATHENVIRONMENTVAR " = '" DEFAULT_ASCENDLIBRARY "'"
				);
				defaultpath = DEFAULT_ASCENDLIBRARY;
			}
			
			string s = string(PATHENVIRONMENTVAR "=") + defaultpath;
			//ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Setting %s",s.c_str());;
			Asc_PutEnv(s.c_str());
		}
		Asc_ImportPathList(PATHENVIRONMENTVAR);
		//cerr << PATHENVIRONMENTVAR << " = " << x << endl;
		//cerr << "Created LIBRARY" << endl;
		//cerr << "Registering solvers..." << endl;
		registerStandardSolvers();
	}/*else{
		CONSOLE_DEBUG("Reusing LIBRARY");
	}*/
	have_init=1;
}

Library::~Library(){
	//ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"DESTROYED LIBRARY!");
	//DestroyLibrary();
	// ... need to use some kind of reference counting before you can do that...
}

/**
	Load an ASCEND model file into the Library. It will be parsed such that
	its types will be visible to Library::findType.

	@param filename Filename, will be searched for relative to ASCENDLIBRARY environment
		variable, if necessary.
*/
void
Library::load(const char *filename){

	//std::cerr << "Loading '" << filename << "'" << std::endl;

	int status;
	struct module_t *m=Asc_RequireModule(filename,&status);
	if(m!=NULL){
		//std::cerr << "Loaded module '" << Asc_ModuleName(m) << "'" << std::endl;
	}else{
		std::cerr << "Error: unable to load module '" << filename << "'." << std::endl;
	}

	char *msg;
	switch(status){
		case 5:
			msg = "The module '%s' already exists. "; break;
		case 4:
			msg = "Caught an attempt to do a recursive require under '%s'."; break;
		case 3:
			msg = "A new module was created from '%s', overwriting a module's alias."; break;
		case 2:
			msg = "An existing module is being returned for '%s'." ; break;
		case 1:
			msg = "An new version of an existing module was created for '%s'."; break;
		case 0:
			msg = "Module for '%s' created OK."; break;
		case -1:
			msg = "Error: File not found for '%s'. (-1)"; break;
		case -2:
			msg = "Error: Unable to open '%s' for reading. (-2)";break;
		case -3:
			msg = "Error: Insuffient memory to create module for '%s'. (-3)"; break;
		case -4:
			msg = "Error: bad input, null or zero length filename in '%s'. (-4)"; break;
		default:
			throw std::runtime_error("Invalid status code in library.cpp");
	}

	char msg1[100];
	sprintf(msg1,msg,filename);

	if(status<0 || status>0){
		throw std::runtime_error(msg1);
	}else{
		std::cerr << "Note: Module " << Asc_ModuleName(m) << ": " << msg1 << std::endl;
	}

	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
	error_reporter_tree_start();
	status = zz_parse();
	switch(status){
		case 0: break;
		case 1: ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Parsing of %s was aborted",Asc_ModuleName(m)); break;
		case 2: ERROR_REPORTER_NOLINE(ASC_PROG_FATAL,"Out of memory when parsing %s",Asc_ModuleName(m)); break;
		default: ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Invalid return from zz_parse"); break;
	}
	status = error_reporter_tree_has_error();
	error_reporter_tree_end();
	if(!status){
		CONSOLE_DEBUG("CLEARING TREE...");
		error_reporter_tree_clear();
		CONSOLE_DEBUG("DONE CLEARING TREE...");
	}else{
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Error(s) when loading '%s'",filename);
		stringstream ss;
		ss << "Errors found in '" << filename <<  "'";
		throw runtime_error(ss.str());
	}


	struct gl_list_t *l = Asc_TypeByModule(m);
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l), filename);
}

/**
	Return a vector of all the Modules which have been loaded into
	the current library.
*/
vector<Module>
Library::getModules(const int module_type){
	if(module_type < 0 || module_type > 2){
		throw std::runtime_error("Library::getModules: invalid module_type parameter");
	}

	vector<Module> v;
	struct gl_list_t *l = Asc_ModuleList(module_type);
	if(l==NULL){
		CONSOLE_DEBUG("list is empty");
		return v;
	}
	for(int i=0, end=gl_length(l); i<end; ++i){
		symchar *name = (symchar *)gl_fetch(l,i+1);
		if(AscFindSymbol(name)==NULL){
			throw runtime_error("Library::getModules: invalid symchar *");
		}
		//cerr << "GOT " << SCP( name ) << endl;
		const module_t *m = Asc_GetModuleByName((const char *)name);
		v.push_back(Module(m));
	}
	/*cerr << "LENGTH OF V IS " << v.size() << endl;
	if(v.size()){
		cerr << "MODULE 0's NAME IS " << v[0].getName() << endl;
	}*/
	return v;
}

/**
	Output to stderr the names of the modules loaded into the current Library.
*/
void
Library::listModules(const int module_type){
	if(module_type < 0 || module_type > 2){
		throw std::runtime_error("Library::listModules: invalid module_type parameter");
	}

	char *type = NULL;
	switch(module_type){
		case 0: type = "defined types"; break;
		case 1: type = "string definitions"; break;
		case 2: type = "statements"; break;
	}

	vector<Module> v = getModules(module_type);
	if(v.size()){
		std::cerr << "Listing " << v.size() << " modules with " << type << std::endl;
		for(vector<Module>::const_iterator i=v.begin(); i < v.end(); ++i){
			cerr << " - " << i->getName() << endl;
			vector<Type> tt = getModuleTypes(*i);
			for(vector<Type>::const_iterator j = tt.begin(); j < tt.end(); ++j){
				cerr << "    * " << j->getName() << endl;
			}
		}
	}else{
		std::cerr << "Notice: No " << type << " found in module list." << std::endl;
	}
}

Type &
Library::findType(const SymChar &sym){
	TypeDescription *t = FindType(sym.getInternalType());
	if(t==NULL){
		stringstream ss;
		ss << "Library::findType: type '" << sym << "' not found in library";
		throw runtime_error(ss.str());
	}/*else{
		cerr << "Found something for type " << sym << endl;
	}*/
	Type *t2=new Type(t);
	return *t2;
}

/**
	This could be quite a bit more efficient if we could get a gl_list_t of TypeDescription rather than names
*/
vector<Type>
Library::getModuleTypes(const Module &m){
	//cerr << "GET MODULE TYPES\n" << endl;
	vector<Type> v;
	struct gl_list_t *l =  Asc_TypeByModule(m.getInternalType());
	for(int i=0,end=gl_length(l); i<end; ++i){
		char *name = (char *)gl_fetch(l,i+1);
		//CONSOLE_DEBUG("Found type %s",name);
		TypeDescription *t = FindType((const symchar *)name);
		v.push_back(Type(t));
	}
	return v;
}

/**
	This function is kinda fighting against the Table implementation of the external function library. What we really need is some kind of iterator on the Table struct, but it doesn't seem to be implemented. Instead there is a C-style equivalent of the STL 'bind1st' function which we can use, but it's not exported from the current extfunc.h so we need to add it.
*/
vector<ExtMethod>
Library::getExtMethods(){
	// Clear the vector
	extmethod_vector = vector<ExtMethod>();

	// Traverse the vector
	TraverseExtFuncLibrary(Library::extMethodTraverse, (void *)this);

	return extmethod_vector;
}

/**
	This method exists only to allow the TraverseExtFuncLibrary function
	to make callbacks to the Library class from C.

	@NOTE there might be issues with C/C++ linking here?
*/
void
Library::extMethodTraverse(void *a1, void *a2){
	Library *self = (Library *)a2;
	self->appendToExtMethodVector(a1);
}

void
Library::appendToExtMethodVector(void *a1){
	struct ExternalFunc *e = (struct ExternalFunc *)a1;
	extmethod_vector.push_back(ExtMethod(e));
}

/**
	Clear the library: 'DESTROY TYPES'

	@TODO do this more efficiently, don't destroy the whole ASCEND compiler.
*/
void
Library::clear(){
/*	Asc_CompilerDestroy();
	cerr << "COMPLETED ASC_COMPILERDESTROY" << endl;
	Asc_CompilerInit(1);
	cerr << "... ASC_COMPILERINIT OK" << endl;
	Asc_ImportPathList(PATHENVIRONMENTVAR);
	registerStandardSolvers();
	cerr << "... REGISTER_STANDARD_SOLVERS" << endl;
	DefineFundamentalTypes();
	cerr << "... DEFINED FUND TYPES" << endl;
	\*SetUniversalProcedureList(NULL);
*/

	//CONSOLE_DEBUG("Displaying library modules and types...");
	//listModules();

	CONSOLE_DEBUG("Destroying simulations...");
	Asc_DestroySimulations();

	CONSOLE_DEBUG("Clearing library...");
	DestroyNotesDatabase(LibraryNote());
	SetUniversalProcedureList(NULL);
	DestroyLibrary();
	DestroyPrototype();
	EmptyTrash();
	Asc_DestroyModules((DestroyFunc)DestroyStatementList);
	//importhandler_destroylibrary();
	WriteChildMissing(NULL,NULL,NULL);
	//Asc_CompilerInit(1)
	DefineFundamentalTypes();
	InitNotesDatabase(LibraryNote());
	ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"LIBRARY CLEARED!");
}

AnnotationDatabase
Library::getAnnotationDatabase(){
	return AnnotationDatabase(SCP(LibraryNote()));
}
