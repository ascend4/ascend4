/*
 *  Ascend Environment Variable Imitation Tcl interface
 *  by Ben Allan
 *  Created: 6/3/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: EnvVarProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Ascend Environment Variable Imitation Tcl interface.
 *
 *  This file exists because win32, among others, can't keep their
 *  POSIX compliance up. In particular, getting and setting
 *  environment vars is exceedingly unreliable.  This file implements 
 *  a general way to store and fetch multiple paths.
 *  It does not interact in any way the the Tcl global "env" array.
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASCTK_ENVVARPROC_H
#define ASCTK_ENVVARPROC_H

STDHLF_H(Asc_EnvVarCmd);

extern int Asc_EnvVarCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[]);
/**<
 * <!--  Asc_EnvVarCmd(cdata,interp,argc,argv);                        -->
 * This is the tcl callback for our commandline help facility.
 */

/** Registered as */
#define Asc_EnvVarCmdHN "asc_env"
/** Usage */
#define Asc_EnvVarCmdHU \
 "asc_env option args\n\
  options are append, export, get, import, list, names, put, and set."
/** Short help text */
#define Asc_EnvVarCmdHS \
 "asc_env manipulates a database of strings for the ascend application"
/** Long help text part 1 */
#define Asc_EnvVarCmdHL1 \
"\
 * These strings are only loosely tied to either the C or Tcl environment\n\
 * on flakey non-UNIX platforms.\n\
 * The options are as follows:\n\
"
/** Long help text part 2 */
#define Asc_EnvVarCmdHL2 \
"\
 * 	append	var pathelement\n\
 *		Adds pathelement to the ascend environment string var.\n\
 *		Path elements may contain spaces, but should not\n\
 *		contain path dividers.\n\
"
/** Long help text part 3 */
#define Asc_EnvVarCmdHL3 \
"\
 * 	export	var\n\
 *		Adds the definition of var to the Tcl global env array.\n\
 *		This does not LINK the var and Tcl env, so changes\n\
 *		with asc_env may need to be followed by reexporting.\n\
 *		Whether or not the C environment sees changes to Tcl\n\
 *		env depends entirely on the Tcl core implementation.\n\
"
/** Long help text part 4 */
#define Asc_EnvVarCmdHL4 \
"\
 * 	get 	var\n\
 *		Returns the ascend environment string var. If var has\n\
 *		more than one element, then the elements are glued\n\
 *		together into a single string using the pathdiv separator.\n\
 *		UNIX pathdiv is : and MS pathdiv is ; .\n\
"
/** Long help text part 5 */
#define Asc_EnvVarCmdHL5 \
"\
 * 	import	var\n\
 *		Sets the ascend environment var using the result of a \n\
 *		call to C getenv(). Tcl env may or may not match.\n\
 * 	list	var\n\
 *		Returns a Tcl list of the elements stored for ascend var.\n\
"
/** Long help text part 6 */
#define Asc_EnvVarCmdHL6 \
"\
 * 	names	\n\
 *		Takes no arguments and returns the names of known\n\
 *		ascend environment variables as a Tcl list.  \n\
"
/** Long help text part 7 */
#define Asc_EnvVarCmdHL7 \
"\
 * 	put	input_string\n\
 *		Parses the input string using the form %s = %s to extract\n\
 *		a varname (before the =) and path value. The path value is\n\
 *		assumed to be separated by the platform value of pathdiv\n\
 *		as defined for getenv above and split accordingly into\n\
 *		pathelements.\n\
"
/** Long help text part 8 */
#define Asc_EnvVarCmdHL8 \
"\
 * 	set	var path \n\
 *		Like putenv, but you have done the split on = for us.\n\
 *  \n\
"
/** Long help text part 9 */
#define Asc_EnvVarCmdHL9 \
"\
 * These commands manage a global array of strings in the process. \n\
 * This global array is somewhat more intelligent than the average\n\
 * UNIX environment variable implementation for our purposes. \n\
 * These strings are accessible to other clients through the C api \n\
"
/** Long help text part 10 */
#define Asc_EnvVarCmdHL10 \
"\
 * in utilities/ascEnvVar.h. We do this because we don't want our \n\
 * application core including Tcl code, and because Windoze doesn't\n\
 * handle environment variables reliably. \n\
 * These functi/ns will not work until the C api has been initialized \n\
 * elsewhere.\n\
"

#endif  /* ASCTK_ENVVARPROC_H */

