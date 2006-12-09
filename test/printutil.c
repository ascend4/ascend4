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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPrint.h>
#include <compiler/redirectFile.h>

#define f_vtable_name "asc_test_vtable"

static struct Asc_PrintVTable f_vtable = {f_vtable_name, vfprintf, fflush, NULL};
static int f_vtable_registered = FALSE;

int test_enable_printing(void){
	fprintf(stderr,"PRINTING ENABLED\n");
	return TRUE;

	/* old code... */
	if (TRUE == f_vtable_registered) {
		fprintf(stderr,"PRINTING *ALREADY* ENABLED\n");
		return TRUE;
	}
	else {
		fprintf(stderr,"PRINTING ENABLED\n");
		f_vtable_registered = TRUE;
		return (0 == Asc_PrintPushVTable(&f_vtable)) ? TRUE : FALSE ;
	}
}


void test_disable_printing(void){
	fprintf(stderr,"PRINTING DISABLED\n");
	return;

	/* old code... */
	if (TRUE == f_vtable_registered) {
		f_vtable_registered = FALSE;
		Asc_PrintRemoveVTable(f_vtable_name);
	}
}


int test_printing_enabled(void){
	return TRUE;

	return f_vtable_registered;
}

