/**< 
 *  Instance Checking Routines
 *  by Tom Epperly
 *  Created: 5/4/1990
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: check.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:19 $
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

/**< 
 *  When #including check.h, make sure these files are #included first:
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 */


#ifndef __CHECK_H_SEEN__
#define __CHECK_H_SEEN__
/**< requires
 *# #include<stdio.h>
 *# #include"compiler.h"
 *# #include"instance_enum.h"
 */

#define CheckInstance(a,b) CheckInstanceLevel((a),(b),5)
extern void CheckInstanceLevel(FILE *,CONST struct Instance *,int);
/**< 
 *  void CheckInstanceLevel(f,i,pass)
 *  FILE *f;
 *  const struct Instance *i;
 *  int pass;
 *  Perform all the possible consistency checks possible, and check for
 *  as many errors as possible.  This won't modify anything.
 *  The value of pass determines which pending statements will be
 *  printed.
 *  pass == 0: do not print pendings.
 *  pass == 1: IS_As and other constructors only.
 *  pass == 2: relations also
 *  pass == 3: logical relations also
 *  pass == 4: whens also
 *  pass == 5: defaults also
 */

extern void CheckInstanceStructure(FILE *,CONST struct Instance *);
/**< 
 *  void CheckInstanceStructure(f,i)
 *  FILE *f;
 *  const struct Instance *i;
 *  Perform popular consistency checks possible, and check for
 *  as many errors as possible.  This won't modify anything.
 *  This doesn't warn about unassigned real constants, basically.
 */

extern void InstanceTokenStatistics(FILE *, CONST struct Instance *);
/**< 
 *  void InstanceTokenStatistics(f,i)
 *  FILE *f;
 *  const struct Instance *i;
 *  This compiles and prints various token relation statistics for
 *  this instance tree.
 */

extern void InstanceStatistics(FILE *, CONST struct Instance *);
/**< 
 *  void InstanceStatistics(f,i)
 *  FILE *f;
 *  const struct Instance *i;
 *  This compiles and prints various statistics about this instance tree.
 */
#endif /**< __CHECK_H_SEEN__ */
