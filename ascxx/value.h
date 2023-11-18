/*	ASCEND modelling environment
	Copyright (C) 2010 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Wrapper class for the `value_t` from the C API.
	
	`value_t` is used in the compiler, in childdef.c, evaluate.c, find.c,
	instantiate.c, slvreq.c, temp.h. It is not used for storing values within 
	the instance hierarchy (for which 'atom' data structures are used instead.
	Rather, it is used for passing values in the context of METHODs and when
	getting/setting parameters associated with the solvers.
	
	Values can be real, integer, symbol (ie string), set, list and error.
	The `value_t` structures do not track units of measurement.

	See also SlvReqHooks in ascend/compiler/slvreq.h.
	
	In relation to the C++/SWIG wrapper for libascend, we might need to 
	receive value_t in the context of external relations, perhaps, or 
	and we definitely need to be able to pass through values for the slvreq 
	functionality (hence SolverParameter::setValueValue). It would be useful 
	to have a reliable __repr__ function for value_t in Python. But it is not
	clear whether we need a full-featured wrapping of this type.
*/

#ifndef ASCXX_VALUE_H
#define ASCXX_VALUE_H

struct value_t;
class SolverParameter;

typedef enum{
	VALUE_INT, VALUE_REAL, VALUE_BOOL, VALUE_CHAR, VALUE_UNIMPLEMENTED
} ValueType;

class Value{
	friend class SolverParameter;
protected:
	const value_t *v;
public:
	Value();
	Value(const value_t *val);
	~Value();
	ValueType getType();
};

#endif
