/** 
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


/** 
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

#ifndef __ASCDYNALOAD_H_SEEN__
#define __ASCDYNALOAD_H_SEEN__
/** use of this header requires
 * #include "utilities/ascConfig.h"
 */


/** 
 * error = Asc_DynamicLoad(path, initFun);
 * Returns 1 if fails to load the file named in path and find 
 * the symbol in initFun as an int function.
 * Otherwise returns the result of the call to initFun.
 * If initFun == NULL, calls nothing and returns 0 after opening
 * library.
 *
 * This is our wrapping dlopen. It makes
 * provision for dynamic unloading.
 * Once all references to the previously loaded library have been
 * scheduled to be removed without further ado, it can be unloaded
 * on most architectures.
 */
extern int Asc_DynamicLoad(CONST char *,CONST char *);

/** 
 * error = DynamicLoad(path, initFun);
 * Returns 1 if fails to load the file named in path and find 
 * the symbol in initFun as an int function.
 * Otherwise returns the result of the call to initFun.
 *
 * This is the standard function wrapping dlopen. It makes no
 * provision for dynamic unloading, and therefore should not
 * be used very often.
 */
extern int DynamicLoad(CONST char *, CONST char *);
/** 
 * note on some systems (ultrix) this header hides a lot of non-static
 * function names which don't appear terribly
 * standard.
 */

/** 
 * Asc_DynamicUnLoad(path);
 * Attempts to find our record of loading a module of the same
 * path and unload it. 
 *
 * This is our wrapping dlclose. It provides unloading.
 * Once all references to the previously loaded library have been
 * scheduled to be removed without further ado, it can be unloaded
 * on most architectures. Once you call this function, you damn
 * well better not reference functions or data that came from
 * the path given.
 *
 * returns the output of dlclose (shl_unload) or -3 if confused.
 */
extern int Asc_DynamicUnLoad(CONST char *);

/** 
 * yourFuncOrVar = (YOURCAST)Asc_DynamicSymbol(libraryname,symbolname);
 * rPtr =
 *  (double (*)(double *, double *))Asc_DynamicSymbol("lib.dll","calc");
 * returns you a pointer to a symbol exported from the dynamically
 * linked library named, if the library is loaded and the
 * symbol can be found in it.
 */
extern void *Asc_DynamicSymbol(CONST char *, CONST char *);

#endif /** __ASCDYNALOAD_H_SEEN__ */
