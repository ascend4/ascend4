/*
 *  Assert implementation override for ASCEND unit tests
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascPrint.h>
#include <ascend/compiler/redirectFile.h>

#define f_vtable_name "asc_test_vtable"

/** @NOTE
the problem with these functions is that they only do what they appear to do
when USE_ASC_PRINTF is defined in ascend/general/platform.h, which currently
only occurs on Windows. An alternative way to suppress debug output is via
error_reporter_set_callback, but this doesn't prevent ALL output, eg via
Asc_FPrintF or CONSOLE_DEBUG. There is also some code possibility of using the
test/redirectStdStreams.h code but not attempting that at this stage
-- JP, Jan 2010
*/

static struct Asc_PrintVTable f_vtable = {f_vtable_name, vfprintf, fflush, NULL};
static int f_vtable_registered = FALSE;

int test_enable_printing(void){
	if(TRUE == f_vtable_registered){
		return TRUE;
	}else{
		f_vtable_registered = TRUE;
		return (0 == Asc_PrintPushVTable(&f_vtable)) ? TRUE : FALSE ;
	}
}


void test_disable_printing(void){
	if (TRUE == f_vtable_registered) {
		f_vtable_registered = FALSE;
		Asc_PrintRemoveVTable(f_vtable_name);
	}
}


int test_printing_enabled(void){
	return TRUE;

	return f_vtable_registered;
}

