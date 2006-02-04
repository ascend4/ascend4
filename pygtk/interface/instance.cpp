#include <iostream>
#include <stdexcept>
#include <sstream>
using namespace std;

extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <general/dstring.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instantiate.h>
#include <compiler/bintoken.h>
#include <compiler/instance_enum.h>
#include <compiler/instquery.h>
#include <compiler/check.h>
#include <compiler/name.h>
#include <compiler/parentchild.h>
#include <compiler/instance_name.h>
#include <compiler/atomvalue.h>
#include <utilities/readln.h>
#include <compiler/plot.h>
#include <compiler/types.h>
#include <compiler/find.h>
#include <compiler/relation_type.h>
#include <compiler/exprs.h>
#include <compiler/relation.h>
#include <compiler/relation_io.h>
}

#include "instance.h"
#include "variable.h"
#include "name.h"
#include "set.h"
#include "plot.h"
#include "instanceinterfacedata.h"

/**
	Create an instance of a type. @see Simulation for instantiation.
*/
Instanc::Instanc(Instance *i) : i(i), name("unnamed1"){
	if(i==NULL){
		stringstream ss;
		ss << "Attempted to create Instance object will null 'Instance *', name " << name;
		throw runtime_error(ss.str());
	}
	//throw runtime_error("Created unnamed instance");
	// nothing else;
}

Instanc::Instanc(Instance *i, const SymChar &name) : i(i), name(name){
	/*if(i==NULL){
		stringstream ss;
		ss << "Attempted to create Instance object will null 'Instance *', name " << name;
		throw runtime_error(ss.str());
	}*/
	cerr << "A NEW INSTANCE " << name << endl;
}

Instanc::Instanc(const Instanc&old) : i(old.i), name(old.name){
	// nothing else
}

Instanc::Instanc() : name("unnamed2"){
	throw runtime_error("Can't construct empty instances");
}

/*
	Create a child instance given the parent
*/
Instanc::Instanc(const Instanc &parent, const unsigned long &childnum)
		: i( InstanceChild(parent.i,childnum) ), name( ChildName(parent.i,childnum) ){
	// cerr << "CREATED CHILD #" << childnum << ", named '" << getName() << "' OF " << parent.getName() << endl;
}

const SymChar &
Instanc::getName() const{
	return name;
}

void
Instanc::setName(SymChar name){
	this->name=name;
}

Instance *
Instanc::getInternalType() const{
	return i;
}

const enum inst_t
Instanc::getKind() const{
	if(i==NULL)throw runtime_error("NULL instance in Instanc::getKind");
	return InstanceKind(i);
}

/**
	Return the type of this instance as a string
*/
const string
Instanc::getKindStr() const{
	enum inst_t k = getKind();
	stringstream ss;

	switch(k){
		case ERROR_INST: ss << "Error"; break;
		case SIM_INST: ss << "Simulation"; break;
		case MODEL_INST: ss << "Model"; break;
		case REL_INST: ss << "Numerical Relation"; break;
		case LREL_INST: ss << "Logical relation"; break;
		case WHEN_INST: ss << "WHEN"; break;
		case ARRAY_INT_INST: ss << "Indexed Array"; break;
		case ARRAY_ENUM_INST: ss << "Enumerated Array"; break;
		case REAL_INST: ss << "Real"; break;
		case INTEGER_INST: ss << "Integer"; break;
		case BOOLEAN_INST: ss << "Boolean"; break;
		case SYMBOL_INST: ss << "Symbol"; break;
		case SET_INST: ss << "Set"; break;
		case REAL_ATOM_INST: ss << "Real atom"; break;
		case INTEGER_ATOM_INST: ss << "Integer atom"; break;
		case BOOLEAN_ATOM_INST: ss << "Boolean atom"; break;
		case SYMBOL_ATOM_INST: ss << "Symbol atom"; break;
		case SET_ATOM_INST: ss << "Set atom"; break;
		case REAL_CONSTANT_INST: ss << "Real constant"; break;
		case BOOLEAN_CONSTANT_INST: ss << "Boolean constant"; break;
		case INTEGER_CONSTANT_INST: ss << "Integer constant"; break;
		case SYMBOL_CONSTANT_INST: ss << "Symbol constant"; break;
		case DUMMY_INST: ss << "Dummy"; break;
		default:
			throw runtime_error("Invalid instance type");
	}

	ss << " instance";
	return ss.str();
}

const Type
Instanc::getType() const{
	try{
		const TypeDescription *t = InstanceTypeDesc(i);
		if(t==NULL)throw runtime_error("No type defined");

		return Type(t);
	}catch(runtime_error &e){
		stringstream ss;
		ss << "Instance::getType: with name=" << getName() << ":" << e.what();
		throw runtime_error(ss.str());
	}
}

const bool
Instanc::isAtom() const{
	return getKind() & IATOM;
	//return IsAtomicInstance(i);
}

const bool
Instanc::isFixed() const{
	if(getKind()==REAL_ATOM_INST && getType().isRefinedSolverVar()){
		return getChild("fixed").getBoolValue();
	}
	throw runtime_error("Instanc::isFixed: Not a solver_var");
}

const bool
Instanc::isCompound() const{
	return getKind() & ICOMP;
}

const bool
Instanc::isRelation() const{
	switch(getKind()){
 		case REL_INST:
		case LREL_INST:
			return true;
	}
	return false;
}

const bool
Instanc::isWhen() const{
	if(getKind()==WHEN_INST)return true;
	return false;
}

const bool
Instanc::isSet() const{
	return (getKind() == SET_INST || getKind() == SET_ATOM_INST);
}

const bool
Instanc::isSetInt() const{
	return isSet() && getSetType()==integer_set;
}

const bool
Instanc::isSetString() const{
	return isSet() && getSetType()==string_set;
}

const bool
Instanc::isSetEmpty() const{
	return isSet() && getSetType()==empty_set;
}

const bool
Instanc::isFund() const{
	return getKind() & IFUND;
	//return IsFundamentalInstance(i);
}

const bool
Instanc::isConst() const{
	return getKind() & ICONS;
	//return IsConstantInstance(i);
}

const bool
Instanc::isAssigned() const{
	if(!isAtom()){
		throw runtime_error("Instanc::isAssigned: not an Atom");
	}
	return AtomAssigned(i);
}

const bool
Instanc::isArray() const{
	return getKind() & IARR;
	//return IsArrayInstance(i);
}

const bool
Instanc::isChildless() const{
	return getKind() & ICHILDLESS;
	//return IsChildlessInstance(i);
}

const bool
Instanc::isBool() const{
	switch(getKind()){
		case BOOLEAN_INST:
		case BOOLEAN_ATOM_INST:
		case BOOLEAN_CONSTANT_INST:
			return true;
	}
	return false;
}

const bool
Instanc::isInt() const{
	switch(getKind()){
		case INTEGER_INST:
		case INTEGER_ATOM_INST:
		case INTEGER_CONSTANT_INST:
			return true;
	}
	return false;
}

const bool
Instanc::isReal() const{
	switch(getKind()){
		case REAL_INST:
		case REAL_ATOM_INST:
		case REAL_CONSTANT_INST:
			return true;
	}
	return false;
}

const bool
Instanc::isSymbol() const{
	switch(getKind()){
		case SYMBOL_INST:
		case SYMBOL_ATOM_INST:
		case SYMBOL_CONSTANT_INST:
			return true;
	}
	return false;
}

const bool
Instanc::isDefined() const{
	if(!isAtom() && !isFund())throw runtime_error("Instanc::isDefined: not an atom/fund");
	return AtomAssigned(i);
}

const double
Instanc::getRealValue() const{
	// Check that the instance has a real value:
	switch(getKind()){
		case REAL_INST:
		case REAL_CONSTANT_INST:
		case REAL_ATOM_INST:
			//cerr << "REAL VALUE FOR " << getName() << endl;
			break;
		default:
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not real-valued (%s)", \
					getName().toString(),getKindStr().c_str());
			return 0;
	}
	if(!isConst() && !isDefined()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not defined (%s)", \
				getName().toString(),getKindStr().c_str());
		return 0;
	}
	return RealAtomValue(i);
}

const bool
Instanc::isDimensionless() const{
	if(!isReal())return true;
	return Dimensions( RealAtomDims(i) ).isDimensionless();
}

const Dimensions
Instanc::getDimensions() const{
	if(!isReal())throw runtime_error("Instanc::getDimensions: not a real-valued instance");
	return Dimensions( RealAtomDims(i) );
}

const bool
Instanc::getBoolValue() const{
	// Check that the instance has a bool value:
	switch(getKind()){
		case BOOLEAN_INST:
		case BOOLEAN_ATOM_INST:
		case BOOLEAN_CONSTANT_INST:
			//cerr << "BOOL VALUE FOR " << getName() << endl;
			break;
		default:
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not boolean-valued",getName().toString());
			return false;
	}
	if(!isConst() && !isDefined()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not defined",getName().toString());
		return false;
	}
	return GetBooleanAtomValue(i);
}


const long
Instanc::getIntValue() const{
	// Check that the instance has a bool value:
	switch(getKind()){
		case INTEGER_INST:
		case INTEGER_ATOM_INST:
		case INTEGER_CONSTANT_INST:
			//cerr << "INT VALUE FOR " << getName() << endl;
			break;
		default:
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not integer-valued",getName().toString());
			return 0;
	}
	if(!isConst() && !isDefined()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not defined",getName().toString());
		return 0;
	}
	return GetIntegerAtomValue(i);
}

const SymChar
Instanc::getSymbolValue() const{
	if(!isSymbol()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not symbol-valued",getName().toString());
		return SymChar("ERROR");
	}
	if(!isConst() && !isDefined()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not defined",getName().toString());
		return SymChar("UNDEFINED");
	}
	return SCP(GetSymbolAtomValue(i));
}

const string
Instanc::getRelationAsString(const Instanc &relative_to) const{
	stringstream ss;
	if(isRelation()){
		int len;
		char *str = WriteRelationString(i,relative_to.getInternalType()
				,NULL,NULL,relio_ascend,&len);
		ss << str;
		ascfree(str);
	}else{
		throw runtime_error("getRelationString: Instance is not a relation");
	}
	return ss.str();
}
		
/**
	Return the numerical value of an instance if it is an assigned atom.
	If it is a relation, return the string form of the relation (ie the equation)
	Else return the string 'undefined'.
*/
const string
Instanc::getValueAsString() const{
	stringstream ss;

	if(isAssigned()){
		if(isReal()){
			ss << getRealValue();
		}else if(isInt()){
			ss << getIntValue();
		}else if(isSymbol()){
			ss << getSymbolValue();
		}else if(isBool()){
			ss << getBoolValue();
		}else{
			throw runtime_error("Invalid type in Instanc::getValueAsString");
		}
	}else{
		ss << "undefined";
	}
	return ss.str();
}

const bool
Instanc::isPlottable() const{
	if(plot_allowed(i)){
		return true;
	}
	return false;
}

const enum set_kind
Instanc::getSetType() const{
	if(!isSet() || (!isConst() && !isDefined())){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not set-valued or not defined",getName().toString());
	}
	return SetKind(SetAtomList(i));
}

/// Get the child instances :)
vector<Instanc> &
Instanc::getChildren()
{
	Type t = getType();

	children = vector<Instanc>();

	if(i==NULL)throw runtime_error("NULL 'i' in Instanc::getChildren");

	unsigned long len = NumberChildren(i);
	if(!len)return children;
	//cerr << "FOUND " << len << " CHILDREN" << endl;

	if(isArray()){
		for(unsigned long ci=1; ci<=len; ++ci){
			Instanc c(*this, ci);
			if(!TypeShow(c.getType().getInternalType()))continue;
			children.push_back(c);
		}
		return children;
		/* stringstream ss;
		ss << "Instance '" << getName() << "' is an array, type '" << t.getName() << "'";
		ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,ss.str().c_str()); */
	}


	ChildListPtr clist = GetChildList(t.getInternalType());
	if(!clist){
		stringstream ss;
		ss << "Child list of instance '" << getName() << "' of type '" << t.getName() << "' (" << getKindStr() << ") is NULL";
		ss << " (isChildless=" << isChildless() << ")";
		ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,ss.str().c_str());
		return children;
		//throw runtime_error(ss.str());
	}

	/// FIXME 1-based array:
	for(unsigned long ci=1; ci<=len; ++ci){
		if(!ChildVisible(clist,ci))continue;

		Instanc c( *this, ci );
		//cerr << "FOUND CHILD #" << ci << ": " << c.getName() << endl;

		children.push_back(c);
	}
	return children;
}

Instanc
Instanc::getChild(const SymChar &name) const{
	struct Instance *c = ChildByChar(i,name.getInternalType());
	if(c==NULL)throw runtime_error("Child not found");
	return Instanc(c);
}

Plot
Instanc::getPlot() const{
	if(isPlottable()){
		return Plot(*this);
	}
	throw runtime_error("Not a plottable instance");
}

void
Instanc::write(){
	WriteInstance(stderr,i);
}

//----------------------------
// SETTING VALUES of stuff

void
Instanc::setFixed(const bool &val){
	if(isFixed()==val)return;
	getChild("fixed").setBoolValue(val);
}

void
Instanc::setBoolValue(const bool &val, const unsigned &depth){
	SetBooleanAtomValue(i, val, depth);
}

void
Instanc::setRealValue(const double &val, const unsigned &depth){
	SetRealAtomValue(i,val, depth);
	//ERROR_REPORTER_HERE(ASC_USER_NOTE,"Set %s to %f",getName().toString(),val);
}

/**
	Borrow the workings of this from tcltk98 UnitsProc.c
*/
void
Instanc::setRealValueWithUnits(double val, const char *units, const unsigned &depth){

	if(isConst()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Can't change the value of a constant");
		return;
	}

	if(!isReal() || !isAtom()){
		throw runtime_error("Instanc::setRealValueWithUnits: not a real-valued instance");
	}

	if(units == NULL || strlen(units)==0 || strcmp(units,"*")==0){
		// No units were specified, so the base (SI) units are implied.
	}else{
		// We need to parse the units string
		UnitsM u = UnitsM(units);
		Dimensions d = u.getDimensions();

		// If no dimensions yet assigned, assign them. Otheriwse check for consistency.
		if(getDimensions().isWild()){
			// Set the dimensions for a wildcard atom:
			SetRealAtomDims(i, d.getInternalType());
		}else if(d != getDimensions()){
			throw runtime_error("Dimensionally incompatible units");
		}

		// Not going to worry about FPEs here, let the program crash if it must.
		val = val * u.getConversion();
	}

	SetRealAtomValue(i,val,depth);
}

/**
	Set the instance variable status. See @getVarStatus
*/
void 
Instanc::setVarStatus(const VarStatus &s){
	InstanceInterfaceData *d;
	d = (InstanceInterfaceData *)GetInterfacePtr(i);
	if(d==NULL && s!=ASCXX_VAR_STATUS_UNKNOWN){
		d = new InstanceInterfaceData();
		SetInterfacePtr(i,d);
	}
	d->status = s;
}

/** 
	Return the instance variable status.
	This data is stored in the 'interface_ptr' of the instance, so
	that we can be sure we'll get it, regardless of which 
	instance of an Instanc we have in our hands :-)
*/
const VarStatus
Instanc::getVarStatus() const{
	InstanceInterfaceData *d;
	d = (InstanceInterfaceData *)GetInterfacePtr(i);
	if(d==NULL){
		return ASCXX_VAR_STATUS_UNKNOWN;
	}
    return d->status;
}



/*------------------------------------------------------
	Macros to declare
		setUpperBound
		setLowerBound
		setNominal
	and their 'get' equivalents
*/

#define DEFINE_GET_REAL_CHILD(METHOD,CHILD) \
	const double \
	Instanc::get##METHOD() const{ \
		Instanc c = getChild(CHILD); \
		return c.getRealValue(); \
	}

#define DEFINE_SET_REAL_CHILD(METHOD,CHILD) \
	void \
	Instanc::set##METHOD(const double &v){ \
		Instanc c = getChild(CHILD); \
		c.setRealValue(v); \
	}

#define DEFINE_CHILD_METHODS(D) \
	D(LowerBound,"lower_bound") \
	D(UpperBound,"upper_bound") \
	D(Nominal,"nominal")

DEFINE_CHILD_METHODS(DEFINE_SET_REAL_CHILD)
DEFINE_CHILD_METHODS(DEFINE_GET_REAL_CHILD)

//------------------------------------------------------
	

// static properties
SymChar
Instanc::fixedsym = SymChar("fixed");

SymChar
Instanc::solvervarsym = SymChar("solver_var");

