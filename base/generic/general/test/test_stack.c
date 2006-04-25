/*
 *  Unit test functions for ASCEND: general/stack.c
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPrint.h>
#include <general/stack.h>
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_stack.h"

/* transform function used in test_stack(). */
static
void mult_by_2(VOIDPTR p)
{
  if (NULL != p)
    *((unsigned long*)p) = *((unsigned long*)p) * 2;
}

static void test_stack(void)
{
  struct gs_stack_t *p_stack1;
  unsigned long i;
  unsigned long *pint_array[20];
  unsigned long *pint;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NDEBUG
  CU_FAIL("test_stack() compiled with NDEBUG - some features not tested.");
#endif
#ifndef MALLOC_DEBUG
  CU_FAIL("test_stack() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /*  NOTE:  Each test section assumes that
   *    1. the local gs_stack_t* have been destroyed
   *    2. pint_array[0..19] is allocated and initialized to [0..19]
   *
   *  If a test section messes with any of these, then it must restore
   *  this state before finishing.
   */

  for (i=0 ; i<20 ; ++i) {              /* create some test data */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }

  /* test gs_stack_create(), gs_stack_destroy() */

  p_stack1 = gs_stack_create(0);        /* create a stack having initial capacity = 0 */
  CU_TEST(0 == gs_stack_size(p_stack1));
  CU_TEST(0 != gs_stack_empty(p_stack1));

#ifdef MALLOC_DEBUG
  CU_TEST(0 != AllocatedMemory((VOIDPTR)p_stack1, sizeof(VOIDPTR)));
#endif

  gs_stack_destroy(p_stack1, TRUE);     /* destroy the stack and check for deallocation */

#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory((VOIDPTR)p_stack1, sizeof(VOIDPTR)));
#endif

  p_stack1 = gs_stack_create(10);       /* create a new stack with capacity = 10 */
  CU_TEST(0 == gs_stack_size(p_stack1));
  CU_TEST(0 != gs_stack_empty(p_stack1));

  for (i=0 ; i<10 ; ++i) {              /* push some data onto the stack */
      gs_stack_push(p_stack1, pint_array[i]);
  }
  CU_TEST(10 == gs_stack_size(p_stack1));
  CU_TEST(0 == gs_stack_empty(p_stack1));

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gs_stack_destroy(p_stack1, FALSE);    /* clean up, leaving data in tact */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that data is still in tact */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  p_stack1 = gs_stack_create(5);        /* create the stack again, fill it with half the data */
  for (i=0 ; i<5 ; ++i) {
      gs_stack_push(p_stack1, pint_array[i]);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gs_stack_destroy(p_stack1, TRUE);     /* clean up, deallocating data stored in stack */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that some data was deallocated */
    if (i < 5) {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  for (i=5 ; i<10 ; ++i) {              /* deallocate the rest of the data */
    ascfree(pint_array[i]);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* confirm that all data is now deallocated */
    if (i < 10) {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  for (i=0 ; i<10 ; ++i) {              /* restore test array */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }
  /* test gs_stack_push(), gs_stack_pop() */

  p_stack1 = gs_stack_create(10);       /* create and fill a stack */

  gs_stack_push(p_stack1, pint_array[15]);

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gs_stack_push(NULL, pint_array[15]);/* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gs_stack_pop(NULL);           /* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  pint = (unsigned long*)gs_stack_pop(p_stack1);
  CU_TEST(pint_array[15] == pint);
  CU_TEST(*pint_array[15] == *pint);
  CU_TEST(0 != gs_stack_empty(p_stack1));   /* stack should be empty */

  CU_TEST(NULL == gs_stack_pop(p_stack1));  /* popping an empty stack should be ok */
  CU_TEST(NULL == gs_stack_pop(p_stack1));
  CU_TEST(NULL == gs_stack_pop(p_stack1));
  CU_TEST(NULL == gs_stack_pop(p_stack1));

  gs_stack_clear(p_stack1);

  for (i=0 ; i<10 ; ++i) {              /* push some data onto the stack */
      gs_stack_push(p_stack1, pint_array[i]);
  }

  for (i=0 ; i<10 ; ++i) {
    pint = (unsigned long*)gs_stack_pop(p_stack1);
    CU_TEST(pint_array[9-i] == pint);
    CU_TEST(*pint_array[9-i] == *pint);
    if (i < 9) {
      CU_TEST(0 == gs_stack_empty(p_stack1));
    }
    else {
      CU_TEST(0 != gs_stack_empty(p_stack1));
    }
  }

  CU_TEST(NULL == gs_stack_pop(p_stack1));  /* popping an empty stack should be ok */
  CU_TEST(NULL == gs_stack_pop(p_stack1));
  CU_TEST(NULL == gs_stack_pop(p_stack1));
  CU_TEST(NULL == gs_stack_pop(p_stack1));

  gs_stack_destroy(p_stack1, FALSE);    /* clean up the stack, preserving data */

  /* test gs_stack_size(), gs_stack_empty() */

  p_stack1 = gs_stack_create(10);       /* create a stack */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gs_stack_size(NULL);          /* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gs_stack_empty(NULL);         /* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 == gs_stack_size(p_stack1));
  CU_TEST(0 != gs_stack_empty(p_stack1));
  for (i=0 ; i<10 ; ++i) {
    gs_stack_push(p_stack1, pint_array[i]);
    CU_TEST((i+1) == gs_stack_size(p_stack1));
    CU_TEST(0 == gs_stack_empty(p_stack1));
  }

  gs_stack_destroy(p_stack1, FALSE);    /* clean up the stack, preserving data */

  /* test gs_stack_apply() */

  p_stack1 = gs_stack_create(10);       /* create a stack and fill with data */
  for (i=0 ; i<10 ; ++i) {
    gs_stack_push(p_stack1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gs_stack_apply(NULL, mult_by_2);    /* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gs_stack_apply(p_stack1, NULL);     /* error if NULL func* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gs_stack_apply(p_stack1, mult_by_2);  /* execute function on each data element */

  for (i=0 ; i<10 ; ++i) {
    pint = (unsigned long*)gs_stack_pop(p_stack1);
    CU_TEST((2*(9-i)) == *pint);
  }

  gs_stack_destroy(p_stack1, FALSE);    /* clean up the stack, preserving data */
  for (i=0 ; i<20 ; ++i)                /* need to restore our integer array */
    *pint_array[i] = i;

  /* test gs_stack_clear() */

  p_stack1 = gs_stack_create(0);        /* create an empty stack */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gs_stack_clear(NULL);               /* error if NULL stack* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gs_stack_clear(p_stack1);             /* reset an empty stack */
  gs_stack_destroy(p_stack1, FALSE);    /* clean up the stack, preserving data */

  p_stack1 = gs_stack_create(10);       /* create and fill a stack */
  for (i=0 ; i<10 ; ++i) {
    gs_stack_push(p_stack1, pint_array[i]);
  }

  gs_stack_clear(p_stack1);             /* reset the stack */
  CU_TEST(0 == gs_stack_size(p_stack1));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gs_stack_destroy(p_stack1, FALSE);    /* clean up the stack, preserving data */

  /* clean up and exit */
  for (i=0 ; i<20 ; ++i) {
    ascfree(pint_array[i]);
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo stack_test_list[] = {
  {"test_stack", test_stack},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_stack", NULL, NULL, stack_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_stack(void)
{
  return CU_register_suites(suites);
}
