#include "compiler.h"

#include <compiler/compiler.h>

#include <iostream>
using namespace std;

Compiler::Compiler(){
	cerr << "CREATED COMPILER" << endl;
}

Compiler::~Compiler(){
	cerr << "DESTROYED COMPILER" << endl;
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
		return TRUE;
	}
	return FALSE;
}

void
Compiler::setUseRelationSharing(const bool &use_relation_sharing){
	if(use_relation_sharing){
		g_use_copyanon = 1;
	}
	g_use_copyanon = 0;
}

Compiler *getCompiler(){
	return Compiler::instance();
}
