/**< 
 *  Ascend Instance Atom Size Functions.
 *  by Tom Epperly & Ben Allan
 *  8/16/89
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: atomsize.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:03 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *  based on instance.c
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
#ifndef __ATOMSIZE_H_SEEN__
#define __ATOMSIZE_H_SEEN__
/**< requires
 *# #include"instance_enum.h"
 *# #include"childinfo.h"
 */

/**< 
 *  Byte length calculation routines for atoms/relations.
 *  Routines provided to fill the byte length field of the type description.
 */

extern unsigned long ConstantByteLength(enum inst_t);
/**< 
 *  unsigned long ConstantByteLength(it)
 *  enum inst_t it;
 *  Returns the number of bytes needed for the Constant instance of type it.
 */

extern unsigned long RealAtomByteLength(unsigned long,
     CONST struct ChildDesc *);
/**< 
 *  unsigned long RealAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long IntegerAtomByteLength(unsigned long,
        CONST struct ChildDesc *);
/**< 
 *  unsigned long IntegerAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long BooleanAtomByteLength(unsigned long,
        CONST struct ChildDesc *);
/**< 
 *  unsigned long BooleanAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long SetAtomByteLength(unsigned long,
           CONST struct ChildDesc *);
/**< 
 *  unsigned long SetAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long SymbolAtomByteLength(unsigned long,
       CONST struct ChildDesc *);
/**< 
 *  unsigned long SymbolAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long RelationAtomByteLength(unsigned long,
         CONST struct ChildDesc *);
/**< 
 *  unsigned long RelationAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

extern unsigned long LogRelAtomByteLength(unsigned long,
         CONST struct ChildDesc *);
/**< 
 *  unsigned long LogRelAtomByteLength(num_children,c)
 *  unsigned long num_children;
 *  const struct ChildDesc *c;
 *  Given the number of children and descriptions of the children, this
 *  routine will calculate the number of bytes needed for the instance.
 */

#endif
/**< __ATOMSIZE_H_SEEN__ */
