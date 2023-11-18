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

ValueType Value::getType(){
	switch(this->v->t){
	case real_value:
		return VALUE_REAL;
	case integer_value:
		return VALUE_INT;
	case symbol_value:
		return VALUE_CHAR;
	case boolean_value:
		return VALUE_BOOL;
	default:
		return VALUE_UNIMPLEMENTED;
	};
}
