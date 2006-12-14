#ifndef ASCXX_UNITS_H
#define ASCXX_UNITS_H

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/units.h>
}

#include "symchar.h"

class Dimensions;

/**
	This class will be renamed Units for use in Python
*/
class UnitsM{
private:
	const struct Units *u;
protected:
	const struct Units *getInternalType() const;

public:
	UnitsM();
	UnitsM(const struct Units *u);
	UnitsM(const char *units);

	const SymChar getName() const;
	const Dimensions getDimensions() const;
	const double getConversion() const;
};

#endif
