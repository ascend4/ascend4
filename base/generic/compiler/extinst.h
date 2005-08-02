/*
 *  Ascend Instance Tree Type Definitions
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: extinst.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:35 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Ascend Instance Tree Type Definitions.
 *  Special stuff for External Relations.
 *  <pre>
 *  When #including extinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef __EXTINST_H_SEEN__
#define __EXTINST_H_SEEN__

extern struct Instance **g_ExtVariablesTable;
/**<
 *  A global variable which is non NULL if external relations have been
 *  processed and have hence added variables to the table. After use it
 *  should be appropriately reset.
 */

extern struct Instance **AddVarToTable(struct Instance *inst, int *added);
/**<
 *  <!--  struct Instance **AddVarToTable(inst, added);                -->
 *  <!--  struct Instane *inst;                                        -->
 *  <!--  int *added;                                                  -->
 *  Given an instance will store it in the ExtVariablesTable and will return
 *  the 'handle' to the instance. If variable existed already, it will not
 *  be added. If there was a failure then the variable will not be added.
 *  This is reflected in the variable "added".
 */

extern void FixExternalVars(struct Instance *old, struct Instance *new);
/**< 
 *  <!--  FixExternalVars(old,new)                                     -->
 *  Replaces old with new in the table.
 *  This will be called only for MODEL_INSTS.
 */

extern void SetSimulationExtVars(struct Instance *i, struct Instance **extvars);
/**< 
 *  <!--  void SetSimulationExtVars(i,extvars);                        -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  struct Instance **extvars;                                   -->
 *  Will set the given extvar table to the instance. Not for the casual user !!
 *  Could not avoid exporting this one. I would rather have not.
 *  However instantiate needs to be able to set this table when finished.
 */

#endif  /* __EXTINST_H_SEEN__ */

