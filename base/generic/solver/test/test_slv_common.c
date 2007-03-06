/*
 *  Unit test functions for ASCEND: solver/slv_common.c
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include <stdio.h>
#include <utilities/ascConfig.h>
#ifdef __WIN32__
#include <io.h>
#endif

#include "CUnit/CUnit.h"

#include <utilities/ascMalloc.h>

#include <general/list.h>

#include <linear/mtx.h>
#include <linear/mtx_vector.h>

#include <system/slv_types.h>
#include <system/rel.h>
#include <system/logrel.h>
#include <system/slv_common.h>

#include "test_slv_common.h"
#include "assertimpl.h"
#include "printutil.h"

/*
 *  Independent calculation of a vector dot product.
 *  Nothing fancy, no validation of input.  Assumes valid vectors.
 */
static real64 slow_inner_product(struct vec_vector *vec1, struct vec_vector *vec2)
{
  int32 i;
  real64 product = 0.0;
  real64 *p1 = vec1->vec + vec1->rng->low;
  real64 *p2 = vec2->vec + vec2->rng->low;
  int32 len = vec1->rng->high - vec1->rng->low + 1;

  for (i=0 ; i<len ; ++i, ++p1, ++p2)
    product += *p1 * *p2;

  return product;
}

/*
 *  Independent calculation of an array dot product.
 *  Nothing fancy, no validation of input.
 *  Assumes valid arrays of length at least len.
 */
static real64 slow_dot_product(int32 len, real64 *array1, real64 *array2)
{
  int32 i;
  real64 product = 0.0;

  for (i=0 ; i<len ; ++i, ++array1, ++array2)
    product += *array1 * *array2;

  return product;
}

/*
 *  Independent calculation of a vector-matrix product.
 *  Nothing fancy, no validation of input.  Assumes valid vector & matrix.
 */
static void slow_vector_matrix_product(mtx_matrix_t mtx,
                                       struct vec_vector *vec,
                                       struct vec_vector *prod,
                                       real64 scale)
{
  int32 row, col;
  mtx_coord_t coord;
  int32 limit = vec->rng->high;

  coord.row = vec->rng->low;
  for (row=vec->rng->low ; row<=limit ; ++row) {
    coord.col = vec->rng->low;
    prod->vec[coord.row] = 0.0;
    for (col=vec->rng->low ; col<=limit ; ++col) {
      prod->vec[coord.row] += vec->vec[coord.col] * mtx_value(mtx, &coord);
      ++coord.col;
    }
    prod->vec[coord.row] *= scale;
    ++coord.row;
  }
}

/* int comparison function for list searches */
static int compare_int32s(CONST VOIDPTR p1, CONST VOIDPTR p2)
{
  assert((NULL != p1) && (NULL != p2));
  return *((int32*)p1) - *((int32*)p2);
}


/*
 *  This function tests the slv_common.c functions and data structures.
 *  Note that some of the implementation declarated in slv_common.h is
 *  defined in slv.c rather than slv_common.c.  This subset of slv_common.h
 *  will be tested along with slv.c elsewhere.
 */
static void test_slv_common(void)
{
  struct vec_vector *pvec1;
  struct vec_vector *pvec2;
  struct vec_vector *pvec3;
  mtx_matrix_t mtx;
  mtx_coord_t coord;
  mtx_region_t region;
  real64 rarray[100];
  real64 rarray2[100];
  int i;
  FILE *file_normal;
  int32 hi[11];
  int32 hj[11];
  int32 **lnkmap;
  int32 *lnkvars;
  struct gl_list_t *col_list;
  struct gl_list_t *lnkindex_list;
  int32 lnkindexes[11];
  unsigned int pos;
  unsigned long prior_meminuse;
  unsigned long cur_meminuse;
  unsigned long test_meminuse;
  int i_initialized_lists = FALSE;
  int i_enabled_printing = FALSE;

#ifdef NDEBUG
  CU_FAIL("test_slv_common() compiled with NDEBUG - some features not tested.");
#endif
#ifndef MALLOC_DEBUG
  CU_FAIL("test_slv_common() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  prior_meminuse = ascmeminuse();

  /* set up pooling & recycling */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }

  for (i=0 ; i<100 ; ++i) {                           /* create some reals to use later */
    rarray[i] = 7/2 * i;
  }

  /* test vec_create(), vec_destroy() */

  test_meminuse = ascmeminuse();

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(-1, 0);                       /* error - low < 0 */
  CU_TEST(NULL == pvec1);

  vec_destroy(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(0, -1);                       /* error - high < 0 */
  CU_TEST(NULL == pvec1);

  vec_destroy(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(10, 0);                       /* error - low > high */
  CU_TEST(NULL == pvec1);

  vec_destroy(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(0, 0);                        /* ok - low == high */
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(0 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, sizeof(real64)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(0, 10);                       /* ok - low < high */
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(10 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_init() */

  test_meminuse = ascmeminuse();

  cur_meminuse = ascmeminuse();
  CU_TEST(2 == vec_init(NULL, 0, 10));             /* error - NULL vec */
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(1 == vec_init(pvec1, -1, 10));           /* error - low < 0 */
  CU_TEST(NULL == pvec1->rng);
  CU_TEST(NULL == pvec1->vec);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(1 == vec_init(pvec1, 10, -1));           /* error - high < 0 */
  CU_TEST(NULL == pvec1->rng);
  CU_TEST(NULL == pvec1->vec);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(0 == vec_init(pvec1, 10, 10));           /* ok - low == high */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(10 == pvec1->rng->low);
  CU_TEST(10 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(0 == vec_init(pvec1, 10, 100));          /* ok - low < high */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(10 == pvec1->rng->low);
  CU_TEST(100 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 101 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 101 * sizeof(real64)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = vec_create(0,0);                         /* create a vector with data */
  CU_TEST_FATAL(NULL != pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vec_vector)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#endif

  pvec1->accurate = TRUE;
  pvec1->vec[0] = rarray[0];

  CU_TEST(1 == vec_init(pvec1, -1, 100));          /* error - low < 0 */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(0 == pvec1->rng->high);
  CU_TEST_FATAL(NULL != pvec1->vec);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#endif

  CU_TEST(1 == vec_init(pvec1, 1, 0));          /* error - high < low */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(0 == pvec1->rng->high);
  CU_TEST_FATAL(NULL != pvec1->vec);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#endif

  CU_TEST(0 == vec_init(pvec1, 0, 1));          /* ok - high > low */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(1 == pvec1->rng->high);
  CU_TEST_FATAL(NULL != pvec1->vec);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 2 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 2 * sizeof(real64)));
#endif

  pvec1->accurate = TRUE;
  pvec1->vec[1] = rarray[1];

  CU_TEST(0 == vec_init(pvec1, 9, 10));         /* ok - high > low */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(9 == pvec1->rng->low);
  CU_TEST(10 == pvec1->rng->high);
  CU_TEST_FATAL(NULL != pvec1->vec);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[1], rarray[1], 0.00001);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#endif

  vec_destroy(pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, 0));
#endif
  CU_TEST(cur_meminuse == ascmeminuse());

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_zero() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_zero(NULL);                       /* error - NULL vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = ASC_NEW_ARRAY(real64,10 );

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_zero(pvec1);                      /* error - NULL vec->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  ascfree(pvec1->vec);
  pvec1->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_zero(pvec1);                      /* error - NULL vec->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->vec = ASC_NEW_ARRAY(real64,10 );
  pvec1->rng->low = -1;
  pvec1->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_zero(pvec1);                      /* error - low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_zero(pvec1);                      /* error - low > high */
  CU_TEST(TRUE == asc_assert_failed());

  vec_destroy(pvec1);

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = vec_create(0,0);                     /* create & initialize a 1-element vector */
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = rarray[0];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);

  vec_zero(pvec1);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], 0.0, 0.00001);

  CU_TEST_FATAL(0 == vec_init(pvec1, 0, 9));   /* redimension to larger vector */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  vec_zero(pvec1);
  for (i=0 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], 0.0, 0.00001);  /* all data should now be 0.0 */
  }

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize again */
  }

  pvec1->rng->low = 5;
  pvec1->rng->high = 7;

  vec_zero(pvec1);
  for (i=0 ; i<5 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }
  for (i=5 ; i<8 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], 0.0, 0.00001);
  }
  for (i=8 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  vec_destroy(pvec1);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_copy() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  pvec1 = vec_create(0,10);
  CU_TEST_FATAL(NULL != pvec1);

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(NULL, pvec1);                /* error - NULL srcvec */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec1, NULL);                /* error - NULL destvec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng */
  CU_TEST_FATAL(NULL != pvec2);
  pvec2->rng = NULL;
  pvec2->vec = ASC_NEW_ARRAY(real64,10 );

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec2, pvec1);              /* error - NULL srcvec->rng */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec1, pvec2);              /* error - NULL destvec->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  ascfree(pvec2->vec);
  pvec2->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec2, pvec1);              /* error - NULL srcvec->vec */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec1, pvec2);              /* error - NULL destvec->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->vec = ASC_NEW_ARRAY(real64,10 );
  pvec2->rng->low = -1;
  pvec2->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec2, pvec1);              /* error - srcvec->rng->low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec1, pvec2);              /* error - destvec->rng->low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_copy(pvec2, pvec1);              /* error - srcvec low > high */
  CU_TEST(TRUE == asc_assert_failed());

  vec_destroy(pvec1);
  vec_destroy(pvec2);

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = vec_create(0,0);                     /* create & initialize a 1-element vectors */
  pvec2 = vec_create(0,0);
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = rarray[0];
  pvec2->vec[0] = rarray[5];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  vec_copy(pvec1, pvec2);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[0], 0.00001);

  CU_TEST_FATAL(0 == vec_init(pvec1, 0, 9));   /* redimension pvec1 to larger vector */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  pvec2->vec[0] = rarray[8];
  vec_copy(pvec2, pvec1);                  /* copy 1 element*/
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[8], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[8], 0.00001);
  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* rest of data should be intact */
  }

  pvec2->vec[0] = rarray[3];
  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  vec_copy(pvec1, pvec2);                  /* copy 1 element other way*/
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[9], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[8], 0.00001);
  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in src should be intact */
  }

  CU_TEST_FATAL(0 == vec_init(pvec2, 0, 9));   /* redimension pvec2 to larger vector */
  vec_zero(pvec2);                         /* zero the destvec */
  pvec1->rng->low = 0;
  pvec1->rng->high = 9;
  vec_copy(pvec1, pvec2);                  /* copy all elements */
  for (i=0 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], pvec2->vec[i], 0.00001);  /* data should be the same */
  }

  for (i=0 ; i<10 ; ++i) {
    pvec2->vec[i] = rarray[9-i];                  /* reinitialize & check the data */
  }
  pvec2->rng->low = 3;
  pvec2->rng->high = 6;
  vec_copy(pvec2, pvec1);                  /* copy a subset of elements to start of destvec */
  for (i=3 ; i<7 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i-3], rarray[9-i], 0.00001);  /* data should be the same */
  }
  for (i=4 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data should be the same */
  }

  vec_destroy(pvec1);
  vec_destroy(pvec2);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_inner_product() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  pvec1 = vec_create(0,10);
  CU_TEST_FATAL(NULL != pvec1);

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(NULL, pvec1);              /* error - NULL vec1 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec1, NULL);              /* error - NULL vec2 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng */
  CU_TEST_FATAL(NULL != pvec2);
  pvec2->rng = NULL;
  pvec2->vec = ASC_NEW_ARRAY(real64,10 );

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec2, pvec1);              /* error - NULL vec1->rng */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec1, pvec2);              /* error - NULL vec2->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  ascfree(pvec2->vec);
  pvec2->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec2, pvec1);              /* error - NULL vec1->vec */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec1, pvec2);              /* error - NULL vec2->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->vec = ASC_NEW_ARRAY(real64,10 );
  pvec2->rng->low = -1;
  pvec2->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec2, pvec1);              /* error - vec1->rng->low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec1, pvec2);              /* error - vec2->rng->low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_inner_product(pvec2, pvec1);             /* error - vec1 low > high */
  CU_TEST(TRUE == asc_assert_failed());

  vec_destroy(pvec1);
  vec_destroy(pvec2);

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = vec_create(0,0);                     /* create & initialize a 1-element vectors */
  pvec2 = vec_create(0,0);
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec2);

  pvec1->vec[0] = rarray[0];
  pvec2->vec[0] = rarray[5];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec1, pvec2), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec2, pvec1), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  CU_TEST_FATAL(0 == vec_init(pvec1, 0, 9));   /* redimension vectors larger */
  CU_TEST_FATAL(0 == vec_init(pvec2, 0, 9));

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    pvec2->vec[i] = 2.0;
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec1, pvec2), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec2, pvec1), slow_inner_product(pvec1, pvec2), 0.00001);

  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  pvec2->rng->low = 5;
  pvec2->rng->high = 5;                           /* check 1 element subrange */
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec1, pvec2), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec2, pvec1), slow_inner_product(pvec1, pvec2), 0.00001);

  pvec1->rng->low = 0;
  pvec1->rng->high = 3;
  pvec2->rng->low = 2;
  pvec2->rng->high = 5;                           /* check 4 element subrange */
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec1, pvec2), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec2, pvec1), slow_inner_product(pvec1, pvec2), 0.00001);

  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in vecs should be intact */
    CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[i], 2.0, 0.00001);
  }

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    pvec2->vec[i] = rarray[9-i];
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec1, pvec2), slow_inner_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_inner_product(pvec2, pvec1), slow_inner_product(pvec1, pvec2), 0.00001);

  vec_destroy(pvec1);
  vec_destroy(pvec2);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_square_norm() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                       /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_square_norm(NULL);                      /* error - NULL vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));  /* create a vector with NULL rng */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = ASC_NEW_ARRAY(real64,10 );

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_square_norm(pvec1);                     /* error - NULL vec->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  ascfree(pvec1->vec);
  pvec1->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_square_norm(pvec1);                     /* error - NULL vec->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->vec = ASC_NEW_ARRAY(real64,10 );
  pvec1->rng->low = -1;
  pvec1->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_square_norm(pvec1);                     /* error - vec->rng->low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_square_norm(pvec1);                     /* error - vec low > high */
  CU_TEST(TRUE == asc_assert_failed());

  vec_destroy(pvec1);

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = vec_create(0,0);                     /* create & initialize a 1-element vector */
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = 0.0;
  CU_ASSERT_DOUBLE_EQUAL(vec_square_norm(pvec1), slow_inner_product(pvec1, pvec1), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], 0.0, 0.00001);

  pvec1->vec[0] = rarray[7];
  CU_ASSERT_DOUBLE_EQUAL(vec_square_norm(pvec1), slow_inner_product(pvec1, pvec1), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[7], 0.00001);

  CU_TEST_FATAL(0 == vec_init(pvec1, 0, 9));   /* redimension vectors larger */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize the data */
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(vec_square_norm(pvec1), slow_inner_product(pvec1, pvec1), 0.00001);

  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  CU_ASSERT_DOUBLE_EQUAL(vec_square_norm(pvec1), slow_inner_product(pvec1, pvec1), 0.00001);

  pvec1->rng->low = 0;
  pvec1->rng->high = 3;
  CU_ASSERT_DOUBLE_EQUAL(vec_square_norm(pvec1), slow_inner_product(pvec1, pvec1), 0.00001);

  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in vecs should be intact */
  }

  vec_destroy(pvec1);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_matrix_product() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                       /* prepare to test assertions */

  mtx = mtx_create();
  CU_TEST_FATAL(NULL != mtx);
  mtx_set_order(mtx, 10);
  pvec1 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  pvec1->rng->low = 0;
  pvec1->rng->high = 10;
  pvec1->vec = ASC_NEW_ARRAY(real64,11 );
  pvec2 = (struct vec_vector *)ascmalloc(sizeof(struct vec_vector));
  CU_TEST_FATAL(NULL != pvec2);
  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  pvec2->rng->low = 0;
  pvec2->rng->high = 10;
  pvec2->vec = ASC_NEW_ARRAY(real64,11 );

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(NULL, pvec1, pvec2, 1.0, FALSE);   /* error - NULL mtx */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, NULL, pvec2, 1.0, FALSE);   /* error - NULL vec */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, NULL, 1.0, FALSE);   /* error - NULL prod */
  CU_TEST(TRUE == asc_assert_failed());

  ascfree(pvec1->rng);
  pvec1->rng = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - NULL vec->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  pvec1->rng->low = 0;
  pvec1->rng->high = 10;
  ascfree(pvec2->rng);
  pvec2->rng = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - NULL prod->rng */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
  pvec2->rng->low = 0;
  pvec2->rng->high = 10;
  ascfree(pvec1->vec);
  pvec1->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - NULL vec->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->vec = ASC_NEW_ARRAY(real64,11 );
  ascfree(pvec2->vec);
  pvec2->vec = NULL;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - NULL prod->vec */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->vec = ASC_NEW_ARRAY(real64,11 );
  pvec1->rng->low = -1;
  pvec1->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - vec low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - vec low > high */
  CU_TEST(TRUE == asc_assert_failed());

  pvec1->rng->low = 0;
  pvec1->rng->high = 10;
  pvec2->rng->low = -1;
  pvec2->rng->high = 10;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - prod low < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  pvec2->rng->low = 11;

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);   /* error - prod low > high */
  CU_TEST(TRUE == asc_assert_failed());

  mtx_destroy(mtx);
  vec_destroy(pvec1);
  vec_destroy(pvec2);

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  mtx = mtx_create();
  mtx_set_order(mtx, 10);

  pvec1 = vec_create(0,0);
  pvec1->vec[0] = 10.0;

  pvec2 = vec_create(0,0);
  pvec3 = vec_create(0,0);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);    /* mtx with all zero's */
  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* 1-element vector */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);

  mtx_fill_value(mtx, mtx_coord(&coord,0,0), 20.0);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);    /* normal mtx */
  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* 1-element vector */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(200.0, pvec2->vec[0], 0.000001);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, TRUE);     /* transpose should have no effect */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(200.0, pvec2->vec[0], 0.000001);

  mtx_clear(mtx);

  vec_init(pvec1,0,1);
  pvec1->vec[0] = 10.0;
  pvec1->vec[1] = 20.5;

  vec_init(pvec2, 0,1);
  vec_init(pvec3, 0,1);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);    /* empty mtx */
  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* 2-element vector */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[1], pvec3->vec[1], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[1], 0.000001);

  mtx_fill_value(mtx, mtx_coord(&coord,0,0), 20.0);
  mtx_fill_value(mtx, mtx_coord(&coord,0,1), 0.5);
  mtx_fill_value(mtx, mtx_coord(&coord,1,1), -0.455);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);    /* normal mtx, but not all non-zeros */
  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* 2-element vector */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[1], pvec3->vec[1], 0.000001);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, TRUE);     /* transpose of normal mtx, not all non-zeros */

  mtx_clear(mtx);
  mtx_fill_value(mtx, mtx_coord(&coord,0,0), 20.0);
  mtx_fill_value(mtx, mtx_coord(&coord,1,0), 0.5);
  mtx_fill_value(mtx, mtx_coord(&coord,1,1), -0.455);

  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* confirm transpose works */
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], pvec3->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[1], pvec3->vec[1], 0.000001);

  vec_init(pvec1,0,10);
  pvec1->vec[0] = 10.0;
  pvec1->vec[1] = 20.0;
  pvec1->vec[2] = 30.0;
  pvec1->vec[3] = 40.0;
  pvec1->vec[4] = 50.0;
  pvec1->vec[5] = 60.0;
  pvec1->vec[6] = 70.0;
  pvec1->vec[7] = 80.0;
  pvec1->vec[8] = 90.0;
  pvec1->vec[9] = 100.0;
  pvec1->vec[10] = 110.0;
  pvec1->rng->low = 2;                              /* only use a subset of vector */
  pvec1->rng->high = 4;

  vec_init(pvec2, 0,10);
  vec_init(pvec3, 0,10);
  for (i=0 ; i<11 ; ++i) {                          /* zero product vecs so can detect subset */
    pvec2->vec[i] = 0.0;
    pvec3->vec[i] = 0.0;
  }

  mtx_clear(mtx);
  mtx_fill_value(mtx, mtx_coord(&coord,2,2), 1.0);  /* only give values in vector range */
  mtx_fill_value(mtx, mtx_coord(&coord,2,3), 1.0);
  mtx_fill_value(mtx, mtx_coord(&coord,2,4), 1.0);
  mtx_fill_value(mtx, mtx_coord(&coord,3,2), 2.0);
  mtx_fill_value(mtx, mtx_coord(&coord,3,3), 2.0);
  mtx_fill_value(mtx, mtx_coord(&coord,3,4), 2.0);
  mtx_fill_value(mtx, mtx_coord(&coord,4,2), 3.0);
  mtx_fill_value(mtx, mtx_coord(&coord,4,3), 3.0);
  mtx_fill_value(mtx, mtx_coord(&coord,4,4), 3.0);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, FALSE);    /* normal mtx */
  slow_vector_matrix_product(mtx, pvec1, pvec3, 1.0);   /* vector subset*/

  for (i=0 ; i<11 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[i], pvec3->vec[i], 0.000001);
  }
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[1], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(120.0, pvec2->vec[2], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(240.0, pvec2->vec[3], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(360.0, pvec2->vec[4], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[5], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[6], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[7], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[8], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[9], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[10], 0.000001);

  vec_matrix_product(mtx, pvec1, pvec2, 0.5, FALSE);    /* different scale */

  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[1], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(60.0, pvec2->vec[2], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(120.0, pvec2->vec[3], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(180.0, pvec2->vec[4], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[5], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[6], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[7], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[8], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[9], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[10], 0.000001);

  vec_matrix_product(mtx, pvec1, pvec2, 1.0, TRUE);     /* transpose */

  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[1], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(260.0, pvec2->vec[2], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(260.0, pvec2->vec[3], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(260.0, pvec2->vec[4], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[5], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[6], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[7], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[8], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[9], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[10], 0.000001);

  vec_matrix_product(mtx, pvec1, pvec2, 2.0, TRUE);     /* transpose */

  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[0], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[1], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(520.0, pvec2->vec[2], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(520.0, pvec2->vec[3], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(520.0, pvec2->vec[4], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[5], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[6], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[7], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[8], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[9], 0.000001);
  CU_ASSERT_DOUBLE_EQUAL(0.0, pvec2->vec[10], 0.000001);

  mtx_destroy(mtx);
  vec_destroy(pvec1);
  vec_destroy(pvec2);
  vec_destroy(pvec3);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_write() - not much to do but make sure something gets written */

  test_meminuse = ascmeminuse();

  pvec1 = vec_create(0,10);

  if (FALSE == test_printing_enabled()) {
    test_enable_printing();
    i_enabled_printing = TRUE;
  }

  if (NULL != (file_normal = fopen("slvcommontempfile1.tmp", "w+"))) {

    vec_write(file_normal, pvec1);/* write to normal open file */
    rewind(file_normal);
    CU_TEST(EOF != fgetc(file_normal)); /* test that file is not empty */
    fclose(file_normal);
    remove("slvcommontempfile1.tmp");
  }
  else {
    CU_FAIL("Error opening output file 1 in test_slv_common.c");
  }

  if (TRUE == i_enabled_printing) {
    test_disable_printing();
    i_enabled_printing = FALSE;
  }

  vec_destroy(pvec1);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test vec_dot() */

  test_meminuse = ascmeminuse();

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                         /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_dot(10, NULL, rarray2);                   /* error - NULL a1 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_dot(10, rarray, NULL);                    /* error - NULL a2 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    vec_dot(-10, rarray, rarray2);                /* error - len < 0 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  rarray2[0] = rarray[5];

  CU_ASSERT_DOUBLE_EQUAL(vec_dot(1, rarray, rarray2), slow_dot_product(1, rarray, rarray2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_dot(1, rarray2, rarray), slow_dot_product(1, rarray, rarray2), 0.00001);

  for (i=0 ; i<10 ; ++i) {
    rarray2[i] = 2.0;
  }

  CU_ASSERT_DOUBLE_EQUAL(vec_dot(11, rarray, rarray2), slow_dot_product(11, rarray, rarray2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_dot(11, rarray2, rarray), slow_dot_product(11, rarray, rarray2), 0.00001);

  CU_ASSERT_DOUBLE_EQUAL(vec_dot(5, rarray, rarray2), slow_dot_product(5, rarray, rarray2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_dot(5, rarray2, rarray), slow_dot_product(5, rarray, rarray2), 0.00001);

  CU_ASSERT_DOUBLE_EQUAL(vec_dot(0, rarray, rarray2), slow_dot_product(0, rarray, rarray2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_dot(0, rarray2, rarray), slow_dot_product(0, rarray, rarray2), 0.00001);

  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(7/2 * i, rarray[i], 0.00001);  /* data in arrays should be intact */
    CU_ASSERT_DOUBLE_EQUAL(rarray2[i], 2.0, 0.00001);
  }

  for (i=0 ; i<10 ; ++i) {
    rarray2[i] = rarray[9-i];
  }

  CU_ASSERT_DOUBLE_EQUAL(vec_dot(11, rarray, rarray2), slow_dot_product(11, rarray, rarray2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(vec_dot(11, rarray2, rarray), slow_dot_product(11, rarray, rarray2), 0.00001);

  CU_TEST(test_meminuse == ascmeminuse());

  /* test slv_get_output_file() */

  test_meminuse = ascmeminuse();

  file_normal = (FILE *)100;
  CU_TEST(file_normal == slv_get_output_file(file_normal)); /* non-NULL fp */
  CU_TEST(NULL != slv_get_output_file(NULL));               /* NULL fp */
  fprintf(slv_get_output_file(NULL), "\n If you see this then test_slv_common:slv_get_output_file() failed!");

  /* MIF(), LIF(), PMIF(), PLIF() - macros accessing members - not tested */

  /* not tested - revisit later:
   *    - slv_print_obj_name()
   *    - slv_print_rel_name()
   *    - slv_print_var_name()
   *    - slv_print_logrel_name()
   *    - slv_print_dis_name()
   *    - slv_print_obj_index()
   *    - slv_print_rel_sindex()
   *    - slv_print_var_sindex()
   *    - slv_print_logrel_sindex()
   *    - slv_print_dis_sindex()
   *    - slv_print_obj_index()
   */

  test_meminuse = ascmeminuse();

  /* CU_FAIL("slv_print_*_name() and slv_print_*_sindex() not tested."); */

  CU_TEST(test_meminuse == ascmeminuse());

  /* test slv_direct_solve() */

  test_meminuse = ascmeminuse();

  /* CU_FAIL("slv_direct_solve() test not implemented."); */

  CU_TEST(test_meminuse == ascmeminuse());

  /* test slv_direct_log_solve() */

  test_meminuse = ascmeminuse();

  /* CU_FAIL("slv_direct_log_solve() test not implemented."); */

  CU_TEST(test_meminuse == ascmeminuse());

  /* test slv_create_lnkmap(), slv_write_lnkmap(), slv_destroy_lnkmap() */

  test_meminuse = ascmeminuse();

  hi[0] = 100;
  hj[0] = 1;

  CU_TEST(NULL == slv_create_lnkmap(10, 10, 1, hi, hj));  /* error - hi contains invalid index */

  hi[0] = 1;
  hj[0] = 100;

  CU_TEST(NULL == slv_create_lnkmap(10, 10, 1, hi, hj));  /* error - hj contains invalid index */

  lnkmap = slv_create_lnkmap(10, 10, 0, hi, hj);    /* 0 element arrays  */
  CU_TEST_FATAL(NULL != lnkmap);
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(0 == *lnkmap[i]);
  }

  CU_TEST(0 != AllocatedMemory((VOIDPTR)lnkmap, 0));

  slv_destroy_lnkmap(lnkmap);

#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory((VOIDPTR)lnkmap, 0));
#else
  CU_TEST(1 == AllocatedMemory((VOIDPTR)lnkmap, 0));
#endif

  hi[0] = 1;
  hj[0] = 1;

  lnkmap = slv_create_lnkmap(10, 10, 1, hi, hj);    /* 1 element arrays  */
  CU_TEST_FATAL(NULL != lnkmap);
  for (i=0 ; i<10 ; ++i) {
    lnkvars = lnkmap[i];
    if (i == 1) {
      CU_TEST_FATAL(1 == lnkvars[0]);   /* number of non-zero elements */
      CU_TEST(1 == lnkvars[1]);         /* column # of 1st element */
      CU_TEST(0 == lnkvars[2]);         /* link map index */
    } else {
      CU_TEST(0 == lnkvars[0]);
    }
  }

  CU_TEST(0 != AllocatedMemory((VOIDPTR)lnkmap, 0));

  slv_destroy_lnkmap(lnkmap);

#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory((VOIDPTR)lnkmap, 0));
#else
  CU_TEST(1 == AllocatedMemory((VOIDPTR)lnkmap, 0));
#endif

  /* link map order: (5,1) (3,0) (3,8) (2,10) (2,0) (2,4) (2,7) (2,2) (1,10) (1,5) (1,1) */
  hi[0] = 5;                      /* row 5: (5,1) */
  hj[0] = 1;
  hi[1] = 3;                      /* row 3: (3,0) (3,8) */
  hj[1] = 0;
  hi[2] = 3;
  hj[2] = 8;
  hi[3] = 2;                      /* row 2: (2,0) (2,2) (2,4) (2,7) (2,10) */
  hj[3] = 10;
  hi[4] = 2;
  hj[4] = 0;
  hi[5] = 2;
  hj[5] = 4;
  hi[6] = 2;
  hj[6] = 7;
  hi[7] = 2;
  hj[7] = 2;
  hi[8] = 1;                      /* row 1:  (1,1) (1,5) (1,10) */
  hj[8] = 10;
  hi[9] = 1;
  hj[9] = 5;
  hi[10] = 1;
  hj[10] = 1;

  lnkindex_list = gl_create(11);
  for (i=0 ; i<11 ; ++i) {
    lnkindexes[i] = i;
    gl_append_ptr(lnkindex_list, &lnkindexes[i]);
  }

  lnkmap = slv_create_lnkmap(6, 11, 11, hi, hj);    /* multi element arrays  */
  CU_TEST_FATAL(NULL != lnkmap);

  lnkvars = lnkmap[0];
  CU_TEST(0 == lnkvars[0]);         /* number of non-zero elements */

  col_list = gl_create(10);
  CU_TEST_FATAL(NULL != col_list);
  gl_append_ptr(col_list, &hj[8]);
  gl_append_ptr(col_list, &hj[9]);
  gl_append_ptr(col_list, &hj[10]);

  lnkvars = lnkmap[1];
  CU_TEST(3 == lnkvars[0]);         /* number of non-zero elements */
  for (i=0 ; i<lnkvars[0] ; ++i) {
    CU_TEST(0 != (pos = gl_search(col_list, &lnkvars[2*i+1], compare_int32s)));
    gl_delete(col_list, pos, FALSE);
    CU_TEST(0 != (pos = gl_search(lnkindex_list, &lnkvars[2*i+2], compare_int32s)));
    gl_delete(lnkindex_list, pos, FALSE);
  }

  gl_reset(col_list);
  gl_append_ptr(col_list, &hj[3]);
  gl_append_ptr(col_list, &hj[4]);
  gl_append_ptr(col_list, &hj[5]);
  gl_append_ptr(col_list, &hj[6]);
  gl_append_ptr(col_list, &hj[7]);

  lnkvars = lnkmap[2];
  CU_TEST(5 == lnkvars[0]);         /* number of non-zero elements */
  for (i=0 ; i<lnkvars[0] ; ++i) {
    CU_TEST(0 != (pos = gl_search(col_list, &lnkvars[2*i+1], compare_int32s)));
    gl_delete(col_list, pos, FALSE);
    CU_TEST(0 != (pos = gl_search(lnkindex_list, &lnkvars[2*i+2], compare_int32s)));
    gl_delete(lnkindex_list, pos, FALSE);
  }

  gl_reset(col_list);
  gl_append_ptr(col_list, &hj[1]);
  gl_append_ptr(col_list, &hj[2]);

  lnkvars = lnkmap[3];
  CU_TEST(2 == lnkvars[0]);         /* number of non-zero elements */
  for (i=0 ; i<lnkvars[0] ; ++i) {
    CU_TEST(0 != (pos = gl_search(col_list, &lnkvars[2*i+1], compare_int32s)));
    gl_delete(col_list, pos, FALSE);
    CU_TEST(0 != (pos = gl_search(lnkindex_list, &lnkvars[2*i+2], compare_int32s)));
    gl_delete(lnkindex_list, pos, FALSE);
  }

  lnkvars = lnkmap[4];
  CU_TEST(0 == lnkvars[0]);         /* number of non-zero elements */

  gl_reset(col_list);
  gl_append_ptr(col_list, &hj[0]);

  lnkvars = lnkmap[5];
  CU_TEST_FATAL(1 == lnkvars[0]);   /* number of non-zero elements */
  for (i=0 ; i<lnkvars[0] ; ++i) {
    CU_TEST(0 != (pos = gl_search(col_list, &lnkvars[2*i+1], compare_int32s)));
    gl_delete(col_list, pos, FALSE);
    CU_TEST(0 != (pos = gl_search(lnkindex_list, &lnkvars[2*i+2], compare_int32s)));
    gl_delete(lnkindex_list, pos, FALSE);
  }

  CU_TEST(0 == gl_length(lnkindex_list));   /* all lnkindexes should have been used */

  if (FALSE == test_printing_enabled()) {
    test_enable_printing();
    i_enabled_printing = TRUE;
  }

  if (NULL != (file_normal = fopen("slvcommontempfile2.tmp", "w+"))) {

    slv_write_lnkmap(file_normal, 6, lnkmap); /* write to normal open file */
    rewind(file_normal);
    CU_TEST(EOF != fgetc(file_normal)); /* test that file is not empty */
    fclose(file_normal);
    remove("slvcommontempfile2.tmp");
  }
  else {
    CU_FAIL("Error opening output file 2 in test_slv_common.c");
  }

  if (TRUE == i_enabled_printing) {
    test_disable_printing();
    i_enabled_printing = FALSE;
  }

  gl_destroy(col_list);
  gl_destroy(lnkindex_list);
  slv_destroy_lnkmap(lnkmap);

  gl_emptyrecycler();
  CU_TEST(test_meminuse == ascmeminuse());

  /* test slv_lnkmap_from_mtx() */

  test_meminuse = ascmeminuse();

  mtx = mtx_create();
  CU_TEST_FATAL(NULL != mtx);
  mtx_set_order(mtx, 11);

  region.row.low = 0;
  region.row.high = 10;
  region.col.low = 0;
  region.col.high = 10;

  CU_TEST(NULL == slv_lnkmap_from_mtx(NULL, &region));  /* error - NULL mtx */

  region.row.low = -1;
  region.row.high = 10;
  region.col.low = 0;
  region.col.high = 10;

  CU_TEST(NULL == slv_lnkmap_from_mtx(mtx, &region));   /* error - region.row.low < 0 */

  region.row.low = 0;
  region.row.high = 11;

  CU_TEST(NULL == slv_lnkmap_from_mtx(mtx, &region));   /* error - region.row.high >= order */

  region.col.low = -1;
  region.col.high = 10;

  CU_TEST(NULL == slv_lnkmap_from_mtx(mtx, &region));   /* error - region.col.low < 0 */

  region.col.low = 0;
  region.col.high = 11;

  CU_TEST(NULL == slv_lnkmap_from_mtx(mtx, &region));   /* error - region.col.high >= order */

  region.row.low = 0;
  region.row.high = 10;
  region.col.low = 0;
  region.col.high = 10;

  lnkmap = slv_lnkmap_from_mtx(mtx, &region);           /* empty matrix */
  CU_TEST_FATAL(NULL != lnkmap);
  for (i=0 ; i<11 ; ++i) {
    CU_TEST(0 == *lnkmap[i]);
  }

  slv_destroy_lnkmap(lnkmap);

  mtx_fill_value(mtx, mtx_coord(&coord,5,1),10.0); /* row 5: (5,1) (5,7) */
  mtx_fill_value(mtx, mtx_coord(&coord,2,0),20.1); /* row 2: (2,0) (2,6) */
  mtx_fill_value(mtx, mtx_coord(&coord,6,4),30.2); /* row 6: (6,4) (6,5) */
  mtx_fill_value(mtx, mtx_coord(&coord,2,6),40.3);
  mtx_fill_value(mtx, mtx_coord(&coord,0,2),50.4); /* row 0: (0,2) */
  mtx_fill_value(mtx, mtx_coord(&coord,5,7),59.5);
  mtx_fill_value(mtx, mtx_coord(&coord,6,5),69.6);
  mtx_fill_value(mtx, mtx_coord(&coord,3,8),79.7); /* row 3: (3,8) */
  mtx_fill_value(mtx, mtx_coord(&coord,9,9),89.8); /* row 9: (9,9) (9,10) */
  mtx_fill_value(mtx, mtx_coord(&coord,9,10),99.9);

  region.row.low  = 3;
  region.row.high = 4;
  region.col.low  = 0;
  region.col.high = 10;
  lnkmap = slv_lnkmap_from_mtx(mtx, &region);     /* region with 1 non-zero */
  CU_TEST_FATAL(NULL != lnkmap);
  lnkvars = lnkmap[3];
  CU_TEST(1 == lnkvars[0]);
  CU_TEST(8 == lnkvars[1]);
  CU_TEST(80 == lnkvars[2]);
  for (i=0 ; i<3 ; ++i) {
    CU_TEST(0 == *lnkmap[i]);
  }
  for (i=4 ; i<11 ; ++i) {
    CU_TEST(0 == *lnkmap[i]);
  }

  slv_destroy_lnkmap(lnkmap);

  lnkmap = slv_lnkmap_from_mtx(mtx, mtx_ENTIRE_MATRIX);     /* entire matrix */
  CU_TEST_FATAL(NULL != lnkmap);
  CU_TEST(0 == *lnkmap[1]);
  CU_TEST(0 == *lnkmap[4]);
  CU_TEST(0 == *lnkmap[7]);
  CU_TEST(0 == *lnkmap[8]);
  CU_TEST(0 == *lnkmap[10]);

  lnkvars = lnkmap[0];
  CU_TEST(1 == lnkvars[0]);
  CU_TEST(2 == lnkvars[1]);
  CU_TEST(50 == lnkvars[2]);

  lnkvars = lnkmap[2];
  CU_TEST(2 == lnkvars[0]);
  if (0 == lnkvars[1]) {
    CU_TEST(20 == lnkvars[2]);
    CU_TEST(6 == lnkvars[3]);
    CU_TEST(40 == lnkvars[4]);
  } else if (6 == lnkvars[1]) {
    CU_TEST(40 == lnkvars[2]);
    CU_TEST(0 == lnkvars[3]);
    CU_TEST(20 == lnkvars[4]);
  } else {
    CU_FAIL("Unexpected col for lnkmap row 2.");
  }

  lnkvars = lnkmap[3];
  CU_TEST(1 == lnkvars[0]);
  CU_TEST(8 == lnkvars[1]);
  CU_TEST(80 == lnkvars[2]);

  lnkvars = lnkmap[5];
  CU_TEST(2 == lnkvars[0]);
  if (1 == lnkvars[1]) {
    CU_TEST(10 == lnkvars[2]);
    CU_TEST(7 == lnkvars[3]);
    CU_TEST(60 == lnkvars[4]);
  } else if (7 == lnkvars[1]) {
    CU_TEST(60 == lnkvars[2]);
    CU_TEST(1 == lnkvars[3]);
    CU_TEST(10 == lnkvars[4]);
  } else {
    CU_FAIL("Unexpected col for lnkmap row 5.");
  }

  lnkvars = lnkmap[6];
  CU_TEST(2 == lnkvars[0]);
  if (4 == lnkvars[1]) {
    CU_TEST(30 == lnkvars[2]);
    CU_TEST(5 == lnkvars[3]);
    CU_TEST(70 == lnkvars[4]);
  } else if (5 == lnkvars[1]) {
    CU_TEST(70 == lnkvars[2]);
    CU_TEST(4 == lnkvars[3]);
    CU_TEST(30 == lnkvars[4]);
  } else {
    CU_FAIL("Unexpected col for lnkmap row 6.");
  }

  lnkvars = lnkmap[9];
  CU_TEST(2 == lnkvars[0]);
  if (9 == lnkvars[1]) {
    CU_TEST(90 == lnkvars[2]);
    CU_TEST(10 == lnkvars[3]);
    CU_TEST(100 == lnkvars[4]);
  } else if (10 == lnkvars[1]) {
    CU_TEST(100 == lnkvars[2]);
    CU_TEST(9 == lnkvars[3]);
    CU_TEST(90 == lnkvars[4]);
  } else {
    CU_FAIL("Unexpected col for lnkmap row 9.");
  }

  slv_destroy_lnkmap(lnkmap);

  mtx_destroy(mtx);

  CU_TEST(test_meminuse == ascmeminuse());

/*

extern int slv_direct_solve(slv_system_t server,
                            struct rel_relation *rel,
                            struct var_variable *var,
                            FILE *file,
                            real64 epsilon,
                            int ignore_bounds,
                            int scaled);
*<
 *  Attempts to directly solve the given relation (equality constraint) for
 *  the given variable, leaving the others fixed.  Returns an integer
 *  signifying the status as one of the following three:
 *  <pre>
 *     0  ==>  Unable to determine anything.
 *             Not symbolically invertible.
 *     1  ==>  Solution(s) found.
 *             Variable value set to first found if more than one.
 *    -1  ==>  No solution found.
 *             Function invertible, but no solution exists satisfying
 *             var bounds (if active) and the epsilon given.
 *  </pre>
 *  The variable bounds will be upheld, unless ignore_bounds=FALSE.
 *  Residual testing will be against epsilon and either scaled or
 *  unscaled residual according to scaled (no scale -> 0).
 *  If file != NULL and there are leftover possible solutions, we
 *  will write about them to file.
 *
 *  @param server        The slv_system_t (mostly ignored).
 *  @param rel           The relation to attempt to solve.
 *  @param var           The variable for which to solve.
 *  @param file          File stream to receive other possible solutions.
 *  @param epsilon       Tolerance for testing convergence.
 *  @param ignore_bounds If TRUE, ignore bounds on variable.
 *  @param scaled        If TRUE, test scaled residuals against epsilon.


extern int slv_direct_log_solve(slv_system_t sys,
                                struct logrel_relation *lrel,
                                struct dis_discrete *dvar,
                                FILE *file,
                                int perturb,
                                struct gl_list_t *instances);
*<
 *  Attempt to directly solve the given logrelation for the given
 *  discrete variable, leaving the others fixed.  Returns an integer
 *  signifying the status as one of the following three:
 *  <pre>
 *     0  ==>  Unable to determine anything. Bad logrelation or dvar
 *     1  ==>  Solution found.
 *     2  ==>  More than one solution found. It does not modify the value
 *             of dvar. Conflicting.
 *    -1  ==>  No solution found. Inconsistency
 *  </pre>
 *  If file != NULL and there are leftover possible solutions, we
 *  will write about them to file.
 *  The flag perturb and the gl_list are used to change the truth
 *  value of some boundaries. This is sometimes useful in
 *  conditional modeling.
 *
 *  @param sys        The slv_system_t (mostly ignored).
 *  @param lrel       The logical relation to attempt to solve.
 *  @param dvar       The discrete variable for which to solve.
 *  @param file       File stream to receive other possible solutions.
 *  @param perturb    If TRUE, perturbs the truth values if necessary to find the solution.
 *  @param instances  List of instances.

 */

  if (TRUE == i_initialized_lists) {          /* clean up list system if necessary */
    gl_destroy_pool();
  }
  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo slv_common_test_list[] = {
  {"test_slv_common", test_slv_common},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_solver_slv_common", NULL, NULL, slv_common_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_solver_slv_common(void)
{
  return CU_register_suites(suites);
}
