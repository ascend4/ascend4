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
*//** @file
	Ascend Module Control
*//*
	by Tom Epperly
	Created: 1/11/90
	Last in CVS: $Revision: 1.25 $ $Date: 1998/03/17 22:09:12 $ $Author: ballan $
*/

#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include "scanner.h"
#include "compiler.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "symtab.h"
#include "module.h"
#include "library.h"
#include <general/ospath.h>

#ifndef lint
static CONST char ModuleRCSid[] = "$Id: module.c,v 1.25 1998/03/17 22:09:12 ballan Exp $";
#endif /* RCS ID keyword */


#ifdef __WIN32__
#define SLASH '\\'
#define PATHDIV ';'
#else /* ! __WIN32__ */
#define SLASH '/'
#define PATHDIV ':'
#endif


struct module_t {
  symchar *name;                /* module's name, including extension
                                 * and version number, no path information.
                                 * This is how clients access and find
                                 * modules.
                                 */
  symchar *filename;            /* module's "full" path name---i.e., what
                                 * we need to feed to fopen() to open the
                                 * file. NULL implies a string module.
                                 */
  symchar *base_name;           /* module's name including extension
                                 * and WITHOUT version number.  Clients
                                 * should never see this.
                                 */
  FILE *f;                      /* module's FILE pointer.  Non null when
                                 * reading file, NULL otherwise---the result
                                 * of calling fopen() on filename.
                                 */
  CONST char *s;		/* module's string pointer. Never NULL 
                                 * except in creation process, destruction
                                 * process or if module wraps a file instead
                                 * of a string. NULL implies a file.
                                 * Not a symchar pointer.
                                 */
  struct module_t *required_by; /* A pointer to the module that issued
                                 * the REQUIRE statement that caused this
                                 * module to be loaded.
                                 * Always NULL for string modules.
                                 */
  CONST struct module_t *provided_by;
                                /* A pointer to the module that issued
                                 * the PROVIDE statement that caused this
                                 * module-alias to be created.
                                 */
  time_t time_last_modified;    /* when filename was last modified */
  unsigned long linenum;        /* used to store line number when parsing */
  unsigned long open_count;     /* the number of times this has been opened */
  unsigned long version;        /* the module's version. Initially zero; is
                                 * incremented when a module with the same
                                 * name is loaded.
                                 */
  struct gl_list_t *stats;	/* A pointer to a list of 
                                 * struct StatementList maintained at
                                 * NULL or length 1. It's a gl_list
                                 * so interfaces using module don't
                                 * have to see all the compiler headers.
                                 */
  void *scanbuffer;		/* A YY_BUFFER_STATE while the string
                                 * is being parsed or in the stack.
                                 * NULL otherwise.
                                 */
};



CONST char *g_alt_ending[MOD_FILE_EXTS] = {
  MOD_OLD_CODE,
  MOD_OLD_LIBRARY,
  MOD_NEW_CODE,
  MOD_NEW_LIBRARY,
  MOD_NEW_UNITS,
  MOD_CATCHALL
};


/* extern */
struct module_t *g_current_module = NULL;
/**<
 *  The current module.  Even though this variable is "extern",
 *  do NOT use this variable directly.  Instead, use a call to
 *  Asc_CurrentModule() to get the current module.
 */

static int g_string_modules_processed = 0;
/**<
 * This is a counter, to be incremented each time it is used to
 * create a string module name. Should not be reset to 0
 * unless all modules have been destroyed.
 */

static struct gl_list_t *g_module_list = NULL;
#define G_MODULE_LIST_INIT_SIZE 20L

/*----------
  forward declarations
*/

static int CmpModulesNameVers(CONST struct module_t*, CONST struct module_t*);
static struct module_t *FindModuleFile(CONST char *, int * CONST, int);
static struct module_t *CreateStringModule(CONST char *, int * CONST, CONST char *);
/* jds20041214 - winbase.h defines FreeModule(), so changed here to DeleteModule(). */
#define DeleteModule(m) ascfree(m)
static unsigned long ModuleNameToInternalNameVers(CONST char *, char * CONST);
static int ModuleSearchPath(CONST char*, char*, struct module_t*, int * CONST);
static struct module_t *NewModule(CONST char *);
static struct module_t *OpenModuleInternal(CONST char *, int * CONST,
                                           int, CONST char *);
static void RemoveModule(CONST struct module_t *);
static struct module_t *SearchForModule(CONST struct module_t *);
static int StoreModule(CONST struct module_t *);
static void WriteWhyNotFound(symchar *,  int);

/*------------------------------------------------------------------------------
  MODULE HANDLING
*/

/* see module.h for details of functions not documented here */

int Asc_InitModules(unsigned long init_size){
  if( g_module_list != NULL ) {
    return 0;
  }
  g_module_list = gl_create(init_size);
  if( g_module_list == NULL ) {
	FPRINTF(ASCERR,"FAILED TO CREATE MODULE LIST\n");
    return 1;
  }
  return 0;
}


static DestroyFunc g_sldestroy;

static void DestroyModule(struct module_t *m){
  if (m == NULL) return;
  if (m->s != NULL) {
    ascfree((char *)m->s);
    m->s = NULL;
    if (m->stats != NULL) {
      gl_iterate(m->stats,g_sldestroy);
      gl_destroy(m->stats);
      m->stats = NULL;
    }
  }
  if (m->scanbuffer!= NULL) {
    FPRINTF(ASCERR, "Module %s being destroyed while opened by lexer\n",
             SCP(m->name));
  }
  DeleteModule(m);
}


void Asc_DestroyModules(DestroyFunc f){
  g_sldestroy = f;
  if (g_module_list != NULL) {
    gl_iterate(g_module_list,(DestroyFunc)DestroyModule);
    gl_destroy( g_module_list );
    g_module_list = NULL;
  }
}


extern struct module_t *Asc_OpenStringModule(CONST char *inputstring,
    int *status,
    CONST char *prefix
){
  char *name = NULL;
  char *keep_string;
  int dummy;
  int input_len;
  struct module_t *result;

  if (inputstring != NULL) {
    /* copy and configure to string we keep for flex processing */
    input_len = strlen(inputstring);
    keep_string = (char *)ascmalloc(input_len+3);
    if (keep_string != NULL) {
      strcpy(keep_string,inputstring);
      keep_string[++input_len] = '\0';
      keep_string[++input_len] = '\0';
      /* create name only if we successfully copied inputstring */
      name = (char *)ascmalloc(strlen((prefix != NULL)? prefix : "")+35);
      if (name != NULL) {
        sprintf(name, "%s_global_%d", ((prefix != NULL) ? prefix : ""),
                                      ++g_string_modules_processed);
      }
    }
  }else{
	keep_string = NULL;
  }

  if( status != NULL ) {
    result = OpenModuleInternal(name, status, FALSE, keep_string);
  } else {
    result = OpenModuleInternal(name, &dummy, FALSE, keep_string);
  }
  ascfree(name);
  return result;
}


extern struct module_t *Asc_OpenModule(CONST char *name, int *status){
  if( status != NULL ) {
    return OpenModuleInternal(name, status, FALSE, NULL);
  } else {
    int dummy;
    return OpenModuleInternal(name, &dummy, FALSE, NULL);
  }
}


extern struct module_t *Asc_RequireModule(CONST char *name, int *status){
  if( status != NULL ) {
    return OpenModuleInternal(name, status, TRUE, NULL);
  } else {
    int dummy;
    return OpenModuleInternal(name, &dummy, TRUE, NULL);
  }
}


/**
	This function is the glue between the user callable Asc_OpenModule()
	and Asc_RequireModule() functions and FindModuleFile().  Consult
	those functions' documentation for more information.  The
	`do_not_overwrite' flag tells if we were called from
	Asc_OpenModule() (==FALSE) or Asc_RequireModule() (==TRUE).

	@param name filename of the module to find
	@param status status to return to caller
	@param do_not_overwrite Should we keep existing modules?
	@param keep_string String we keep and parse if not NULL.

	When str is NULL:
		This function calls FindModuleFile() to find the module named `name'.
		The status of FindModuleFile is returned to the caller in `status'.
		If the call to FindModuleFile is successful and do_not_overwrite is
		FALSE, we make the returned module the current module and inform
		the scanner of the change.
	When str is NOT NULL:
		Calls FindModuleFile to guard against duplication, then sets up
		module and scanner for string scanning.
*/
static
struct module_t *OpenModuleInternal(CONST char *name,
                                    int * CONST status,
                                    int do_not_overwrite,
                                    CONST char *keep_string)
{
  struct module_t *new_module;

  /*
   *  Make sure the user gave us valid data.
   *  This is valid for both string and file modules.
   *  String modules do this if user gives bad input or if
   *  malloc failed in copying string.
   */
  if(( name == NULL ) || ( *name == '\0' )) {
    *status = -4;
    return NULL;
  }

  /*
   *  Find the file and create the struct module_t
   *  or create module from string.
   */
  if (keep_string == NULL) {
    new_module = FindModuleFile(name, status, do_not_overwrite);
    if( new_module == NULL ) {
      /* FindModuleFile set *status */
      return NULL;
    }
  }else{
    new_module = CreateStringModule(name, status, keep_string);
    if( new_module == NULL ) {
      /* CreateStringModule set *status */
      return NULL;
    }
  }

  /*
   *  Check the return status of FindModuleFile() to determine
   *  if we should tell the scanner about the module
   */
  if( *status == 5 ) {
    /*  We were requiring the module and it already exists, do
     *  not change the file in the scanner
     */
    return new_module;
  }
  if( *status == 4 ) {
    /* Recursive require, do not change the files in the scanner */
    return new_module;
  }

  /*
   *  Store the scanner's position in the current module
   */
  if( g_current_module != NULL ) {
    g_current_module->linenum = LineNum();
  }

  /*
   *  Wire up the modules: push existing module onto the new
   *  module's required_by pointer
   */
  new_module->required_by = g_current_module;
  g_current_module = new_module;

  /*
   *  Tell the scanner to parse the new module
   */
  if (keep_string == NULL) {
    Asc_ScannerAssignFile(new_module->f,1);
  } else {
    assert(new_module->scanbuffer != NULL);
    Asc_ScannerAssignString(new_module->scanbuffer,1,1);
  }

  /*
   *  Return the new module
   */
  return new_module;
}


/**
	Find or create a module named `name'.  Return the module and put
	an exit code in `status'.  If `do_not_overwrite' is TRUE, an existing
	module named `name' will be returned; otherwise, a new module will
	be created, overwriting any existing module having that name.

	The value put into `status' is one of:
	   -3  a memory error occurred: not enough memory to create module;
	       returning NULL
	   -2  a potential file matching module name was found but the
	       file could not be opened for reading; returning NULL
	   -1  could not find a file for the module `name'; returning NULL
	    0  a new module was successfully created and is being returned
	    1  a module with `name' already existed; the file it points to
	       does NOT match the file just found for module `name'; the old
	       module was overwritten and the new module is being returned
	    2  a module with `name' already exited; the file it points to
	       matches the file just found for module `name'; the existing
	       module is being returned
	    3  a module with `name' already existed; it was an alias for
	       for another module; the old module was overwritten and the
	       new module is being returned
	    4  a module with `name' already existed; it was being read when
	       we tried to open it again for reading (recursive require).
	       The existing module is being returned.
	    5  The argument `do_not_overwrite' is TRUE and a module named
	       `name' was found, returning it
*/
static
struct module_t *FindModuleFile(CONST char *name,
                                int * CONST status,
                                int do_not_overwrite)
{
  struct module_t *new_module;  /* the newly created module */
  struct module_t *dup = NULL;  /* an existing module with the same name */
  char filename[PATH_MAX];      /* work area to find module's filename */
  int result;                   /* return value when searching for module */
  int error;                    /* error number returned by fopen() or stat() */

  assert(name != NULL);
  assert(status != NULL);

  /*
	Create space for the module and set its base_name to a "proper"
	name---i.e,, to the first character after the rightmost slash in
	the name the user passed to us.
  */
  new_module = NewModule( name );
  if( new_module == NULL ) {
	/*FPRINTF(ASCERR,"NEW MODULE RETURNED NULL\n");*/
    *status = -3;
    return NULL;
  }
  /*FPRINTF(ASCERR,"NEW MODULE RETURNED NON-NULL\n");*/


  /*
	Check to see if a module having the same base_name exists.
	If so, fetch it.
   */
  dup = SearchForModule( new_module );

  /*
	If were we called from RequireModule, return if a module
	having this name already exists
   */
  if(( do_not_overwrite == TRUE ) && ( dup != NULL )) {
    DeleteModule( new_module );
    *status = 5;
    return dup;
  }

  /*
	Search for the module in ASCENDLIBRARY and, if found,  set its
	`f', `time_last_modified', and `line_number' fields.
  */
  result = ModuleSearchPath( name, filename, new_module, &error );
 
  /*
	Check for a memory error in ModuleSearchPath.
  */
  if( result == -3 ) {
    DeleteModule( new_module );
    *status = -4;
    return NULL;
  }/* else{
	CONSOLE_DEBUG("FOUND MODULE FILE, result=%d\n",result);
  }*/

  /*
	If we couldn't find the module or a fopen error occurred, print
	a message and exit the function
  */
  if( result == -1 ) {
    WriteWhyNotFound( new_module->filename, error );
    DeleteModule(new_module);
    *status = -2;
    return NULL;
  }
  if( result == 1 ) {
    ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to locate file for module '%s'", name);
    DeleteModule(new_module);
    *status = -1;
    return NULL;
  }

  /*
	Create a symbol for the filename.  We created a symbol for the
	base_name when we created the module.
  */
  new_module->filename = AddSymbol(filename);

  assert(new_module->f != NULL);

  /*
	If a module having the same base_name does not exist,
	set the version number to zero, insert the new module into
	the module list and return it.
  */
  if( dup == NULL ) {
    /* no module with name 'name' exists.  Save it and return */
    new_module->open_count = 1;
    new_module->version = 0;
    sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
	/*FPRINTF(ASCERR,"SYMBOL FOR FILE IS %s\n",filename);*/
    new_module->name = AddSymbol(filename);

	
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
	  ERROR_REPORTER_HERE(ASC_PROG_ERROR,"COULDN'T STORE MODULE %s",new_module->filename);
      *status = -3;
      return NULL;
    }
    *status = 0;
    return new_module;
  }

  /*
	If the existing module's (dup) provided_by pointer is not null,
	we are overwriting a module-alias.
  */
  if( dup->provided_by != NULL ) {
    /* remove the module-alias from the module list and destroy it */
    RemoveModule( dup );
    DeleteModule( dup );
    /* add the new_module to the list and return */
    new_module->open_count = 1;
    new_module->version = 0;
    sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
    new_module->name = AddSymbol(filename);
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
      *status = -3;
      return NULL;
    }
    *status = 3;
    return new_module;
  }

  /*
	If the existing module's (dup) file pointer `f' is not null,
	we have some bizarre situation.  Either return or PANIC.
  */
  if( dup->f != NULL ) {
    if( CmpSymchar( dup->filename, new_module->filename ) != 0 ) {
      /*  
		we were reading `dup' as if it were module `mod_name' when we
		were told to treat `new_module' as if it were module `mod_name'.
		PANIC!
      */
      fclose(dup->f);
      fclose(new_module->f);
      Asc_Panic(2, "FindModuleFile", "While reading file \"%s\" for module %s,"
                "\n\treceived a request to read \"%s\" as module %s",
                SCP(dup->filename), SCP(dup->base_name),
                SCP(new_module->filename), SCP(new_module->base_name));
    }
    if( dup->time_last_modified != new_module->time_last_modified ) {
      /* timestamp changed while reading the file. PANIC! */
      fclose(dup->f);
      fclose(new_module->f);
      Asc_Panic(2, "FindModuleFile", "Timestamp on file \"%s\" changed\n"
                "while file was open for reading", SCP(dup->filename));
    }
    /* recursive require! */
    ERROR_REPORTER_HERE(ASC_USER_WARNING
		,"Module '%s' includes itself either directly or indirectly (ignoring)"
		, SCP(new_module->base_name)
	);
    DeleteModule( new_module );
    *status = 4;
    return dup;
  }

  if(( CmpSymchar(dup->filename, new_module->filename) == 0 )
     && ( dup->time_last_modified == new_module->time_last_modified ))
  {
    /*
		The same module.  Copy the file pointer (f) from `new_module'
		to `dup', increment dup's open count, free `new_module', and
		return `dup'
    */
    dup->f = new_module->f;
    dup->linenum = new_module->linenum;
    dup->open_count++;
    DeleteModule(new_module);
    *status = 2;
    return dup;
  }

  /*
	Either two different files both want to be called module `nmae',
	or the timestamp on the file has changed.  In either case, treat
	`new_module' as a new version of `dup'.
  */
  new_module->open_count = 1;
  new_module->version = 1 + dup->version;
  sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
  new_module->name = AddSymbol(filename);
  if( StoreModule( new_module ) != 0 ) {
    DeleteModule( new_module );
    *status = -3;
    return NULL;
  }
  *status = 1;
  return new_module;
}

/** Create a module named `name'.  Return the module and put
 *  an exit code in `status'. 
 *  Name is expected to be unique for all time (or at least until we
 *  reinit the compiler).
 *
 *  The value put into `status' is one of (following FindModuleFile):
 *      -3  a memory error occurred: not enough memory to create module;
 *          returning NULL.
 *      -2  a potential file matching module name was found but the
 *          file could not be opened for reading; returning NULL.
 *          (never occurs).
 *      -1  could not find a file for the module `name'; returning NULL
 *          (never occurs).
 *       0  a new module was successfully created and is being returned
 *       1  a module with `name' already existed; the file it points to
 *          does NOT match the file just found for module `name'; the old
 *          module was overwritten and the new module is being returned
 *          (never occurs).
 *       2  a module with `name' already exited; the file it points to
 *          matches the file just found for module `name'; the existing
 *          module is being returned
 *          (never occurs).
 *       3  a module with `name' already existed; it was an alias for
 *          for another module; the old module was overwritten and the
 *          new module is being returned
 *          (never occurs).
 *       4  a module with `name' already existed; it was being read when
 *          we tried to open it again for reading (recursive require).
 *          The existing module is being returned.
 *          (never occurs).
 *       5  The argument `do_not_overwrite' is TRUE and a module named
 *          `name' was found, returning it
 *          (never occurs).
 *
 *  In summary, only -3 and 0 are expected returns. If we decide to
 *  have some less fascist string module naming scheme, we may reinstate
 *  some of these returns.
 */
static
struct module_t *CreateStringModule(CONST char *name,
                                int * CONST status,
                                CONST char *keep_string)
{
/* if this is 1, lots of inappropriate for string buffers code is
 * uncommented
 */
#define USE_FOR_STRINGS 0

  struct module_t *new_module;  /* the newly created module */
  struct module_t *dup = NULL;  /* an existing module with the same name */
  char filename[PATH_MAX];      /* work area to find module's filename */
#if USE_FOR_STRINGS
  int error;                    /* errno returned by fopen() or stat() */
  int result;                   /* return value when searching for module */
#endif /* use for strings */

  assert(name != NULL);
  assert(keep_string != NULL);
  assert(status != NULL);

  /*
   *  Create space for the module and set its base_name to a "proper"
   *  name---i.e,, to the first character after the rightmost slash in
   *  the name the user passed to us.
   */
  new_module = NewModule( name );
  if( new_module == NULL ) {
    *status = -3;
    return NULL;
  }

  /*
   *  Check to see if a module having the same base_name exists.
   *  If so, fetch it.
   */
  dup = SearchForModule( new_module );
  assert(dup == NULL); /* string modules are to be unique */
  /* probably should be ascpanic to avoid mystery messages */

#if USE_FOR_STRINGS
  /*
   *  If were we called from RequireModule, return if a module
   *  having this name already exists
   */
  if(( do_not_overwrite == TRUE ) && ( dup != NULL )) {
    DeleteModule( new_module );
    *status = 5;
    return dup;
  }
#endif /* use for strings */


#if USE_FOR_STRINGS
  /*
   *  Search for the module in ASCENDLIBRARY and, if found,  set its
   *  `f', `time_last_modified', and `line_number' fields.
   */
  result = ModuleSearchPath( name, filename, new_module, &error );

  /*
   * Check for a memory error in ModuleSearchPath.
   */
  if( result == -3 ) {
    DeleteModule( new_module );
    *status = -3;
    return NULL;
  }

  /*
   *  If we couldn't find the module or a fopen error occurred, print
   *  a message and exit the function
   */
  if( result == -1 ) {
    WriteWhyNotFound( new_module->filename, error );
    DeleteModule(new_module);
    *status = -2;
    return NULL;
  }
  if( result == 1 ) {
    ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to locate file for module '%s'", name);
    DeleteModule(new_module);
    *status = -1;
    return NULL;
  }

#else
  new_module->s = keep_string;
  new_module->f = NULL;
  new_module->linenum = 1;
  /* The following is really dumb, but should keep everyone happy.
   * I'll be damned if I can find a function call that returns
   * time since the epoch without some other input pointer I don't have
   * just by looking at the man pages.
   */
  new_module->time_last_modified = (time_t)g_string_modules_processed;
#endif /* use for strings */

  /*
   *  Create a symbol for the filename.  We created a symbol for the
   *  base_name when we created the module.
   */
#if USE_FOR_STRINGS
  new_module->filename = AddSymbol(filename);
#else
  new_module->filename = NULL;
#endif /* use for strings */

  /*
   *  If a module having the same base_name does not exist,
   *  set the version number to zero, insert the new module into
   *  the module list and return it.
   */
  if( dup == NULL ) { /* always TRUE */
    /* no module with name 'name' exists.  Save it and return */
    new_module->open_count = 1;
    new_module->version = 0;
    sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
    new_module->name = AddSymbol(filename);
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
      *status = -3;
      return NULL;
    }
    new_module->scanbuffer = 
      Asc_ScannerCreateStringBuffer(keep_string,strlen(keep_string));
    *status = 0;
    return new_module;
  }

#if USE_FOR_STRINGS
  /*
   *  If the existing module's (dup) provided_by pointer is not null,
   *  we are overwriting a module-alias.
   */
  if( dup->provided_by != NULL ) {
    /* remove the module-alias from the module list and destroy it */
    RemoveModule( dup );
    DeleteModule( dup );
    /* add the new_module to the list and return */
    new_module->open_count = 1;
    new_module->version = 0;
    sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
    new_module->name = AddSymbol(filename);
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
      *status = -3;
      return NULL;
    }
    *status = 3;
    return new_module;
  }

  /*
   *  If the existing module's (dup) file pointer `f' is not null,
   *  we have some bizarre situation.  Either return or PANIC.
   */
  if( dup->f != NULL ) {
    if( CmpSymchar( dup->filename, new_module->filename ) != 0 ) {
      /*  we were reading `dup' as if it were module `mod_name' when we
       *  were told to treat `new_module' as if it were module `mod_name'.
       *  PANIC!
       */
      fclose(dup->f);
      fclose(new_module->f);
      Asc_Panic(2, "FindModuleFile", "While reading file \"%s\" for module %s,"
                "\n\treceived a request to read \"%s\" as module %s",
                SCP(dup->filename), SCP(dup->base_name),
                SCP(new_module->filename), SCP(new_module->base_name));
    }
    if( dup->time_last_modified != new_module->time_last_modified ) {
      /* timestamp changed while reading the file. PANIC!
       */
      fclose(dup->f);
      fclose(new_module->f);
      Asc_Panic(2, "FindModuleFile", "Timestamp on file \"%s\" changed\n"
                "while file was open for reading", SCP(dup->filename));
    }
    /* recursive require! */
    ERROR_REPORTER_HERE(ASC_USER_WARNING
		,"Module %s includes itself either directly or indirectly (ignoring)"
		, SCP(new_module->base_name)
	);
    DeleteModule( new_module );
    *status = 4;
    return dup;
  }

  if(( CmpSymchar(dup->filename, new_module->filename) == 0 )
     && ( dup->time_last_modified == new_module->time_last_modified ))
  {
    /*
     *  The same module.  Copy the file pointer (f) from `new_module'
     *  to `dup', increment dup's open count, free `new_module', and
     *  return `dup'
     */
    dup->f = new_module->f;
    dup->linenum = new_module->linenum;
    dup->open_count++;
    DeleteModule(new_module);
    *status = 2;
    return dup;
  }

  /*
   *  Either two different files both want to be called module `nmae',
   *  or the timestamp on the file has changed.  In either case, treat
   *  `new_module' as a new version of `dup'.
   */
  new_module->open_count = 1;
  new_module->version = 1 + dup->version;
  sprintf(filename,"%s<%lu>",SCP(new_module->base_name),new_module->version);
  new_module->name = AddSymbol(filename);
  if( StoreModule( new_module ) != 0 ) {
    DeleteModule( new_module );
    *status = -3;
    return NULL;
  }
  *status = 1;
  return new_module;
#endif /* use for strings */
  Asc_Panic(2, "CreateStringModule", "String buffer \"%s\" misunderstood"
            "while opening for reading", new_module->name);
  exit(2); /* Needed to keep gcc from whining */
}


/*------------------------------------------------------------------------------
  OPENING A MODULE FROM THE SEARCHPATH
*/

struct ModuleSearchData{
	struct FilePath *fp; /**< the relative path we're searching for */
	int error;           /**< error code (in case stat or fopen failed) */
	time_t mtime;        /**< mtime (from the stat command) */
	FILE *f;             /**< return the open file pointer */
	struct stat buf;
	struct FilePath *fp_found; /**< the full path we found */
};

FilePathTestFn module_searchpath_test;

/**
	@return 1 on success
*/
int module_searchpath_test(struct FilePath *path,void *searchdata){
	struct FilePath *fp, *fp1, *fp2;
	struct ModuleSearchData *sd;
	FILE *f;
	char *tmp;

	sd = (struct ModuleSearchData *)searchdata;
	assert(sd!=NULL);
	assert(sd->fp!=NULL);

	tmp  = ospath_str(sd->fp);
	/* CONSOLE_DEBUG("About to concat path '%s'...",tmp); */
	ospath_free_str(tmp);

	fp1 = ospath_concat(path,sd->fp);

	tmp  = ospath_str(sd->fp);
	/* CONSOLE_DEBUG("Checking for path '%s'...",tmp); */
	ospath_free_str(tmp);

	if(ospath_stat(fp1,&sd->buf)){
		sd->error = errno;
		/* CONSOLE_DEBUG("Stat failed");*/
		ospath_free(fp1);
		return 0;
	}

	f = ospath_fopen(fp1, "r");
	if(f==NULL){
		sd->error = errno;
		/* CONSOLE_DEBUG("Fopen failed"); */
		ospath_free(fp1);
		return 0;
	}

	sd->mtime = sd->buf.st_mtime;
	sd->f = f;
	sd->fp_found = fp1;
	
	/* CONSOLE_DEBUG("File found"); */
	return 1;
};


/** 
	This function tries to find a file corresponding to the argument
	"name" by sending "name" to the function ModuleStatFile() which
	will attempt to open "name" as a file.  If that fails, this
	function then prepends each entry in the search path
	PATHENVIRONMENTVAR to "name" and attempts to open the resulting
	file (by once again calling ModuleStatFile()).
	
	On success, the argument "filename" will be set to the path to the
	file, and the `f', `time_last_modified', and `linenum' members of
	the module `m' will be set.
	
	If ModuleStatFile() encounters an error opening the file, the
	value of errno will be passed back to the caller in the `error'
	argument.

	@return
		-4  Invalid partial path in parameter 'filename'.
	    -3  Memory error occurred when trying to get PATHENVIRONMENTVAR
	    -1  Error encountered in ModuleStatFile, check `error' argument
	     0  Success
	     1  Could not find a file named "name"
*/
static
int ModuleSearchPath(CONST char *name,
                     char *filename,
                     struct module_t *m,
                     int * CONST error
){
	register size_t length;
	int result;
	int path_entries;
	int j;
	register CONST char *t;
	struct FilePath *fp1, *fp2;
	char *tmp;
	struct FilePath **sp1 = NULL;
	struct ModuleSearchData sd;

	assert( name != NULL );
	assert( filename != NULL );
	assert( m != NULL );
	assert( error != NULL );

	/* CONSOLE_DEBUG("Launching ModuleSearchPath with '%s'",name); */

	fp1 = ospath_new_noclean(name);
	if(fp1==NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid partial path '%s'",name);
		ospath_free(fp1);
		return -4;
	}

	tmp = ospath_str(fp1);
	/* CONSOLE_DEBUG("Searching for '%s'",tmp); */
	ospath_free_str(tmp);

	/* attempt to open "name" directly */
	if(0==ospath_stat(fp1,&sd.buf) && NULL!=(sd.f = ospath_fopen(fp1,"r")) ){

		CONSOLE_DEBUG("File '%s' opened directly, without path search",name);
		sd.fp_found = fp1;

	}else{

 		/* CONSOLE_DEBUG("ENV var name is '%s'",PATHENVIRONMENTVAR); */

		tmp = Asc_GetEnv(PATHENVIRONMENTVAR);
		if(tmp==NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERROR,"No paths to search (is env var '%s' set?)",PATHENVIRONMENTVAR);
			return 1;
		}

		/*  CONSOLE_DEBUG("ENV var value is '%s'",tmp); */

		sp1 = ospath_searchpath_new(tmp);
		if(sp1==NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Unable to process %s value '%s'",PATHENVIRONMENTVAR,tmp);
			/* memory error */
			ascfree(tmp);
			return -3;
		}
		ascfree(tmp);

		/* CONSOLE_DEBUG("Created SP with %d elements",ospath_searchpath_length(sp1)); */

		sd.fp = fp1;

		fp2 = ospath_searchpath_iterate(sp1, &module_searchpath_test, &sd);

		if(fp2==NULL){
			*error = sd.error;
			ospath_searchpath_free(sp1);		
			CONSOLE_DEBUG("File '%s' not found in search path",name);		
			return -1;
		}

		tmp = ospath_str(fp2);
		ospath_searchpath_free(sp1);					
		assert(tmp!=NULL);
		/* CONSOLE_DEBUG("Found file in '%s' in search path",tmp); */
		ospath_free_str(tmp);
	}

	m->f = sd.f;
	m->time_last_modified = sd.mtime;
	m->linenum = 1;
	ospath_strcpy(sd.fp_found,filename,PATH_MAX);
	if(fp1!=sd.fp_found){
		ospath_free(fp1);
	}
	ospath_free(sd.fp_found);
	return 0; /* success */
}


/**
	Print an error (based on the errno `error') explaining why we could
	not open/stat the file named `filename'.
*/
static
void WriteWhyNotFound(symchar *filename, int error)
{
  switch( error ) {
  case EACCES:
    ERROR_REPORTER_HERE(ASC_USER_ERROR,
	    "File or directory permissions don't allow you to access '%s'.",
	    SCP(filename)
	);
    break;
  case EFAULT:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"Filename pointer or buffer pointer was bad."
	);
    break;
  case EIO:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"I/O error in reading '%s'.",SCP(filename)
	);
    break;
  case ENAMETOOLONG:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"The path for '%s' is too long.",SCP(filename)
	);
    break;
  case ENOENT:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"File '%s' doesn't exist.",SCP(filename)
	);
    break;
  case ENOTDIR:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"A component of the path name '%s' is not a directory.",SCP(filename)
	);
    break;
#ifndef __WIN32__
  case ELOOP:
    /*  no symlinks in windows land  */
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"There are too many symbolic links in '%s'.",SCP(filename)
	);
    break;
#endif  /*  __WIN32__  */
  default:
    ERROR_REPORTER_HERE(ASC_USER_ERROR
		,"File not available for unknown reasons (error %d)"
        ,error
	);
    break;
  }
}


/**  See the header file for this function's documentation
 **/
extern int Asc_ModuleCreateAlias(CONST struct module_t *m, CONST char *name)
{
  struct module_t *new_module;
  struct module_t *dup;
  char mod_name[PATH_MAX];

  assert( m != NULL );

  /*
   *  Make sure the user gave us good data
   */
  if(( name == NULL ) || ( *name == '\0' )) {
    return -4;
  }

  /*
   *  Make sure m is not a module-alias.  This shouldn't happen since
   *  the user should not be able to get his hands on module-aliases.
   */
  if( m->provided_by != NULL ) {
    ERROR_REPORTER_HERE(ASC_USER_ERROR
	    ,"Module '%s' is a module-alias. Module to alias must not be a module-alias\n"
	    ,SCP(m->name)
	);
    return -4;
  }

  /*
   *  Create a new module or return -3 if we could not
   */
  new_module = NewModule( name );
  if( new_module == NULL ) {
    return -3;
  }

  dup = SearchForModule( new_module );

  if( dup == NULL ) {
    /* no module with this name exists.  Set up the module-alias
     * and store it
     */
    new_module->provided_by = m;
    /* next line probably redundant */
    new_module->base_name = AddSymbol(SCP(new_module->base_name));
    new_module->version = 0;
    sprintf(mod_name,"%s<%lu>",SCP(new_module->base_name),new_module->version);
    new_module->name = AddSymbol(mod_name);
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
      return -3;
    }
    return 0;
  }

  /*
   *  Check to see if a module-alias with this name already exists
   *  that points to this module.  If so, destroy the new one
   *  silently and return.
   */
  if( dup->provided_by == m ) {
    DeleteModule( new_module );
    return 2;
  }

  /*
   *  Check to see if a module-alias with this name already exists
   *  that points to different module.  If so, check to see if the
   *  filenames are the same.  If the filename not the same, overwrite
   *  the existing module noisly; else, overwrite it silently.
   */
  if( dup->provided_by != NULL ) {
    if( CmpSymchar( dup->provided_by->filename, m->filename ) != 0 ) {
      ERROR_REPORTER_HERE(ASC_USER_WARNING,
        "PROVIDE \"%s\" in file '%s' overwrites PROVIDE \"%s\" in file '%s'"
        ,SCP(new_module->base_name), SCP(m->filename)
        ,SCP(dup->base_name), SCP(dup->provided_by->filename)
      );
    }
    RemoveModule( dup );
    DeleteModule( dup );
    new_module->provided_by = m;
    /* probably redundant addsymbol next line */
    new_module->base_name = AddSymbol(SCP(new_module->base_name));
    new_module->version = 0;
    sprintf(mod_name,"%s<%lu>",SCP(new_module->base_name),new_module->version);
    new_module->name = AddSymbol(mod_name);
    if( StoreModule( new_module ) != 0 ) {
      DeleteModule( new_module );
      return -3;
    }
    return 3;
  }
  
  /*
   *  Check to see if the duplicate module is actually the current
   *  module---i.e., module ``foo.a4c'' contains the statement
   *  ``PROVIDE "foo.a4c";''  If so, destroy the new_module silently.
   */
  if( dup == g_current_module ) {
    DeleteModule( new_module );
    return 1;
  }

  /*
   *  If we made it here, we attempting to PROVIDE a real module that
   *  already exists.  Issue an error, destroy the new_module, and
   *  return.
   */
  ERROR_REPORTER_HERE(ASC_USER_ERROR,
    "File \"%s\" cannot PROVIDE '%s' because a module with that name already exists (%s)."
    , SCP(m->filename), SCP(new_module->base_name), SCP(dup->name)
  );
  DeleteModule( new_module );
  return -2;
}

/** 
	Allocate space for a new module and set its fields to some
	reasonable defaults.  If `name' is not NULL, set the module's
	base_name to point to the first character after the rightmost
	slash (`/' on UNIX, `/' on Windows) in `name', or to `name' if
	it contains no slashes.  Note that this function will create
	a symbol for base_name.  Return NULL if malloc fails.
*/
static
struct module_t *NewModule(CONST char *name){
  struct module_t *newmodule;     /* the new module */
  char *tmp;                /* result of strrchr(); used to get base_name */
  struct FilePath *fp1;

  newmodule = (struct module_t *)ascmalloc(sizeof(struct module_t));
  if( newmodule == NULL ) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"UNABLE TO MALLOC FOR NEW MODULE");
    return NULL;
  }
  newmodule->name = NULL;
  newmodule->filename = NULL;
  newmodule->f = NULL;
  newmodule->s = NULL;
  newmodule->stats = NULL;
  newmodule->scanbuffer = NULL;
  newmodule->required_by = NULL;
  newmodule->provided_by = NULL;
  newmodule->time_last_modified = (time_t)0;
  newmodule->linenum = 0;
  newmodule->open_count = 0;
  newmodule->version = ULONG_MAX;

  /* CONSOLE_DEBUG("New path: %s",name); */
  fp1 = ospath_new(name);
  tmp = ospath_getbasefilename(fp1);
  if(tmp!=NULL && strlen(tmp)!=0){
	newmodule->base_name = AddSymbol(tmp);
  }
  ospath_free(fp1);

  /* CONSOLE_DEBUG("Module base-name: %s",newmodule->base_name); */
  return newmodule;
}

/*------------------------------------------------------------------------------
  GLOBAL MODULE LIST MANAGEMENT

 *  The following four functions are used to store, remove, and search
 *  for modules in the global module list, the gl_list `g_module_list'.
 *  We do this (instead of inlining the calls to gl_*()) to provide a
 *  bit of data abstraction, though I'm sure there are some implicit
 *  assumptions in the code as to how modules are stored on the gl_list.
 *
 *  These functions handle a NULL g_module_list gracefully.
 *
 *  Modules are appended to the g_module_list in the order they are
 *  OPENED, so later versions of the same file appear toward the end of
 *  the list.  Note that a module in the back of the list may define
 *  types needed by module at the front of the list, due to the magic of
 *  the REQUIRE statement.
 *
 *  Since the modules are in the order opened, implementing a
 *  Re-Read-All-Files function wouldn't be too difficult, but the
 *  REQUIRE statements in each file would still have to be followed.
 *
 *  Since module-aliases are implemented as modules and are stored on
 *  the g_module_list, they must sometimes be replaced by actual
 *  modules or new module-aliases.  When this occurs, a module-alias
 *  must be removed from the g_module_list.
 *
 *  When searching for a module, we start at the end of the list and
 *  move towards the front.  The comparison function looks for the
 *  base-names to be equal and for the version number of the found
 *  module being less than or equal to the version number of the module
 *  passed into the SearchForModule() function.  The comparison
 *  function uses less than or equal for the version number because one
 *  doesn't know the version number of the most recently read version.
 *  By setting the version number in the module passed to
 *  SearchForModule() to be ULONG_MAX, you will get the first module
 *  where the base-names match, which should be the most recently read
 *  version of that module.
 *-------------------------------------------------
 */


/** Store `m' in the global module gl_list `g_module_list', creating
 *  the g_module_list if needed.  Return 0 for success or 1 if the
 *  g_module_list could not be created.
 *
 *  Storing `m' in g_module_list simply appends it to the list.
 */
static
int StoreModule(CONST struct module_t *m)
{
  /* initialize the global module list if required */
  if((g_module_list==NULL) && (Asc_InitModules(G_MODULE_LIST_INIT_SIZE)!=0)) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"FAILED TO ASC_INITMODULES");
    return 1;
  }
  gl_append_ptr( g_module_list, (VOIDPTR)m );
  return 0;
}


/** Remove `m' from the global module gl_list `g_module_list'.
 *
 *  This function searches backwards through g_module_list and uses
 *  CmpModulesNameVers() to determine which module to remove.
 */
static
void RemoveModule(CONST struct module_t *m)
{
  unsigned long place;

  if( g_module_list == NULL ) {
    return;
  }
  place = gl_search_reverse( g_module_list, m, (CmpFunc)CmpModulesNameVers );
  if( place == 0 ) {
    return;
  }
  gl_delete( g_module_list, place, 0 );
  return;
}


/** Search for `m' in the global module gl_list `g_module_list'
 *  and return it.  Return the matching module or NULL if no matching
 *  module exists or if the g_module_list is empty.
 *
 *  This function searches backwards through g_module_list and uses
 *  CmpModulesNameVers() to determine which module return.
 */
static
struct module_t *SearchForModule(CONST struct module_t *m)
{
  unsigned long place;

  assert(m != NULL);

  if( g_module_list == NULL ) {
    return NULL;
  }
  
  place = gl_search_reverse( g_module_list, m, (CmpFunc)CmpModulesNameVers );
  if( place == 0 ) {
    return NULL;
  }
  return gl_fetch( g_module_list, place );
}


/** Compare the base_names and version numbers of the modules
 *  `m1' and `m2'.
 *  Return:
 *      >0  m1->base_name > m2->base_name
 *          || (m1->base_name == m2->base_name
 *              && m1->version > m2->version)
 *      <0  m1->base_name < m2->base_name
 *       0  m1->base_name == m2->base_name
 *          && m1->version <= m2->version
 */
static
int CmpModulesNameVers(CONST struct module_t *m1, CONST struct module_t *m2)
{
  int result;

  if( (result = CmpSymchar(m1->base_name, m2->base_name)) != 0 ) {
    return result;
  }
  if( m1->version > m2->version ) {
    return 1;
  }
  return 0;
}



extern int Asc_CloseCurrentModule(void){
  struct module_t *prev;

  /*
   *  If no current module, return TRUE
   */
  if (g_current_module == NULL) {
    return TRUE;
  }

  /*
   *  Store the scanner's position in the current module
   */
  g_current_module->linenum = LineNum();

  /*
   * Close the current module's file or buffer.
   */
  if (g_current_module->f != NULL) {
    fclose(g_current_module->f);
    g_current_module->f = NULL;
  } else {
    assert(g_current_module->s != NULL);
    Asc_ScannerReleaseStringBuffer(g_current_module->scanbuffer);
    g_current_module->scanbuffer = NULL;
  }

  /*
   *  Pop the module that REQUIRED the current module so
   *  it becomes the current module
   */
  prev = g_current_module->required_by;
  g_current_module->required_by = NULL; /* unlink it */
  g_current_module = prev;

  /*
   *  No more modules
   */
  if (g_current_module == NULL) {
    return TRUE;
  }

  /*
   *  Inform the scanner after pop
   */
  if (g_current_module->s == NULL) {
    Asc_ScannerAssignFile(g_current_module->f,g_current_module->linenum);
  } else {
    assert(g_current_module->scanbuffer != NULL);
    Asc_ScannerAssignString(g_current_module->scanbuffer,
                            g_current_module->linenum,0);
  }
  return FALSE;
}


int Asc_ModuleAddStatements(struct module_t *m, struct gl_list_t *l)
{
  if (l == NULL || gl_length(l) == 0 ||
      m == NULL || m->s == NULL || m->f != NULL) {
    return -1; 
    /* list is empty or module is not a string module.
     * caller should dump l and contents. 
     */
  }
  if (m->stats == NULL) {
    m->stats = l;
    /* we're keeping list and contents. */
    return 0;
  } else {
    gl_append_list(m->stats,l);
    /* we're keeping contents but not list. caller should destroy l. */
    return 1;
  }
}


extern CONST struct module_t *Asc_GetModuleByName(CONST char *module_name){
  char name[PATH_MAX];
  unsigned long vers;
  struct module_t *mod;
  struct module_t *result;

  if( g_module_list == NULL ) {
    return NULL;
  }

  /*
   *  Convert the given name to an base name and version
   */
  vers = ModuleNameToInternalNameVers(module_name, name);
  if( vers == ULONG_MAX ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR
		,"Bad format for module name %s, no version number found."
		,name
	);
    return NULL;
  }

  /*
   *  Create a new module with this name and version---needed to
   *  call SearchForModule()
   */
  mod = NewModule( name );
  if( mod == NULL ) {
    return NULL;
  }
  mod->version = vers;

  /*
   *  Search for the module and free the module we used for searching.
   */
  result = SearchForModule( mod );
  DeleteModule( mod );

  /*
   *  If result is a module-alias, return the module that PROVIDED it
   */
  if(( result != NULL ) && ( result->provided_by != NULL )) {
    return result->provided_by;
  }
  return result;
}


/**
 *  Parse the module name given in `module_name' (e.g., "foo.a4c<0>")
 *  into an base name and a version number.  Copy the base_name
 *  into the string `name' and use the version number as the return
 *  value.  Leading and trailing whitespace on `module_name' is removed.
 *
 *  If `module_name' does not contain a valid module name, return
 *  ULONG_MAX.
 */
static
unsigned long ModuleNameToInternalNameVers(CONST char *module_name,
                                           char * CONST name)
{
  unsigned int t;
  unsigned long vers = ULONG_MAX;

  assert( name != NULL );

  /*
   *  Make sure we got good data
   */
  if(( module_name == NULL ) || ( *module_name == '\0' )) {
    return vers;
  }

  /*
   *  Ignore leading whitespace
   */
  while( isspace(*module_name) ) {
    module_name++;
  }

  /*
   *  Copy all of module_name into name
   */
  for( t = 0 ; ((name[t] = module_name[t]) != '\0') ; t++ );

  /*
   *  Remove trailing whitespace
   *      Use t>1 in the while since we decrement t twice before we
   *      check it again and we don't want to fall off the front of
   *      the string.
   */
  while(( t > 1 ) && ( isspace(name[--t]) ));

  /*
   *  Check for '>' preceded by a number
   */
  if(( name[t] != '>' ) || ( ! isdigit(name[--t]) )) {
    return vers;
  }

  /*
   *  Back up over all the digits that make up the version number
   *      Use t>1 in while since a valid base name has to have
   *      at least one character.
   */
  while(( t > 1 ) && ( isdigit(name[--t]) ));

  /*
   *  Make sure we have a '<'
   */
  if( name[t] != '<' ) {
    return vers;
  }

  /*
   *  Convert the '<' to '\0' since this is where the name ends
   */
  name[t] = '\0';

  /*
   *  Covert the number to an unsigned long in base 10
   */
  vers = strtoul( (name + t + 1), NULL, 10 );
  if( vers == ULONG_MAX ) {
    return vers;
  }

  return vers;
}

CONST char *Asc_ModuleString(CONST struct module_t *m)
{
  if (m==NULL) return NULL;
  return m->s;
}

struct gl_list_t *Asc_ModuleStatementLists(CONST struct module_t *m)
{
  if (m==NULL) return NULL;
  return m->stats;
}



extern struct gl_list_t *Asc_ModuleList(int module_type){
  struct gl_list_t *new = NULL;
  struct module_t *m;
  struct gl_list_t *types = NULL;
  unsigned long c;
  unsigned long length;

  if( g_module_list == NULL ) {
    return NULL;
  }
  length = gl_length(g_module_list);
  new = gl_create(length);
  if( new == NULL ) {
    return NULL;
  }
  for( c = length; c > 0; c-- ) {
    m = (struct module_t *)gl_fetch(g_module_list, c);
    switch (module_type) {
    case 0:
      types = Asc_TypeByModule(m);
      if( types != NULL ) {
        if( gl_length(types) > 0L ) {
          gl_append_ptr(new, (VOIDPTR)(m->name));
        }
        gl_destroy(types);
        types = NULL;
      }
      break;
    case 1:
      if (m != NULL && m->s != NULL) {
        gl_append_ptr(new, (VOIDPTR)(m->name));
      }
      break;
    case 2:
      if (m != NULL && m->stats != NULL) {
        gl_append_ptr(new, (VOIDPTR)(m->name));
      }
      break;
    default:
      break;
    }
  }
  return new;
}


extern void Asc_ModuleWrite(FILE *f, CONST struct module_t *m){
  assert(m!=NULL);
  FPRINTF(f,"MODULE: %s\nFILENAME: %s\n",SCP(m->name),SCP(m->filename));
  FPRINTF(f,(m->f!=NULL)?"OPEN\n":"CLOSED\n");
  FPRINTF(f,"FILE DATE: %s",asctime(localtime( &(m->time_last_modified) )));
}


extern struct module_t *Asc_CurrentModuleF(void){
  return g_current_module;
}


extern CONST char *Asc_ModuleName(CONST struct module_t *m){
  return (( m != NULL ) ? SCP(m->name) : "");
}


extern CONST char *Asc_ModuleFileName(CONST struct module_t *m){
  return ((m != NULL) ? SCP(m->filename) : "");
}

extern CONST char *Asc_ModuleBestName(CONST struct module_t *m){
  static char unk[] = "<UNKNOWN>";
  if (m == NULL) return "";
  if (SCP(m->filename) != NULL) return SCP(m->filename);
  if (SCP(m->name)!=NULL) return SCP(m->name);
  return unk;
}


extern unsigned long Asc_ModuleTimesOpened(CONST struct module_t *m){
  return ((m != NULL) ? (m->open_count) : 0);
}


extern struct tm *Asc_ModuleTimeModified(CONST struct module_t *m){
  return ((m != NULL) ? localtime(&(m->time_last_modified)) : NULL);
}

extern int Asc_ModuleStringIndex(CONST struct module_t *m){
  return ((m != NULL) ? (int)(m->time_last_modified) : -1);
}
