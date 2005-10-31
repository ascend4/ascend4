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
#ifdef __WIN32__
#include <io.h>
#endif
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "solver/slv_types.h"
#include "solver/rel.h"
#include "solver/logrel.h"
#include "solver/mtx.h"
#include "general/list.h"
#include "solver/slv_common.h"
#include "CUnit/CUnit.h"
#include "test_slv_common.h"
#include "assertimpl.h"

/*
 *  Initializes a vector_data structure.
 *  The new range (low..high) is considered proper if both low and
 *  high are zero or positive, and (low <= high).  If the new range is
 *  not proper (or if vec itself is NULL), then no modifications are
 *  made to vec.<br><br>
 *
 *  If the range is proper then vec->rng is allocated if NULL and then
 *  set using low and high.  Then vec->vec is allocated (if NULL) or
 *  reallocated to size (high+1).  The data in vec->vec is not
 *  initialized or changed.  The member vec->accurate is set to FALSE.
 *
 *  @param vec  Pointer to the vector_data to initialize.
 *  @param low  The lower bound of the vector's range.
 *  @param high The upper bound of the vector's range.
 *  @return Returns 0 if the vector is initialized successfully,
 *          1 if an improper range was specified, 2 if vec is NULL,
 *          and 3 if memory cannot be allocated.
 */
static int init_vector(struct vector_data *vec, int32 low, int32 high)
{
  int32 new_size;

  if ((low < 0) || (high < low))
    return 1;

  if (NULL == vec)
    return 2;

  if (NULL == vec->rng) {
    vec->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
    if (NULL == vec->rng)
      return 3;
  }
  vec->rng = mtx_range(vec->rng, low, high);

  new_size = high + 1;
  if (NULL == vec->vec) {
    vec->vec = (real64 *)ascmalloc((new_size)*sizeof(real64));
    if (NULL == vec->vec) {
      ascfree(vec->rng);
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

/*
 *  Returns a new vector_data initialized to the specified range.
 *  This function creates, initializes, and returns a new vector_data
 *  structure.  The vector is initialized using init_vector() and
 *  a pointer to the new struct is returned.  If the specified range
 *  is improper (see init_vector()) then a valid vector cannot be 
 *  created and NULL is returned.<br><br>
 *
 *  Destruction of the returned vector_data is the responsibility of
 *  the caller.  destroy_vector() may be used for this purpose.
 *
 *  @param low  The lower bound of the vector's range.
 *  @param high The upper bound of the vector's range.
 *  @return A new initialized vector_data, or NULL if one could
 *          not be created.
 */
static struct vector_data *create_vector(int32 low, int32 high)
{
  struct vector_data *result;

  result = (struct vector_data *)ascmalloc(sizeof(struct vector_data));
  if (NULL == result)
    return NULL;

  result->rng = NULL;
  result->vec = NULL;
  if (0 != init_vector(result, low, high)) {
    ascfree(result);
    result = NULL;
  }
  return result;
}

/*
 *  Destroys a vector and its assocated data.
 *  Deallocates any memory held in vec->rng and vec->vec,
 *  and then deallocates the vector itself.
 *
 *  @param vec Pointer to the vector_data to destroy.
 */
static void destroy_vector(struct vector_data *vec)
{
  if (NULL != vec) {
    if (NULL != vec->rng)
      ascfree(vec->rng);
    if (NULL != vec->vec)
      ascfree(vec->vec);
    ascfree(vec);
  }
}

/*  
 *  Independent calculation of a vector dot product.
 *  Nothing fancy, no validation of input.  Assumes valid vectors.
 */
real64 slow_dot_product(struct vector_data *vec1, struct vector_data *vec2)
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
 *  This function tests the slv_common.c functions and data structures.
 *  Note that some of the implementation declarated in slv_common.h is
 *  defined in slv.c rather than slv_common.c.  This subset of slv_common.h
 *  will be tested along with slv.c elsewhere.
 */
static void test_slv_common(void)
{
  struct vector_data *pvec1;
  struct vector_data *pvec2;
  mtx_matrix_t mtx;
  real64 rarray[100];
  int i;
  unsigned long prior_meminuse;
  unsigned long cur_meminuse;
  int i_initialized_lists = FALSE;

#ifdef NDEBUG
  CU_FAIL("test_slv_common() compiled with NDEBUG - some features not tested.");
#endif

  prior_meminuse = ascmeminuse();

  /* set up pooling & recycling */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }

  for (i=0 ; i<100 ; ++i) {                           /* create some reals to use later */
    rarray[i+1] = (real64)pow(i+1, i+1);
  }

  /* test create_vector(), destroy_vector() */

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(-1, 0);                       /* error - low < 0 */
  CU_TEST(NULL == pvec1);

  destroy_vector(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(0, -1);                       /* error - high < 0 */
  CU_TEST(NULL == pvec1);

  destroy_vector(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(10, 0);                       /* error - low > high */
  CU_TEST(NULL == pvec1);

  destroy_vector(pvec1);
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(0, 0);                        /* ok - low == high */
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(0 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, sizeof(real64)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(0, 10);                       /* ok - low < high */
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(0 == pvec1->rng->low);
  CU_TEST(10 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  /* test init_vector() */

  cur_meminuse = ascmeminuse();
  CU_TEST(2 == init_vector(NULL, 0, 10));             /* error - NULL vec */
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(1 == init_vector(pvec1, -1, 10));           /* error - low < 0 */
  CU_TEST(NULL == pvec1->rng);
  CU_TEST(NULL == pvec1->vec);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(1 == init_vector(pvec1, 10, -1));           /* error - high < 0 */
  CU_TEST(NULL == pvec1->rng);
  CU_TEST(NULL == pvec1->vec);
  CU_TEST(TRUE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(0 == init_vector(pvec1, 10, 10));           /* ok - low == high */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(10 == pvec1->rng->low);
  CU_TEST(10 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 11 * sizeof(real64)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng, vec */
  CU_TEST_FATAL(NULL != pvec1);
  pvec1->rng = NULL;
  pvec1->vec = NULL;
  pvec1->accurate = TRUE;

  CU_TEST(0 == init_vector(pvec1, 10, 100));          /* ok - low < high */
  CU_TEST_FATAL(NULL != pvec1->rng);
  CU_TEST(10 == pvec1->rng->low);
  CU_TEST(100 == pvec1->rng->high);
  CU_TEST(NULL != pvec1->vec);
  CU_TEST(FALSE == pvec1->accurate);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 101 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 101 * sizeof(real64)));
#endif

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  cur_meminuse = ascmeminuse();
  pvec1 = create_vector(0,0);                         /* create a vector with data */
  CU_TEST_FATAL(NULL != pvec1);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(2 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(2 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#else
  CU_TEST(1 == AllocatedMemory(pvec1, sizeof(struct vector_data)));
  CU_TEST(1 == AllocatedMemory(pvec1->rng, sizeof(mtx_range_t)));
  CU_TEST(1 == AllocatedMemory(pvec1->vec, 1 * sizeof(real64)));
#endif

  pvec1->accurate = TRUE;
  pvec1->vec[0] = rarray[0];

  CU_TEST(1 == init_vector(pvec1, -1, 100));          /* error - low < 0 */
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

  CU_TEST(1 == init_vector(pvec1, 1, 0));          /* error - high < low */
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

  CU_TEST(0 == init_vector(pvec1, 0, 1));          /* ok - high > low */
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

  CU_TEST(0 == init_vector(pvec1, 9, 10));         /* ok - high > low */
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

  destroy_vector(pvec1);
  CU_TEST(0 == AllocatedMemory(pvec1, 0));
  CU_TEST(cur_meminuse == ascmeminuse());

  /* test slv_zero_vector() */

//#ifndef ASC_NO_ASSERTIONS
//  asc_assert_catch(TRUE);                        /* prepare to test assertions */

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(NULL);                       /* error - NULL vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng */
//  CU_TEST_FATAL(NULL != pvec1);
//  pvec1->rng = NULL;
//  pvec1->vec = (real64 *)ascmalloc(10 * sizeof(real64));

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec1);                      /* error - NULL vec->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
//  ascfree(pvec1->vec);
//  pvec1->vec = NULL;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec1);                      /* error - NULL vec->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->vec = (real64 *)ascmalloc(10 * sizeof(real64));
//  pvec1->rng->low = -1;
//  pvec1->rng->high = 10;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec1);                      /* error - low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->rng->low = 11;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec1);                      /* error - low > high */
//  CU_TEST(TRUE == asc_assert_failed());

//  destroy_vector(pvec1);

//  asc_assert_catch(FALSE);                       /* done testing assertions */
//#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = create_vector(0,0);                     /* create & initialize a 1-element vector */
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = rarray[0];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);

  slv_zero_vector(pvec1);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], 0.0, 0.00001);

  CU_TEST_FATAL(0 == init_vector(pvec1, 0, 9));   /* redimension to larger vector */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  slv_zero_vector(pvec1);
  for (i=0 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], 0.0, 0.00001);  /* all data should now be 0.0 */
  }

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize again */
  }

  pvec1->rng->low = 5;
  pvec1->rng->high = 7;

  slv_zero_vector(pvec1);
  for (i=0 ; i<5 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }
  for (i=5 ; i<8 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], 0.0, 0.00001);
  }
  for (i=8 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  destroy_vector(pvec1);

  /* test slv_copy_vector() */

//#ifndef ASC_NO_ASSERTIONS
//  asc_assert_catch(TRUE);                        /* prepare to test assertions */

//  pvec1 = create_vector(0,10);
//  CU_TEST_FATAL(NULL != pvec1);

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(NULL, pvec1);                /* error - NULL srcvec */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec1, NULL);                /* error - NULL destvec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng */
//  CU_TEST_FATAL(NULL != pvec2);
//  pvec2->rng = NULL;
//  pvec2->vec = (real64 *)ascmalloc(10 * sizeof(real64));

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec2, pvec1);              /* error - NULL srcvec->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec1, pvec2);              /* error - NULL destvec->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
//  ascfree(pvec2->vec);
//  pvec2->vec = NULL;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec2, pvec1);              /* error - NULL srcvec->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec1, pvec2);              /* error - NULL destvec->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->vec = (real64 *)ascmalloc(10 * sizeof(real64));
//  pvec2->rng->low = -1;
//  pvec2->rng->high = 10;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec2, pvec1);              /* error - srcvec->rng->low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_zero_vector(pvec1, pvec2);              /* error - destvec->rng->low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->rng->low = 11;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_copy_vector(pvec2, pvec1);              /* error - srcvec low > high */
//  CU_TEST(TRUE == asc_assert_failed());

//  destroy_vector(pvec1);
//  destroy_vector(pvec2);

//  asc_assert_catch(FALSE);                       /* done testing assertions */
//#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = create_vector(0,0);                     /* create & initialize a 1-element vectors */
  pvec2 = create_vector(0,0);
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = rarray[0];
  pvec2->vec[0] = rarray[5];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  slv_copy_vector(pvec1, pvec2);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[0], 0.00001);

  CU_TEST_FATAL(0 == init_vector(pvec1, 0, 9));   /* redimension pvec1 to larger vector */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);
  }

  pvec2->vec[0] = rarray[8];
  slv_copy_vector(pvec2, pvec1);                  /* copy 1 element*/
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[8], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[8], 0.00001);
  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* rest of data should be intact */
  }

  pvec2->vec[0] = rarray[3];
  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  slv_copy_vector(pvec1, pvec2);                  /* copy 1 element other way*/
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[9], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[8], 0.00001);
  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in src should be intact */
  }

  CU_TEST_FATAL(0 == init_vector(pvec2, 0, 9));   /* redimension pvec2 to larger vector */
  slv_zero_vector(pvec2);                         /* zero the destvec */
  pvec1->rng->low = 0;
  pvec1->rng->high = 9;
  slv_copy_vector(pvec1, pvec2);                  /* copy all elements */
  for (i=0 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], pvec2->vec[i], 0.00001);  /* data should be the same */
  }

  for (i=0 ; i<10 ; ++i) {
    pvec2->vec[i] = rarray[9-i];                  /* reinitialize & check the data */
  }
  pvec2->rng->low = 3;
  pvec2->rng->high = 6;
  slv_copy_vector(pvec2, pvec1);                  /* copy a subset of elements to start of destvec */
  for (i=3 ; i<7 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i-3], rarray[9-i], 0.00001);  /* data should be the same */
  }
  for (i=4 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data should be the same */
  }

  destroy_vector(pvec1);
  destroy_vector(pvec2);
         
  /* test slv_inner_product() */

//#ifndef ASC_NO_ASSERTIONS
//  asc_assert_catch(TRUE);                        /* prepare to test assertions */

//  pvec1 = create_vector(0,10);
//  CU_TEST_FATAL(NULL != pvec1);

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(NULL, pvec1);              /* error - NULL vec1 */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec1, NULL);              /* error - NULL vec2 */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng */
//  CU_TEST_FATAL(NULL != pvec2);
//  pvec2->rng = NULL;
//  pvec2->vec = (real64 *)ascmalloc(10 * sizeof(real64));

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec2, pvec1);              /* error - NULL vec1->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec1, pvec2);              /* error - NULL vec2->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
//  ascfree(pvec2->vec);
//  pvec2->vec = NULL;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec2, pvec1);              /* error - NULL vec1->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec1, pvec2);              /* error - NULL vec2->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->vec = (real64 *)ascmalloc(10 * sizeof(real64));
//  pvec2->rng->low = -1;
//  pvec2->rng->high = 10;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec2, pvec1);              /* error - vec1->rng->low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec1, pvec2);              /* error - vec2->rng->low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec2->rng->low = 11;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_inner_product(pvec2, pvec1);             /* error - vec1 low > high */
//  CU_TEST(TRUE == asc_assert_failed());

//  destroy_vector(pvec1);
//  destroy_vector(pvec2);

//  asc_assert_catch(FALSE);                       /* done testing assertions */
//#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = create_vector(0,0);                     /* create & initialize a 1-element vectors */
  pvec2 = create_vector(0,0);
  CU_TEST_FATAL(NULL != pvec1);
  CU_TEST_FATAL(NULL != pvec2);

  pvec1->vec[0] = rarray[0];
  pvec2->vec[0] = rarray[5];
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec1, pvec2), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec2, pvec1), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[0], 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[0], rarray[5], 0.00001);

  CU_TEST_FATAL(0 == init_vector(pvec1, 0, 9));   /* redimension vectors larger */
  CU_TEST_FATAL(0 == init_vector(pvec2, 0, 9));

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    pvec2->vec[i] = 2.0;
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec1, pvec2), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec2, pvec1), slow_dot_product(pvec1, pvec2), 0.00001);

  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  pvec2->rng->low = 5;
  pvec2->rng->high = 5;                           /* check 1 element subrange */
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec1, pvec2), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec2, pvec1), slow_dot_product(pvec1, pvec2), 0.00001);

  pvec1->rng->low = 0;
  pvec1->rng->high = 3;
  pvec2->rng->low = 2;
  pvec2->rng->high = 5;                           /* check 4 element subrange */
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec1, pvec2), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec2, pvec1), slow_dot_product(pvec1, pvec2), 0.00001);

  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in vecs should be intact */
    CU_ASSERT_DOUBLE_EQUAL(pvec2->vec[i], 2.0, 0.00001);
  }

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize & check the data */
    pvec2->vec[i] = rarray[9-i];
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec1, pvec2), slow_dot_product(pvec1, pvec2), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(slv_inner_product(pvec2, pvec1), slow_dot_product(pvec1, pvec2), 0.00001);

  destroy_vector(pvec1);
  destroy_vector(pvec2);

  /* test slv_square_norm() */

//#ifndef ASC_NO_ASSERTIONS
//  asc_assert_catch(TRUE);                       /* prepare to test assertions */

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_square_norm(NULL);                      /* error - NULL vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1 = (struct vector_data *)ascmalloc(sizeof(struct vector_data));  /* create a vector with NULL rng */
//  CU_TEST_FATAL(NULL != pvec1);
//  pvec1->rng = NULL;
//  pvec1->vec = (real64 *)ascmalloc(10 * sizeof(real64));

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_square_norm(pvec1);                     /* error - NULL vec->rng */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->rng = (mtx_range_t *)ascmalloc(sizeof(mtx_range_t));
//  ascfree(pvec1->vec);
//  pvec1->vec = NULL;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_square_norm(pvec1);                     /* error - NULL vec->vec */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->vec = (real64 *)ascmalloc(10 * sizeof(real64));
//  pvec1->rng->low = -1;
//  pvec1->rng->high = 10;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_square_norm(pvec1);                     /* error - vec->rng->low < 0 */
//  CU_TEST(TRUE == asc_assert_failed());

//  pvec1->rng->low = 11;

//  asc_assert_reset();
//  if (0 == setjmp(g_asc_test_env))
//    slv_square_norm(pvec1);                     /* error - vec low > high */
//  CU_TEST(TRUE == asc_assert_failed());

//  destroy_vector(pvec1);

//  asc_assert_catch(FALSE);                       /* done testing assertions */
//#endif    /* !ASC_NO_ASSERTIONS */

  pvec1 = create_vector(0,0);                     /* create & initialize a 1-element vector */
  CU_TEST_FATAL(NULL != pvec1);

  pvec1->vec[0] = 0.0;
  CU_ASSERT_DOUBLE_EQUAL(slv_square_norm(pvec1), slow_dot_product(pvec1, pvec1), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], 0.0, 0.00001);

  pvec1->vec[0] = rarray[7];
  CU_ASSERT_DOUBLE_EQUAL(slv_square_norm(pvec1), slow_dot_product(pvec1, pvec1), 0.00001);
  CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[0], rarray[7], 0.00001);

  CU_TEST_FATAL(0 == init_vector(pvec1, 0, 9));   /* redimension vectors larger */

  for (i=0 ; i<10 ; ++i) {
    pvec1->vec[i] = rarray[i];                    /* initialize the data */
  }
                                                  /* check entire vectors */
  CU_ASSERT_DOUBLE_EQUAL(slv_square_norm(pvec1), slow_dot_product(pvec1, pvec1), 0.00001);

  pvec1->rng->low = 9;
  pvec1->rng->high = 9;
  CU_ASSERT_DOUBLE_EQUAL(slv_square_norm(pvec1), slow_dot_product(pvec1, pvec1), 0.00001);

  pvec1->rng->low = 0;
  pvec1->rng->high = 3;
  CU_ASSERT_DOUBLE_EQUAL(slv_square_norm(pvec1), slow_dot_product(pvec1, pvec1), 0.00001);

  for (i=1 ; i<10 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL(pvec1->vec[i], rarray[i], 0.00001);  /* data in vecs should be intact */
  }
slv_write_vector(stdout, pvec1);
  destroy_vector(pvec1);

  /* test slv_matrix_product() */

  mtx = mtx_create();
  
  
  mtx_destroy(mtx);

/*

extern void slv_matrix_product(mtx_matrix_t mtx,
                               struct vector_data *vec,
                               struct vector_data *prod,
                               real64 scale,
                               boolean transpose);

 *  Calculates the product of a vector, matrix, and scale factor.
 *  Stores prod := (scale)*(mtx)*(vec) if transpose = FALSE,
 *  or prod := (scale)*(mtx-transpose)(vec) if transpose = TRUE.
 *  vec and prod must be completely different.
 *  If (!transpose) vec->vec is assumed indexed by current col and
 *                 prod->vec is indexed by current row of mtx.
 *  If (transpose) vec->vec is assumed indexed by current row and
 *                 prod->vec is indexed by current col of mtx.
 *  The following are not allowed and are checked by assertion:
 *    - NULL mtx
 *    - NULL vec
 *    - NULL vec->rng
 *    - NULL vec->vec
 *    - vec->rng->low < 0
 *    - vec->rng->low > vec->rng->high
 *    - NULL prod
 *    - NULL prod->rng
 *    - NULL prod->vec
 *    - prod->rng->low < 0
 *    - prod->rng->low > prod->rng->high
 *
 *  @param mtx       The matrix for the product.
 *  @param vec       The vector for the product.
 *  @param prod      The vector to receive the matrix product.
 *  @param scale     The scale factor by which to multiply the matrix product.
 *  @param transpose Flag for whether to use mtx or its transpose.
 *
 *  @todo solver/slv_common:slv_mtx_product needs attention -
 *        does it go into mtx?


extern void slv_write_vector(FILE *fp, struct vector_data *vec);
 *  Write vector information to a file stream.
 *  Prints general information about the vector followed by the
 *  values in the range of the vector to file fp.
 *
 *  @param fp  The file stream to receive the report.
 *  @param vec The vector on which to report.


extern real64 slv_dot(int32 len, const real64 *a1, const real64 *a2);

 *  Calculates the dot product of 2 arrays of real64.
 *  This is an optimized routine (loop unrolled).  It takes
 *  advantage of identical vectors.  The 2 arrays must have
 *  at least len elements.
 *  The following are not allowed and are checked by assertion:
 *    - NULL a1
 *    - NULL a2
 *    - len < 0
 *
 *  The same algorithm is used inside slv_inner_product(), so there
 *  is no need to use this function directly if you are using the
 *  vector_data type.
 *
 *  @param len The length of the 2 arrays.
 *  @param a1  The 1st array for the dot product.
 *  @param a2  The 2nd array for the dot product.
 *  @param a2  The 2nd array for the dot product.

 * --------------------------------
 *  General input/output routines
 * --------------------------------

extern FILE *slv_get_output_file(FILE *fp);
*<
 *  Checks a file pointer, and if NULL returns a pointer to the nul device.
 *  If you are in environment that doesn't have something like
 *  /dev/null (nul on Windows), you'd better be damn sure your 
 *  sys->p.output.*_important are not NULL.
 *
 *  @param fp The file stream to check.
 *  @return fp if it is not NULL, a pointer to the nul device otherwise.
 


 * FILE pointer macros.
 *     fp = MIF(sys)
 *     fp = LIF(sys)
 *     fp = PMIF(sys)
 *     fp = PLIF(sys)
 *     or fprintf(MIF(sys),"stuff",data...);
 *  Use of these is requested on grounds of readability but not required.
 *  MIF and LIF are macros, which means any specific solver interface
 *  to ASCEND can use them, since all interfaces are supposed to
 *  support a parameters structure p somewhere in a larger system
 *  structure (sys) they keep privately.
 *  Use the PMIF or PLIF flavors if the parameters sys->p is a pointer
 *  rather than a in-struct member.
 
#define MIF(sys) slv_get_output_file( (sys)->p.output.more_important )
*<
 *  Retrieve the "more important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "more important" output file for sys.
 
#define LIF(sys) slv_get_output_file( (sys)->p.output.less_important )
*<
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "less important" output file for sys.
 
#define PMIF(sys) slv_get_output_file( (sys)->p->output.more_important )
*<
 *  Retrieve the "more important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "more important" output file for sys.
 
#define PLIF(sys) slv_get_output_file( (sys)->p->output.less_important )
*<
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "less important" output file for sys.
 

------------------- begin compiler dependent functions -------------------
#if SLV_INSTANCES

#ifdef NEWSTUFF
extern void slv_print_obj_name(FILE *outfile, obj_objective_t obj);
*<
 *  Not implemented.
 *  Prints the name of obj to outfile.  If obj_make_name() can't
 *  generate a name, the global index is printed instead.
 *  @todo Implement solver/slv_common:slv_print_obj_name() or remove prototype.
 
#endif
extern void slv_print_rel_name(FILE *outfile,
                               slv_system_t sys,
                               struct rel_relation *rel);
*<
 *  Prints the name of rel to outfile.  If rel_make_name() can't
 *  generate a name, the global index is printed instead.
 
extern void slv_print_var_name(FILE *outfile,
                               slv_system_t sys,
                               struct var_variable *var);
*<
 *  Prints the name of var to outfile. If var_make_name() can't
 *  generate a name, the global index is printed instead.
 
extern void slv_print_logrel_name(FILE *outfile,
                                  slv_system_t sys,
                                  struct logrel_relation *lrel);
*<
 *  Prints the name of lrel to outfile. If logrel_make_name() can't
 *  generate a name, the global index is printed instead.
 
extern void slv_print_dis_name(FILE *outfile,
                               slv_system_t sys,
                               struct dis_discrete *dvar);
*<
 *  Prints the name of dvar to outfile. If dis_make_name() can't
 *  generate a name, the global index is printed instead.
 

#ifdef NEWSTUFF
extern void slv_print_obj_index(FILE *outfile, obj_objective_t obj);
*<
 *  Not implemented.
 *  Prints the index of obj to outfile.
 *  @todo Implement solver/slv_common:slv_print_obj_index() or remove prototype.
 
#endif
extern void slv_print_rel_sindex(FILE *outfile, struct rel_relation *rel);
*<  Prints the index of rel to outfile. 
extern void slv_print_var_sindex(FILE *outfile, struct var_variable *var);
*<  Prints the index of var to outfile. 
extern void slv_print_logrel_sindex(FILE *outfile, struct logrel_relation *lrel);
*<  Prints the index of lrel to outfile. 
extern void slv_print_dis_sindex(FILE *outfile, struct dis_discrete *dvar);
*<  Prints the index of dvar to outfile. 

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
 

#endif
-------------------- END compiler dependent functions --------------------


 * --------------------
 *  lnkmap functions
 * --------------------
 

extern int32 **slv_create_lnkmap(int32 m, int32 n, int32 hl, int32 *hi, int32 *hj);
*<
 *  Builds a row biased mapping array from the hi,hj lists given.
 *  The map returned has the following format:
 *    - map[i] is a vector describing the incidence in row i of the matrix.
 *    - Let vars=map[i], where vars is int32 *.
 *    - vars[0]=number of incidences in the relation.
 *    - For all 0<=k<vars[0]
 *       - vars[2*k+1]= original column index of some var in the eqn.
 *       - vars[2*k+2]= the lnk list index of element(i,vars[2*k+1])
 *
 *  The map should only be deallocated by destroy_lnkmap().
 *  The memory allocation for a lnkmap is done efficiently.<br><br>
 *
 *  These create an odd compressed row mapping, given the hi and hj
 *  subscript vectors. The primary utility of the lnkmap is that
 *  it can be traversed rapidly when one wants to conditionally map a row of
 *  a Harwell style (arbitrarily ordered) link representation
 *  back into another representation where adding elements to a row
 *  is easily done.<br><br>
 *
 *  hi and hj should specify a unique incidence pattern, that is no
 *  duplicate elements are allowed.  Rowindex and colindex refer to
 *  the data in hi,hj.
 *
 *  @param m  The number of rows expected. The map returned will be this long.
 *  @param n  The number of columns expected.
 *  @param hl The length of hi and hj.
 *  @param hi The eqn indices of a C numbered sparse matrix list.
 *  @param hj The var indices of a C numbered sparse matrix list.
 

extern int32 **slv_lnkmap_from_mtx(mtx_matrix_t mtx, int32 len, int32 m);
*<
 *  Generates a map from a matrix.
 *  Empty rows and columns are allowed in the matrix.
 *
 *  @param mtx  The matrix to map.
 *  @param m    The number of rows expected. The map returned will be this long.
 *  @param len  The number of nonzeros in mtx.
 *
 *  @see slv_create_lnkmap()
 

extern void slv_destroy_lnkmap(int32 **map);
*<
 *  Deallocate a map created by slv_create_lnkmap() or slv_destroy_lnkmap().
 *  destroy_lnkmap() will tolerate a NULL map as input.
 *
 *  @param map The lnkmap to destroy.
 

extern void slv_write_lnkmap(FILE *fp, int m, int32 **map);
*<
 *  Prints a link map to a file.
 *  write_lnkmap() will tolerate a NULL map as input.
 *
 *  @param fp  The file stream to receive the report.
 *  @param m   The number of rows in map to print.
 *  @param map The lnkmap to print.
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
