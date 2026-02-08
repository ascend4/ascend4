/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*/

#include <ascend/general/platform.h>
#include "test_register_compiler.h"

#define TESTS(T) \
	T(basics) \
	T(relation) \
	T(autodiff) \
	T(expr) \
	T(bintok) \
	T(fixfree) \
	T(blackbox) \
	T(fixassign) \
	T(dimen) \
	T(fractions) \
	T(units) \
	T(name) \
	T(symtab) \
	T(qlfdid) \
	T(func) \
	T(notes) \
	T(chkdim) \
	T(instantiate_set_enum) \
	T(instantiate_relation_logrel_bool) \
	T(instantiate_logrel_bool_algebra) \
	T(instantiate_alike) \
	T(instantiate_context) \
	T(instantiate_anontype) \
	T(instantiate_array) \
	T(instantiate_for) \
	T(instantiate_when_select) \
	T(instantiate_alias) \
	T(instantiate_set_type_errors) \
	T(instantiate_cond) \
	T(merge) \
	T(merge_extra) \
	T(merge_values) \
	T(merge_parents) \
	T(merge_relparents) \
	T(merge_children) \
	T(merge_arrays) \
	T(merge_dims) \
	T(merge_model_children) \
	T(merge_model_values)


#define PROTO_TEST(NAME) PROTO(compiler,NAME)
TESTS(PROTO_TEST)
#undef PROTO_TEST

#define REGISTER_TEST(NAME) \
	result = TESTREGISTER(compiler,NAME); \
	if(CUE_SUCCESS!=result){ \
		return result; \
	}

REGISTER_SUITE(compiler,TESTS)
