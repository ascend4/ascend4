/*
 *  Qlfdid.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: Qlfdid.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Interface Qualified Id Routines.
 */

#ifndef ASC_QLFDID_H
#define ASC_QLFDID_H

#include "utilities/ascConfig.h"
#include "general/list.h"
#include "instance_enum.h"

#define MAXIMUM_ID_LENGTH 80

struct SearchEntry {
  char *name;            /**< name of the instance */
  struct Instance *i;    /**< pointer to the instance */
};

extern struct Instance *g_search_inst;
/**< Result of the last qlfdid search of any kind in c or tcl. */

extern struct Instance *g_relative_inst;
/**<
 *  Result of the last qlfdid search of any kind in tcl.
 *  (C calls to qlfdid routines do not change this.)
 *  'qlfdid' and 'probe qlfdid' are known to set it.
 */

extern char *Asc_MakeInitString(int len);
/**<
 *  <!--  Asc_MakeInitString(int len);                                 -->
 *  Will make a string of the specified length of a predefine default
 *  length if garbage is given. Will initialize the string to the mt string.
 *  Will return a pointer to the string;
 */

extern void Asc_ReInitString(char *str);
/**<
 *  <!--  Asc_ReInitString;                                            -->
 *  <!--  char *str;                                                   -->
 *  We will reinitialize a string of non-zero length to the mt string.
 *  If an ivalid string will leave it untouched.
 */

extern struct SearchEntry *Asc_SearchEntryCreate(char *name,
                                                 struct Instance *i);
/**<
 *  <!--  Asc_SearchEntryCreate(name,i);                               -->
 *  <!--  char *name;                                                  -->
 *  <!--  struct Instance *i;                                          -->
 *  Accepts a string and a pointer to an Instance and will create a
 *  and return a SearchEntry. Will save a copy of the string. Will always
 *  try to make space for the string.
 */

extern struct Instance *Asc_SearchEntryInstance(struct SearchEntry *se);
/**<
 *  <!--  Asc_SearchEntryInstance(se);                                 -->
 *  <!--  struct SearchEntry *se;                                      -->
 *  Returns a the instance pointer from a search entry;
 */

extern char *Asc_SearchEntryName(struct SearchEntry *se);
/**<
 *  <!--  Asc_SearchEntryName(se)                                      -->
 *  <!--  struct SearchEntry *se;                                      -->
 *  Returns the name of a search entry;
 */

extern void Asc_SearchEntryDestroy(struct SearchEntry *se);
/**<
 *  <!--  Asc_SearchEntryDestroy(se);                                  -->
 *  <!--  struct SearchEntry *se;                                      -->
 *  Will delete the memory associated with its string, set its pointers
 *  to NULL, and free the memory associated with a SearchEntry.
 */

extern void Asc_SearchListDestroy(struct gl_list_t *search_list);
/**<
 *  <!--  Asc_SearchListDestroy(search_list);                          -->
 *  <!--  struct gl_list_t *search_list;                               -->
 *  Frees up the memory that was allocated for the search_list which is
 *  a list of SearchEntry(s).
 */

extern struct gl_list_t *Asc_BrowQlfdidSearch(char *str, char *temp);
/**<
 *  <!--  Asc_BrowQlfdidSearch(str,temp)                               -->
 *  <!--  char *str, char *temp;                                       -->
 *  Will accept two strings, both of which will be destroyed by the
 *  operation. Searches for an instance of the qualified name. Will
 *  return of a list (of SearchEntries), with each list element being
 *  a pointer to a level in the instance tree of the qualified id and
 *  'g_search_inst' looking at the last part of the instance name.
 *  Will return NULL, whenever the qualified instance name cannot be found.
 *  If it returns NULL, the list does not exist; don't try to destroy it.
 */

extern int Asc_QlfdidSearch2(char *str);
/**<
 *  <!--  Asc_QlfdidSearch2(str);                                      -->
 *  <!--  char *str;                                                   -->
 *  This function is perhaps the entry point suited for most users.
 *  It uses the function Asc_BrowQlfdidSearch(), and will leave the g_search_inst
 *  looking at the found instance, or NULL. It attempts to avoid the
 *  overhead of creating search entries, for cases where the user only
 *  wants to find the instance associated with the given qualfied id.
 *  However because it still uses Asc_BrowQlfdidSearch, it incurs the cost
 *  of creating the path to the instance. For cases where this path is
 *  of no interest, but just the final instance use Asc_QlfdidSearch3 below.
 *  Returns 0 if found, 1 otherwise.
 */

extern int Asc_QlfdidSearch3(CONST char *str, int relative);
/**<
 *  <!--  Asc_QlfdidSearch3(str,relative);                             -->
 *  <!--  const char *str;                                             -->
 *  <!--  int relative;                                                -->
 *  This function is still yet another entry point to the search routines.
 *  It uses the function BrowQlfdidSearch3, and will leave the g_search_inst
 *  looking at the found instance, or NULL. It attempts to avoid the
 *  overhead of creating search entries, for cases where the user only
 *  wants to find the instance associated with the given qualfied id.
 *  Returns 0 if found, 1 otherwise. This is perhaps the fastest
 *  version of the search codes.<br><br>
/ *
 *  If relative = 1 will start search at g_relative_inst whose value
 *  should be set by a call to qlfdid (tcl) or Asc_BrowQlfdidSearchCmd (c).
 *  Note that if the relative option is used str should be relative to
 *  g_relative_inst NOT to the simulation root.
 */

#endif  /* ASC_QLFDID_H */

