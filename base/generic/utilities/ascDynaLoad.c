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


/*
 * Small changes made by Michael Moore (mdm@cis.ohio-state.edu)
 * December 24th, 1993.
 * The tcl sections ripped out by Kirk Abbott (ka0p@edrc.cmu.edu)
 * September 3rd, 1994.
 * To date the architectures supported are:
 * sun - pure sunos, defines also handle MACH.
 * osf,
 * solaris,
 * hpux
 * sgi
 * ultrix
 *
 * Remember that under most systems, profiling does not work
 * with dynamic libraries. !
 */

#include <stdio.h>
#include <stdlib.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPrint.h"
#include "utilities/ascMalloc.h"
struct ascend_dlrecord {
  char *path;   /* library name */
  void *dlreturn; /* return from dlopen */
  struct ascend_dlrecord *next;
};

static
struct ascend_dlrecord *g_ascend_dllist = NULL;

/*
 * adds a record of the path and handle to the list.
 * if it fails to do this, returns 1, else 0.
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
  new = (struct ascend_dlrecord *)malloc(sizeof(struct ascend_dlrecord));
  if (new==NULL) {
    free(keeppath);
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
 * deletes the record from the list.
 */
static
void *AscDeleteRecord(char *path)
{
  struct ascend_dlrecord *nextptr, *lastptr, *old;
  void *dlreturn = NULL;
  
  if (g_ascend_dllist==NULL) return NULL;
  if (strcmp(path,g_ascend_dllist->path)==0) {
    /* head case */
    old = g_ascend_dllist;
    g_ascend_dllist = old->next;
    dlreturn = old->dlreturn;
    free(old->path);
    free(old);
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
      free(old->path);
      free(old);
    }
  }
  return dlreturn;
}

/*
 * checks the list for a conflicting handle so we can issue
 * a more helpful warning, if need be, than the standard message.
 */
static 
void AscCheckDuplicateLoad(CONST char *path)
{
  struct ascend_dlrecord *r;
  r = g_ascend_dllist;
  while (r != NULL) {
    if (strcmp(path,r->path)==0) {
      FPRINTF(stderr,"Attempt to load already loaded %s\n",path);
      return;
    }
    r = r->next;
  }
}

#ifdef __WIN32__
#include <windows.h>
int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  HINSTANCE xlib;
  int (*install)();
  int result, addresult;

  AscCheckDuplicateLoad(path); /* whine if we've see it before */
  /*
   *    If the named library does not exist, if it's not loadable or if
   *    it does not define the named install proc, report an error
   */


  xlib = LoadLibrary(path);
  if (xlib == NULL) {
    FPRINTF(stderr,"Asc_DynamicLoad: LoadLibrary failed\n");
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  install = (int (*)())GetProcAddress(xlib,initFun);
  if (install == NULL) {
    FPRINTF(stderr,"Asc_DynamicLoad: Required function not found\n");
        FreeLibrary(xlib);
        return 1;
  }
  /*
   *    Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}
#endif /* __WIN32__ */
#if defined(sun) || defined(linux)
#ifndef MACH
#include <dlfcn.h>

int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  int (*install)();
  int result, addresult;

  AscCheckDuplicateLoad(path); /* whine if we've see it before */
  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}

int DynamicLoad(CONST char *path, CONST char *initFun)
{
  void *xlib;
  int (*install)();

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  return (*install)();
}
#else /* MACH */
int DynamicLoad(CONST char *path, CONST char *initFun)
{
  return 0;
}
#endif /* MACH */
#endif /* sun */



#ifdef __osf__
#include <dlfcn.h>
int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  int (*install)();
  int result, addresult;
  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen((char *)path, 1);
  install = (int (*)())dlsym(xlib,(char *)initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}
int DynamicLoad(CONST char *path, CONST char *initFun)
{
  void *xlib;
  int (*install)();

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen((char *)path, 1);
  install = (int (*)())dlsym(xlib,(char *)initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  return (*install)();
}
#endif /* osf */

#if defined(solaris) || defined(_AIX)
#if defined(solaris)
#include <dlfcn.h>
#else
/* under aix we have do also define dlopen and the rest of the
 * dl interface, so swipe it from tcl.
 * :r ../compat/dlfcn.h
 * :r tclLoadAix.c
 */
/* 
 * dlfcn.h --
 *
 *	This file provides a replacement for the header file "dlfcn.h"
 *	on systems where dlfcn.h is missing.  It's primary use is for
 *	AIX, where Tcl emulates the dl library.
 *
 *	This file is subject to the following copyright notice, which is
 *	different from the notice used elsewhere in Tcl but rougly
 *	equivalent in meaning.
 *
 *	Copyright (c) 1992,1993,1995,1996, Jens-Uwe Mager, Helios Software GmbH
 *	Not derived from licensed software.
 *
 *	Permission is granted to freely use, copy, modify, and redistribute
 *	this software, provided that the author is not construed to be liable
 *	for any results of using the software, alterations are clearly marked
 *	as such, and this notice is not modified.
 *
 * SCCS: @(#) dlfcn.h 1.4 96/09/17 09:05:59
 */

/*
 * @(#)dlfcn.h	1.4 revision of 95/04/25  09:36:52
 * This is an unpublished work copyright (c) 1992 HELIOS Software GmbH
 * 30159 Hannover, Germany
 */

#ifndef __dlfcn_h__
#define __dlfcn_h__

/*_#ifndef _TCL */
/*_#include <tcl.h> */
/*_#endif */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mode flags for the dlopen routine.
 */
#define RTLD_LAZY	1	/* lazy function call binding */
#define RTLD_NOW	2	/* immediate function call binding */
#define RTLD_GLOBAL	0x100	/* allow symbols to be global */

/*
 * To be able to intialize, a library may provide a dl_info structure
 * that contains functions to be called to initialize and terminate.
 */
struct dl_info {
	void (*init)(void);
	void (*fini)(void);
};

void *dlopen(const char *path, int mode);
void *dlsym(void *handle, const char *symbol);
char *dlerror(void);
int dlclose(void *handle);

#ifdef __cplusplus
}
#endif

/* 
 * tclLoadAix.c --
 *
 *	This file implements the dlopen and dlsym APIs under the
 *	AIX operating system, to enable the Tcl "load" command to
 *	work.  This code was provided by Jens-Uwe Mager.
 *
 *	This file is subject to the following copyright notice, which is
 *	different from the notice used elsewhere in Tcl.  The file has
 *	been modified to incorporate the file dlfcn.h in-line.
 *
 *	Copyright (c) 1992,1993,1995,1996, Jens-Uwe Mager, Helios Software GmbH
 *	Not derived from licensed software.

 *	Permission is granted to freely use, copy, modify, and redistribute
 *	this software, provided that the author is not construed to be liable
 *	for any results of using the software, alterations are clearly marked
 *	as such, and this notice is not modified.
 *
 * SCCS: @(#) tclLoadAix.c 1.11 96/10/07 10:41:24
 *
 * Note:  this file has been altered from the original in a few
 * ways in order to work properly with Tcl.
 */

/*
 * @(#)dlfcn.c	1.7 revision of 95/08/14  19:08:38
 * This is an unpublished work copyright (c) 1992 HELIOS Software GmbH
 * 30159 Hannover, Germany
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ldr.h>
#include <a.out.h>
#include <ldfcn.h>

/*
 * We simulate dlopen() et al. through a call to load. Because AIX has
 * no call to find an exported symbol we read the loader section of the
 * loaded module and build a list of exported symbols and their virtual
 * address.
 */

typedef struct {
	char		*name;		/* the symbols's name */
	void		*addr;		/* its relocated virtual address */
} Export, *ExportPtr;

/*
 * xlC uses the following structure to list its constructors and
 * destructors. This is gleaned from the output of munch.
 */
typedef struct {
	void (*init)(void);		/* call static constructors */
	void (*term)(void);		/* call static destructors */
} Cdtor, *CdtorPtr;

/*
 * The void * handle returned from dlopen is actually a ModulePtr.
 */
typedef struct Module {
	struct Module	*next;
	char		*name;		/* module name for refcounting */
	int		refCnt;		/* the number of references */
	void		*entry;		/* entry point from load */
	struct dl_info	*info;		/* optional init/terminate functions */
	CdtorPtr	cdtors;		/* optional C++ constructors */
	int		nExports;	/* the number of exports found */
	ExportPtr	exports;	/* the array of exports */
} Module, *ModulePtr;

/*
 * We keep a list of all loaded modules to be able to call the fini
 * handlers and destructors at atexit() time.
 */
static ModulePtr modList;

/*
 * The last error from one of the dl* routines is kept in static
 * variables here. Each error is returned only once to the caller.
 */
static char errbuf[BUFSIZ];
static int errvalid;

static void caterr(char *);
static int readExports(ModulePtr);
static void terminate(void);
static void *findMain(void);

void *dlopen(const char *path, int mode)
{
	register ModulePtr mp;
	static void *mainModule;

	/*
	 * Upon the first call register a terminate handler that will
	 * close all libraries. Also get a reference to the main module
	 * for use with loadbind.
	 */
	if (!mainModule) {
		if ((mainModule = findMain()) == NULL)
			return NULL;
		atexit(terminate);
	}
	/*
	 * Scan the list of modules if we have the module already loaded.
	 */
	for (mp = modList; mp; mp = mp->next)
		if (strcmp(mp->name, path) == 0) {
			mp->refCnt++;
			return (void *) mp;
		}
	if ((mp = (ModulePtr)calloc(1, sizeof(*mp))) == NULL) {
		errvalid++;
		strcpy(errbuf, "calloc: ");
		strcat(errbuf, strerror(errno));
		return (void *) NULL;
	}
	mp->name = malloc((unsigned) (strlen(path) + 1));
	strcpy(mp->name, path);
	/*
	 * load should be declared load(const char *...). Thus we
	 * cast the path to a normal char *. Ugly.
	 */
	if ((mp->entry = (void *)load((char *)path, L_NOAUTODEFER, NULL)) == NULL) {
		free(mp->name);
		free(mp);
		errvalid++;
		strcpy(errbuf, "dlopen: ");
		strcat(errbuf, path);
		strcat(errbuf, ": ");
		/*
		 * If AIX says the file is not executable, the error
		 * can be further described by querying the loader about
		 * the last error.
		 */
		if (errno == ENOEXEC) {
			char *tmp[BUFSIZ/sizeof(char *)];
			if (loadquery(L_GETMESSAGES, tmp, sizeof(tmp)) == -1)
				strcpy(errbuf, strerror(errno));
			else {
				char **p;
				for (p = tmp; *p; p++)
					caterr(*p);
			}
		} else
			strcat(errbuf, strerror(errno));
		return (void *) NULL;
	}
	mp->refCnt = 1;
	mp->next = modList;
	modList = mp;
	if (loadbind(0, mainModule, mp->entry) == -1) {
		dlclose(mp);
		errvalid++;
		strcpy(errbuf, "loadbind: ");
		strcat(errbuf, strerror(errno));
		return (void *) NULL;
	}
	/*
	 * If the user wants global binding, loadbind against all other
	 * loaded modules.
	 */
	if (mode & RTLD_GLOBAL) {
		register ModulePtr mp1;
		for (mp1 = mp->next; mp1; mp1 = mp1->next)
			if (loadbind(0, mp1->entry, mp->entry) == -1) {
				dlclose(mp);
				errvalid++;
				strcpy(errbuf, "loadbind: ");
				strcat(errbuf, strerror(errno));
				return (void *) NULL;
			}
	}
	if (readExports(mp) == -1) {
		dlclose(mp);
		return (void *) NULL;
	}
	/*
	 * If there is a dl_info structure, call the init function.
	 */
	if (mp->info = (struct dl_info *)dlsym(mp, "dl_info")) {
		if (mp->info->init)
			(*mp->info->init)();
	} else
		errvalid = 0;
	/*
	 * If the shared object was compiled using xlC we will need
	 * to call static constructors (and later on dlclose destructors).
	 */
	if (mp->cdtors = (CdtorPtr)dlsym(mp, "__cdtors")) {
		while (mp->cdtors->init) {
			(*mp->cdtors->init)();
			mp->cdtors++;
		}
	} else
		errvalid = 0;
	return (void *) mp;
}

/*
 * Attempt to decipher an AIX loader error message and append it
 * to our static error message buffer.
 */
static void caterr(char *s)
{
	register char *p = s;

	while (*p >= '0' && *p <= '9')
		p++;
	switch(atoi(s)) {
	case L_ERROR_TOOMANY:
		strcat(errbuf, "to many errors");
		break;
	case L_ERROR_NOLIB:
		strcat(errbuf, "can't load library");
		strcat(errbuf, p);
		break;
	case L_ERROR_UNDEF:
		strcat(errbuf, "can't find symbol");
		strcat(errbuf, p);
		break;
	case L_ERROR_RLDBAD:
		strcat(errbuf, "bad RLD");
		strcat(errbuf, p);
		break;
	case L_ERROR_FORMAT:
		strcat(errbuf, "bad exec format in");
		strcat(errbuf, p);
		break;
	case L_ERROR_ERRNO:
		strcat(errbuf, strerror(atoi(++p)));
		break;
	default:
		strcat(errbuf, s);
		break;
	}
}

void *dlsym(void *handle, const char *symbol)
{
	register ModulePtr mp = (ModulePtr)handle;
	register ExportPtr ep;
	register int i;

	/*
	 * Could speed up the search, but I assume that one assigns
	 * the result to function pointers anyways.
	 */
	for (ep = mp->exports, i = mp->nExports; i; i--, ep++)
		if (strcmp(ep->name, symbol) == 0)
			return ep->addr;
	errvalid++;
	strcpy(errbuf, "dlsym: undefined symbol ");
	strcat(errbuf, symbol);
	return NULL;
}

char *dlerror(void)
{
	if (errvalid) {
		errvalid = 0;
		return errbuf;
	}
	return NULL;
}

int dlclose(void *handle)
{
	register ModulePtr mp = (ModulePtr)handle;
	int result;
	register ModulePtr mp1;

	if (--mp->refCnt > 0)
		return 0;
	if (mp->info && mp->info->fini)
		(*mp->info->fini)();
	if (mp->cdtors)
		while (mp->cdtors->term) {
			(*mp->cdtors->term)();
			mp->cdtors++;
		}
	result = unload(mp->entry);
	if (result == -1) {
		errvalid++;
		strcpy(errbuf, strerror(errno));
	}
	if (mp->exports) {
		register ExportPtr ep;
		register int i;
		for (ep = mp->exports, i = mp->nExports; i; i--, ep++)
			if (ep->name)
				free(ep->name);
		free(mp->exports);
	}
	if (mp == modList)
		modList = mp->next;
	else {
		for (mp1 = modList; mp1; mp1 = mp1->next)
			if (mp1->next == mp) {
				mp1->next = mp->next;
				break;
			}
	}
	free(mp->name);
	free(mp);
	return result;
}

static void terminate(void)
{
	while (modList)
		dlclose(modList);
}

/*
 * Build the export table from the XCOFF .loader section.
 */
static int readExports(ModulePtr mp)
{
	LDFILE *ldp = NULL;
	SCNHDR sh, shdata;
	LDHDR *lhp;
	char *ldbuf;
	LDSYM *ls;
	int i;
	ExportPtr ep;

	if ((ldp = ldopen(mp->name, ldp)) == NULL) {
		struct ld_info *lp;
		char *buf;
		int size = 4*1024;
		if (errno != ENOENT) {
			errvalid++;
			strcpy(errbuf, "readExports: ");
			strcat(errbuf, strerror(errno));
			return -1;
		}
		/*
		 * The module might be loaded due to the LIBPATH
		 * environment variable. Search for the loaded
		 * module using L_GETINFO.
		 */
		if ((buf = malloc(size)) == NULL) {
			errvalid++;
			strcpy(errbuf, "readExports: ");
			strcat(errbuf, strerror(errno));
			return -1;
		}
		while ((i = loadquery(L_GETINFO, buf, size)) == -1 && errno == ENOMEM) {
			free(buf);
			size += 4*1024;
			if ((buf = malloc(size)) == NULL) {
				errvalid++;
				strcpy(errbuf, "readExports: ");
				strcat(errbuf, strerror(errno));
				return -1;
			}
		}
		if (i == -1) {
			errvalid++;
			strcpy(errbuf, "readExports: ");
			strcat(errbuf, strerror(errno));
			free(buf);
			return -1;
		}
		/*
		 * Traverse the list of loaded modules. The entry point
		 * returned by load() does actually point to the data
		 * segment origin.
		 */
		lp = (struct ld_info *)buf;
		while (lp) {
			if (lp->ldinfo_dataorg == mp->entry) {
				ldp = ldopen(lp->ldinfo_filename, ldp);
				break;
			}
			if (lp->ldinfo_next == 0)
				lp = NULL;
			else
				lp = (struct ld_info *)((char *)lp + lp->ldinfo_next);
		}
		free(buf);
		if (!ldp) {
			errvalid++;
			strcpy(errbuf, "readExports: ");
			strcat(errbuf, strerror(errno));
			return -1;
		}
	}
	if (TYPE(ldp) != U802TOCMAGIC) {
		errvalid++;
		strcpy(errbuf, "readExports: bad magic");
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	/*
	 * Get the padding for the data section. This is needed for
	 * AIX 4.1 compilers. This is used when building the final
	 * function pointer to the exported symbol.
	 */
	if (ldnshread(ldp, _DATA, &shdata) != SUCCESS) {
		errvalid++;
		strcpy(errbuf, "readExports: cannot read data section header");
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	if (ldnshread(ldp, _LOADER, &sh) != SUCCESS) {
		errvalid++;
		strcpy(errbuf, "readExports: cannot read loader section header");
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	/*
	 * We read the complete loader section in one chunk, this makes
	 * finding long symbol names residing in the string table easier.
	 */
	if ((ldbuf = (char *)malloc(sh.s_size)) == NULL) {
		errvalid++;
		strcpy(errbuf, "readExports: ");
		strcat(errbuf, strerror(errno));
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	if (FSEEK(ldp, sh.s_scnptr, BEGINNING) != OKFSEEK) {
		errvalid++;
		strcpy(errbuf, "readExports: cannot seek to loader section");
		free(ldbuf);
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	if (FREAD(ldbuf, sh.s_size, 1, ldp) != 1) {
		errvalid++;
		strcpy(errbuf, "readExports: cannot read loader section");
		free(ldbuf);
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	lhp = (LDHDR *)ldbuf;
	ls = (LDSYM *)(ldbuf+LDHDRSZ);
	/*
	 * Count the number of exports to include in our export table.
	 */
	for (i = lhp->l_nsyms; i; i--, ls++) {
		if (!LDR_EXPORT(*ls))
			continue;
		mp->nExports++;
	}
	if ((mp->exports = (ExportPtr)calloc(mp->nExports, sizeof(*mp->exports))) == NULL) {
		errvalid++;
		strcpy(errbuf, "readExports: ");
		strcat(errbuf, strerror(errno));
		free(ldbuf);
		while(ldclose(ldp) == FAILURE)
			;
		return -1;
	}
	/*
	 * Fill in the export table. All entries are relative to
	 * the entry point we got from load.
	 */
	ep = mp->exports;
	ls = (LDSYM *)(ldbuf+LDHDRSZ);
	for (i = lhp->l_nsyms; i; i--, ls++) {
		char *symname;
		char tmpsym[SYMNMLEN+1];
		if (!LDR_EXPORT(*ls))
			continue;
		if (ls->l_zeroes == 0)
			symname = ls->l_offset+lhp->l_stoff+ldbuf;
		else {
			/*
			 * The l_name member is not zero terminated, we
			 * must copy the first SYMNMLEN chars and make
			 * sure we have a zero byte at the end.
			 */
			strncpy(tmpsym, ls->l_name, SYMNMLEN);
			tmpsym[SYMNMLEN] = '\0';
			symname = tmpsym;
		}
		ep->name = malloc((unsigned) (strlen(symname) + 1));
		strcpy(ep->name, symname);
		ep->addr = (void *)((unsigned long)mp->entry +
					ls->l_value - shdata.s_vaddr);
		ep++;
	}
	free(ldbuf);
	while(ldclose(ldp) == FAILURE)
		;
	return 0;
}

/*
 * Find the main modules entry point. This is used as export pointer
 * for loadbind() to be able to resolve references to the main part.
 */
static void * findMain(void)
{
	struct ld_info *lp;
	char *buf;
	int size = 4*1024;
	int i;
	void *ret;

	if ((buf = malloc(size)) == NULL) {
		errvalid++;
		strcpy(errbuf, "findMain: ");
		strcat(errbuf, strerror(errno));
		return NULL;
	}
	while ((i = loadquery(L_GETINFO, buf, size)) == -1 && errno == ENOMEM) {
		free(buf);
		size += 4*1024;
		if ((buf = malloc(size)) == NULL) {
			errvalid++;
			strcpy(errbuf, "findMain: ");
			strcat(errbuf, strerror(errno));
			return NULL;
		}
	}
	if (i == -1) {
		errvalid++;
		strcpy(errbuf, "findMain: ");
		strcat(errbuf, strerror(errno));
		free(buf);
		return NULL;
	}
	/*
	 * The first entry is the main module. The entry point
	 * returned by load() does actually point to the data
	 * segment origin.
	 */
	lp = (struct ld_info *)buf;
	ret = lp->ldinfo_dataorg;
	free(buf);
	return ret;
}

#endif /* __dlfcn_h__ */

#endif /* solaris or aix dlfcn */

int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  int (*install)();
  int result, addresult;
  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}

int DynamicLoad(CONST char *path, CONST char *initFun)
{
  void *xlib;
  int (*install)();

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  return (*install)();
}
#endif /* solaris, aix */


#ifdef _SGI_SOURCE
#include <dlfcn.h>
int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  int (*install)();
  int result, addresult;
  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}

int DynamicLoad(CONST char *path, CONST char *initFun)
{
  void *xlib;
  int (*install)();

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  return (*install)();
}
#endif /* _SGI_SOURCE */


#ifdef __hpux
/*
 * Modified to work with HP/UX 9.X Operating System.
 * Michael Moore (mdm@cis.ohio-state.edu)
 * December 24th, 1993.
 * Further modified by Kirk Abbott (ka0p@edrc.cmu.edu)
 * to fit in with the ASCEND system.
 */

#include <dl.h>
#include <errno.h>

int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  shl_t xlib;
  int (*install)();
  int i;
  int result, addresult;
  AscCheckDuplicateLoad(path); /* whine if we've see it before */

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = shl_load(path, BIND_IMMEDIATE | BIND_VERBOSE, 0L);
  if (xlib == (shl_t) NULL)  {
    FPRINTF(stderr,"Unable to load shared library : %s\n",strerror(errno));
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  i = shl_findsym(&xlib, initFun, TYPE_PROCEDURE, &install);
  if (i == -1) {
    FPRINTF(stderr,"Unable to find needed symbol %s %s\n",
		       initFun, strerror(errno));
    shl_unload(xlib); /* baa */
    return 1;
  }
  if (install == NULL) {
    FPRINTF(stderr,"Unable to find needed symbol %s\n",initFun);
    FPRINTF(stderr,"Error type unknown\n");
    shl_unload(xlib); /* baa */
    return 1;
  }
  /*
   *	Try to install the extension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord((void *)xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}

int DynamicLoad(CONST char *path, CONST char *initFun)
{
  shl_t xlib;
  int (*install)();
  int i;

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = shl_load(path, BIND_IMMEDIATE | BIND_VERBOSE, 0L);
  if (xlib == (shl_t) NULL)  {
    FPRINTF(stderr,"Unable to load shared library : %s\n",strerror(errno));
    return 1;
  }
  i = shl_findsym(&xlib, initFun, TYPE_PROCEDURE, &install);
  if (i == -1) {
    FPRINTF(stderr,"Unable to find needed symbol %s %s\n",
		       initFun, strerror(errno));
    return 1;
  }
  if (install == NULL) {
    FPRINTF(stderr,"Unable to find needed symbol %s\n",initFun);
    FPRINTF(stderr,"Error type unknown\n");
    return 1;
  }
  /*
   *	Try to install the extension and report success or failure
   */
  return (*install)();
}
#endif /* __hpux */



#ifdef ultrix
/*
 *  Ultrix 4.x Dynamic Loader Library Version 1.0
 *
 *  dl.h--
 *      header file for the Dynamic Loader Library
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */
#include <filehdr.h>
#include <syms.h>
#include <reloc.h>
#include <scnhdr.h>
#include <fcntl.h>
#include <ar.h>

typedef long CoreAddr;


typedef struct ScnInfo {
    CoreAddr	addr;		/* starting address of the section */
    SCNHDR	hdr;		/* section header */
    RELOC      *relocEntries;	/* relocation entries */
} ScnInfo;

typedef enum {
    DL_NEEDRELOC,		/* still need relocation */
    DL_RELOCATED,		/* no relocation necessary */
    DL_INPROG			/* relocation in progress */
} dlRStatus;

typedef struct JmpTbl {
    char *block;		/* the jump table memory block */
    struct JmpTbl *next;	/* next block */
} JmpTbl;

typedef struct dlFile {
    char	*filename;	/* file name of the object file */

    int		textSize;	/* used by mprotect */
    CoreAddr 	textAddress;	/* start addr of text section */
    long     	textVaddr;	/* vaddr of text section in obj file */
    CoreAddr 	rdataAddress;	/* start addr of rdata section */
    long     	rdataVaddr;	/* vaddr of text section in obj file */
    CoreAddr 	dataAddress;	/* start addr of data section */
    long 	dataVaddr;	/* vaddr of text section in obj file */
    CoreAddr	bssAddress;	/* start addr of bss section */
    long     	bssVaddr;	/* vaddr of text section in obj file */

    int		nsect;		/* number of sections */
    ScnInfo 	*sect;		/* details of each section (array) */

    int		issExtMax;	/* size of string space */
    char 	*extss;		/* extern sym string space (in core) */
    int		iextMax;	/* maximum number of Symbols */
    pEXTR	extsyms;	/* extern syms */

    dlRStatus 	relocStatus;	/* what relocation needed? */
    int 	needReloc;

    JmpTbl	*jmptable;	/* the jump table for R_JMPADDR */

    struct dlFile *next;	/* next member of the archive */
} dlFile;

typedef struct dlSymbol {
    char *name;			/* name of the symbol */
    long addr;			/* address of the symbol */
    dlFile *objFile;		/* from which file */
} dlSymbol;

/*
 * prototypes for the dl* interface
 */
extern void *dlopen(/* char *filename, int mode */);
extern void *dlsym(/* void *handle, char *name */);
extern void dlclose(/* void *handle */);
extern char *dlerror(/* void */);

#define   DL_LAZY	0	/* lazy resolution */
#define   DL_NOW	1	/* immediate resolution */

/*
 * Miscellaneous utility routines:
 */
extern char **dl_undefinedSymbols(/* int *count */);
extern void dl_printAllSymbols(/* void *handle */);
extern void dl_setLibraries(/* char *libs */);

/* here we are in ultrix land */
int Asc_DynamicLoad(CONST char *path, CONST char *initFun)
{
#define ASCDL_OK /* this line should appear inside each Asc_DynamicLoad */
  void *xlib;
  int (*install)();
  int result, addresult;

  AscCheckDuplicateLoad(path); /* whine if we've see it before */
  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  if (initFun == NULL) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
    return 0;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  result = (*install)();
  if (result == 0) {
    addresult = AscAddRecord(xlib,path);
    if (addresult) {
      FPRINTF(stderr,"Asc_DynamicLoad malloc fail (%s)\n",path);
    }
  }
  return result;
}
/*
 * This is where we put a wrapper around all of the
 * ultrix based dynamic loading code.
 */
int DynamicLoad(CONST char *path, CONST char *initFun)
{
  void *xlib;
  int (*install)();

  /*
   *	If the named library does not exist, if it's not loadable or if
   *	it does not define the named install proc, report an error
   */
  xlib = dlopen(path, 1);
  install = (int (*)())dlsym(xlib, initFun);
  if ((xlib == NULL) || (install==NULL)) {
    FPRINTF(stderr,"%s\n",(char *)dlerror());
    if ( xlib != NULL ) dlclose(xlib);
    return 1;
  }
  /*
   *	Try to install the exstension and report success or failure
   */
  return (*install)();
}


/*
 *  dlPriv.h--
 *      the Private header file for the Dynamic Loader Library. Normal
 *      users should have no need to include this file.
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */

extern dlSymbol *dl_hashSearchSymbol();
extern dlSymbol *dl_hashInsertSymbolStrIdx();

#define STRCOPY(x)  (char *)strcpy((char *)malloc(strlen(x)+1), x)

#define HASHTABSZ	1001

typedef struct HEnt_ {
    dlSymbol *symbol;
    struct HEnt_ *next;
} HEnt;

typedef struct JmpTblHdr {
    int	current;		/* current empty slot */
    int max;			/* max no. of slots */
    int dummy[2];		/* padding to make this 4 words */
} JmpTblHdr;

extern HEnt **dlHashTable;
extern int _dl_undefinedSymbolCount;

extern dlFile *_dl_openObject();
extern void _dl_closeObject();

extern int _dl_loadSections();
extern int _dl_loadSymbols();

dlFile *_dl_loadEntireArchive();



/*
 * dlArch.c--
 *     handles loading of library archives.
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */
#include <stdio.h>
#include <strings.h>
/* #include <sys/types.h>  */
/* #include <sys/stat.h>   */

/*
 * Sometimes, you might want to have undefined symbols searched from
 * standard libraries like libc.a and libm.a automatically. dl_setLibraries
 * is the interface to do this. dl_stdLibraries contain an arrary of
 * strings specifying the libraries to be searched.
 */
char **dl_stdLibraries= NULL;

static int searchArchive();
static int readArchiveRanlibs();
static int archGetObjectName();
static int archLoadObject();
static int loadArchSections();

/*****************************************************************************
 *                                                                           *
 *     Searching of pre-set libraries                                        *
 *                                                                           *
 *****************************************************************************/

/*
 * dl_searchLibraries returns 1 if undefined symbols are found during the
 * searching. 0 otherwise.
 */
int dl_searchLibraries()
{
    char **libs= dl_stdLibraries;
    int result= 0;

    if (dl_stdLibraries && _dl_undefinedSymbolCount) {
	while(*libs) {
	    result|= searchArchive(*libs);
	    libs++;
	}
    }
    return (result);
}

/*
 * dl_setLibraries--
 *      takes a string of the form <library>[:<library> ... ] which
 *      specifies the libraries to be searched automatically when there are
 *      undefined symbols.
 *
 *      eg. dl_setLibraries("/usr/lib/libc_G0.a:/usr/lib/libm_G0.a");
 */
void dl_setLibraries( libs )
     char *libs;
{
    char *name, *t;
    char **stnlib;
    int numlibs= 0;
    int maxlibs= 4;

    if(!libs)
	return;
    stnlib= (char **)malloc(sizeof(char *) * maxlibs);
    name=t= libs;
    while(*t!='\0') {
	while(*t!=':' && *t!='\0')
	    t++;
	if (t-name>0) {
	    stnlib[numlibs]= strncpy((char*)malloc(t-name+1),name,t-name);
	    stnlib[numlibs++][t-name]='\0';
	    if(numlibs==maxlibs-1) {
		maxlibs*= 2;
		stnlib= (char **)realloc(stnlib, sizeof(char *) * maxlibs);
	    }
	}
	if (*t==':') {
	    t++;
	    name= t;
	}
    }
    stnlib[numlibs]= NULL;
    if (dl_stdLibraries) {
	char **s= dl_stdLibraries;
	while(*s!=NULL) {
	    free(*s);
	    s++;
	}
	free(dl_stdLibraries);
    }
    dl_stdLibraries= stnlib;

    return;
}

/*****************************************************************************
 *                                                                           *
 *       Internal Rountines                                                  *
 *                                                                           *
 *****************************************************************************/

static int searchArchive( archname )
     char *archname;
{
    int found= 0, done;
    struct ranlib *pran;
    char *pstr;
    int i;
    int fd;

    /*
     * opens the archive file and reads in the ranlib hashtable
     */
    if ((fd=readArchiveRanlibs(archname, &pran, &pstr)) < 0) {
	FPRINTF(stderr, "dl: cannot open \"%s\"", archname);
	return 0;
    }
    /*
     * look through our symbol hash table and see if we find anything.
     * We have to scan until no undefined symbols can be found in the
     * archive. (Note that bringing in an object file might require another
     * object file in the archive. We'll have missed the symbol if we
     * do this one pass and the symbol happens to be inserted into buckets
     * we've examined already.)
     */
    do {
	done= 1;
	for(i=0; i < HASHTABSZ; i++) {
	    HEnt *ent= dlHashTable[i];
	    struct ranlib *r;
	    while(ent) {
		if (!ent->symbol->objFile) {
		    r= (struct ranlib *)ranlookup(ent->symbol->name);
		    if (r->ran_off) {
			/*
			 * we've found the undefined symbol in the archive
			 */
#if DEBUG
			PRINTF("*** found %s in ", ent->symbol->name);
#endif
			if (archLoadObject(fd, r->ran_off)) {
			    found= 1;
			    done= 0;
			}
		    }
		}
		ent=ent->next;
	    }
	}
    } while (!done);
    /*
     * be a good citizen.
     */
    free(pran);
    free(pstr);
    close(fd);

    return found;
}

/*
 * readArchiveRanlibs--
 *     opens a library and reads in the ranlib hash table and its
 *     associated string table. It returns -1 if fails, the opened
 *     file descriptor otherwise. It also inits the ranhashtable.
 */
static int readArchiveRanlibs( archfile, pran, pstr )
     char *archfile; struct ranlib **pran; char **pstr;
{
    int numRanlibs, numStrings;
    struct ranlib *ranlibs;
    char *strings;
    ARHDR  ar_hdr;
    int fd, size;
    char mag[SARMAG];

    *pran= NULL;
    *pstr= NULL;
    /*
     * opens the library and check the magic string
     */
    if ((fd= open(archfile, O_RDONLY)) < 0 ||
	read(fd, mag, SARMAG)!=SARMAG ||
	strncmp(mag, ARMAG, SARMAG)!=0) {
	close(fd);
	return -1;
    }
    /*
     * reads in the archive header (not used) and the number of ranlibs.
     */
    if (read(fd, &ar_hdr, sizeof(ARHDR))!=sizeof(ARHDR) ||
	read(fd, &numRanlibs, sizeof(int))!= sizeof(int)) {
	close(fd);
	return -1;
    }
    /*
     * reads in the ranlib hash table and the string table size.
     */
    size= sizeof(struct ranlib)*numRanlibs;
    ranlibs= (struct ranlib *)malloc(size);
    if (read(fd, ranlibs, size)!=size ||
	read(fd, &numStrings, sizeof(int))!=sizeof(int)) {
	close(fd);
	return -1;
    }
    /*
     * reads in the string table.
     */
    strings= (char *)malloc(numStrings);
    if (read(fd, strings, numStrings)!=numStrings) {
	close(fd);
	return -1;
    }
    *pran= ranlibs;
    *pstr= strings;
    ranhashinit(ranlibs, strings, numRanlibs);
    return fd;
}

static int archLoadObject( fd, offset )
     int fd; int offset;
{
    dlFile *dlfile;
    FILHDR filhdr;
    ARHDR arhdr;
    char ar_name[17];

    if (lseek(fd, offset, SEEK_SET)==-1 ||
	read(fd, &arhdr, sizeof(ARHDR))!=sizeof(ARHDR) ||
	read(fd, &filhdr, sizeof(filhdr))!=sizeof(filhdr))
	return 0;
    sscanf(arhdr.ar_name, "%16s", ar_name);
    ar_name[16]='\0';
#if DEBUG
    PRINTF("%.16s\n", ar_name);
#endif

    dlfile= (dlFile *)malloc(sizeof(dlFile));
    bzero(dlfile, sizeof(dlFile));
    dlfile->filename= STRCOPY(ar_name);
    dlfile->relocStatus= DL_NEEDRELOC;

    if (!_dl_loadSymbols(dlfile, fd, filhdr, offset+sizeof(ARHDR)) ||
	!_dl_loadSections(dlfile, fd, offset+sizeof(ARHDR))) {
	_dl_closeObject(dlfile);
	return 0;
    }

    if (!_dl_enterExternRef(dlfile)) {
	_dl_closeObject(dlfile);
	return 0;
    }

    /*
     * need to relocate now and see if we need to bring in more files.
     */
    _dl_relocateSections(dlfile);

    return 1;
}

dlFile *_dl_loadEntireArchive( filename, fd )
     char *filename; int fd;
{
    dlFile *dlfile, *df;
    int offset;
    FILHDR filhdr;
    ARHDR arhdr;
    int size;
    struct stat stat_buf;

    /*
     * read in the header of the symbol list (the so-called symdef)
     */
    if (lseek(fd, SARMAG, SEEK_SET)==-1 ||
	read(fd, &arhdr, sizeof(arhdr))!=sizeof(arhdr)) {
	return 0;
    }
    /*
     * go after each member of the archive:
     */
    fstat(fd, &stat_buf);
    sscanf(arhdr.ar_size, "%d", &size);
    offset= SARMAG + sizeof(ARHDR) + size;
    dlfile= NULL;
    while( offset < stat_buf.st_size) {
	if (!lseek(fd, offset, SEEK_SET)==-1 ||
	    read(fd, &arhdr, sizeof(arhdr))!=sizeof(arhdr) ||
	    read(fd, &filhdr, sizeof(filhdr))!=sizeof(filhdr)) {
	    _dl_closeObject(dlfile);
	    return NULL;
	}
	offset+= sizeof(ARHDR);
	if (!(df=_dl_openObject(fd, filename, filhdr, offset, DL_LAZY))) {
	    _dl_closeObject(dlfile);
	    return NULL;
	}
	sscanf(arhdr.ar_size, "%d", &size);
	offset+= size;
	df->next= dlfile;
	dlfile= df;
    }

    return dlfile;
}

/*
 *  dlRef.c--
 *      handles symbol references
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */

HEnt **dlHashTable;			/* hash table for the symbols */
int _dl_undefinedSymbolCount= 0;	/* number of undefined symbols */

static unsigned hash();

/*****************************************************************************
 *                                                                           *
 *     Hash Table for symbols                                                *
 *                                                                           *
 *****************************************************************************/

/*
 * hash is taken from Robert Sedgewick's "Algorithms in C" (p.233). Okay, so
 * this is not the most sophisticated hash function in the world but this
 * will do the job quite nicely.
 */
static unsigned hash( str )
     char *str;
{
    int h;
    for(h=0; *str!='\0'; str++)
	h = (64*h + *str) % HASHTABSZ;
    return h;
}

void _dl_hashInit()
{
    dlHashTable= (HEnt **)malloc(sizeof(HEnt *) * HASHTABSZ);
    bzero(dlHashTable, sizeof(HEnt *) * HASHTABSZ);
    return;
}

dlSymbol *dl_hashInsertSymbolStrIdx( extSs, idx, found )
     char *extSs; long idx; int *found;
{
    dlSymbol *symbol;
    char *symname= extSs + idx;
    int hval= hash(symname);
    HEnt *ent, *prev, *e;

    prev= e= dlHashTable[hval];
    while(e) {
	if(!strcmp(e->symbol->name, symname)) {
	    *found= 1;
	    return (e->symbol);   /* return existing symbol */
	}
	prev= e;
	e= e->next;
    }
    ent= (HEnt *)malloc(sizeof(HEnt));
    symbol= (dlSymbol *)malloc(sizeof(dlSymbol));
    bzero(symbol, sizeof(dlSymbol));
    symbol->name= symname;
    ent->symbol= symbol;
    ent->next= NULL;
    if (!prev) {
	dlHashTable[hval]= ent;
    }else {
	prev->next= ent;
    }
    *found= 0;
    return symbol;
}

dlSymbol *dl_hashSearchSymbol( symname )
     char *symname;
{
    int hval= hash(symname);
    HEnt *ent= dlHashTable[hval];
    while(ent) {
	if(!strcmp(ent->symbol->name, symname))
	    return ent->symbol;
	ent= ent->next;
    }
    return NULL;
}

/*****************************************************************************
 *                                                                           *
 *     Entering External References                                          *
 *                                                                           *
 *****************************************************************************/

int _dl_enterInitialExternRef( dlfile )
     dlFile *dlfile;
{
    char *extSs= dlfile->extss;
    pEXTR extsyms= dlfile->extsyms;
    dlSymbol *symbol;
    int i, found;

    /*
     * this is done by init. Just enter the symbols and values:
     * (It cannot contain undefined symbols and the multiple defs.)
     */
    for(i=0; i < dlfile->iextMax ; i++) {
	pEXTR esym= &extsyms[i];

	symbol= dl_hashInsertSymbolStrIdx(extSs, esym->asym.iss, &found);
	if (found) {
	    _dl_setErrmsg("init error: symbol \"%s\" multiply defined",
			      extSs+esym->asym.iss);
	    return 0;
	}
	symbol->addr= esym->asym.value;
	symbol->objFile= dlfile;
    }
    free(dlfile->extsyms);
    dlfile->extsyms= NULL;
    dlfile->iextMax= 0;
    return 1;
}

int _dl_enterExternRef( dlfile )
     dlFile *dlfile;
{
    int i;
    char *extSs= dlfile->extss;
    dlSymbol *symbol;
    int found;
    long textAddress= (long)dlfile->textAddress - dlfile->textVaddr;
    long rdataAddress= (long)dlfile->rdataAddress - dlfile->rdataVaddr;
    long dataAddress= (long)dlfile->dataAddress - dlfile->dataVaddr;
    long bssAddress= (long)dlfile->bssAddress - dlfile->bssVaddr;

    for(i=0; i < dlfile->iextMax ; i++) {
	pEXTR esym= &dlfile->extsyms[i];
	int found;

	if (esym->asym.sc!=scNil && esym->asym.sc!=scUndefined) {
	    symbol= dl_hashInsertSymbolStrIdx(extSs, esym->asym.iss,
					      &found);
	    if (symbol->objFile!=NULL) {
		_dl_setErrmsg("\"%s\" multiply defined",
			      extSs+esym->asym.iss);
		return 0;
	    }
	    if (found) {
		/*
		 * finally, we now have the undefined symbol. (A kludge
		 * here: the symbol name of the undefined symbol is
		 * malloc'ed. We need to free it.)
		 */
		free(symbol->name);
		symbol->name= extSs+esym->asym.iss;
		_dl_undefinedSymbolCount--;
	    }
	    switch(esym->asym.sc) {
	    case scAbs:
		symbol->addr= esym->asym.value;
		break;
	    case scText:
		symbol->addr= textAddress + esym->asym.value;
		break;
	    case scData:
		symbol->addr= dataAddress + esym->asym.value;
		break;
	    case scBss:
		symbol->addr= bssAddress + esym->asym.value;
		break;
	    case scRData:
		symbol->addr= rdataAddress + esym->asym.value;
		break;
	    case scCommon: {
		char *block= (char *)malloc(esym->asym.value);
		bzero(block, esym->asym.value);
		symbol->addr= (long)block;
		break;
	    }
	    default:
		FPRINTF(stderr, "dl: extern symbol in unexpected section (%d)\n",
			esym->asym.sc);
		break;
	    }
	    symbol->objFile= dlfile;
	}
    }
    return 1;
}

/*****************************************************************************
 *                                                                           *
 *     Misc. utilities                                                       *
 *                                                                           *
 *****************************************************************************/

/*
 * dl_undefinedSymbols--
 *      returns the number of undefined symbols in count and an array of
 *      strings of the undefined symbols. The last element of the array
 *      is guaranteed to be NULL.
 */
char **dl_undefinedSymbols( count )
     int *count;
{
    char **syms= NULL;
    int i, j;

    *count= _dl_undefinedSymbolCount;
    if (_dl_undefinedSymbolCount) {
	syms= (char **)malloc(sizeof(char *) * (_dl_undefinedSymbolCount+1));
	for(i=0, j=0; i<HASHTABSZ && j<_dl_undefinedSymbolCount; i++) {
	    HEnt *ent= dlHashTable[i];
	    while(ent) {
		if (!ent->symbol->objFile) {
		    syms[j++]= STRCOPY(ent->symbol->name);
		    if (j==_dl_undefinedSymbolCount)
			break;
		}
		ent=ent->next;
	    }
	}
	syms[j]=NULL;
    }
    return syms;
}

/*
 * dl_printAllSymbols--
 *      aids debugging. Prints out symbols in the hash table that matches
 *      the file handle. For library archives, prints those that matches
 *      any member or the archive. A NULL handle matches everything.
 */
void dl_printAllSymbols( handle )
     void *handle;
{
    int i, count= 0;

    for(i=0; i < HASHTABSZ; i++) {
	HEnt *ent= dlHashTable[i];
	while(ent) {
	    if (!handle || handle==ent->symbol->objFile) {
		PRINTF("(%3d) %-20s addr=0x%x dlfile=0x%x\n",
		       i, ent->symbol->name, ent->symbol->addr,
		       ent->symbol->objFile);
		count++;
	    }else if (((dlFile *)handle)->next) {
		dlFile *dlfile= ((dlFile *)handle)->next;
		while(dlfile) {
		    if (dlfile==ent->symbol->objFile) {
			PRINTF("(%3d) %-20s addr=0x%x dlfile=0x%x\n",
			       i, ent->symbol->name, ent->symbol->addr,
			       ent->symbol->objFile);
			count++;
		    }
		    dlfile= dlfile->next;
		}
	    }
	    ent= ent->next;
	}
    }
    PRINTF("total number of symbols= %d\n", count);
    return;
}


/*
 *  dlReloc.c--
 *      handles the relocation
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */

#include <sys/mman.h>
#include <mips/cachectl.h>

static void patchLongjump();
static void protectText();

int _dl_relocateSections(dlfile)
     dlFile *dlfile;
{
    long textAddress= dlfile->textAddress - dlfile->textVaddr;
    long rdataAddress= dlfile->rdataAddress - dlfile->rdataVaddr;
    long dataAddress= dlfile->dataAddress - dlfile->dataVaddr;
    long bssAddress= dlfile->bssAddress - dlfile->bssVaddr;
    int i, j;
    int hasUndefined= 0;
    pEXTR extsyms= dlfile->extsyms;
    char *extSs= dlfile->extss;

    if (dlfile->relocStatus==DL_RELOCATED)	/* just in case */
	return 1;

    /* prevent circular relocation */
    dlfile->relocStatus= DL_INPROG;

    for(i=0; i < dlfile->nsect; i++) {
	SCNHDR *hdr= &(dlfile->sect[i].hdr);
	RELOC *relocEnt= dlfile->sect[i].relocEntries;
	long sectAddr= dlfile->sect[i].addr - hdr->s_vaddr;
	long relocAddr;
	int *addr;
	int undefinedCount= 0;

	for(j= 0; j < hdr->s_nreloc; j++) {
	    if (relocEnt->r_extern) {
		pEXTR esym= &extsyms[relocEnt->r_symndx];
		char *symname= extSs + esym->asym.iss;
		dlSymbol *symbol;

		symbol= dl_hashSearchSymbol(symname);
		if(!symbol || !symbol->objFile) {
		    RELOC *rents= dlfile->sect[i].relocEntries;
		    int found;

		    if (j!=undefinedCount)
			rents[undefinedCount]=rents[j];
		    if (!symbol) {
			(void)dl_hashInsertSymbolStrIdx(STRCOPY(symname), 0,
							&found);
			_dl_undefinedSymbolCount++;
		    }
		    undefinedCount++;
		    relocEnt++;
		    continue;	/* skip this one */
		}
		if(symbol->objFile->relocStatus==DL_NEEDRELOC) {
		    /*
		     * trigger an avalanche of relocates! (In fact, we
		     * don't need to relocate unless the symbol references
		     * unrelocated text but we do it anyway.)
		     *
		     * if we fail to relocate the object file containing the
		     * symbol, we treat it as undefined and keep it around
		     * to trigger relocation next time.
		     */
		    if (!_dl_relocateSections(symbol->objFile)) {
			RELOC *rents= dlfile->sect[i].relocEntries;

			if (j!=undefinedCount)
			    rents[undefinedCount]=rents[j];
			undefinedCount++;
			relocEnt++;
			continue;	/* skip this one */
		    }
		}
		relocAddr= symbol->addr;
	    }else {
		switch(relocEnt->r_symndx) {
		case R_SN_TEXT:
		    relocAddr= textAddress;
		    break;
		case R_SN_RDATA:
		    relocAddr= rdataAddress;
		    break;
		case R_SN_DATA:
		    relocAddr= dataAddress;
		    break;
		case R_SN_BSS:
		    relocAddr= bssAddress;
		    break;
		case R_SN_NULL:
		case R_SN_SDATA:
		case R_SN_SBSS:
		    _dl_setErrmsg("unknown section %d referenced",
			    relocEnt->r_symndx);
		    return 0;
		    break;
		case R_SN_INIT:
		case R_SN_LIT8:
		case R_SN_LIT4:
		    /*
		     * I've never encounter these. (-G 0 should kill the
		     * LIT4's and LIT8's. I'm not sure if INIT is even used.)
		     */
		    _dl_setErrmsg("section %d not implemented",
			    relocEnt->r_symndx);
		    return 0;
		    break;
		default:
		    FPRINTF(stderr, "dl: unknown section %d\n",
			    relocEnt->r_symndx);
		}
	    }
	    addr= (int *)(sectAddr + relocEnt->r_vaddr);
	    switch(relocEnt->r_type) {
	    case R_ABS:
		break;
	    case R_REFWORD:
		*addr += relocAddr;
		break;
	    case R_JMPADDR:
		/*
		 * relocAddr has the absolute address when referenced symbol
		 * is external; otherwise, need to add the most significant
		 * 4 bits of the address of the instruction to the jump target.
		 */
		patchLongjump(dlfile, addr, relocAddr,
			      (relocEnt->r_extern)?0:relocEnt->r_vaddr);
		break;
	    case R_REFHI: {
		RELOC *nxtRent= relocEnt+1;
		if (nxtRent->r_type != R_REFLO) {
		    /* documentation says this will not happen: */
		    FPRINTF(stderr, "dl: R_REFHI not followed by R_REFLO\n");

		    /*
		     * use old way-- just relocate R_REFHI. This will break if
		     * R_REFLO has a negative offset.
		     */
		    if((short)(relocAddr&0xffff) < 0) {
			*addr += (((unsigned)relocAddr>>16)+ 1);
		    }else {
			*addr += ((unsigned)relocAddr>> 16);
		    }
		}else {
		    int hi_done= 0;
		    int hi_newaddr=0;
		    /*
		     * documentation lies again. You can have more than
		     * one R_REFLO following a R_REFHI.
		     */
		    while(j<hdr->s_nreloc && nxtRent->r_type==R_REFLO) {
			int *lo_addr= (int *)(sectAddr + nxtRent->r_vaddr);
			int oldaddr, newaddr;
			int temphi;

			oldaddr= ((*addr)<<16) + (short)((*lo_addr) & 0xffff);
			newaddr= relocAddr + oldaddr;
			if((short)(newaddr&0xffff) < 0) {
			    temphi= (((unsigned)newaddr>>16)+ 1);
			}else {
			    temphi= ((unsigned)newaddr>> 16);
			}
			if(!hi_done) {
			    hi_newaddr= temphi;
			    hi_done=1;
			}else {
			    if(temphi!=hi_newaddr) {
				FPRINTF(stderr, "dl: REFHI problem: %d %d don't match\n",
				    temphi, hi_newaddr);
			    }
			}
			*lo_addr &= 0xffff0000;
			*lo_addr |= (newaddr & 0xffff);
			j++; /* the following R_REFLO(s) has been relocated */
			relocEnt++;
			nxtRent++;
		    }
		    *addr &= 0xffff0000;	/* mask the immediate fields */
		    *addr |= (hi_newaddr & 0xffff);
		}
		break;
	    }
	    case R_REFLO:
		/*
		 * shouldn't be here (REFHI should have taken care of these)
		 * -- just in case
		 */
		FPRINTF(stderr, "dl: warning: dangling R_REFLO.\n");
		*addr += (relocAddr & 0xffff);
		break;
	    case R_GPREL:
		FPRINTF(stderr,"dl: Hopeless: $gp used.\n");
		break;
	    default:
		FPRINTF(stderr,"dl: This local relocation not implemented yet.\n");
	    }
	    relocEnt++;
	}
	hdr->s_nreloc= undefinedCount;
	if(undefinedCount>0) {
	    hasUndefined= 1;
	}else {
	    free(dlfile->sect[i].relocEntries);
	    dlfile->sect[i].relocEntries= NULL;
	}
    }
    dlfile->relocStatus= hasUndefined? DL_NEEDRELOC : DL_RELOCATED;
    if(!hasUndefined) {
	free(dlfile->extsyms);
	dlfile->extsyms= NULL;
	dlfile->iextMax= 0;
	protectText(dlfile);
    }
    return (!hasUndefined);
}

/*
 * patchLongjump patches R_JMPADDR references. The problem is that the
 * immediate field is only 28 bits and the references are often out of
 * range. We need to jump to a near place first (ie. the jmptable here)
 * and do a "jr" to jump farther away.
 */
static void patchLongjump( dlfile, addr, relocAddr, vaddr )
    dlFile *dlfile; int *addr; long relocAddr; long vaddr;
{
    int *patch, instr;
    JmpTbl *jmptable= dlfile->jmptable;
    JmpTblHdr *jhdr= (jmptable) ? (JmpTblHdr *)jmptable->block : NULL;

    if (!jmptable || jhdr->current==jhdr->max) {
	int pagesize;

	/* need new jump table */
	jmptable= (JmpTbl *)malloc(sizeof(JmpTbl));
	pagesize= getpagesize();
	jmptable->block= (char *)valloc(pagesize);
	bzero(jmptable->block, pagesize);
	jmptable->next= dlfile->jmptable;
	jhdr= (JmpTblHdr *)jmptable->block;
	jhdr->current= 0;
	jhdr->max= (pagesize - sizeof(JmpTblHdr))/16;
	dlfile->jmptable= jmptable;
    }

    if ((unsigned)addr>>28!=relocAddr>>28) {
	patch= (int *)jmptable->block + jhdr->current*4 + 4;
	jhdr->current++;
	if ((unsigned)patch>>28!=(unsigned)patch>>28) {
	    FPRINTF(stderr,"dl: out of luck! Can't jump.\n");
	    return;
	}
	if ((*addr)&0x3ffffff) {
	    relocAddr+= (*addr & 0x3ffffff)<<2;
	}
	if (vaddr) {
	    relocAddr+= (vaddr & 0xf0000000);
	}
	if (relocAddr&0x3) {
	    FPRINTF(stderr,"dl: relocation address not word-aligned!\n");
	}
	/* lui $at, hiOffset */
	*patch= 0x3c010000 | (relocAddr>>16);
	/* ori $at, $at, loOffset */
	*(patch+1)=0x34210000|(relocAddr&0xffff);
	/* jr $at */
	*(patch+2)= 0x00200008;
	/* nop */
	*(patch+3)= 0;
	*addr &= 0xfc000000;	/* leave the jal */
	*addr |= (((long)patch>>2) & 0x3ffffff);
    }else {
	if (relocAddr&0x3) {
	    FPRINTF(stderr,"dl: relocation address not word-aligned!\n");
	}
	*addr += (relocAddr>>2) & 0x3ffffff;
    }
    return;
}

/*
 * change memory protection so that text and the jumptables cannot be
 * accidentally overwritten.
 */
static void protectText( dlfile )
     dlFile *dlfile;
{
    int pagesize= getpagesize();
    JmpTbl *jmptable= dlfile->jmptable;

    if (dlfile->textAddress) {
	/* protect the text */
	if (mprotect((char *)dlfile->textAddress, dlfile->textSize,
		      PROT_EXEC) == -1) {
	    FPRINTF(stderr, "dl: fail to protect text of %s\n", dlfile->filename);
	}
	/* flush the caches */
	if (cacheflush((char *)dlfile->textAddress, dlfile->textSize,
		       BCACHE) != 0) {
	    FPRINTF(stderr, "dl: fail to flush text of %s\n", dlfile->filename);
	}
	/* protect jump tables, if any */
	while(jmptable) {
	    if (mprotect((char *)jmptable->block, pagesize,
			 PROT_EXEC) == -1) {
		FPRINTF(stderr, "dl: fail to protect a jump table of %s\n",
			dlfile->filename);
	    }
	    /* flush the caches */
	    if (cacheflush((char *)jmptable->block, pagesize, BCACHE) != 0) {
		FPRINTF(stderr, "dl: fail to flush a jump table of %s\n", dlfile->filename);
	    }
	    jmptable= jmptable->next;
	}
    }
    return;
}

/*
 *  dlInterf.c--
 *      implements the dl* interface
 *
 *
 *  Copyright (c) 1993 Andrew K. Yu, University of California at Berkeley
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for educational, research, and non-profit purposes and
 *  without fee is hereby granted, provided that the above copyright
 *  notice appear in all copies and that both that copyright notice and
 *  this permission notice appear in supporting documentation. Permission
 *  to incorporate this software into commercial products can be obtained
 *  from the author. The University of California and the author make
 *  no representations about the suitability of this software for any
 *  purpose. It is provided "as is" without express or implied warranty.
 *
 */
#include <stdio.h>
#include <varargs.h>

static char errmsg[500];		/* the error message buffer */

static int _dl_openFile();

#define	OBJECT_MAGIC(x) \
    ((x)==MIPSEBMAGIC || (x)==MIPSELMAGIC || (x)==SMIPSEBMAGIC || \
     (x)==SMIPSELMAGIC || (x)==MIPSEBUMAGIC || (x)==MIPSELUMAGIC)


/*****************************************************************************
 *                                                                           *
 *    dl_init, dl_open, dl_sym, dl_close, dl_error interface routines        *
 *                                                                           *
 *****************************************************************************/

/*
 * dl_init--
 *      takes the pathname of the current executable and reads in the
 *      symbols. It returns 1 if successful and 0 otherwise.
 */
int dl_init( filename )
     char *filename;
{
    int fd;
    dlFile *dlfile;
    FILHDR filhdr;

    if (!filename) return 0;

    /*
     * open the executable for extracting the symbols
     */
    if((fd=_dl_openFile(filename, &filhdr, 0)) < 0) {
	_dl_setErrmsg("cannot open \"%s\"", filename);
	return 0;
    }
    /*
     * create a dlFile entry for the executable
     */
    dlfile= (dlFile *)malloc(sizeof(dlFile));
    bzero(dlfile, sizeof(dlFile));
    dlfile->filename= STRCOPY(filename);
    dlfile->relocStatus= DL_RELOCATED;

    /*
     * load in and enter the symbols
     */
    _dl_hashInit();
    if (!_dl_loadSymbols(dlfile, fd, filhdr, 0)) {
	_dl_closeObject(dlfile);
	_dl_setErrmsg("Cannot load symbol table from \"%s\"", filename);
	return 0;
    }
    if (!_dl_enterInitialExternRef(dlfile)) {
	_dl_closeObject(dlfile);
	return 0;
    }

    close(fd);

    return 1;
}

/*
 * dl_open--
 *     opens the object file or library archive specified by filename
 *     It can be opened with either DL_LAZY or DL_NOW mode. DL_LAZY does
 *     no perform symbol resolution until a symbol in the file is accessed.
 *     DL_NOW performs symbol resolution at load time. It returns a
 *     handle if successful, NULL otherwise.
 */
void *dlopen( filename, mode )
     char *filename; int mode;
{
    int fd;
    dlFile *dlfile;
    FILHDR filhdr;

    if((fd=_dl_openFile(filename, &filhdr, 0)) < 0) {
	_dl_setErrmsg("cannot open \"%s\"", filename);
	return NULL;
    }

    /*
     * determine whether we have an object file or a library
     */
    if (OBJECT_MAGIC(filhdr.f_magic)) {
	/*
	 * an ordinary object file.
	 */
	dlfile= _dl_openObject(fd, filename, filhdr, 0, mode);
	close(fd);
    }else if (!strncmp((char*)&filhdr, ARMAG, SARMAG)) {
	dlFile *df;

	/*
	 * a library: load in every object file.
	 */
	if ((dlfile=_dl_loadEntireArchive(filename, fd))==NULL) {
	    close(fd);
	    _dl_setErrmsg("Cannot load archive \"%s\"", filename);
	    return NULL;
	}

	/*
	 * do the relocation now if mode==DL_NOW. (note that we couldn't
	 * relocate in the above loop since archive members might reference
	 * each other.)
	 */
	if (mode==DL_NOW) {
	    int search= 0;

	    df= dlfile;
	    while(df) {
		if (!_dl_relocateSections(df))
		    search= 1;
		df= df->next;
	    }
	    if (search) {
		if (!dl_searchLibraries()) {
		    _dl_setErrmsg("\"%s\" contains undefined symbols",
			      filename);
		    _dl_closeObject(dlfile);
		    return NULL;
		}
		df= dlfile;	/* one more time */
		while(df) {
		    if (!_dl_relocateSections(df)) {
			_dl_setErrmsg("\"%s\" contains undefined symbols",
				      filename);
			_dl_closeObject(dlfile);
			return NULL;
		    }
		    df= df->next;
		}
	    }
	}
    }else {
	_dl_setErrmsg("\"%s\" neither an object file nor an archive",
		      filename);
    }

    return (void *)dlfile;
}


/*
 * dl_sym--
 *      returns the location of the specified symbol. handle is not
 *      actually used. It returns NULL if unsuccessful.
 */
void *dlsym( handle, name )
     void *handle; char *name;
{
    dlFile *dlfile;
    dlSymbol *symbol;

    symbol = dl_hashSearchSymbol(name);
    if (symbol) {
	dlfile= symbol->objFile;
	/*
	 * might have undefined symbols or have not been relocated yet.
	 */
	if (dlfile->relocStatus==DL_NEEDRELOC) {
	    if (!_dl_relocateSections(dlfile)) {
		if (dl_searchLibraries()) {
		    /* find some undefined symbols, try again! */
		    _dl_relocateSections(dlfile);
		}
	    }
	}
	/*
	 * only returns the symbol if the relocation has completed
	 */
	if (dlfile->relocStatus==DL_RELOCATED)
	    return (void *)symbol->addr;
    }
    if (symbol) {
	_dl_setErrmsg("\"%s\" has undefined symbols", dlfile->filename);
    }else {
	_dl_setErrmsg("no such symbol \"%s\"", name);
    }
    return NULL;
}

/*
 * dl_close--
 *      closes the file and deallocate all resources hold by the file.
 *	note that any references to the deallocated resources will result
 *      in undefined behavior.
 */
void dlclose( handle )
     void *handle;
{
    _dl_closeObject((dlFile *)handle);
    return;
}

/*
 * dl_error--
 *      returns the error message string of the previous error.
 */
char *dlerror()
{
    return errmsg;
}

/*****************************************************************************
 *                                                                           *
 *    Object files handling Rountines                                        *
 *                                                                           *
 *****************************************************************************/

dlFile *_dl_openObject( fd, filename, filhdr, offset, mode )
     int fd; char *filename; FILHDR filhdr; int offset; int mode;
{
    dlFile *dlfile;

    dlfile= (dlFile *)malloc(sizeof(dlFile));
    bzero(dlfile, sizeof(dlFile));
    dlfile->relocStatus= DL_NEEDRELOC;
    dlfile->filename= STRCOPY(filename);

    if (!_dl_loadSymbols(dlfile, fd, filhdr, offset) ||
	!_dl_loadSections(dlfile, fd, offset)) {
	_dl_setErrmsg("Cannot load symbol table or sections from \"%s\"",
		      filename);
	_dl_closeObject(dlfile);
	return NULL;
    }

    if (!_dl_enterExternRef(dlfile)) {
	_dl_closeObject(dlfile);
	return NULL;
    }

    if(mode==DL_NOW) {
	if (!_dl_relocateSections(dlfile)) {
	    /*
	     * attempt to search the "standard" libraries before aborting
	     */
	    if (!dl_searchLibraries() ||
		!_dl_relocateSections(dlfile)) {

		_dl_setErrmsg("\"%s\" contains undefined symbols",
			      filename);
		_dl_closeObject(dlfile);
		return NULL;
	    }
	}
    }else {
	dlfile->relocStatus= DL_NEEDRELOC;
    }

    return dlfile;
}

void _dl_closeObject( dlfile )
     dlFile *dlfile;
{
    int i;
    dlFile *next;

    while(dlfile) {
	next= dlfile->next;
	if (dlfile->filename)
	    free(dlfile->filename);
	if (dlfile->sect)
	    free(dlfile->sect);
	if (dlfile->extss)
	    free(dlfile->extss);
	if (dlfile->extsyms)
	    free(dlfile->extsyms);
	/* frees any symbols associated with it */
	for(i=0; i < HASHTABSZ; i++) {
	    HEnt *ent= dlHashTable[i], *prev, *t;
	    prev= dlHashTable[i];
	    while(ent) {
		if (ent->symbol->objFile==dlfile) {
		    t= ent->next;
		    if (prev==dlHashTable[i]) {
			dlHashTable[i]= prev= ent->next;
		    }else {
			prev->next= ent->next;
		    }
		    free(ent);
		    ent= t;
		}else {
		    prev= ent;
		    ent=ent->next;
		}
	    }
	}
	free(dlfile);
	dlfile= next;
    }
}

int _dl_loadSymbols( dlfile, fd, filhdr, offset )
     dlFile *dlfile; int fd; FILHDR filhdr; int offset;
{
    SCNHDR *scnhdr;
    HDRR symhdr;
    char *pssext;
    pEXTR pext;
    int nscn, size, i;

    /*
     * load in section headers (don't need this for the executable during
     * init)
     */
    if (dlfile->relocStatus!=DL_RELOCATED) {
	nscn= filhdr.f_nscns;
	scnhdr= (SCNHDR *)malloc(sizeof(SCNHDR) * nscn);
	if (lseek(fd, filhdr.f_opthdr, SEEK_CUR)==-1 ||
	    read(fd, scnhdr, sizeof(SCNHDR)*nscn)!= sizeof(SCNHDR)*nscn)
	    return 0;
    }
    /*
     * load in symbolic header
     */
    if (lseek(fd, offset+filhdr.f_symptr, SEEK_SET)==-1 ||
	read(fd, &symhdr, sizeof(symhdr))!=sizeof(symhdr) ||
	symhdr.magic!=magicSym)
	return 0;
    /*
     * read external strings table
     */
    size= symhdr.issExtMax;
    pssext= (char *)malloc(size);
    if (lseek(fd, offset+symhdr.cbSsExtOffset, SEEK_SET)==-1 ||
	read(fd, pssext, size)!=size)
	return 0;
    /*
     * read external symbols table
     */
    size= symhdr.iextMax * sizeof(EXTR);
    pext= (pEXTR)malloc(size);
    if (lseek(fd, offset+symhdr.cbExtOffset, SEEK_SET)==-1 ||
	read(fd, pext, size)!=size)
	return 0;
    /*
     * copy the extern string space and symbols. The string space is
     * referenced by the hash table. We need the symbols either in
     * lazy resolution or when there are undefined symbols.
     */
    dlfile->extss= pssext;
    dlfile->issExtMax= symhdr.issExtMax;
    dlfile->extsyms= pext;
    dlfile->iextMax= symhdr.iextMax;
    /*
     * relocStatus should only be DL_RELOCATED for the executable's dlfile
     * during init. (Don't need to relocate the executable!)
     */
    if (dlfile->relocStatus==DL_RELOCATED)
	return 1;
    /*
     * otherwise, create the ScnInfo's
     */
    dlfile->nsect= nscn;
    size= sizeof(ScnInfo)*nscn;
    dlfile->sect= (ScnInfo *)malloc(size);
    bzero(dlfile->sect, size);
    for(i=0; i < nscn; i++) {
	dlfile->sect[i].hdr = scnhdr[i];
    }
    free(scnhdr);
    return 1;
}

/*
 * _dl_loadSections--
 *      loads all sections of an object file into memory.
 */
int _dl_loadSections(dlfile, fd, offset)
     dlFile *dlfile; int fd; int offset;
{
    int nsect= dlfile->nsect;
    int i;
    RELOC *relocEnt;
    int size;

    for(i=0; i < nsect; i++) {
	SCNHDR *hdr= &(dlfile->sect[i].hdr);
	char *sectnam;
	char *addr;
	int isBss= 0;

	sectnam= hdr->s_name;
	if(!strncmp(sectnam, ".text", 5)) {
	    int pagesize= getpagesize();
	    int size= (hdr->s_size%pagesize==0)? hdr->s_size :
		hdr->s_size - hdr->s_size % pagesize + pagesize;

	    /* page aligned */
	    addr= (char *)valloc(size);
	    dlfile->textAddress= (CoreAddr)addr;
	    dlfile->textVaddr= hdr->s_vaddr;
	    dlfile->textSize= size;
	}else {
	    addr= (char *)malloc(hdr->s_size);
	    if (!strncmp(sectnam, ".rdata", 6)) {
		dlfile->rdataAddress= (CoreAddr)addr;
		dlfile->rdataVaddr= hdr->s_vaddr;
	    }else if (!strncmp(sectnam, ".data", 5)) {
		dlfile->dataAddress= (CoreAddr)addr;
		dlfile->dataVaddr= hdr->s_vaddr;
	    }else if (!strncmp(sectnam, ".bss", 4)) {
		dlfile->bssAddress= (CoreAddr)addr;
		dlfile->bssVaddr= hdr->s_vaddr;
		bzero(addr, hdr->s_size);	/* zero out bss segment */
		isBss= 1;
	    }
	}
	dlfile->sect[i].addr= (CoreAddr)addr;
	if(!isBss) {
	    /*
	     * read in raw data from the file
	     */
	    if (hdr->s_size &&
		(lseek(fd, offset+hdr->s_scnptr, SEEK_SET)==-1 ||
		 read(fd, addr, hdr->s_size)!=hdr->s_size))
		return 0;
	    /*
	     * read in relocation entries from the file
	     */
	    if (hdr->s_nreloc) {
		size= sizeof(RELOC)* hdr->s_nreloc;
		dlfile->sect[i].relocEntries= relocEnt=
		    (RELOC *)malloc(size);
		if (lseek(fd, offset+hdr->s_relptr, SEEK_SET)==-1 ||
		    read(fd, relocEnt, size)!=size)
		    return 0;
	    }
	}
    }
    return 1;
}

static int _dl_openFile( filename, fhdr, offset )
     char *filename; FILHDR *fhdr; int offset;
{
    int fd;
    if ((fd=open(filename, O_RDONLY)) >= 0) {
	/*
	 * load in file header
	 */
	if (lseek(fd, offset, SEEK_SET)==-1 ||
	    read(fd, fhdr, sizeof(FILHDR))!=sizeof(FILHDR))
	    return -1;

    }
    return fd;
}


_dl_setErrmsg( va_alist )
     va_dcl
{
    va_list pvar;
    char *fmt;

    va_start(pvar);
    fmt= va_arg(pvar, char *);
    vsprintf(errmsg, fmt, pvar);
    va_end(pvar);
}
#endif /* ultrix */

#ifndef ASCDL_OK
#error "Unable to build ascDynaload.o. Turn off strict ansi cc options."
#endif /* adlok */
#ifdef __hpux
#define UNLOAD shl_unload
#define DLL_CAST (shl_t)
#define ASC_DLERRSTRING "NULL definition"
#endif /* __hpux */
#ifdef __WIN32__
#define UNLOAD FreeLibrary
#define DLLSYM GetProcAddress
#define DLL_CAST (HINSTANCE)
#define ASC_DLERRSTRING "unknown"
#endif /* __WIN32__ */
#ifndef UNLOAD
#define UNLOAD dlclose
#define DLLSYM dlsym
#define DLL_CAST (void *)
#define ASC_DLERRSTRING dlerror()
#endif /* UNLOAD */

int Asc_DynamicUnLoad(char *path)
{
  void *dlreturn;
  dlreturn = AscDeleteRecord(path);
  if (dlreturn == NULL) {
    FPRINTF(stderr, "Asc_DynamicUnLoad: unable to remember %s\n", path);
    return -3;
  }
  FPRINTF(stderr, "Asc_DynamicUnLoad: forgetting %s \n", path);
  return UNLOAD(DLL_CAST dlreturn);
}

/*
 * yourFuncOrVar = (YOURCAST)Asc_DynamicSymbol(libraryname,symbolname);
 * rPtr =
 *  (double (*)(double *, double *))Asc_DynamicSymbol("lib.dll","calc");
 * returns you a pointer to a symbol exported from the dynamically
 * linked library named, if the library is loaded with Asc_DynamicLoad 
 * and the symbol can be found in it.
 */
extern void *Asc_DynamicSymbol(CONST char *libname, CONST char *symbol)
{
  void *dlreturn;
  void *symreturn;
#ifdef __hpux
  int i;
#endif

  if (libname == NULL || symbol == NULL) {
    FPRINTF(stderr,"Unable to find needed library or function (%s) in (%s)\n",
      symbol,libname);
    return NULL;
  }
  dlreturn = AscFindDLRecord(libname);
  if (dlreturn == NULL) {
    FPRINTF(stderr,"Unable to find needed library %s\n", libname);
    return NULL;
  }
#ifdef __hpux
  i = shl_findsym(&dlreturn, symbol, TYPE_UNDEFINED, &symreturn);
  if (i == -1) {
    FPRINTF(stderr,"Unable to find needed symbol %s in %s (%s)\n",
                       symbol, libname, strerror(errno));
    symreturn = NULL;
  }
#else
  symreturn = (void *) DLLSYM(DLL_CAST dlreturn,symbol);
#endif
  if (symreturn == NULL) {
    FPRINTF(stderr,"Unable to find needed symbol %s in %s\n",symbol,libname);
    FPRINTF(stderr,"Error type %s\n",ASC_DLERRSTRING);
  }
  return symreturn;
}
