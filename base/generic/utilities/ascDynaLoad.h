/*
 *  -----------------------------------------------------------------
 *    Copyright 1993 D.I.S. - Universita` di Pavia - Italy
 *  -----------------------------------------------------------------
 *
 *  Permission to  use,  copy,   modify,   distribute  this  software
 *  and  its  documentation foar any purpose is hereby granted without
 *  fee, provided that the above copyright  notice   appear   in  all
 *  copies   and  that both that copyright notice and this permission
 *  notice appear in supporting documentation, and that the  name  of
 *  D.I.S.   not  be  used  in advertising or publicity pertaining to
 *  distribution of the software without specific, written prior per-
 *  mission.   D.I.S.  makes no representations about the suitability
 *  of this software for any purpose.  It is provided "as is" without
 *  express or implied warranty.
 *
 *  D.I.S. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, IN-
 *  CLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 *  NO EVENT SHALL D.I.S.  BE LIABLE FOR  ANY  SPECIAL,  INDIRECT  OR
 *  CONSEQUENTIAL  DAMAGES  OR  ANY DAMAGES WHATSOEVER RESULTING FROM
 *  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION  OF  CONTRACT,
 *  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNEC-
 *  TION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* ChangeLog
 * Small changes made by Michael Moore (mdm@cis.ohio-state.edu)
 * December 24th, 1993.
 * The tcl sections ripped out by Kirk Abbott (ka0p@edrc.cmu.edu)
 * September 3rd, 1994.
 * To date the architectures supported are:
 * sun
 * osf,
 * solaris,
 * hpux
 * sgi
 * ultrix
 * possibly aix if we plunder it from tcl8
 *
 * Added Asc_DynamicUnLoad. Ben Allan (ballan@cs.cmu.edu) Jan 1998.
 * Your mileage may vary.
 * UnLoad alleged for sun, hp, sgi, and alpha/osf. It probably works
 * only as well as their dlclose and shl_unload do.
 */

/** @file
 *  Dynamic library routines.
 *  <pre>
 *  Reaquires:
 *        #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __ascdynaload_h_seen__
#define __ascdynaload_h_seen__

extern int Asc_DynamicLoad(CONST char *path, CONST char *initFunc);
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

extern int DynamicLoad(CONST char *path, CONST char *initFunc);
/**<
 *  Loads a dynamic library and calls its initialization function.
 *  This is the standard function wrapping dlopen. It makes no
 *  provision for dynamic unloading, and therefore should not
 *  be used very often.  It is not currently implemented for all
 *  platforms (e.g. Win32).<br><br>
 *
 *  This function returns 1 if it fails to load the file named in
 *  path and find the symbol in initFun as an int function.
 *  Otherwise it returns the result of the call to initFun.  See 
 *  the discussion under Asc_DynamicLoad() for more issues 
 *  arising from this behavior.
 *
 *  @param path     Path to the dynamic library to load (non-NULL).
 *  @param initFunc The DL initialization function to call.
 *  @return The return value from initFunc is returned if specified.
 *          Otherwise, 0 is returned if the library is successfully
 *          loaded, 1 if it is not.
 */
/*
 * note on some systems (ultrix) this header hides a lot of non-static
 * function names which don't appear terribly standard.
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
 *  @return The return value of dlclose/shl_unload/FreeLibrary, or
 *          -3 if there is a problem locating or unloading the library.
 */

extern void *Asc_DynamicSymbol(CONST char *libraryname,
                               CONST char *symbolname);
/**<
 *  Returns a pointer to a symbol exported from a dynamically-linked
 *  library.  It will generally be necessary to cast the returned pointer
 *  to the correct function or data type before use.  If either parameter 
 *  is NULL, or if the library or symbol cannot be located, then NULL will
 *  be returned.
 *  <pre>
 *  Example:
 *    typedef double (*calcfunc)(double *, double *);
 *    calcfunc calc;
 *    calc = (calcfunc))Asc_DynamicSymbol("lib.dll","calc");
 *  </pre>
 *  @param libraryname Name of the dynamic library to query.
 *  @param symbolname  Symbol to look up in the library.
 *  @return A pointer to the symbol in memory, or NULL if not found.
 */

#if (defined(__HPUX__) || defined(__ALPHA_OSF__) || \
     defined(__WIN32__) || defined(__SUN_SOLARIS__) || \
     defined(__SUN_SUNOS__) || defined(__SGI_IRIX__))
#define HAVE_DL_UNLOAD 1
/**<
 *  Set if a platform has a library unload function.
 *  We don't know about ultrix, aix, and others.
 */
#endif

#endif /* __ascdynaload_h_seen__ */

