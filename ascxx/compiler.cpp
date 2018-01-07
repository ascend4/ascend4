#include "compiler.h"

#define COMPILER_DEBUG
//#define BINTOKEN_DEBUG

extern "C"{
#include <ascend/compiler/compiler.h>
#include <ascend/compiler/bintoken.h>
#include <ascend/utilities/error.h>
}

using namespace std;

Compiler::Compiler(){
#ifdef COMPILE_DEBUG
	CONSOLE_DEBUG("Creating compiler");
#endif

	/* set some default for bintoken compilation */
	use_bintoken = false;
	bintoken_options_sent = false;
#if 0
	bt_targetstem = "/tmp/asc_bintoken";
	bt_srcname = bt_targetstem + ".c";
	bt_objname = bt_targetstem + ".o";
	bt_libname = bt_targetstem + ".so";
	bt_cmd = "make -f ascend/bintokens/Makefile ASCBT_TARGET=" + bt_libname + " ASCBT_SRC=" + bt_srcname;
	bt_rm = "/bin/rm";
#endif
}

Compiler::~Compiler(){
#ifdef COMPILER_DEBUG
	CONSOLE_DEBUG("Destroying compiler...");;
#endif
}

Compiler *
Compiler::instance(){
	static Compiler *_instance = NULL;
	if(_instance == NULL){
		_instance = new Compiler();
	}
	return _instance;
}

const bool
Compiler::getUseRelationSharing() const{
	if(g_use_copyanon){
		return true;
	}
	return false;
}

void
Compiler::setUseRelationSharing(const bool &use_relation_sharing){
	g_use_copyanon = 0;
	if(use_relation_sharing){
		g_use_copyanon = 1;
	}
}

void
Compiler::setBinaryCompilation(const bool &use_bintoken){
	this->use_bintoken = use_bintoken;
#ifdef BINTOKEN_DEBUG
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"usebintoken = %d",int(use_bintoken));
#endif
}

void
Compiler::setVerbosity(const int &verbosity){
	g_compiler_warnings = verbosity;
#ifdef COMPILER_DEBUG
	CONSOLE_DEBUG("g_compiler_warnings = %d",g_compiler_warnings);
#endif
}

void
Compiler::sendBinaryCompilationOptions(){
	if(use_bintoken && !bintoken_options_sent){
#if 0
#ifdef BINTOKEN_DEBUG
		CONSOLE_DEBUG("SETUP BINTOKENS...");
#endif
		BinTokenSetOptions(bt_srcname.c_str(), bt_objname.c_str(), bt_libname.c_str()
			, bt_cmd.c_str(), bt_rm.c_str(), 1000/*maxrels*/, 1/*verbose*/, 0/*housekeep*/
		);
#ifdef BINTOKEN_DEBUG
		CONSOLE_DEBUG("srcname = %s, objname = %s, libname = %s, cmd = %s, rm = %s",
			bt_srcname.c_str(), bt_objname.c_str(), bt_libname.c_str(), bt_cmd.c_str(), bt_rm.c_str()
		);
#endif
#else
		BinTokenSetOptionsDefault();
#endif
		bintoken_options_sent = true;
	}else{
		/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"disabling bintoken compilation\n"); */
		bintoken_options_sent = false;
		BinTokenSetOptions(NULL,NULL,NULL,NULL,NULL,0,1,1);
	}
}

Compiler *getCompiler(){
	return Compiler::instance();
}
