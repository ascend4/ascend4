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

	This file *should* support unix/linux-style systems (dlfcn.h)
	and Windows.

	Note that under many systems, profiling does not work
	with dynamic libraries!
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ascConfig.h"
#include "error.h"
#include "ascPrint.h"
#include "ascPanic.h"
#include "ascMalloc.h"
#include "ascDynaLoad.h"
#include "ascEnvVar.h"

#include <general/env.h>
#include <general/ospath.h>
#include <compiler/instance_enum.h>
#include <general/list.h>
#include <compiler/compiler.h>
#include <compiler/extfunc.h>
#include <compiler/importhandler.h>

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
static struct ascend_dlrecord *g_ascend_dllist = NULL;

/*
 * Adds a record of the path and handle to the list.
 * If it fails to do this, returns 1, else 0.
 */
static int AscAddRecord(void *dlreturn, CONST char *path){
  struct ascend_dlrecord *new;
  char *keeppath;
  if (dlreturn == NULL || path == NULL) {
    return 1;
  }
  keeppath = ascstrdup((char *)path);
  if (keeppath==NULL) return 1;
  new = ASC_NEW(struct ascend_dlrecord);
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Null path");
    return;
  }

  r = g_ascend_dllist;
  while (r != NULL) {
    if (strcmp(path,r->path)==0) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Attempt to load already loaded '%s'.",path);
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

int Asc_DynamicLoad(CONST char *path, CONST char *initFun){
  HINSTANCE xlib;
  ExternalLibraryRegister_fptr_t install = NULL;

  if (NULL == path) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed: Null path\n");
    return 1;
  }

  AscCheckDuplicateLoad(path); /* whine if we've see it before */
  /*
   *    If the named library does not exist, if it's not loadable or if
   *    it does not define the named install proc, report an error
   */

  xlib = LoadLibrary(path);
  if (xlib == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"LoadLibrary failed\n'%s'",path);
    return 1;
  }
  ERROR_REPORTER_HERE(ASC_PROG_NOTE,"LoadLibrary succeeded, '%s'\n",path);

  if (NULL != initFun) {
    install = (int (*)(void))GetProcAddress(xlib,initFun);
    if (install == NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Required function '%s' not found", initFun);
      (void)FreeLibrary(xlib);
      return 1;
    }else{
		FPRINTF(ASCERR,"FOUND INITFCN %s AT %d\n",initFun,install);
	}
  }
  if (0 != AscAddRecord(xlib,path)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to record library (%s)\n",path);
  }
  return (install == NULL) ? 0 : (*install)();
}
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */

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
  void *xlib;
  ExternalLibraryRegister_fptr_t install = NULL;

  if (NULL == path) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed: null path");
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to record library (%s)",path);
  }
  return (install == NULL) ? 0 : (*install)();
}
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */

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
  shl_t xlib;
  ExternalLibraryRegister_fptr_t install = NULL;
  int i;

  if (NULL == path) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed: Null path");
    return 1;
  }

  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = shl_load(path, BIND_IMMEDIATE | BIND_VERBOSE, 0L);
  if (xlib == (shl_t) NULL)  {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to load shared library: %s",strerror(errno));
    return 1;
  }
  if (NULL != initFun) {
    i = shl_findsym(&xlib, initFun, TYPE_PROCEDURE, &install);
    if (i == -1) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find needed symbol '%s': %s",
  		       initFun, strerror(errno));
      shl_unload(xlib); /* baa */
      return 1;
    }
    if(install == NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find needed symbol '%s'. Error type unknown",initFun);
      shl_unload(xlib); /* baa */
      return 1;
    }
  }
  if (0 != AscAddRecord(xlib,path)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to record library (%s)",path);
  }
  return (install == NULL) ? 0 : (*install)();
}
# define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */

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
    ERROR_REPORTER_HERE(ASC_PROG_ERR, "Failed: Null path");
    return -3;
  }

  dlreturn = AscDeleteRecord(path);
  if (dlreturn == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR, "Unable to remember or unload %s", path);
    return -3;
  }
  CONSOLE_DEBUG("Asc_DynamicUnLoad: forgetting & unloading %s", path);
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed:  Null libname");
    return NULL;
  }
  if (symbol == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed:  Null symbol");
    return NULL;
  }

  dlreturn = AscFindDLRecord(libname);
  if (dlreturn == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested library %s", libname);
    return NULL;
  }
#ifdef __hpux
  i = shl_findsym(&dlreturn, symbol, TYPE_UNDEFINED, &symreturn);
  if (i == -1) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested symbol '%s' in %s (%s)",
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested symbol '%s' in %s",symbol,libname);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Error type: %s",ASC_DLERRSTRING);
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed: null library name");
    return NULL;
  }
  if (symbol == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed: null function name");
    return NULL;
  }

  dlreturn = AscFindDLRecord(libname);
  if (dlreturn == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested library %s", libname);
    return NULL;
  }
#ifdef __hpux
  i = shl_findsym(&dlreturn, symbol, TYPE_UNDEFINED, &symreturn);
  if (i == -1) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested function '%s' in %s (%s)",
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
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find requested function '%s' in %s",symbol,libname);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Error type: %s",ASC_DLERRSTRING);
  }
  return symreturn;
}


/*-----------------------------------------------------------------------------
  SEARCHING FOR LIBRARIES
*/

/**
	A little structure to help with searching for libraries

	@see test_librarysearch
*/
struct LibrarySearch{
	struct FilePath *partialpath;
	char fullpath[PATH_MAX];
};

FilePathTestFn test_librarysearch;

/**
	A 'test' function for passing to the ospath_searchpath_iterate function.
	This test function will return a match when a library having the required
	name is present in the fully resolved path.
*/
int test_librarysearch(struct FilePath *path, void *userdata){
	/*  user data = the relative path, plus a place
		to store the full path when found */
	FILE *f;
	struct LibrarySearch *ls;
	struct FilePath *fp;

	ls = (struct LibrarySearch *)userdata;
	fp = ospath_concat(path,ls->partialpath);
	if(fp==NULL){
		char *tmp;
		tmp = ospath_str(path);
		CONSOLE_DEBUG("Unable to concatenate '%s'...",tmp);
		ospath_free_str(tmp);
		tmp = ospath_str(ls->partialpath);
		CONSOLE_DEBUG("... and '%s'...",tmp);
		ospath_free_str(tmp);
		return 0;
	}

	ospath_strncpy(fp,ls->fullpath,PATH_MAX);
	/* CONSOLE_DEBUG("SEARCHING FOR %s",ls->fullpath); */

	f = ospath_fopen(fp,"r");
	if(f==NULL){
		ospath_free(fp);
		return 0;
	}
	fclose(f);

	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"FOUND! %s\n",ls->fullpath); */
	ospath_free(fp);
	return 1;
}

/**
	@DEPRECATED this function needs to be rewritten to use 'ImportHandler'
	functionality.
*/
char *SearchArchiveLibraryPath(CONST char *name, char *dpath, char *envv){
	struct FilePath *fp1, *fp2, *fp3; /* relative path */
	char *s1;
	char *buffer;

	struct LibrarySearch ls;
	struct FilePath **sp;
	char *path, *foundpath;
	ospath_stat_t buf;
	FILE *f;

	fp1 = ospath_new_noclean(name);
	if(fp1==NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid partial path '%s'",name);
		ospath_free(fp1);
		return NULL;
	}

	s1 = ospath_getfilestem(fp1);
	if(s1==NULL){
		/* not a file, so fail... */
		return NULL;
	}

	fp2 = ospath_getdir(fp1);
	if(fp2==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"unable to retrieve file dir");
		return NULL;
	}

	buffer = importhandler_extlib_filename(s1);

	fp3 = ospath_new(buffer);
	ASC_FREE(buffer);
	ospath_free(fp1);
	fp1 = ospath_concat(fp2,fp3);
	ospath_free(fp2);
	ospath_free(fp3);
	ospath_free_str(s1);

	/* attempt to open "name" directly */
	if(0==ospath_stat(fp1,&buf) && NULL!=(f = ospath_fopen(fp1,"r")) ){
		char *tmp;
		tmp = ospath_str(fp1);
		CONSOLE_DEBUG("Library '%s' opened directly, without path search",tmp);
		ospath_free_str(tmp);
		fp2 = ospath_getabs(fp1);
		foundpath = ospath_str(fp2);
		ospath_free(fp2);
		fclose(f);
	}else{

		ls.partialpath = fp1;

		path=Asc_GetEnv(envv);
		if(path==NULL){
			/* CONSOLE_DEBUG("Library search path env var '%s' not found, using default path '%s'",envv,dpath); */
			path=dpath;
		}

		/* CONSOLE_DEBUG("SEARCHPATH IS %s",path); */
		sp = ospath_searchpath_new(path);

		if(NULL==ospath_searchpath_iterate(sp,&test_librarysearch,&ls)){
			ospath_free(fp1);
			ospath_searchpath_free(sp);
			return NULL;
		}

		foundpath = ASC_NEW_ARRAY(char,strlen(ls.fullpath)+1);
		strcpy(foundpath,ls.fullpath);
		ospath_searchpath_free(sp);
	}

	ospath_free(fp1);
	return foundpath;
}
