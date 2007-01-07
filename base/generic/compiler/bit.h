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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
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

#ifndef __BIT_H_SEEN__
#define __BIT_H_SEEN__

/** Data structure for holding bit lists. */
struct BitList {
  unsigned long length;
};

extern struct BitList *CreateBList(unsigned long len);
/**<
 *  <!--  struct BitList *CreateBList(len)                             -->
 *  <!--  unsigned long len;                                           -->
 *  Create a bit list with len elements.  The elements are all initialized to 0.
 */

extern struct BitList *CreateFBList(unsigned long len);
/**<
 *  <!--  struct BitList *CreateFBList(len)                            -->
 *  <!--  unsigned long len;                                           -->
 *  Create a bit list with len elements.  The elements are all initialized to 1.
 */

extern struct BitList *ExpandBList(struct BitList *bl, unsigned long len);
/**<
 *  <!--  struct BitList *ExpandBList(bl,len)                          -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long len;                                           -->
 *  The function will expand bl into a longer bitlist.  It copies all the
 *  previous values from the original bit list, and it initializes all
 *  the added entries to 0.  It returns the new bitlist and the old bitlist
 *  is not usable anymore.
 */

extern struct BitList *ExpandFBList(struct BitList *bl, unsigned long len);
/**<
 *  <!--  struct BitList *ExpandBList(bl,len)                          -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long len;                                           -->
 *  The function will expand bl into a longer bitlist.  It copies all the
 *  previous values from the original bit list, and it initializes all
 *  the added entries to 1.  It returns the new bitlist and the old bitlist
 *  is not usable anymore.
 */

extern void DestroyBList(struct BitList *bl);
/**<
 *  <!--  void DestroyBList(bl)                                        -->
 *  <!--  struct BitList *bl;                                          -->
 *  Deallocate the memory for bl.
 */

extern struct BitList *CopyBList(CONST struct BitList *bl);
/**<
 *  <!--  struct BitList *CopyBList(bl)                                -->
 *  <!--  struct BitList *bl;                                          -->
 *  Make a copy of bl and return it.  The length of the copy equals that of bl,
 *  and all the elements of the copy have the same value as the corresponding
 *  elements in bl.
 */

extern void OverwriteBList(CONST struct BitList *src, struct BitList *target);
/**<
 *  <!--  struct BitList *OverwriteBList(src,target)                   -->
 *  <!--  struct BitList *src, *target;                                -->
 *  <!--  src and target must be the same length or this function will -->not return.
 *  Copies the bit information from src into target.  Overwrites anything
 *  that is in target.
 *  src and target must be the same length or this function will not return.
 *  src and target must not be NULL.
 */

extern unsigned long BitListBytes(CONST struct BitList *bl);
/**<
 *  <!--  unsigned long BitListBytes(bl)                               -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long bytes;                                         -->
 *  Returns the number of bytes allocated to the bitlist.
 */

extern void SetBit(struct BitList *bl, unsigned long pos);
/**<
 *  <!--  void SetBit(bl,pos)                                          -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long pos;                                           -->
 *  Set the pos'th bit of bl to 1.
 */

extern void ClearBit(struct BitList *bl, unsigned long pos);
/**<
 *  <!--  void ClearBit(bl,pos)                                        -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long pos;                                           -->
 *  Set the pos'th bit of bl to 0.
 */

extern void CondSetBit(struct BitList *bl, unsigned long pos, int cond);
/**<
 *  <!--  void CondSetBit(bl,pos,cond)                                 -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long pos;                                           -->
 *  <!--  int cond;                                                    -->
 *  If cond is true, set the pos'th bit of bl to 1; otherwise, set it to 0.
 */

ASC_DLLSPEC int ReadBit(CONST struct BitList *bl, unsigned long pos);
/**<
 *  <!--  int ReadBit(bl,pos)                                          -->
 *  <!--  struct BitList *bl;                                          -->
 *  <!--  unsigned long pos;                                           -->
 *  Return a true value if bit pos is 1, otherwise return FALSE.
 */

extern void IntersectBLists(struct BitList *bl1, CONST struct BitList *bl2);
/**<
 *  <!--  void IntersectBList(bl1,bl2)                                 -->
 *  <!--  struct BitList *bl1;                                         -->
 *  <!--  const struct BitList *bl2;                                   -->
 *  This routine returns the intersection of bl1 and bl2 which is stored in
 *  bl1.  bl2 is unchanged, unless of course bl1 and bl2 are the same.
 */

extern void UnionBLists(struct BitList *bl1, CONST struct BitList *bl2);
/**<
 *  <!--  void UnionBLists(bl1,bl2)                                    -->
 *  <!--  struct BitList *bl1;                                         -->
 *  <!--  const struct BitList *bl2;                                   -->
 *  This routine returns the union of bl1 and bl2 which is stored in bl1.
 *  bl2 is unchanged.
 */

#ifdef NDEBUG
/** Retrieve the bit list length. */
#define BLength(bl) ((bl)->length)
#else
/** Retrieve the bit list length. */
#define BLength(bl) BLengthF(bl)
#endif
extern unsigned long BLengthF(CONST struct BitList *bl);
/**< 
 *  <!--  macro BLength(bl)                                            -->
 *  <!--  unsigned long BLengthF(bl)                                   -->
 *  <!--  const struct BitList *bl;                                    -->
 *  Return the length of bl.
 */

ASC_DLLSPEC int BitListEmpty(CONST struct BitList *bl);
/**<
 *  <!--  int BitListEmpty(bl)                                         -->
 *  <!--  const struct BitList *bl;                                    -->
 *  Return a true value if bl is empty; otherwise, it returns a false value.
 */

extern int CompBList(struct BitList *b1, struct BitList *b2);
/**<
 *  <!--  int CompBList(bl,b2)                                         -->
 *  <!--  const struct BitList *bl;                                    -->
 *  <!--  const struct BitList *b2;                                    -->
 *  Return 1 if bl is equal to b2, return 0 otherwise
 */

extern unsigned long FirstNonZeroBit(CONST struct BitList *bl);
/**<
 *  <!--  unsigned long FirstNonZeroBit(bl)                            -->
 *  <!--  const struct BitList *bl;                                    -->
 *  Return the first non-zero bit.  If it is unable to find a non-zero
 *  it will return a number greater than BLength(bl).
 */

#endif  /* __BIT_H_SEEN__ */

