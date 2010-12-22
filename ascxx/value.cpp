#include "value.h"

#include <ascend/compiler/value_type.h>

/* wow, this is a stoopid class, eh! but see solverparameter.h for its use */

Value::Value(){
	v = NULL;
}

Value::Value(const value_t *v){
	this->v = v;
}

Value::~Value(){
	// nothing to do
}

