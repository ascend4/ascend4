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

#ifndef __INSTANCE_NAME_H_SEEN__
#define __INSTANCE_NAME_H_SEEN__

enum NameTypes {
  IntArrayIndex,  /**< integer array index */
  StrArrayIndex,  /**< string array index */
  StrName	        /**< string name */
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
 *  <!--  macro InstanceNameType(in)                                   -->
 *  <!--  struct InstanceName in;                                      -->
 *
 *  Return the type of InstanceName structure in.
 */

#define InstanceNameStr(in) ((in).u.name)
/**< 
 *  <!--  macro InstanceNameStr(in)                                    -->
 *  <!--  struct InstanceName in;                                      -->
 *
 *  Return the name of InstanceName structure in.
 *  in must be of type StrName.
 */

#define InstanceIntIndex(in) ((in).u.index)
/**< 
 *  <!--  macro InstanceIntIndex(in)                                   -->
 *  <!--  struct InstanceName in;                                      -->
 *
 *  Return the integer index value of InstanceName structure in.
 */

#define InstanceStrIndex(in) ((in).u.name)
/**< 
 *  <!--  macro InstanceStrIndex(in)                                   -->
 *  <!--  struct InstanceName in;                                      -->
 *
 *  Return the string index value of InstanceName structure in.
 */

#define SetInstanceNameType(in,type) ((in).t) = (type)
/**< 
 *  <!--  macro SetInstanceNameType(in,type)                           -->
 *  <!--  struct InstanceName in;                                      -->
 *  <!--  enum NameTypes type;                                         -->
 *
 *  Set the type of InstanceName structure in to type (a NameTypes).
 */

#define SetInstanceNameStrPtr(in,str) ((in).u.name) = (str)
/**< 
 *  <!--  macro SetInstanceNameStrPtr(in,str)                          -->
 *  <!--  struct InstanceName in;                                      -->
 *  <!--  symchar *str;                                                -->
 *
 *  Set the string pointer of InstanceName structure in to str (a symchar*).
 */

#define SetInstanceNameStrIndex(in,str) ((in).u.name) = (str)
/**<
 *  <!--  macro SetInstanceNameStrIndex(in,str)                        -->
 *  <!--  struct InstanceName in;                                      -->
 *  <!--  symchar *str;                                                -->
 *
 *  Set the string index of InstanceName structure in to str (a symchar*).
 */

#define SetInstanceNameIntIndex(in,int_index) ((in).u.index) = (int_index)
/**< 
 *  <!--  macro SetInstanceNameIndex(in,int_index)                     -->
 *  <!--  struct InstanceName in;                                      -->
 *  <!--  long int_index;                                              -->
 *
 *  Set the integer index of InstanceName structure in to int_index (a long).
 */

#endif /* __INSTANCE_NAME_H_SEEN__ */

