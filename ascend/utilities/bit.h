/*
 *  Bit lists
 *  by Tom Epperly
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: bit.h,v $
 *  Date last modified: $Date: 1997/09/08 18:07:32 $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  The module is used for keeping bit lists (lists of 0/1 values).
 *  Routines are provided for creating lists, set list elements,
 *  changing list elements, and other set-like operations.<br><br>
 *
 *  Bits are numbered 0 .. (BLength(list)-1)
 *  <pre>
 *  When #including bit.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_BIT_H
#define ASC_BIT_H

#include <ascend/general/platform.h>

/**	@addtogroup utilities_bit Utilities Bit-list
	@{
*/

/** Data structure for holding bit lists. */
struct BitList {
  unsigned long length;
};

ASC_DLLSPEC struct BitList *CreateBList(unsigned long len);
/**<
	Create a bit list with len elements.  The elements are all initialized to 0.
*/

ASC_DLLSPEC struct BitList *CreateFBList(unsigned long len);
/**<
	Create a bit list with len elements.  The elements are all initialized to 1.
*/

ASC_DLLSPEC struct BitList *ExpandBList(struct BitList *bl, unsigned long len);
/**<
	Expand bl into a longer bitlist. It copies all the previous values from
	the original bit list, and it initializes all the added entries to 0. 
	@return the new bitlist 
	@note the old bitlist is not usable anymore.
*/

ASC_DLLSPEC struct BitList *ExpandFBList(struct BitList *bl, unsigned long len);
/**<
	The function will expand bl into a longer bitlist.  It copies all the
	previous values from the original bit list, and it initializes all
	the added entries to 1.
	@return the new bitlist (the old bitlist is not usable anymore)
*/

ASC_DLLSPEC void DestroyBList(struct BitList *bl);
/**<
	Deallocate the memory for bl.
*/

ASC_DLLSPEC struct BitList *CopyBList(CONST struct BitList *bl);
/**<
	Make a copy of bl and return it.  The length of the copy equals that of bl,
	and all the elements of the copy have the same value as the corresponding
	elements in bl.
*/

ASC_DLLSPEC void OverwriteBList(CONST struct BitList *src, struct BitList *target);
/**<
	Copies the bit information from src into target.  Overwrites anything
	that is in target.
	src and target must be the same length or this function will not return.
	src and target must not be NULL.
*/

ASC_DLLSPEC unsigned long BitListBytes(CONST struct BitList *bl);
/**<
	Returns the number of bytes allocated to the bitlist (including
	both the data bits as well as the 'struct BitList' overhead).
*/

ASC_DLLSPEC void SetBit(struct BitList *bl, unsigned long pos);
/**<
	Set the pos'th bit of bl to 1. The value of 'pos' must be in the range
	from 0 to bl->length.
*/

ASC_DLLSPEC void ClearBit(struct BitList *bl, unsigned long pos);
/**<
	Set the pos'th bit of bl to 0. The value of 'pos' must be in the range
	from 0 to bl->length.
*/

ASC_DLLSPEC void CondSetBit(struct BitList *bl, unsigned long pos, int cond);
/**<
	If cond is true, set the pos'th bit of bl to 1; otherwise, set it to 0.
*/

ASC_DLLSPEC int ReadBit(CONST struct BitList *bl, unsigned long pos);
/**<
	Return a true value if bit pos is 1, otherwise return FALSE.
	The value of 'pos' must be in the range from 0 to bl->length.
	@note: this 'true value' will not equal one; it will just be non-zero.
*/

ASC_DLLSPEC void IntersectBLists(struct BitList *bl1, CONST struct BitList *bl2);
/**<
	This routine returns the intersection of bl1 and bl2 which is stored in
	bl1.  bl2 is unchanged, unless of course bl1 and bl2 are the same.
*/

ASC_DLLSPEC void UnionBLists(struct BitList *bl1, CONST struct BitList *bl2);
/**<
	This routine returns the union of bl1 and bl2 which is stored in bl1.
	bl2 is unchanged.
*/

#ifdef NDEBUG
/** Retrieve the bit list length. */
#define BLength(bl) ((bl)->length)
#else
/** Retrieve the bit list length. */
#define BLength(bl) BLengthF(bl)
#endif
ASC_DLLSPEC unsigned long BLengthF(CONST struct BitList *bl);
/**<
	Return the length of bl.
*/

ASC_DLLSPEC int BitListEmpty(CONST struct BitList *bl);
/**<
	Return a true value if bl is empty; otherwise, it returns a false value.
*/

ASC_DLLSPEC int CompBList(struct BitList *b1, struct BitList *b2);
/**<
	Return 1 if bl is equal to b2, return 0 otherwise
*/

ASC_DLLSPEC unsigned long FirstNonZeroBit(CONST struct BitList *bl);
/**<
	Return the position of the first non-zero bit (where 0 is the first position)
	If it is unable to find a non-zero it will return a number greater than BLength(bl).
*/

/* @} */

#endif  /* ASC_BIT_H */

