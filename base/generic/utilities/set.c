/*
 *  Set module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: set.c,v $
 *  Date last modified: $Date: 1997/07/18 12:04:29 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  COPYING.  COPYING is in ../compiler.
 */

#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/set.h"


void set_change_member(unsigned *set, int k, boolean value)
{
  int ndx = set_ndx(k);
  unsigned int mask = set_mask(k);

  asc_assert(NULL != set);
  if(value) {
    set[ndx] |= mask;
  } else {
    set[ndx] &= ~mask;
  }
}

extern unsigned *set_null(unsigned *set,int n)
{
  int sz = set_size(n);   /* Size in words */

  asc_assert(NULL != set);
  while( sz-- > 0 )
    set[sz] = 0;
  return set;
}

#ifdef THIS_IS_DEAD_CODE
#include "utilities/mask.h"


#define	last_bits_used(n)	       (((n)-1)%WORDSIZE + 1)
/**
 ***  Returns number of bits used in last unsigned array element
 **/

#define	end_mask(n)	       mask_I_L(last_bits_used(n))
/**
 ***  Returns 1's in the used bits of the last unsigned array element
 **/

static int minbit(unsigned n)
/**
 ***  Returns the location of the least significant bit of n which is set
 ***  to 1.  It is assumed that n is not 0.
 **/
{
  int cnt;
  for( cnt = 0; !(n&1); cnt++ )
    n >>= 1;
  return( cnt );
}

static int bitcount(unsigned n)
/**
 ***  Returns the number of bits in n which are 1.
 **/
{
  int cnt;
  for( cnt = 0; n!=0; n >>= 1 )
    cnt += (n&1);
  return(cnt);
}

unsigned *set_copy(unsigned *set,unsigned *target,int n,int n2)
{
  int sz = set_size(n);
  int sz2 = set_size(n2);
  int min_sz = MIN(sz,sz2);
  unsigned m2 = end_mask(n2);
  int i;

  for( i = 0; i < min_sz; i++ )
    target[i] = set[i];

  for( /* i=min_sz, like it does already */; i < sz2; ++i );
    target[i] = 0;

  target[sz2-1] &= m2;
  return target;
}

void set_change_member_rng(unsigned *set, int k1, int k2, boolean value)
{
  int ndx1,ndx2,mask1,mask2,i;

  if(k2 < k1) {
    return;
  }

  ndx1=set_ndx(k1);
  ndx2=set_ndx(k2);
  mask1=mask_I_GE(k1%WORDSIZE);
  mask2=mask_I_LE(k2%WORDSIZE);

  if(ndx1==ndx2) {
    mask1 = mask2 &= mask1;
  }

  if(value) {
    set[ndx1] |= mask1;
    for(i=ndx1+1 ; i<ndx2 ; ++i) {
      set[i]=~(unsigned)0;
    }
    set[ndx2] |= mask2;
  } else {
    set[ndx1] &= ~mask1;
    for(i=ndx1+1 ; i<ndx2 ; ++i) {
      set[i]=0;
    }
    set[ndx2] &= ~mask2;
  }
}

int set_find_next(unsigned *set,int k,int n)
{
  int ndx,mask;
  int sz = set_size(n);

  ++k;
  mask=mask_I_GE(k%WORDSIZE);
  for(ndx=set_ndx(k) ; (ndx<sz) && !(set[ndx]&mask) ; ++ndx)
    mask=~0;

  return(ndx*WORDSIZE + ( (ndx<sz) ? minbit(set[ndx]&mask) : 0 ));
}

int set_count(unsigned *set,int n)
{
  int sz = set_size(n);
  int cnt=0;

  while( --sz >= 0 )
    cnt += bitcount(set[sz]);
  return(cnt);
}

unsigned *set_complement(unsigned *set,int n)
{
  int	sz = set_size(n);
  unsigned m = end_mask(n);
  int i;

  for(i=0 ; i<sz ; i++)
    set[i] = ~set[i];
  set[sz-1] &= m;
  return set;
}

unsigned *set_intersect(unsigned *set,unsigned *set2,int n,int n2)
{
  int sz = set_size(n);
  int sz2 = set_size(n2);
  int min_sz = MIN(sz,sz2);
  int i;

  for(i=0 ; i<min_sz ; i++)
    set[i] &= set2[i];

  for(/* i=min_sz, like it does already */ ; i<sz2 ; ++i);
    set[i]=0;
  return set;
}

unsigned *set_union(unsigned *set,unsigned *set2,int n,int n2)
{
  int sz = set_size(n);
  int sz2 = set_size(n2);
  int min_sz = MIN(sz,sz2);
  unsigned m = end_mask(n);
  int i;

  for(i=0 ; i<min_sz ; i++)
    set[i] |= set2[i];

  set[sz-1] &= m;
  return set;
}
#endif /* THIS_IS_DEAD_CODE */
