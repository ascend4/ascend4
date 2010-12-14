/* 
 *  Instance name routines
 *  by Tom Epperly
 *  Part Of Ascend
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: instance_name.h,v $
 *  Date last modified: $Date: 1998/02/05 16:36:20 $
 *  Last modified by: $Author: ballan $
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
 *  Instance name routines.
 *  <pre>
 *  When #including instance_name.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_INSTANCE_NAME_H
#define ASC_INSTANCE_NAME_H

#include "compiler.h"

/**	@addtogroup compiler_inst Compiler Instance Hierarchy
	@{
*/

enum NameTypes {
  IntArrayIndex,  /**< integer array index */
  StrArrayIndex,  /**< string array index */
  StrName         /**< string name */
};

union InstanceNameUnion {
  long index;
  symchar *name;
};

struct InstanceName {
  enum NameTypes t;
  union InstanceNameUnion u;
};

#define InstanceNameType(in) ((in).t)
/**< 
 *  Return the type of InstanceName structure in.
 */

#define InstanceNameStr(in) ((in).u.name)
/**< 
 *  Return the name of InstanceName structure in.
 *  in must be of type StrName.
 */

#define InstanceIntIndex(in) ((in).u.index)
/**< 
 *  Return the integer index value of InstanceName structure in.
 */

#define InstanceStrIndex(in) ((in).u.name)
/**< 
 *  Return the string index value of InstanceName structure in.
 */

#define SetInstanceNameType(in,type) ((in).t) = (type)
/**< 
 *  Set the type of InstanceName structure in to type (a NameTypes).
 */

#define SetInstanceNameStrPtr(in,str) ((in).u.name) = (str)
/**< 
 *  Set the string pointer of InstanceName structure in to str (a symchar*).
 */

#define SetInstanceNameStrIndex(in,str) ((in).u.name) = (str)
/**<
 *  Set the string index of InstanceName structure in to str (a symchar*).
 */

#define SetInstanceNameIntIndex(in,int_index) ((in).u.index) = (int_index)
/**< 
 *  Set the integer index of InstanceName structure in to int_index (a long).
 */

/* @} */

#endif /* ASC_INSTANCE_NAME_H */

