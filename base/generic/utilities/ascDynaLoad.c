/* What is the source of this file ?? -- johnpye */
/*
 *  -----------------------------------------------------------------
 *    Copyright 1993 D.I.S. - Universita` di Pavia - Italy
 *  -----------------------------------------------------------------
 *
 *  Permission to  use,  copy,   modify,   distribute  this  software
 *  and  its  documentation for any purpose is hereby granted without
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

/*
	This file *should* support unix/linux-style systems (dlfcn.h)
	and Windows.

	Note that under many systems, profiling does not work
	with dynamic libraries!
*/




/*#ifdef DYNAMIC_PACKAGES*/
/* no, no, no. This is used regardless of whether we have dynamic packages. */




#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ascConfig.h"
#include "error.h"
#include "ascPrint.h"
#include "ascPanic.h"
#include "ascMalloc.h"
#include "ascDynaLoad.h"

#include <compiler/instance_enum.h>
#include <general/list.h>
#include <compiler/compiler.h>
#include <compiler/extfunc.h>

typedef int (*ExternalLibraryRegister_fptr_t)(void);

/*--------------------------------------
  GENERIC STUFF
*/

struct ascend_dlrecord {
  char *path;     /* library name */
  void *dlreturn; /* return from dlopen */
  struct ascend_dlrecord *next;
};

/* Linked list of library names & dlopen() return values. */
static
struct ascend_dlrecord *g_ascend_dllist = NULL;

/*
 * Adds a record of the path and handle to the list.
 * If it fails to do this, returns 1, else 0.
 */
static
int AscAddRecord(void *dlreturn, CONST char *path)
{
  struct ascend_dlrecord *new;
  char *keeppath;
  if (dlreturn == NULL || path == NULL) {
    return 1;
  }
  keeppath = ascstrdup((char *)path);
  if (keeppath==NULL) return 1;
  new = (struct ascend_dlrecord *)ascmalloc(sizeof(struct ascend_dlrecord));
  if (new==NULL) {
    ascfree(keeppath);
    return 1;
  }
  new->next = g_ascend_dllist; /* insert at head */
  g_ascend_dllist = new;
  new->path = keeppath;
  new->dlreturn = dlreturn;
  return 0;
}

/*
 * Finds a record of the path given and returns the associated handle.
 * If it fails to do this, returns NULL.
 */
static
void *AscFindDLRecord(CONST char *path)
{
  struct ascend_dlrecord *new;
  if (path == NULL) {
    return NULL;
  }
  new = g_ascend_dllist;
  while (new != NULL && strcmp(new->path,path) != 0) {
    /* advance new until no more new or new with path found */
    new = new->next;
  }
  return (new != NULL) ? new->dlreturn : NULL;
}

/*
 * Finds and returns the handle to path, if one matches, and
 * deletes the record from the list.  Returns NULL if not found.
 */
static
void *AscDeleteRecord(CONST char *path)
{
  struct ascend_dlrecord *nextptr, *lastptr, *old;
  void *dlreturn = NULL;

  if ((g_ascend_dllist == NULL) || (NULL == path)) return NULL;

  if (strcmp(path,g_ascend_dllist->path)==0) {
    /* head case */
    old = g_ascend_dllist;
    g_ascend_dllist = old->next;
    dlreturn = old->dlreturn;
    ascfree(old->path);
    ascfree(old);
  } else {
    lastptr = g_ascend_dllist;
    nextptr = lastptr->next;
    while (nextptr != NULL && strcmp(nextptr->path,path) != 0) {
      lastptr = nextptr;
      nextptr = nextptr->next;
    }
    /* so either nextptr is NULL and not in list, or nextptr is
     * what we want to delete and lastptr is the link to it.
     */
    if (nextptr != NULL) {
      old = nextptr;
      lastptr->next = nextptr->next;
      dlreturn = old->dlreturn;
      ascfree(old->path);
      ascfree(old);
    }
  }
  return dlreturn;
}

/*
 * Checks the list for a conflicting handle so we can issue
 * a more helpful warning, if need be, than the standard message.
 */
static
void AscCheckDuplicateLoad(CONST char *path)
{
  struct ascend_dlrecord *r;

  if (NULL == path) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Null path in AscCheckDuplicateLoad.");
    return;
  }

  r = g_ascend_dllist;
  while (r != NULL) {
    if (strcmp(path,r->path)==0) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Attempt to load already loaded '%s'.",path);
      return;
    }
    r = r->next;
  }
}

/*-----------------------------------------------
	WINDOWS
*/
#if defined(__WIN32__)
# include <windows.h>

int DLEXPORT Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  HINSTANCE xlib;
  ExternalLibraryRegister_fptr_t install = NULL;

  if (NULL == path) {
    FPRINTF(stderr,"Asc_DynamicLoad failed: Null path\n");
    return 1;
  }

  AscCheckDuplicateLoad(path); /* whine if we've see it before */
  /*
   *    If the named library does not exist, if it's not loadable or if
   *    it does not define the named install proc, report an error
   */

  xlib = LoadLibrary(path);
  if (xlib == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad: LoadLibrary failed\n");
    return 1;
  }
  if (NULL != initFun) {
    install = (int (*)())GetProcAddress(xlib,initFun);
    if (install == NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad: Required function %s not found\n", initFun);
      (void)FreeLibrary(xlib);
      return 1;
    }else{
		FPRINTF(ASCERR,"FOUND INITFCN %s AT %d\n",initFun,install);
	}
  }
  if (0 != AscAddRecord(xlib,path)) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad failed to record library (%s)\n",path);
  }
  return (install == NULL) ? 0 : (*install)();
}

# define UNLOAD FreeLibrary
# define DLLSYM GetProcAddress
# define DLL_CAST (HINSTANCE)
# define ASC_DLERRSTRING "unknown"
# define UNLOAD_SUCCESS TRUE

#endif /* __WIN32__ */

/*-----------------------------------------------
	UNIX/LINUX
*/
/*
	SOLARIS and LINUX
*/
/* NOTE, added defined(__unix__) here, not sure if that's a bad thing or not -- johnpye */
/*
	From a quick Google, it appears that AIX 5.1 now provides dlfcn.h,
	so I'll remove the code that was emulating it here. -- johnpye
*/
#if (defined(sun) || defined(linux) || defined(__unix__) || defined(solaris) || defined(_AIX) || defined(_SGI_SOURCE))
# ifndef MACH
#  include <dlfcn.h>
# else
#  error "MACH unsupported"
# endif /* mach */

int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  ExternalLibraryRegister_fptr_t install = NULL;

  if (NULL == path) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad failed: Null path\n");
    return 1;
  }

  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  if (xlib == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"%s",(char *)dlerror());
    return 1;
  }
  if (NULL != initFun) {
    install = (int (*)())dlsym(xlib, initFun);
    if (install == NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"%s",(char *)dlerror());
      dlclose(xlib);
      return 1;
    }
  }

  if (0 != AscAddRecord(xlib,path)) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad failed to record library (%s)\n",path);
  }
  return (install == NULL) ? 0 : (*install)();
}

# define UNLOAD dlclose
# define DLLSYM dlsym
# define DLL_CAST (void *)
# define ASC_DLERRSTRING dlerror()
# define UNLOAD_SUCCESS 0

#endif /* posix: linux, unix, solaris,sgi */

/*-----------------------------------------------
	HPUX
*/
#ifdef __hpux
/*
	Kirk Abbott last fiddled with the following, which was
	originally put in place my Michael Moore for an
	HP/UX 9.X Operating Sys back in 1993. Arrr. No idea if
	it still works.
*/

# include <dl.h>
# include <errno.h>

int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
# define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  shl_t xlib;
  ExternalLibraryRegister_fptr_t install = NULL;
  int i;

  if (NULL == path) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad failed: Null path\n");
    return 1;
  }

  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = shl_load(path, BIND_IMMEDIATE | BIND_VERBOSE, 0L);
  if (xlib == (shl_t) NULL)  {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to load shared library : %s\n",strerror(errno));
    return 1;
  }
  if (NULL != initFun) {
    i = shl_findsym(&xlib, initFun, TYPE_PROCEDURE, &install);
    if (i == -1) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to find needed symbol %s %s\n",
  		       initFun, strerror(errno));
      shl_unload(xlib); /* baa */
      return 1;
    }
    if (install == NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to find needed symbol %s\n",initFun);
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Error type unknown\n");
      shl_unload(xlib); /* baa */
      return 1;
    }
  }
  if (0 != AscAddRecord(xlib,path)) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicLoad failed to record library (%s)\n",path);
  }
  return (install == NULL) ? 0 : (*install)();
}

# define UNLOAD shl_unload
# define DLL_CAST (shl_t)
# define ASC_DLERRSTRING "NULL definition"
# define UNLOAD_SUCCESS 0

#endif /* __hpux */

/*-----------------------------------------------
	Did we get something from the above?
*/

#ifndef ASCDL_OK 
# error "Unable to define an Asc_DynamicLoad function. Check your compiler options and installed system libraries."
#endif

/**-----------------------------------------------
	DYNAMIC UNLOADING
*/

int Asc_DynamicUnLoad(CONST char *path)
{
  void *dlreturn;
  int retval;

  if (NULL == path) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR, "Asc_DynamicUnLoad failed: Null path\n");
    return -3;
  }

  dlreturn = AscDeleteRecord(path);
  if (dlreturn == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR, "Asc_DynamicUnLoad: unable to remember or unload %s\n", path);
    return -3;
  }
  CONSOLE_DEBUG("Asc_DynamicUnLoad: forgetting & unloading %s \n", path);
  /*
   *  dlclose() returns 0 on success, FreeLibrary() returns TRUE.
   *  A uniform convention is preferable, so trap and return 0 on success.
   */
  retval = UNLOAD(DLL_CAST dlreturn);
  return (retval == UNLOAD_SUCCESS) ? 0 : retval;
}

/**-----------------------------------------------
	DYNAMIC VARIABLE LINKING
*/
void *Asc_DynamicVariable(CONST char *libname, CONST char *symbol)
{
  void *dlreturn;
  void *symreturn;
#ifdef __hpux
  int i;
#endif

  if (libname == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicSymbol failed:  Null libname\n");
    return NULL;
  }
  if (symbol == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicSymbol failed:  Null symbol\n");
    return NULL;
  }

  dlreturn = AscFindDLRecord(libname);
  if (dlreturn == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicSymbol: Unable to find requested library %s\n", libname);
    return NULL;
  }
#ifdef __hpux
  i = shl_findsym(&dlreturn, symbol, TYPE_UNDEFINED, &symreturn);
  if (i == -1) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicSymbol: Unable to find requested symbol %s in %s (%s)\n",
                       symbol, libname, strerror(errno));
    symreturn = NULL;
  }
#elif defined(__WIN32__)
  /*
   * Here's a bit of possibly-misdirected casting horror.
   * ISO C forbids casting between function and data pointers, so, naturally,
   * we cast between function and data pointers.  Well, we don't have much
   * choice.  GetProcAddress() returns a function pointer for both functions
   * and variables so we have to do the cast for variables.  This is ok on
   * 32 bit Windows since the pointers are compatible.  Then, to avoid
   * being reminded by the compiler that we're doing something illegal,
   * we apply convoluted casting to shut it up.
   * Oh, the crap you can find on the internet...    JDS
   */
  *(FARPROC*)(&symreturn) = GetProcAddress((HINSTANCE)dlreturn, symbol);
#else
  /* no problem on POSIX systems - dlsym() returns a void *. */
  symreturn = dlsym(dlreturn, symbol);
#endif
  if (symreturn == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicSymbol: Unable to find requested symbol %s in %s\n",symbol,libname);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Error type %s\n",ASC_DLERRSTRING);
  }
  return symreturn;
}

/**-----------------------------------------------
	DYNAMIC FUNCTION LINKING
*/
DynamicF Asc_DynamicFunction(CONST char *libname, CONST char *symbol)
{
  void *dlreturn;
  DynamicF symreturn;
#ifdef __hpux
  int i;
#endif

  if (libname == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicFunction failed:  Null library name\n");
    return NULL;
  }
  if (symbol == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicFunction failed:  Null function name\n");
    return NULL;
  }

  dlreturn = AscFindDLRecord(libname);
  if (dlreturn == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicFunction: Unable to find requested library %s\n", libname);
    return NULL;
  }
#ifdef __hpux
  i = shl_findsym(&dlreturn, symbol, TYPE_UNDEFINED, &symreturn);
  if (i == -1) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicFunction: Unable to find requested function %s in %s (%s)\n",
                       symbol, libname, strerror(errno));
    symreturn = NULL;
  }
#elif defined(__WIN32__)
  /* no problem on Windows - GetProcAddress() returns a function pointer. */
  symreturn = (DynamicF)GetProcAddress((HINSTANCE)dlreturn, symbol);
#else
  /*
   * Here's the corresponding bit of possibly-misdirected casting horror.
   * ISO C forbids casting between function and data pointers, so, naturally,
   * we cast between function and data pointers.  Well, we don't have much
   * choice.  dlsym() returns a void* for both variables and functions so we
   * have to do the cast for functions.  This is ok on POSIX systems since the
   * pointer types are compatible.  Then, to avoid being reminded by the
   * compiler that we're doing something illegal, we apply convoluted casting
   * to shut it up.  Oh, the crap you can find on the internet...   JDS
   */
  *(void**)(&symreturn) = dlsym(dlreturn, symbol);
#endif
  if (symreturn == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Asc_DynamicFunction: Unable to find requested function %s in %s\n",symbol,libname);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Error type %s\n",ASC_DLERRSTRING);
  }
  return symreturn;
}



/* #endif */


