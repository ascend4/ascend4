/*	ASCEND modelling environment
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
	Dynamic library routines.

	Requires:
	      #include "utilities/ascConfig.h"
	</pre>
*//*
	ChangeLog
	Added Asc_DynamicUnLoad. Ben Allan (ballan@cs.cmu.edu) Jan 1998.
	Your mileage may vary.
	UnLoad alleged for sun, hp, sgi, and alpha/osf. It probably works
	only as well as their dlclose and shl_unload do.
	Split Asc_DynamicSymbol() into Asc_DynamicVariable() and
	Asc_DynamicFunction() so callers don't have to cast between
	data and function pointers (forbidden by ISO C).  JDS Dec 2005
*/

#ifndef ASC_ASCDYNALOAD_H
#define ASC_ASCDYNALOAD_H

#include <utilities/ascConfig.h>

ASC_DLLSPEC(int) Asc_DynamicLoad(CONST char *path, CONST char *initFunc);
/**<
 *  Loads a dynamic library and calls its initialization function.
 *  This is our function wrapping dlopen/LoadLibrary.  It makes
 *  provision for dynamic unloading using Asc_DynamicUnLoad().<br><br>
 *
 *  Returns 1 if it fails to load the library named in path and find
 *  the symbol initFunc as an int function.  Otherwise, it returns
 *  the result of the call to (*initFunc).  If initFunc == NULL,
 *  nothing is called and 0 is returned after opening the library.
 *  If path == NULL, 1 is always returned.<br><br>
 *
 *  A consequence of this behavior is that initFunc had better not
 *  return 1, or you won't be able to tell whether the library was
 *  successfully loaded or not.  If it's under your control, have
 *  initFunc only return values other than 1 so you can detect the
 *  proper status.<br><br>
 *
 *  @param path     Path to the dynamic library to load (non-NULL).
 *  @param initFunc The DL initialization function to call.
 *  @return The return value from initFunc is returned if specified.
 *          Otherwise, 0 is returned if the library is successfully
 *          loaded, 1 if it is not.
 */

extern int Asc_DynamicUnLoad(CONST char *path);
/**<
 *  Attempts to unload a dynamic library.
 *  This function tries to look up the previously-loaded library
 *  in path and unload it.  Only libraries successfully loaded using
 *  Asc_DynamicLoad() may be unloaded using this function.<br><br>
 *
 *  This is our function wrapping dlclose/shl_unload/FreeLibrary
 *  which provides unloading.  Once all references to the
 *  previously-loaded library have been scheduled to be removed
 *  without further ado, it can be unloaded on most architectures.
 *  Once you call this function, you damn well better not reference
 *  functions or data that came from the path given.  Passing a NULL
 *  path will always result in an error condition (-3) being returned.
 *
 *  @param path     Path to the dynamic library to unload.
 *  @return Returns 0 if the library was successfully located and unloaded,
 *          -3 if the library was not located, or the return value of
 *          dlclose/shl_unload/FreeLibrary otherwise.  Note that the
 *          return value conventions differ between platforms, so if you
 *          get a return value that is not 0 or -3, you are in platform-
 *          specific hell.
 */

#define Asc_DynamicSymbol(a,b) Asc_DynamicVariable((a),(b))
/**< For backward compatibility to old name of Asc_DynamicVariable() */

extern void *Asc_DynamicVariable(CONST char *libraryname,
                                 CONST char *varname);
/**<
 *  Returns a pointer to a variable exported from a dynamically-linked
 *  library.  It will generally be necessary to cast the returned
 *  pointer to the correct data type before use.  If either parameter
 *  is NULL, or if the library or symbol cannot be located, then NULL
 *  will be returned.<br><br>
 *
 *  This function was previously called Asc_DynamicSymbol() and could
 *  be used to retrieve either variables or functions from a library.
 *  This necessitated casting the returned void* to a function pointer
 *  for exported functions, which is forbidden by ISO C.  Functions
 *  may now be retrieved using Asc_DynamicFunction(), thus avoiding the
 *  need for the caller to cast between data and function pointers.
 *  Never mind what the implementation does to achieve this.
 *  <pre>
 *  Example:
 *    int *value;
 *    value = (int *)Asc_DynamicVariable("lib.dll", "g_variable");
 *  </pre>
 *  @param libraryname Name of the dynamic library to query.
 *  @param varname     Name of variable to look up in the library.
 *  @return A pointer to the variable in memory, or NULL if not found.
 */

typedef void (*DynamicF)(void);
/**<  Function pointer type returned by Asc_DynamicFunction(). */

extern DynamicF Asc_DynamicFunction(CONST char *libraryname,
                                    CONST char *funcname);
/**<
 *  Returns a pointer to a function exported from a dynamically-linked
 *  library.  It will generally be necessary to cast the returned pointer
 *  to the correct function type before use.  If either parameter
 *  is NULL, or if the library or function cannot be located, then NULL
 *  will be returned.
 *  <pre>
 *  Example:
 *    typedef double (*calcfunc)(double *, double *);
 *    calcfunc calc;
 *    calc = (calcfunc))Asc_DynamicFunction("lib.dll","calc");
 *  </pre>
 *  @param libraryname Name of the dynamic library to query.
 *  @param funcname   Name of function to look up in the library.
 *  @return A pointer to the function in memory, or NULL if not found.
 */

#if defined(__GNUC__) || (defined(__HPUX__) || defined(__ALPHA_OSF__) || \
     defined(__WIN32__) || defined(__SUN_SOLARIS__) || \
     defined(__SUN_SUNOS__) || defined(__SGI_IRIX__))
#define HAVE_DL_UNLOAD 1
/**<
 *  Set if a platform has a library unload function.
 *  We don't know about aix, and others.
 */
#endif /* gnu,hp,alpha,win,solaris,sunos,irix */

#endif /* ASC_ASCDYNALOAD_H */
