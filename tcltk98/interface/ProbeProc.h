/*
 *  ProbeProc.h
 *  by Ben Allan
 *  Created: 6/97
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: ProbeProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 *  This probe implementation constitutes a total rework of the probe.
 */

#ifndef ProbeProc_module_loaded
#define ProbeProc_module_loaded

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "interface/ProbeProc.h" (which is sort of obvious)
 */

STDHLF_H(Asc_ProbeCmd);
/* Defines a long help string function */

extern int Asc_ProbeCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[]);
/*
 * status = Asc_ProbeCmd(cdata,interp,argc,argv);
 * (I'm tempted to quote Lord of the Rings, but it suffices to
 * say we now have ONE probe command.)
 * This is the tcl callback for our commandline help facility.
 * Registered as: */
#define Asc_ProbeCmdHN "__probe"
/* Usage: */
#define Asc_ProbeCmdHU \
 "__probe option args\noption is one of: add, clear, destroy, expand, " \
 "filters, get,	qlfdid, name, size, trash, update."
#define Asc_ProbeCmdHS \
 "The probe is a set of instance name collections. Each collection is " \
 "indicated by a number >= 0 contains 0 or more names."
#define Asc_ProbeCmdHL1 \
"\
 * A collection created can be empty, or become empty.\n\
 * Each name may be defined by an ASCEND instance or may be UNCERTAIN\n\
 * if the existence of the name has not been determined lately by\n\
 * update. Names are indexed from 0 up in a collection.\n\
 *\n\
"
#define Asc_ProbeCmdHL2 \
"\
 *	add	<number> <instance-name> [filter-vars]\n\
 *		Adds the named instance to the numbered collection.\n\
 *		If filter-booleans are provided, adds the instances\n\
 *		resulting from a search in the named instance tree\n\
 *		instead of the instance itself.\n\
"
#define Asc_ProbeCmdHL3 \
"\
 *	clear	<number> [indices]\n\
 *		Deletes all entries in numth collection unless specific\n\
 *		indices are given. Valid indices are from 0 on up.\n\
 *		Ranges are not supported. Indices must be increasing\n\
"
#define Asc_ProbeCmdHL4 \
"\
 *	destroy\n\
 *		Destroys all probe information everywhere in C land.\n\
 *	expand	\n\
 *		Adds a collection to the C structures and returns\n\
 *		the number of the new (empty) collection.\n\
"
#define Asc_ProbeCmdHL5 \
"\
 *	filters\n\
 *		Returns an ordered list of names/explanations for the \n\
 *		filter-vars, each is a boolean flag.\n\
"
#define Asc_ProbeCmdHL6 \
"\
 *	get	<number> [indices]\n\
 *		Returns strings in a list describing the indices in\n\
 *		the collection numbered. If no indices given, the complete\n\
 *		collection list is returned.\n\
"
#define Asc_ProbeCmdHL7 \
"\
 *	invalidate\n\
 *		Changes the status of all names in all collections to\n\
 *		UNCERTAIN.\n\
 *	name	<number> <index>\n\
 *		Returns the name corresponding to number and index.\n\
"
#define Asc_ProbeCmdHL8 \
"\
 *	qlfdid	<number> <index>\n\
 *		Sets the g_search_inst to point at the specified instance.\n\
 *		This will not force an UNCERTAIN instance to become defined.\n\
 *		If the name is uncertain, g_search_inst will be as if\n\
 *		qlfdid had failed and '0' will be returned, OTHERWISE '1'.\n\
"
#define Asc_ProbeCmdHL9 \
"\
 *	size	[number]\n\
 *		Returns the number of names in the numbered collection, or\n\
 *		if no number is specified, the number of collections. -1=err\n\
 *	trash	[number]\n\
 *		Removes any UNCERTAIN names in the collection specified or\n\
 *		in all collections if no number is given.\n\
"
#define Asc_ProbeCmdHL10 \
"\
 *	update	[number]\n\
 *		Attempts to resolve all uncertain names in the collection\n\
 *		specified, or in all collections if no number given.\n\
"
#define Asc_ProbeCmdHL11 \
"\
 *\n\
 * FILTER-VARS are a number of values (currently all boolean) \n\
 * with positional meanings.\n\
 * All filters must be specified if any are.\n\
"
/*
 * "__probe" will almost certainly move g_search_inst on most calls.
 */

#endif /*ProbeProc_module_loaded*/



