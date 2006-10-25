/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	@file
	Initialization Routines.

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
	#include "fractions.h"
	#include "compiler.h"
	#include "dimen.h"
	#include "expr_types.h"
*//*
	by Tom Epperly
	Created: 3/24/1990
	Version: $Revision: 1.11 $
	Version control file: $RCSfile: initialize.h,v $
	Date last modified: $Date: 1998/03/17 22:08:40 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_INITIALIZE_H
#define ASC_INITIALIZE_H

#include <utilities/ascConfig.h>

/** Set the procedure stack limit */
extern void SetProcStackLimit(unsigned long l);
/**<
	@param l the stack limit value to be set.

	The stack limit starts out at INITSTACKLIMIT. The limit exists to prevent infinite loops from running the machine out of C automatic variable space.
 */

/** Initial stack limit. */
#define INITSTACKLIMIT 20

/** Get the procedure stack limit currently set. */
extern unsigned long GetProcStackLimit(void);

/** Run a METHOD on a model. */
ASC_DLLSPEC(enum Proc_enum) Initialize(struct Instance *context,
							    struct Name *name,
							    char *cname,
							    FILE *err,
							    wpflags options,
							    struct gl_list_t *watchpoints,
							    FILE *log);
/**<
	@param context instance in which to run the METHOD.
	@param name initialisation METHOD being called
	@param cname string form of the METHOD name (used in error messages)
	@param err file to which error messages are issued
	@param log file to debugging messages are issued. If NULL, no logging is done (faster).
	@param wpflags watchpoint settings
	@param watchpoints list of watchpoints

	@return Proc_all_ok is all went well, else an error code (@see enum Proc_enum)

	This procedure will execute the initialization code indicated by name with respect to the context instance.
	If watchlist is NULL or flog is NULL, the debug output options corresponding to watchlist and flog will be ignored.
	File pointers 'log' and 'err' should not be the same pointer in a good ui design.

	Maximum speed comes from @code Initialize(context,procname,cname,ferr,0,NULL,NULL); @endcode
 */

/** Run a class-access METHOD, eg "RUN MyType::values" */
extern enum Proc_enum ClassAccessInitialize(struct Instance *context,
                                            struct Name *class_name,
                                            struct Name *name,
                                            char *cname,
                                            FILE *err,
                                            wpflags options,
                                            struct gl_list_t *watchpoints,
                                            FILE *log);
/**<
	@param class_name the class being called (eg MyType)
	@param name the METHOD being called (eg values)
	@param cname string version of method name, for use in error messages
	@param err file to which error messages will be output
	@param log file to which debugging messages will be output. NULL if no debug output is deired.
	@param wpflags watchpoint flags
	@param watchpoints list of watchpoints. NULL if no watchpoints are required.

	@return Proc_all_ok if success, else an error code.

	Will attempt to run the initialization procedure given by using so-called class access, i.e., deals with syntax such as "RUN Mytype::values."

	If watchlist is NULL or flog is NULL, the debug output options corresponding to watchlist and flog will be ignored. error (and possibly debugging) messages are issued on the files given. Maximum speed comes from @code ClassAccessInitialize(context,class,procname,cname,ferr,0,NULL,NULL); @endcode
*/

/** Search for a named procedure on an instance */
ASC_DLLSPEC(struct InitProcedure *) FindProcedure(CONST struct Instance *i,
                                           symchar *procname);
/**<
	@param procname the name of the procedure searched for
	@return pointer to found procedure, or NULL if none found.
*/

/** Search a list for a named procedure. */
ASC_DLLSPEC(struct InitProcedure *) SearchProcList(CONST struct gl_list_t *list,
                                            symchar *name);
/**<
	@param list (generally you will use the output from @code GetInitializationList() @endcode)
	@return pointer to found procedure, or NULL if none found.
 */

#endif /* ASC_INITIALIZE_H */
