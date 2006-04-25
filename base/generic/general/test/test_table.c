/*
 *  Unit test functions for ASCEND: general/table.c
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
#include <general/table.h>
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_table.h"

/* transform function used in test_list(). */
static
void mult_by_2(VOIDPTR p)
{
  if (NULL != p)
    *((unsigned long*)p) = *((unsigned long*)p) * 2;
}

/* transform function used in test_list(). */
static
void mult_by_2_and_add(VOIDPTR p, VOIDPTR add)
{
  if ((NULL != p) && (NULL != add))
    *((unsigned long*)p) = *((unsigned long*)p) * 2 + *((unsigned long*)add);
}


static void test_table(void)
{
  struct Table *p_table1;
  unsigned long i;
  unsigned long incr;
  unsigned long *pint_array[50];
  char str50[50];
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NDEBUG
  CU_FAIL("test_table() compiled with NDEBUG - some features not tested.");
#endif
#ifndef MALLOC_DEBUG
  CU_FAIL("test_table() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /*  NOTE:  Each test section assumes that
   *    1. the local Table* have been destroyed
   *    2. pint_array[0..49] is allocated and initialized to [0..49]
   *
   *  If a test section messes with any of these, then it must restore
   *  this state before finishing.
   */

  for (i=0 ; i<50 ; ++i) {              /* create some test data */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }

  /* test CreateTable(), TableDestroy() */

  p_table1 = CreateTable(0);              /* create a table having initial capacity = 0 */
  CU_TEST(0 == TableSize(p_table1));
  CU_TEST(0 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

#ifdef MALLOC_DEBUG
  CU_TEST(0 != AllocatedMemory((VOIDPTR)p_table1, sizeof(VOIDPTR)));
#endif

  DestroyTable(p_table1, TRUE);          /* destroy the table and check for deallocation */
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory((VOIDPTR)p_table1, sizeof(VOIDPTR)));
#endif

  p_table1 = CreateTable(31);            /* create a more typical table */
  CU_TEST(0 == TableSize(p_table1));
  CU_TEST(31 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  for (i=0 ; i<10 ; ++i) {              /* fill the list with data */
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  DestroyTable(p_table1, FALSE);        /* clean up, leaving data in tact */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that data is still in tact */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  p_table1 = CreateTable(31);           /* create the table again, fill it with some of the data */
  for (i=0 ; i<10 ; ++i) {
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  DestroyTable(p_table1, TRUE);         /* clean up, deallocating data stored in list */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* check that some data was deallocated */
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

  /* test AddTableData(), LookupTableData(), RemoveTableData() */

  p_table1 = CreateTable(13);           /* create and fill a table */

  AddTableData(p_table1, NULL, "NULL data");      /* add a NULL data item */
  CU_TEST(1 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AddTableData(NULL, pint_array[0], str50);     /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AddTableData(p_table1, pint_array[0], NULL);  /* error if NULL id */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)LookupTableData(NULL, str50);          /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)LookupTableData(p_table1, NULL);        /* error if NULL id */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)RemoveTableData(NULL, str50);          /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)RemoveTableData(p_table1, NULL);        /* error if NULL id */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  AddTableData(p_table1, pint_array[10], "NULL data");     /* can't add item with a duplicate id */
  CU_TEST(1 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  CU_TEST(NULL == LookupTableData(p_table1, "NULL data")); /* look up NULL item */
  CU_TEST(1 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  CU_TEST(NULL == RemoveTableData(p_table1, "NULL data")); /* remove NULL item */
  CU_TEST(0 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  for (i=0 ; i<50 ; ++i) {             /* add data to the table */
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }
  CU_TEST(50 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(pint_array[49] == TableLastFind(p_table1));

  for (i=0 ; i<50 ; ++i) {             /* look up data in the table */
    sprintf(str50, "string_%lu", i);
    CU_TEST(pint_array[i] == (unsigned long*)LookupTableData(p_table1, str50));
    CU_TEST(*pint_array[i] == *((unsigned long*)LookupTableData(p_table1, str50)));
  }

  sprintf(str50, "string_%d", 0);    /* remove the first item added */
  CU_TEST(pint_array[0] == (unsigned long*)RemoveTableData(p_table1, str50));
  CU_TEST(49 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(pint_array[49] == TableLastFind(p_table1));

  sprintf(str50, "string_%d", 49);   /* remove the last item added */
  CU_TEST(pint_array[49] == (unsigned long*)RemoveTableData(p_table1, str50));
  CU_TEST(48 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  sprintf(str50, "string_%d", 27);   /* remove an item in the middle */
  CU_TEST(pint_array[27] == (unsigned long*)RemoveTableData(p_table1, str50));
  CU_TEST(47 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  DestroyTable(p_table1, FALSE);       /* clean up, deallocating data stored in list */

  /* test TableApplyOne() */

  p_table1 = CreateTable(13);           /* create and fill a table */

  for (i=0 ; i<50 ; ++i) {
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyOne(NULL, mult_by_2, str50);       /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyOne(p_table1, NULL, str50);        /* error if NULL applyfunc */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyOne(p_table1, mult_by_2, NULL);     /* error if NULL id */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  sprintf(str50, "string_%d", 49);     /* apply to the last item added */
  TableApplyOne(p_table1, mult_by_2, str50);
  CU_TEST(98 == *(unsigned long*)LookupTableData(p_table1, str50));
  CU_TEST(98 == *pint_array[49]);

  sprintf(str50, "string_%d", 0);      /* apply to the first item added */
  TableApplyOne(p_table1, mult_by_2, str50);
  CU_TEST(0 == *(unsigned long*)LookupTableData(p_table1, str50));
  CU_TEST(0 == *pint_array[0]);

  sprintf(str50, "string_%d", 31);     /* apply to something in the middle */
  TableApplyOne(p_table1, mult_by_2, str50);
  CU_TEST(62 == *(unsigned long*)LookupTableData(p_table1, str50));
  CU_TEST(62 == *pint_array[31]);

   for (i=0 ; i<50 ; ++i) {            /* restore the test data */
      *pint_array[i] = i;
  }

  DestroyTable(p_table1, FALSE);        /* clean up, deallocating data stored in list */

  /* test TableApplyAll() */

  p_table1 = CreateTable(13);           /* create and fill a table */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyAll(NULL, mult_by_2);     /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyAll(p_table1, NULL);      /* error if NULL applyfunc */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  TableApplyAll(p_table1, mult_by_2);   /* applying function to empty table */

  for (i=0 ; i<50 ; ++i) {             /* add data */
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }

  TableApplyAll(p_table1, mult_by_2);
  for (i=0 ; i<50 ; ++i) {
    sprintf(str50, "string_%lu", i);
    CU_TEST((i*2) == *(unsigned long*)LookupTableData(p_table1, str50));
    CU_TEST((i*2) == *pint_array[i]);
  }

   for (i=0 ; i<50 ; ++i) {            /* restore the test data */
      *pint_array[i] = i;
  }

  DestroyTable(p_table1, FALSE);        /* clean up, deallocating data stored in list */

  /* test TableApplyAllTwo() */

  p_table1 = CreateTable(31);           /* create and fill a table */
  incr = 10;

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyAllTwo(NULL, mult_by_2_and_add, &incr);  /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TableApplyAllTwo(p_table1, NULL, &incr);           /* error if NULL applyfunc */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  TableApplyAllTwo(p_table1, mult_by_2_and_add, &incr); /* applying function to empty table */

  for (i=0 ; i<50 ; ++i) {             /* add data */
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
  }

  TableApplyAllTwo(p_table1, mult_by_2_and_add, &incr);
  for (i=0 ; i<50 ; ++i) {
    sprintf(str50, "string_%lu", i);
    CU_TEST(((i*2)+10) == *(unsigned long*)LookupTableData(p_table1, str50));
    CU_TEST(((i*2)+10) == *pint_array[i]);
  }

   for (i=0 ; i<50 ; ++i) {            /* restore the test data */
      *pint_array[i] = i;
  }

  DestroyTable(p_table1, FALSE);        /* clean up, deallocating data stored in list */

  /* test TableSize(), TableHashSize(), TableLastFind() */

  p_table1 = CreateTable(13);           /* create a table */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)TableSize(NULL);              /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)TableHashSize(NULL);          /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)TableLastFind(NULL);          /* error if NULL table */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 == TableSize(p_table1));
  CU_TEST(13 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  for (i=0 ; i<10 ; ++i) {
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
    CU_TEST((i+1) == TableSize(p_table1));
    CU_TEST(pint_array[i] == (unsigned long*)TableLastFind(p_table1));
  }

  for (i=0 ; i<10 ; ++i) {
    sprintf(str50, "string_%lu", i);
    CU_TEST(pint_array[i] == (unsigned long*)LookupTableData(p_table1, str50));
    CU_TEST(pint_array[i] == (unsigned long*)TableLastFind(p_table1));
  }

  DestroyTable(p_table1, FALSE);        /* clean up, deallocating data stored in list */

  p_table1 = CreateTable(1);            /* create a table */

  CU_TEST(0 == TableSize(p_table1));
  CU_TEST(1 == TableHashSize(p_table1));
  CU_TEST(NULL == TableLastFind(p_table1));

  for (i=0 ; i<10 ; ++i) {
    sprintf(str50, "string_%lu", i);
    AddTableData(p_table1, pint_array[i], str50);
    CU_TEST((i+1) == TableSize(p_table1));
    CU_TEST(pint_array[i] == (unsigned long*)TableLastFind(p_table1));
  }

  for (i=0 ; i<10 ; ++i) {
    sprintf(str50, "string_%lu", i);
    CU_TEST(pint_array[i] == (unsigned long*)LookupTableData(p_table1, str50));
    CU_TEST(pint_array[i] == (unsigned long*)TableLastFind(p_table1));
  }

  DestroyTable(p_table1, FALSE);        /* clean up, deallocating data stored in list */

  /* not tested - PrintTable() */

  /* clean up and exit */
  for (i=0 ; i<50 ; ++i)
    ascfree(pint_array[i]);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo table_test_list[] = {
  {"test_table", test_table},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_table", NULL, NULL, table_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_table(void)
{
  return CU_register_suites(suites);
}
