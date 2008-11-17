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

#ifndef ASCXX_INSTANCE_H
#define ASCXX_INSTANCE_H

#include "symchar.h"
#include "type.h"
#include "set.h"
#include "dimensions.h"

class Plot;

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <compiler/expr_types.h>
#include <compiler/instance_enum.h>
#include <compiler/atomvalue.h>
#include <compiler/plot.h>
}

#include <string>
#include <vector>

typedef enum{
	ASCXX_INST_STATUS_UNKNOWN=0, ASCXX_VAR_FIXED, ASCXX_VAR_UNSOLVED, ASCXX_VAR_ACTIVE, ASCXX_VAR_SOLVED
	, ASCXX_REL_INACTIVE
} InstanceStatus;

/**
	This class has to be called 'Instanc' in C++ to avoid a name clash
	with C. Maybe coulda done it with namespaces but didn't know how.

	This class is renamed back to 'Instance' by SWIG, so use 'Instance'
	when you're in Python.

	The Right Way to implement this class would be as a base class
	with lots of diffent subclasses for the different atom types.
	Maybe even multiple inheritance.

	But until the underlying C code is ported to C++ or modularised in
	some other way, it's not going to be worth the effort. We discussed
	this in the mailing list.
*/
class Instanc{
protected:
	struct Instance *i;
	SymChar name;
	std::vector<Instanc> children;
	void setName(SymChar);
	static SymChar fixedsym;
	static SymChar solvervarsym;
public:
	Instanc();
	Instanc(Instance *i);
	Instanc(Instance *i, const SymChar &name);
	Instanc(const Instanc &parent, const unsigned long &childnum);
	Instanc(const Instanc&);
	~Instanc();
	std::vector<Instanc> &getChildren();
	Instanc getChild(const SymChar &) const;
	Instanc getChild(const long &) const;
	const enum inst_t getKind() const;
	const std::string getKindStr() const;
	const Type getType() const;
	const bool isAtom() const;
	const bool isFixed() const;
	const bool isIncluded() const;

	const bool isFund() const;
	const bool isConst() const;
	const bool isCompound() const;
	const bool isRelation() const;
	const bool isLogicalRelation() const;
	const bool isWhen() const;
	const bool isSet() const;
	const bool isSetInt() const;
	const bool isSetString() const;
	const bool isSetEmpty() const; // set of of type 'empty', NB not same as SetInt::length()==0
	const bool isArray() const;
	const bool isDefined() const;
	const bool isChildless() const;
	const bool isBool() const;
	const bool isInt() const;
	const bool isSymbol() const;
	const bool isReal() const;
	const bool isAssigned() const;
	const bool isModel() const;
	const SymChar &getName() const;
	const double getRealValue() const;
	const bool isDimensionless() const;
	const Dimensions getDimensions() const;
	const bool getBoolValue() const;
	const long getIntValue() const;
	const SymChar getSymbolValue() const;

	const std::string getValueAsString() const;
	const std::string getRelationAsString(const Instanc &relative_to) const;
	Plot getPlot() const;

	const bool isPlottable() const;

	void setFixed(const bool &val=true);
	void setIncluded(const bool &val=true);
	void setBoolValue(const bool&, const unsigned &depth=0);
	void setIntValue(const long&, const unsigned &depth=0);
	void setRealValue(const double&, const unsigned &depth=0);
	void setRealValueWithUnits(double, const char *, const unsigned &depth=0);
	void setSymbolValue(const SymChar &);

	template<class T>
	const ASCXX_Set<T> getSetValue() const{
		if(!isSet()){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not set-valued",getName().toString());
			return ASCXX_Set<T>();
		}
		if(!isConst() && !isDefined()){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Variable '%s' is not defined",getName().toString());
			return ASCXX_Set<T>();
		}
		return ASCXX_Set<T>(SetAtomList(i));
	}

	const enum set_kind getSetType() const;
	void write(FILE *fp) const;
	Instance *getInternalType() const;

	void setStatus(const InstanceStatus &); ///< make this one private, just for friend Simulation?
	const InstanceStatus getStatus() const;

	void setLowerBound(const double &);
	void setUpperBound(const double &);
	void setNominal(const double &);
	const double getLowerBound() const;
	const double  getUpperBound() const;
	const double  getNominal() const;

	const std::vector<Instanc> getClique() const;

	const double getResidual() const;
	const bool getLogicalResidual() const;
};

#endif

