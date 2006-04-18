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
	Ascend Module Control.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler.h"
*//*
	by Tom Epperly
	Created: 1/10/90
	Version: $Revision: 1.11 $
	Version control file: $RCSfile: module.h,v $
	Date last modified: $Date: 1998/04/16 00:43:26 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_MODULE_H
#define ASC_MODULE_H

#include <utilities/ascConfig.h>

#define PATHENVIRONMENTVAR "ASCENDLIBRARY"
/**<
 *  The name of the environment variable containing a list' of paths
 *  to search when trying to open a module.  The list is in the style
 *  of the native OS: colon-separated on UNIX and semicolon separated
 *  under Windows.
 */

#define  MOD_FILE_EXTS   6
#define  MOD_OLD_CODE    ".asc"
/**< Extension for old code files. */
#define  MOD_OLD_LIBRARY ".lib"
/**< Extension for old library files. */
#define  MOD_NEW_CODE    ".a4c"
/**< Extension for new code files. */
#define  MOD_NEW_LIBRARY ".a4l"
/**< Extension for new library files. */
#define  MOD_NEW_UNITS   ".a4u"
/**< Extension for new units files. */
#define  MOD_CATCHALL    ""
/**< Extension for general files. */
extern CONST char *g_alt_ending[MOD_FILE_EXTS];
/**<
 *  This array defines the expected file extensions for
 *  parsable ascend MODEL/ATOM/unit code. Files are not
 *  limited to these, however. Interfaces that see a
 *  trailing .* where * is other than these should
 *  enquire as to the user intent: the user may have
 *  made a mistake.
 */

struct module_t;

extern int Asc_InitModules(unsigned long init_length);
/**<
 *  Create the data structures required by the modules code.  Returns
 *  nonzero if not enough memory to initialize the modules; zero for
 *  success.  Clients to not need to call this function since calls to
 *  OpenModule() or RequireModule() will initialize the data structures
 *  if needed.<br><br>
 *
 *  The initial length of the module list will be set to `init_length';
 *  since the module list will grow as needed, this number isn't too
 *  important, but a better guess will mean better performance.
 */


extern ASC_DLLSPEC(void) Asc_DestroyModules(DestroyFunc func);
/**<
 *  Deallocate all of the modules.  This should be done when all types
 *  are destroyed and just before exiting.
 *  The function argument is not optional.
 */

extern struct module_t *Asc_OpenStringModule(CONST char *inputstring,
                                             int *status,
                                             CONST char *nameprefix);
/**<
 *  This function returns a module which behaves just as a regular
 *  file module, except that it is always there unless there is
 *  insufficient memory to make the required internal copy of the
 *  string. The scanner will use the non-input-modifying version
 *  of its functions when operating on the copy of the input string.
 *  The string module will make up its own unique name based on nameprefix.
 *  Not surprisingly, the module returned should be closed after a call
 *  to zz_lex.
 */

extern struct module_t *Asc_OpenModule(CONST char *name, int *status);
/**<
 *  Attempt to find and open (for reading) a file whose
 *  name is based on `name'.
 *  @see Asc_RequireModule() for additional information.
 */
extern ASC_DLLSPEC(struct module_t*) Asc_RequireModule(CONST char *name, int *status);
/**<
 *  Attempt to find and open (for reading) a file whose
 *  name is based on `name'.
 *
 *  The following applies to both Asc_OpenModule() and Asc_RequireModule().
 *  `name' can either be a full path name to the file (in the native format)
 *  or a string that will be appended to each of the paths listed in the
 *  PATHENVIRONMENTVAR to try and find the file.<br><br>
 *
 *  Past versions (pre July-97) of these functions used to append known
 *  extensions to `name' to try and find the file.  That behavior is no
 *  longer supported.<br><br>
 *
 *  The integer that `status' points to will be set to the return status
 *  of the function.  If you are not concerned about the return status,
 *  pass in NULL as the second argument.<br><br>
 *
 *  The portion of `name' after the rightmost slash (`/' on UNIX, `\' on
 *  Windows) is used as the "base-name" part of the module's
 *  name---i.e., all path information is removed.  The second part of
 *  the module's name is a version number enclosed in angle brackets.
 *  Version numbers start at zero and increase.  For example, the first
 *  time OpenModule("/foo/bar/baz.a4c") is called, a module with the
 *  name "baz.a4c<0>" is created.  Module names are unique.<br><br>
 *
 *  ASC_REQUIREMODULE ONLY:<br>
 *      If the base-name of the module name you specify matches an
 *      existing module's base-name or a module's alias, `status' is set
 *      to 5 and the existing module is returned.  For example, if you
 *      call Asc_OpenModule("/foo/bar/baz.a4c") successfully, and then
 *      Asc_RequireModule("/food/beer/baz.a4c"), the existing module
 *      "baz.a4c<0>" is returned.  The pathnames of the files are not
 *      compared, and no searching of the file system occurs.  See
 *      Asc_ModuleCreateAlias() for details on aliases.<br><br>
 *
 *  If attempting to access `name' directly fails, and searching over
 *  PATHENVIRONMENTVAR produces no valid filenames, `status' is set to
 *  -1 and NULL is returned.  If a matching filename is found but the
 *  fopen() call fails, `status' is set to -2 and NULL is returned.
 *  Note that fopen() is only attempted one time; once we have a name
 *  that looks like a valid directory entry (a stat() call returns 0),
 *  we stop searching.<br><br>
 *
 *  If a filename is found and the file is successfully opened for
 *  reading, the base-name of the module is compared against existing
 *  modules.  If no match is found, a new module is created: its version
 *  number is set to zero, its times-opened counter is set to 1, the
 *  module we are currently scanning (if any---this only occurs when the
 *  user calls RequireModule) is pushed onto a stack, the scanner is
 *  told to scan the newly opened file, `status' is set to 0, and the
 *  new module is returned.<br><br>
 *
 *  If the base-name of the filename we found matches a module alias, we
 *  treat the filename as a new module and the actions listed in the
 *  above paragraph occur, except that `status' is set to 3.  See the
 *  documentation under Asc_ModuleCreateAlias() for details on aliases.<br><br>
 *
 *  If the filename we found and an existing module have the same
 *  base-name, the pathnames and timestamps of the files are compared.
 *  If they match and the existing module is open for reading, the file
 *  is REQUIREing itself; we print an error, set `status' to 4, and
 *  return the existing module.  If they match and the existing module
 *  is not open for reading, the times-opened counter on the existing
 *  module is incremented, the scanner is told to scan the newly
 *  re-opened file, `status' is set to 2, and the existing module is
 *  returned.  If either the filenames or the timestamps of the files do
 *  not match, a new module is created.  The new module has the same
 *  base-name as the existing module, but its version number is one
 *  greater.  The times-opened flag on the new module is set to 1, the
 *  scanner is told to scan the newly opened file, `status' is set to 1,
 *  and the new module is returned.  Note that a string comparison is
 *  used to compare filenames, so "./foo" and "././foo" are seen as two
 *  separate files.<br><br>
 *
 *  If the argument `name' is NULL or has zero length, `status' is set
 *  to -4 and NULL is returned.<br><br>
 *
 *  If any attempts to allocate memory fail, `status' is set to -3 and
 *  NULL is returned.<br><br>
 *
 *  Summary of values put into `status':
 *     - -4  bad input: NULL or zero length name
 *     - -3  a memory error occurred: not enough memory to create module
 *     - -2  a filename was found but it could not be opened for reading
 *     - -1  a filename could not find a file for `name'
 *     -  0  a new module was successfully created
 *     -  1  a new version of an existing module was created
 *     -  2  an existing module is being returned
 *     -  3  a new module was created, overwriting a module's alias
 *     -  4  an attempt to do a recursive require was caught
 *     -  5  the module you are REQUIREing already exists
 */


extern int Asc_ModuleCreateAlias(CONST struct module_t *m, CONST char *name);
/**<
 *  This function takes the string given in `name' and converts it
 *  to a module "base-name" by removing all path information---i.e.,
 *  only characters after the rightmost slash (`/' on UNIX, '\' on
 *  Windows) are used in the base-name.  This base-name---along with
 *  the version number zero---acts as an alias for the module `m'.<br><br>
 *
 *  The number of aliases a module may have is only limited by the
 *  computer's memory.<br><br>
 *
 *  Clients can use the alias to retrieve the module `m'.  For example,
 *  if the following sequence occurs:
 *  <pre>
 *      m = Asc_OpenModule("foo.a4c",NULL);
 *      Asc_ModuleCreateAlias(m, "bar.a4c");
 *  then
 *      m == Asc_GetModuleByName("bar.a4c<0>")
 *        == Asc_GetModuleByName("foo.a4c<0>")
 *  </pre>
 *
 *  The typical reason to create a module-alias is to satisfy subsequent
 *  calls to Asc_RequireModule(); recall that Asc_RequireModule() checks
 *  both existing modules and module-aliases for a matching base-name
 *  before it opens the module itself.  As an example, consider a case
 *  where two modules both provide basic needs (system.lib and
 *  ivpsystem.lib), but one (ivpsystem.lib) defines things differently
 *  for use by higher level modules.  An intermediate module (e.g.,
 *  atoms.lib) doesn't care which version it reads, but it knows what it
 *  needs can come from system.lib, so atoms.lib will REQUIRE system.lib
 *  (the REQUIRE keyword calls Asc_RequireModule()).  However, since
 *  ivpsystem.lib can act as a substitute for system.lib, it should
 *  PROVIDE system.lib (the PROVIDE keyword calls
 *  Asc_ModuleCreateAlias()) to say that ivpsystem.lib satisfies what
 *  system.lib typically provides, in effect, to say that system.lib has
 *  been provided.<br><br>
 *
 *  The base-name of the module-alias that is created must not conflict
 *  with the base-names of any existing modules (it may conflict with
 *  existing module-aliases).  If the alias's base-name conflicts with a
 *  module's base-name, an error message is printed and -2 is returned.
 *  The one exception to this rule is if an alias's base-name matches
 *  the current-module's (as returned by Asc_CurrentModule) base-name.
 *  This can occur if the file for module "foo" contains the statement
 *  ``PROVIDE "foo"''.  In this case, a module-alias is not created and
 *  1 is returned.<br><br>
 *
 *  If the alias's base-name does not conflict with any other alias's
 *  base-name, a new module-alias is created and 0 is returned.<br><br>
 *
 *  Attempts to create an alias with the same base-name to the same
 *  module are ignored, and 2 is returned.<br><br>
 *
 *  If a module-alias with the same base-name exists, the filenames of
 *  `m' and the aliased module are compared.  If they do not match, a
 *  warning is printed; if they match, we are rereading a file and no
 *  warning is printed.  In either case, the old module-alias is
 *  destroyed, a new module-alias is created, and 3 is returned.<br><br>
 *
 *  If `name' is NULL or has zero length, -4 is returned.  `m' not be a
 *  module-alias; if it is a module-alias, an error is printed and -4 is
 *  returned.<br><br>
 *
 *  If any attempts to allocate memory fail, -3 is returned.
 */

extern int Asc_CloseCurrentModule(void);
/**<
 *  This function will close the current module, and restore the module
 *  (tell the scanner to scan the module) that REQUIREd it.<br><br>
 *
 *  If no module REQUIREd the current module (so that this module is the
 *  last open module), return TRUE; otherwise, return FALSE.
 */

extern int Asc_ModuleAddStatements(struct module_t *mod, struct gl_list_t *stats);
/**<
 * Add statements to a Module.
 * Returns 0 or 1 if successful, or -1 if not. The usual cause for
 * failure is that the module is a file module and not a string
 * module. File modules cannot have loose GLOBAL statements in them.
 *  <pre>
 * Return:  Caller should do:
 *  -1      Destroy stats and the statementlists in it.
 *   0      Forget stats-- we now track it.
 *   1      Destroy stats, but not the statementlists in it,
 *          as we are now tracking the statementslists.
 *  </pre>
 */

extern ASC_DLLSPEC(CONST struct module_t*) Asc_GetModuleByName(CONST char *name);
/**<
 *  Return the module whose name is `name.'  `name' should be in the
 *  style returned by Asc_ModuleName(), namely, a base-name and a
 *  version number: "foo.a4c<0>".<br><br>
 *
 *  If `name' refers to a module-alias, the module it is an alias for is
 *  returned.<br><br>
 *
 *  If `name' is not properly formated, print an error and return NULL.
 *  If a module with the requested name is not found, ruturn NULL.
 */

extern ASC_DLLSPEC(struct gl_list_t*) Asc_ModuleList(int module_type);
/**<
 *  <pre>
 *  module_type 0;
 *  Returns a gl_list containing the name of each module that currently
 *  has at least one type definitions based on the module's contents.
 *
 *  module_type 1;
 *  Returns a gl_list containing the name of each module that currently
 *  has a string definition.
 *
 *  module_type 2;
 *  Returns a gl_list containing the name of each module that currently
 *  has statements.
 *  </pre>
 *  This function returns NULL if no modules are currently loaded or if
 *  there is not enough memory to create a new gl_list.<br><br>
 *
 *  The caller is resposible for calling gl_destroy() when finished
 *  using the list.  The names contained in the gl_list are the property
 *  of module.c and should not be touched.  Since the module names are
 *  strored in the symbol table, this list will survive calls to
 *  Asc_DestroyModules().
 */

extern void Asc_ModuleWrite(FILE *f, CONST struct module_t *m);
/**<
 *  Write the name, filename, open/closed-state, and time-last-modified
 *  of module `m' to the file pointer `f'.  */

extern struct module_t *Asc_CurrentModuleF(void);
/**<
 *  Implementation function for debug version of Asc_CurrentModule().
 *  Do not call this function directly - use Asc_CurrentModule() instead.
 */
#ifndef NDEBUG
#define Asc_CurrentModule() Asc_CurrentModuleF()
/**<
 *  Returns a pointer to the current module (debug mode).
 *  @see Asc_CurrentModuleF()
 */
#else
extern struct module_t *g_current_module;
/**<
 *  Pointer to the current module.
 *  Do not access directly.  Use Asc_CurrentModule().
 */
#define Asc_CurrentModule() g_current_module
/**<  Returns a pointer to the current module (release mode). */
#endif /* NDEBUG */

extern ASC_DLLSPEC(CONST char*) Asc_ModuleName(CONST struct module_t *m);
/**<
 *  Return the name of module m.
 */

extern struct gl_list_t *Asc_ModuleStatementLists(CONST struct module_t *m);
/**<
 * Returns a gl_list_t *, or NULL if none, containing pointers to
 * struct StatementList *. The batches of statements are in order
 * parsed. File modules will never have statements.
 * One may, (though we don't recommend it) combine all the statements
 * into one statementlist, resetting the content of the gl_list,
 * to be just one pointer to the combined statement list if and only if:
 * -- the order of the statements is not changed.
 * -- the gl_list_t pointer returned by this function ends up containing
 *    all the statements. module.c owns the gl_list.
 */

extern CONST char *Asc_ModuleString(CONST struct module_t *m);
/**<
 *  Return the string of module m, if it is an interactive
 *  string module, or NULL if it is a file module.
 */

extern ASC_DLLSPEC(CONST char*) Asc_ModuleFileName(CONST struct module_t *m);
/**<
 *  Return the filename of module m.
 */

extern CONST char *Asc_ModuleBestName(CONST struct module_t *m);
/**<
 *  Return the filename of module m, if it is a file, or the
 *  buffer name if it is a string.
 *  The string comes from a symbol table entry so, a) don't
 *  mess with it and b) can be safely stored elsewhere.
 */


extern unsigned long Asc_ModuleTimesOpened(CONST struct module_t *m);
/**<
 *  Return the number of times that this module has been opened.  This is
 *  necessary to determine when you are reloading a module.
 */


extern ASC_DLLSPEC(struct tm*) Asc_ModuleTimeModified(CONST struct module_t *m);
/**<
 *  Return the time that the module was last modified.  The time is
 *  coverted to the local time.
 */

extern int Asc_ModuleStringIndex(CONST struct module_t *m);
/**<
 *  Return the string index (the place in the sequence of parsing
 *  strings.
 */

#define Asc_ModulesEqual(m1,m2) ((m1)==(m2))
/**<
 *  This macro will evaluate to TRUE if module `m1' equals module `m2';
 */

#endif  /* ASC_MODULE_H */
