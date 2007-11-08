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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*  @file
	Test registration function for the 'compiler' component.
*/

#include "CUnit/CUnit.h"

#define SUITE compiler

#define TESTS(T) \
	T(hello)

#define PROTO(SUITE,NAME) CU_ErrorCode test_register_##SUITE##_##NAME(void);

#define PROTO_SUITE(SUITE) CU_ErrorCode test_register_##SUITE(void)

#ifdef __cplusplus
extern "C" {
#endif

PROTO_SUITE(SUITE);
/**< 
 *  Registers all tests for the ASCEND general component.
 *  Returns a CUnit error code (CUE_SUCCESS if no errors).
 */

#ifdef __cplusplus
}
#endif

