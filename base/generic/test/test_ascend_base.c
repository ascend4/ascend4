/*	ASCEND modelling environment
	Copyright (C) 2005 Jerry St.Clair
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
*//**
	Central registration 'base/generic' test routines in ASCEND
*/
#include <stdio.h>

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <CUnit/CUnit.h>
#include <general/test/test_register_general.h>
#include <utilities/test/test_register_utilities.h>
#include <solver/test/test_register_solver.h>

ASC_EXPORT int register_cunit_tests(){
	test_register_general();
	test_register_utilities();
	test_register_solver();
	fprintf(stderr,"Registered ASCEND test suite\n");
	return 0;
}
