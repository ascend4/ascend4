#ifndef ASCXX_DIMENSIONS_H
#define ASCXX_DIMENSIONS_H

#include <string>

#include "units.h"
#include "config.h"

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/dimen.h>
}

/**
	A mapping of dimensions to units
*/
/*
map<Dimensions,UnitsM> defaultUnits
multimaps<Dimensions,UnitsM> allUnits
*/

/**
	A 'dimensions object'. This is a wrapper to the 'dim_type' in ASCEND.
	This object allows querying to find suitable units for outputting
	or inputting data from/for a given type.
*/
class Dimensions{
private:
	const dim_type *d;
public:
	static const unsigned MAX_DIMS = NUM_DIMENS;
	static const std::string BASEUNITS[MAX_DIMS];

	/// Return the string name of fundamental unit for the i-th dimension of measurement.
	static const std::string getBaseUnit(const unsigned &i);

	/// Construct a Dimension object fro mthe ASCEND internal data type
	Dimensions(const dim_type *d);

	/// Copy constructor
	Dimensions(const Dimensions &);

	/// Default constructor, required to keep std::vector happy
	Dimensions();

	/// Return the ASCEND internal datatype
	const dim_type *getInternalType() const;

	// Comparison operators
	const bool operator<(const Dimensions &) const;
	const bool operator==(const Dimensions &) const;
	const bool operator!=(const Dimensions &) const;

	/// Get default units for the given dimension
	// const UnitsM getDefaultUnits() const; # DEFINED IN PYTHON

	/// Get the user's preferred units for the given dimension
	//const UnitsM getUserUnits() const; # DEFINED IN PYTHON

	/// Test for a dimensionless Dimension object
	const bool isDimensionless() const;

	/// Test for a wildcard ('don't care') Dimension object
	const bool isWild() const;

	/// Get the numerator part of the index in the i-th dimension (eg for Area, with i={length}, return 2)
	const FRACPART getFractionNumerator(const unsigned &i) const;

	/// Get the denominator part of the index in the i-th dimension
	const FRACPART getFractionDenominator(const unsigned &i) const;
};

#endif
