/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Abbott.
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
	Code to support dynamic and static loading of user packages.

	'Packages' are bits of code that add functionality to ASCEND, like 'plugins'
	or 'addins' on other projects. In ASCEND we have a few that are pretty much
	essential -- the 'built in' packges. They are only packages in the sense
	that a 'register' function must be called to connect them correctly with
	the rest of the system.
	
	Next there are the 'static' packages. These ones are linked into
	The default state is to have packages. As such it takes an explicit
	definition of NO_PACKAGES, if packages are not to be handled.
	An explicit definition of STATIC_PACKAGES or DYNAMIC_PACKAGES is also
	required.
*//*
	by Kirk Abbott
	Created: July 4, 1994
	Last in CVS: 1.14 ballan 1998/03/06 15:47:14
*/

#if !defined(DYNAMIC_PACKAGES) && !defined(STATIC_PACKAGES) && !defined(NO_PACKAGES)
# error "Package linking option not set!"
#endif

#include <math.h>
#include <ctype.h>  /* was compiler/actype.h */

#include <utilities/ascConfig.h>
#include <utilities/config.h> /* NEW */

#ifndef ASC_DEFAULTPATH
# error "Where is ASC_DEFAULTPATH???"
#endif

#include <general/ospath.h>


#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include <compiler/importhandler.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "extcall.h"
#include "mathinst.h"
#include "instance_enum.h"
#include "instquery.h"
#include "atomvalue.h"
#include "find.h"
#include "rel_blackbox.h"
#include "vlist.h"
#include "relation.h"
#include "safe.h"
#include "relation_util.h"
#include "extfunc.h"
#include <packages/sensitivity.h>
#include <packages/ascFreeAllVars.h>
#include <packages/defaultall.h>
#include "module.h"
#include "packages.h"

/*
	Initialise the slv data structures used when calling external fns
*/
void Init_BBoxInterp(struct BBoxInterp *interp)
{
  if (interp){
    interp->status = calc_all_ok;
    interp->user_data = NULL;
    interp->task = bb_none;
  }
}

/*---------------------------------------------
  BUILT-IN PACKAGES...
*/

/**
	Load builtin packages, unless NO_PACKAGES.

	@return 0 if success, 1 if failure.
*/
static
int Builtins_Init(void){
  int result = 0;

#ifdef NO_PACKAGES
  ERROR_REPORTER_HERE(ASC_USER_WARNING,"Builtins_Init: DISABLED at compile-time");
#else
  /* ERROR_REPORTER_DEBUG("Loading function asc_free_all_variables\n"); */
  result = CreateUserFunctionMethod("asc_free_all_variables"
		,Asc_FreeAllVars
		,1 /* num of args */
		,"Unset 'fixed' flag of all items of type 'solver_var'" /* help */
		,NULL /* user_data */
		,NULL /* destroy fn */
  );

  /* ERROR_REPORTER_DEBUG("Registering EXTERNAL asc_default_self"); */
  result = CreateUserFunctionMethod("asc_default_self"
		,Asc_DefaultSelf
		,1 /* num of args */
		,"Set local child atoms to their ATOMs' DEFAULT values. Also recurses into arrays." /* help */
		,NULL /* user_data */
		,NULL /* destroy fn */
  );

  /* ERROR_REPORTER_DEBUG("Registering EXTERNAL asc_default_all"); */
  result = CreateUserFunctionMethod("asc_default_all"
		,Asc_DefaultAll
		,1 /* num of args */
		,"Call 'default_all' methods on sub-models, then run 'default_self' locally." /* help */
		,NULL /* user_data */
		,NULL /* destroy fn */
  );
#endif
  return result;
}

/* return 0 on success */
int LoadArchiveLibrary(CONST char *partialpath, CONST char *initfunc){

#ifdef DYNAMIC_PACKAGES
	struct FilePath *fp1;
	int result;
	struct ImportHandler *handler=NULL;

	/** 
		@TODO
			* modify SearchArchiveLibraryPath to use the ImportHandler array
			  in each directory in the path.
			* when a file is found, return information about which ImportHandler
			  should be used to open it, then make the call.
	*/

	CONSOLE_DEBUG("Searching for external library '%s'",partialpath);

	importhandler_createlibrary();

	fp1 = importhandler_findinpath(
		partialpath, ASC_DEFAULTPATH, PATHENVIRONMENTVAR,&handler
	);
	if(fp1==NULL){
		CONSOLE_DEBUG("External library '%s' not found",partialpath);
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"External library '%s' not found.",partialpath);
		return 1; /* failure */
	}

	asc_assert(handler!=NULL);
	
	/* CONSOLE_DEBUG("About to import external library..."); */
	/* note the import handler will deal with all the initfunc execution, etc etc */
	result = (*(handler->importfn))(fp1,initfunc,partialpath);
	if(result){
		CONSOLE_DEBUG("Error %d when importing external library of type '%s'",result,handler->name);
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Error importing external library '%s'",partialpath);
		ospath_free(fp1);
		return 1;
	}

	ospath_free(fp1);
  	return 0;
#else

	(void)partialname; (void)initfunc;

# if defined(STATIC_PACKAGES)
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"LoadArchiveLibrary disabled: STATIC_PACKAGES, no need to load dynamically.\n");
	return 0;
# elif defined(NO_PACKAGES)
	ERROR_REPORTER_HERE(ASC_PROG_ERROR,"LoadArchiveLibrary disabled: NO_PACKAGES");
	return 1;
# else
#  error "Invalid package linking flags"
# endif

#endif
}

/*---------------------------------------------
  STATIC_PACKAGES code only...

  Declare the functions which we are expected to be able to call.
*/
#ifndef NO_PACKAGES
# ifdef STATIC_PACKAGES

#include <packages/kvalues.h>
#include <packages/bboxtest.h>
#include <packages/bisect.h>
#include <packages/sensitivity.h>

# endif
#endif

#ifdef STATIC_PACKAGES
/**
	Load all statically-linked packages

	@return 0 on success, >0 if any CreateUserFunction calls failed.
*/
static int StaticPackages_Init(void){
  int result = 0;

  result += sensitivity_register();
  result += kvalues_register();
  result += bboxtest_register();

  return result;
}
#endif

/**
	This is a general purpose function that will load whatever user
	functions are required according to the compile-time settings.

	If NO_PACKAGES, nothing will be loaded. If DYNAMIC_PACKAGES, then
	just the builtin packages will be loaded. If STATIC_PACKAGES then
	builtin plus those called in 'StaticPackages_Init' will be loaded.
*/
void AddUserFunctions(void){

	CONSOLE_DEBUG("ADDING USER FUNCTIONS");

#ifdef NO_PACKAGES
# ifdef __GNUC__
#  warning "NO_PACKAGES was defined so external packages are disabled"
# endif
	ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"AddUserFunctions disabled at compile-time.");
#else

	CONSOLE_DEBUG("ADDING BUILTINS");
	/* Builtins are always statically linked */
	if (Builtins_Init()) {
		ERROR_REPORTER_NOLINE(ASC_PROG_WARNING
			,"Problem in Builtins_Init: Some user functions not created"
		);
	}

# ifdef DYNAMIC_PACKAGES

	CONSOLE_DEBUG("ADDING DYNAMIC PACKAGES...");
	/* do nothing. WHY? */

# elif defined(STATIC_PACKAGES)
#  ifdef __GNUC__
#   warning "STATIC PACKAGES"
#  endif
 
	CONSOLE_DEBUG("ADDING STATIC PACKAGES FUNCTIONS...");
	/*The following need to be reimplemented but are basically useful as is. */
	if (StaticPackages_Init()) {
	    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING
			,"Problem in StaticPackages_Init(): Some user functions not created"
		);
	}

# endif
#endif
}

