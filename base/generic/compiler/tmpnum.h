/**< 
 *  Ascend Instance Tree TmpNum functions.
 *  by Tom Epperly & Kirk Abbott
 *  8/16/89
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: tmpnum.h,v $
 *  Date last modified: $Date: 1997/12/20 17:51:52 $
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
#ifndef __TMPNUM_H_SEEN__
#define __TMPNUM_H_SEEN__

/**< 
 *  About tmp numbers. These are a scratch long for use of transient
 *  clients, including the compiler itself. The tmp num should not be
 *  expected to remain the same above the scope of the function in
 *  which it is set. In particular, all clients are free to stomp on
 *  it at will so don't count on it holding a value you set unless you
 *  know that none of your subroutines will mess with it before you
 *  need it.
 *
 * Note that NULL instances have a tmpnum of LONG_MAX, if anyone
 * is dumb enough to ask. This will generally cause fallout 
 * elsewhere.
 */

extern unsigned long GetTmpNum(CONST struct Instance *);
/**< 
 *  l= GetTmpNum(i);
 *  CONST struct Instance *i;
 *  Return the tmp number, or LONG_MAX if i is NULL,
 *  or 0 if i is subatomic, or exits if i enum is wierd.
 */

extern void SetTmpNum(struct Instance *, unsigned long int);
/**< 
 *  SetTmpNum(i,n);
 *  struct Instance *i;
 *  long n;
 *  Sets the tmp number, or whines if i bad.
 */

extern unsigned long IncrementTmpNum(struct Instance *);
/**< 
 *  l= IncrementTmpNum(i);
 *  CONST struct Instance *i;
 *  unsigned long l;
 *  add 1 to and return the tmp number.
 *  Returns LONG_MAX if i is NULL.
 *  or 0 if i is subatomic, or exits if i enum is wierd.
 *  The numeric wisdom of the incrementing is not checked.
 */

extern unsigned long DecrementTmpNum(struct Instance *);
/**< 
 *  l= DecrementTmpNum(i);
 *  CONST struct Instance *i;
 *  unsigned long l;
 *  Subtract 1 from and return the tmp number.
 *  Returns LONG_MAX if i is NULL
 *  or 0 if i is subatomic, or exits if i enum is wierd.
 *  Tmpnum is not decremented if it is already 0.
 */

extern void ZeroTmpNums(struct Instance *,int);
/**< 
 *  ZeroTmpNums(i);
 *  struct Instance *i;
 *  int order;
 *  Does a VisitInstanceTree to set all i->tmp_num = 0;
 *  The visit order really is irrelevant though.
 */

#endif
/**<  __TMPNUM_H_SEEN__  */
