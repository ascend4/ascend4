/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	Vector math implementation
*/

#include "mtx_vector.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/mem.h>
#include <math.h>

struct vec_vector *vec_create(int32 low, int32 high)
{                                                                
  struct vec_vector *result;

  result = ASC_NEW(struct vec_vector);
  if (NULL == result)
    return NULL;

  result->rng = NULL;
  result->vec = NULL;
  if (0 != vec_init(result, low, high)) {
    ASC_FREE(result);
    result = NULL;
  }
  return result;
}

int vec_init(struct vec_vector *vec, int32 low, int32 high)
{
  int32 new_size;

  if ((low < 0) || (high < low))
    return 1;

  if (NULL == vec)
    return 2;

  if (NULL == vec->rng) {
    vec->rng = ASC_NEW(mtx_range_t);
    if (NULL == vec->rng)
      return 3;
  }
  vec->rng = mtx_range(vec->rng, low, high);

  new_size = high + 1;
  if (NULL == vec->vec) {
    vec->vec = ASC_NEW_ARRAY(real64,new_size);
    if (NULL == vec->vec) {
      ASC_FREE(vec->rng);
      vec->rng = NULL;
      return 3;
    }
  }
  else {
    vec->vec = (real64 *)ascrealloc(vec->vec, (new_size)*sizeof(real64));
  }

  vec->accurate = FALSE;
  return 0;
}

void vec_destroy(struct vec_vector *vec)
{
  if (NULL != vec) {
    if (NULL != vec->rng)
      ASC_FREE(vec->rng);
    if (NULL != vec->vec)
      ASC_FREE(vec->vec);
    ASC_FREE(vec);
  }
}

void vec_zero( struct vec_vector *vec)
{
  real64 *p;
  int32 len;

  asc_assert((NULL != vec) &&
             (NULL != vec->rng) &&
             (NULL != vec->vec) &&
             (vec->rng->low >= 0) &&
             (vec->rng->low <= vec->rng->high));

  p = vec->vec + vec->rng->low;
  len = vec->rng->high - vec->rng->low + 1;
  mtx_zero_real64(p,len);
}

void vec_copy( struct vec_vector *vec1,struct vec_vector *vec2)
{
  real64 *p1,*p2;
  int32 len;

  asc_assert((NULL != vec1) &&
             (NULL != vec1->rng) &&
             (NULL != vec1->vec) &&
             (vec1->rng->low >= 0) &&
             (vec1->rng->low <= vec1->rng->high) &&
             (NULL != vec2) &&
             (NULL != vec2->rng) &&
             (NULL != vec2->vec) &&
             (vec2->rng->low >= 0));

  p1 = vec1->vec + vec1->rng->low;
  p2 = vec2->vec + vec2->rng->low;
  len = vec1->rng->high - vec1->rng->low + 1;
  /*mem_copy_cast not in order here, probably */
  mem_move_cast(p1,p2,len*sizeof(real64));
}

#define USEDOT TRUE
/**< 
	USEDOT = TRUE is a winner on alphas, hps, and sparc20
	@TODO we definitely should be deferring to ATLAS/BLAS routines here, right?
*/

/**
	Computes inner product between vec1 and vec2, returning result.
	vec1 and vec2 may overlap or even be identical.
*/
real64 vec_inner_product(struct vec_vector *vec1 ,
                         struct vec_vector *vec2
){
  real64 *p1,*p2;
#if !USEDOT
  real64 sum;
#endif
  int32 len;

  asc_assert((NULL != vec1) &&
             (NULL != vec1->rng) &&
             (NULL != vec1->vec) &&
             (vec1->rng->low >= 0) &&
             (vec1->rng->low <= vec1->rng->high) &&
             (NULL != vec2) &&
             (NULL != vec2->rng) &&
             (NULL != vec2->vec) &&
             (vec2->rng->low >= 0));

  p1 = vec1->vec + vec1->rng->low;
  p2 = vec2->vec + vec2->rng->low;
  len = vec1->rng->high - vec1->rng->low + 1;
#if !USEDOT
  if (p1 != p2) {
    for( sum=0.0 ; --len >= 0 ; ++p1,++p2 ) {
      sum += (*p1) * (*p2);
    }
    return(sum);
  } else {
    for( sum=0.0 ; --len >= 0 ; ++p1 ) {
      sum += (*p1) * (*p1);
    }
    return(sum);
  }
#else
  return vec_dot(len,p1,p2);
#endif
}

/**
	Computes norm^2 of vector, assigning the result to vec->norm2
	and returning the result as well.
*/
real64 vec_square_norm(struct vec_vector *vec){
  vec->norm2 = vec_inner_product(vec,vec);
  return vec->norm2;
}

/**
	Stores prod := (scale)*(mtx)(vec) or (scale)*(mtx-transpose)(vec).
	vec and prod must be completely different.
*/
void vec_matrix_product(mtx_matrix_t mtx, struct vec_vector *vec,
                        struct vec_vector *prod, real64 scale,
                        boolean transpose
){
  mtx_coord_t nz;
  real64 value, *vvec, *pvec;
  int32 lim;

  asc_assert((NULL != vec) &&
             (NULL != vec->rng) &&
             (NULL != vec->vec) &&
             (vec->rng->low >= 0) &&
             (vec->rng->low <= vec->rng->high) &&
             (NULL != prod) &&
             (NULL != prod->rng) &&
             (NULL != prod->vec) &&
             (prod->rng->low >= 0) &&
             (prod->rng->low <= prod->rng->high) &&
             (NULL != mtx));

  lim = prod->rng->high;
  pvec = prod->vec;
  vvec = vec->vec;
  if( transpose ) {
    for(nz.col = prod->rng->low ; nz.col <= lim ; ++(nz.col) ) {
      pvec[nz.col] = 0.0;
      nz.row = mtx_FIRST;
      while( value = mtx_next_in_col(mtx,&nz,vec->rng),
             nz.row != mtx_LAST )
        pvec[nz.col] += value*vvec[nz.row];
      pvec[nz.col] *= scale;
     }
  } else {
    for(nz.row = prod->rng->low ; nz.row <= lim ; ++(nz.row) ) {
      pvec[nz.row] = 0.0;
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(mtx,&nz,vec->rng),
             nz.col != mtx_LAST )
        pvec[nz.row] += value*vvec[nz.col];
      pvec[nz.row] *= scale;
    }
  }
}

/* outputs a vector */
void vec_write(FILE *fp, struct vec_vector *vec){
  int32 ndx,hi;
  real64 *vvec;

  if (NULL == fp) {
    FPRINTF(ASCERR, "Error writing vector in vec_write:  NULL file pointer.\n");
    return;
  }
  if ((NULL == vec) ||
      (NULL == vec->rng) ||
      (NULL == vec->vec) ||
      (vec->rng->low < 0) ||
      (vec->rng->low > vec->rng->high)) {
    FPRINTF(ASCERR, "Error writing vector in vec_write:  uninitialized vector.\n");
    return;
  }

  vvec = vec->vec;
  hi = vec->rng->high;
  FPRINTF(fp,"Norm = %g, Accurate = %s, Vector range = %d to %d\n",
    sqrt(fabs(vec->norm2)), vec->accurate?"TRUE":"FALSE",
    vec->rng->low,vec->rng->high);
  FPRINTF(fp,"Vector --> ");
  for( ndx=vec->rng->low ; ndx<=hi ; ++ndx )
    FPRINTF(fp, "%g ", vvec[ndx]);
  PUTC('\n',fp);
}

/* Dot product for loop unrolled vector norms */
real64 vec_dot(int32 len, const real64 *p1, const real64 *p2)
{
  register double sum,lsum;
  int m,n;

/* 
	AVMAGIC in fact isn't magic.
	only goes to 2-10. change the code below if you mess with it.
	Default AVMAGIC is 4, which works well on alphas, tika.
*/
#define AVMAGIC 4
#ifdef sun
#undef AVMAGIC
#define AVMAGIC 6
#endif
#ifdef __hpux
#undef AVMAGIC
#define AVMAGIC 4
/* 
	2 was best value on ranier(9000/720) but tika (9000/715) likes 4
	there are no recognizable (defines) compiler differences, so tika
	will set the hp default
*/
#endif
/* 
	hands down best avmagic on atlas is 4, ranier 2, unxi21 6
	under native compilers. no bets on the billions of gcc variants.
	needs to be tried on sgi.
	Note, these are tuned for speed, not for accuracy. on very large
	vectors something like dnrm2 may be more appropriate, though
	it is very slow. upping avmagic to 10 will help accuracy some.
*/

#if (AVMAGIC>10)
#undef AVMAGIC
#define AVMAGIC 10
#endif

  asc_assert((NULL != p1) && (NULL != p2) && (len >= 0));

  m = len / AVMAGIC;
  n = len % AVMAGIC;
  if (p1!=p2) {
    /* get leading leftovers */
    for( sum=0.0 ; --n >= 0 ;  ) {
      sum += (*p1) * (*p2);
      ++p1; ++p2;
    }
    /* p1,p2 now point at first unadded location, or just after the end (m=0)*/
    /* eat m chunks */
    for( n=0; n <m; n++) {
      /* now, as i am too lazy to figure out a macro that expands itself */
      lsum = (*p1) * (*p2); /* zeroth term is assigned */
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 1th */
#if (AVMAGIC>2)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 2th */
#if (AVMAGIC>3)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 3th */
#if (AVMAGIC>4)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 4th */
#if (AVMAGIC>5)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 5th */
#if (AVMAGIC>6)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 6th */
#if (AVMAGIC>7)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 7th */
#if (AVMAGIC>8)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 8th */
#if (AVMAGIC>9)
      p1++; p2++;
      lsum += (*p1) * (*p2); /* add 9th */
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
      p1++; p2++;             /* leave p1,p2 pointing at next zeroth */
      sum += lsum;
    }
  } else {
    /* get leading leftovers */
    for( sum=0.0 ; --n >= 0 ; ++p1 ) {
      sum += (*p1) * (*p1);
    }
    /* p1 now points at first unadded location, or just after the end (m=0)*/
    /* eat m chunks */
    for( n=0; n <m; n++) {
      /* now, as i am too lazy to figure out a macro that expands itself */
      lsum = (*p1) * (*p1); /* zeroth term is assigned */
      p1++;
      lsum += (*p1) * (*p1); /* add 1th */
#if (AVMAGIC>2)
      p1++;
      lsum += (*p1) * (*p1); /* add 2th */
#if (AVMAGIC>3)
      p1++;
      lsum += (*p1) * (*p1); /* add 3th */
#if (AVMAGIC>4)
      p1++;
      lsum += (*p1) * (*p1); /* add 4th */
#if (AVMAGIC>5)
      p1++;
      lsum += (*p1) * (*p1); /* add 5th */
#if (AVMAGIC>6)
      p1++;
      lsum += (*p1) * (*p1); /* add 6th */
#if (AVMAGIC>7)
      p1++;
      lsum += (*p1) * (*p1); /* add 7th */
#if (AVMAGIC>8)
      p1++;
      lsum += (*p1) * (*p1); /* add 8th */
#if (AVMAGIC>9)
      p1++;
      lsum += (*p1) * (*p1); /* add 9th */
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
      p1++;                  /* leave p1 pointing at next zeroth */
      sum += lsum;
    }
  }
  return(sum);
#undef AVMAGIC
}
