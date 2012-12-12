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
	Wrapper class for the 'value_t' from the C API. This class includes
	support for

	See also SlvReqHooks in ascend/compiler/slvreq.h.
*/

#ifndef ASCXX_VALUE_H
#define ASCXX_VALUE_H

struct value_t;
class SolverParameter;

typedef enum{
	VALUE_INT, VALUE_REAL, VALUE_BOOL, VALUE_CHAR
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
