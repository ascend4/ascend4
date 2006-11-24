/* : set syntax=cpp : */
/*
	SWIG interface routines to read a file into the library
*/

%module(directors=1) ascpy

%include <python/std_string.i>
%include <python/std_except.i>
%include <python/std_vector.i>
%include <python/std_map.i>

%{
#include "library.h"
#include "compiler.h"
#include "type.h"
#include "instance.h"
#include "variable.h"
#include "relation.h"
#include "name.h"
#include "reporter.h"
#include "simulation.h"
#include "solver.h"
#include "symchar.h"
#include "set.h"
#include "dimensions.h"
#include "units.h"
#include "extmethod.h"
#include "plot.h"
#include "curve.h"
#include "solverparameters.h"
#include "solverstatus.h"
#include "solverreporter.h"
#include "incidencematrix.h"
#include "registry.h"
#include <compiler/importhandler.h>
#include "annotation.h"

#include <dmalloc.h>
%}

// All STL runtime_errors caught to Python

%exception {
	try {
		$action
	}
	catch (std::range_error &e) {
		SWIG_exception(SWIG_IndexError,e.what());
	}
	catch (std::runtime_error &e) {
		SWIG_exception(SWIG_RuntimeError,e.what());
	}
}

// Import the preferences module
%pythoncode {
	import preferences;
}

// Set-valued instance variable
%pythoncode {
	class SetIter:
		def __init__(self,set):
			self.set=set
			self.index=0
		def next(self):
			if self.index==self.set.length():
				raise StopIteration
			self.index = self.index + 1
			return self.set[self.index]
}

template<class T>
class ASCXX_Set{
private:
	ASCXX_Set();
public:
	const T at(const unsigned long&) const;
	const unsigned long	length() const;
};
%extend ASCXX_Set<long>{
	%pythoncode {
		def __getitem__(self, index):
			return self.at(index)
		def __iter__(self):
			return SetIter(self)
	}
}
%extend ASCXX_Set<SymChar>{
	%pythoncode {
		def __getitem__(self, index):
			return self.at(index)
		def __iter__(self):
			return SetIter(self)
	}
}


%template(ModuleVector) std::vector<Module>;
%template(TypeVector) std::vector<Type>;
%template(MethodVector) std::vector<Method>;
%template(InstancVector) std::vector<Instanc>;
%template(ExtMethodVector) std::vector<ExtMethod>;
%template(SetInt) ASCXX_Set<long>;
%template(SetString) ASCXX_Set<SymChar>;
%template(DoubleVector) std::vector<double>;
%template(IntVector) std::vector<int>;
%template(CurveVector) std::vector<Curve>;
%template(StringVector) std::vector<std::string>;
%template(IntStringMap) std::map<int,std::string>;
%template(AnnotationVector) std::vector<Annotation>;

%rename(Instance) Instanc;
%rename(Name) Nam;#include "incidencematrix.h"
%rename(getSetIntValue) Instanc::getSetValue<long>;
%rename(getSetStringValue) Instanc::getSetValue<SymChar>;
%rename(Units) UnitsM;

// Grab a Python function object as a Python object.
%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

//----------------------------
// REPORTER: callbacks to python
class Reporter{
private:
	~Reporter();
	Reporter();
public:
	// use 'getReporter' instead of 'Reporter::Instance()' in python
	void setErrorCallback(error_reporter_callback_t callback, void *client_data);
	void setPythonErrorCallback(PyObject *pyfunc);
	void clearPythonErrorCallback();
};

%extend Reporter {
	void reportError(const char *msg){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"%s", msg);
	}
	void reportNote(const char *msg){
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"%s",msg);
	}
	void reportWarning(const char *msg){
		ERROR_REPORTER_NOLINE(ASC_USER_WARNING,"%s",msg);
	}
	void reportSuccess(const char *msg){
		ERROR_REPORTER_NOLINE(ASC_USER_SUCCESS,"%s",msg);
	}
}

// There are problems with Instance(), so use this instead:
Reporter *getReporter();

//----------------------------------------
// UNITS AND DIMENSIONS


class UnitsM;

class Dimensions{
public:
	static const unsigned MAX_DIMS;
	static const std::string getBaseUnit(const unsigned &);

	const bool operator==(const Dimensions &) const;
	const bool operator!=(const Dimensions &) const;
	const bool operator<(const Dimensions &) const;
	Dimensions(const Dimensions &);
	const bool isWild() const;
	const bool isDimensionless() const;
	const short getFractionNumerator(const unsigned &) const;
	const short getFractionDenominator(const unsigned &) const;
};

class UnitsM{
public:
	UnitsM(const char *);
	const SymChar getName() const; // the units description string eg 'bar' or 'kJ/kg/K'
	const Dimensions getDimensions() const;
	const double getConversion() const; // multiplication factor to convert eg feet to SI (metres)
};

%extend UnitsM{
	%pythoncode{
		def getConvertedValue(self,si_value):
			_u_value = si_value / self.getConversion()
			return str(_u_value) + " " + self.getName().toString();
	}
}

/*
	This function creates default (SI) units for any dimension given. Most
	of the time you will want to use custom units in place of these, eg
	'N' instead of 'kg*m/s^2'.
*/
%extend Dimensions{
	%pythoncode {

		def getDefaultUnits(self):
			if self.isDimensionless():
				return Units("");

			if self.isWild():
				return Units("?");

			# create a string representation of the current dimensions
			numparts=[]
			denparts=[]
			for i in range(0, self.MAX_DIMS):
				baseunit = self.getBaseUnit(i);
				num = self.getFractionNumerator(i)
				den = self.getFractionDenominator(i)
				if num > 0:
					if den == 1:
						if num == 1:
							numparts.append(baseunit)
						else:
							numparts.append("%s^%d" % (baseunit, num) )
					else:
						numparts.append("%s^(%d/%d)" % (baseunit, num, den) )
				elif num < 0:
					if den == 1:
						if num == -1:
							denparts.append(baseunit)
						else:
							denparts.append("%s^%d" % (baseunit, -num) )
					else:
						denparts.append("%s^(%d/%d)" % (baseunit, -num, den) )

			if len(numparts):
				str = "*".join(numparts)
			else:
				str = "1"

			if len(denparts):
				str = str + "/" + "/".join(denparts)

			return Units(str)

	}
}

/*
some python code for nice unicode unit strings, need to extend the units.c code as well though.

elif num == 2:
	numparts.append(baseunit + ur'\u00b2')
elif num == 3:
	numparts.append(baseunit + ur'\u00b3')

str = ur'\u00b7'.join(numparts)
*/
//----------------------------

%typemap(in) const SymChar& {
	$1 = new SymChar(PyString_AsString($input));
}

%include "library.h"

class SymChar{
public:
	SymChar(const std::string &);
	const char *toString() const;
};
%extend SymChar{
	const char *__repr__(){
		return self->toString();
	}
}

class Module{
public:
	const char *getName() const;
	const char *getFilename() const;
	const struct tm *getMtime() const;
};

%include "method.h"

%ignore Method::getInternalType;
%ignore Method::getSym;

// Renamed in python as 'Name'
class Nam{
public:
	Nam(const SymChar &);
	const std::string getName() const;
};

%include "compiler.h"
/* we can always disown Compiler * as it's a singleton */
%apply SWIGTYPE *DISOWN { Compiler * };

%include "type.h"

%extend Type{
	const char *__repr__(){
		return self->getName().toString();
	}

	%pythoncode{
		def getPreferredUnits(self):
			if not self.isRefinedReal():
				return None

			_pref = preferences.Preferences()
			#print "Checking for preferred units for %s" % self.getName()
			_u = _pref.getPreferredUnits(self.getName().toString())
			if _u == None:
				# no preferred units set
				return None
			_units = Units(_u);

			if _units.getDimensions() != self.getDimensions():
				getReporter().reportWarning("Preferred units '%s' for type '%s' are not dimensionally correct: ignoring." % (_u, self.getName()) );
				return None

			return _units;
	}
}

typedef enum{
	ASCXX_VAR_STATUS_UNKNOWN=0, ASCXX_VAR_FIXED, ASCXX_VAR_UNSOLVED, ASCXX_VAR_ACTIVE, ASCXX_VAR_SOLVED
} VarStatus;

class Instanc{
private:
	Instanc();
public:
	Instanc(Instance *);
	Instanc(Instance *, SymChar &name);
	~Instanc();
	std::vector<Instanc> getChildren();
	const std::string getKindStr() const;
	const SymChar &getName();
	const Type getType() const;
	const bool isAtom() const;
	const bool isFixed() const;
	const bool isActive() const;
	const bool isFund() const;
	const bool isConst() const;
	const bool isAssigned() const;
	const bool isCompound() const;
	const bool isRelation() const;
	const bool isLogicalRelation() const;
	const bool isWhen() const;
	const bool isSet() const; // a set (group) of things
	const bool isSetInt() const;
	const bool isSetString() const;
	const bool isSetEmpty() const;
	const bool isDefined() const;
	const bool isBool() const;
	const bool isInt() const;
	const bool isSymbol() const;
	const bool isReal() const;
	const bool isModel() const;

	const double getRealValue() const;
	const bool isDimensionless() const;
	const Dimensions getDimensions() const;
	const bool getBoolValue() const;
	const long getIntValue() const;
	const SymChar getSymbolValue() const;
	const std::string getValueAsString() const; ///< Use carefully: rounding will occur for doubles!

	const std::string getRelationAsString(const Instanc &relative_to) const;
	const double getResidual() const;

	Plot getPlot() const;

	const bool isPlottable() const;
	const ASCXX_Set<long> getSetValue<long>() const;
	const ASCXX_Set<SymChar> getSetValue<SymChar>() const;
	const bool isChildless() const;
	void setFixed(const bool &val=true);
	void setRealValue(const double &val);
	void setRealValueWithUnits(const double &, const char *);
	void setBoolValue(const bool &val);
	void setSymbolValue(const SymChar &sym);
	void write();

	const VarStatus getVarStatus() const;

	void setLowerBound(const double &);
	void setUpperBound(const double &);
	void setNominal(const double &);
	const double getLowerBound() const;
	const double  getUpperBound() const;
	const double  getNominal() const;

	const std::vector<Instanc> getClique() const;
};

%extend Instanc{
	const char *__repr__(){
		return self->getName().toString();
	}
	Instanc __getattr__(const char *name){
		return self->getChild(SymChar(name));
	}
	double __float__(){
		if(self->isReal()){
			return self->getRealValue();
		}else{
			throw std::runtime_error("Can't cast this instance to float");
		}
	}
		
	%pythoncode {
		def getSetValue(self):
			if self.isSetInt():
				return self.getSetIntValue()
			elif self.isSetString():
				return self.getSetStringValue()
			elif self.isSetEmpty():
				return set()
			else:
				raise RuntimeError("getSetValue: unknown set type");

		def getValue(self):
			#print "GETTING VALUE OF %s" % self.getName()
			if self.isCompound():
				return ""
			elif self.isRelation():
				return self.getResidual()
			elif self.isWhen():
				return "WHEN"
			elif self.isSet():
				_s = set(self.getSetValue());
				#for _v in self.getSetValue():
				#	_s.add( _v )
				return _s

			elif ( self.isAtom() or self.isFund() ) and not self.isDefined():
				return "undefined"
			elif self.isReal():
				return self.getRealValueAndUnits()
			elif self.isBool():
				return self.getBoolValue()
			elif self.isInt():
				return self.getIntValue()
			elif self.isSymbol():
				return self.getSymbolValue()
			else:
				return "UNKNOWN TYPE" 
				#raise RuntimeError("Unknown value model type="+self.getType().getName().toString()+", instance kind=".getKindStr())

		def getRealValueAndUnits(self):
			if not self.isReal():
				raise TypeError
			if self.isDimensionless() or self.isFund():
				return self.getRealValue();
			_u = self.getType().getPreferredUnits();
			if _u == None:
				return str(self.getRealValue()) + ' ' + self.getDimensions().getDefaultUnits().getName().toString()
			return _u.getConvertedValue(self.getRealValue())

		def setFixedValue(self,val):
			if not self.isFixed():
				self.setFixed();
			# getReporter().reportError("Setting value of %s to %s" % (self.getName().toString(),val))
			self.setRealValue(val);

		def __coerce__(self,other):
			return (self.getValue(),other.getValue())
	}
}

/*
	This 'registry' thing is a bit of a hack that allows interface pointers to 
	be registered with libascend so that they can be accessed from external
	script methods
*/
%include "registry.h"
%extend Registry{
	void set(const char *key, PyObject *obj){
		self->setPyObject(key,obj);
	}
}

%include "solver.i"

%include "extmethod.h"

%include "annotation.h"

%include "plot.i"

class Curve : public Instanc{
public:
	std::vector<double> x;
	std::vector<double> y;
	const std::string getLegend() const;
};
