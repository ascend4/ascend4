/*
 *  Unit test functions for ASCEND: utilities/ascSignal.c
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
#include <utilities/ascMalloc.h>
#include <utilities/ascSignal.h>
#include "CUnit/CUnit.h"
#include "test_ascSignal.h"

static jmp_buf my_jmp_buf1;

static int f_handler1_called;
static int f_handler1_sigval;
/*
 *  Signal handler for unit tests.
 *  Resets the signal handlers and sets f_handler1_called to
 *  TRUE and f_handler1_sigval to the signal type code (-1 if
 *  an unsupported sigval).  Then longjmp's using
 *  my_jmp_buf1 and the sigval.
 */
void my_handler1(int sigval)
{
  f_handler1_called = TRUE;
  Asc_SignalRecover(FALSE);
  switch (sigval)
  {
    case SIGFPE:
      f_handler1_sigval = SIGFPE;
      FPRESET;
      break;
    case SIGINT:
      f_handler1_sigval = SIGINT;
      break;
    case SIGSEGV:
      f_handler1_sigval = SIGSEGV;
      break;
    default:
      f_handler1_sigval = -1;
      break;
  }
  longjmp(my_jmp_buf1, sigval);
}

static jmp_buf my_jmp_buf2;

static int f_handler2_called;
static int f_handler2_sigval;
/*
 *  Signal handler for unit tests.
 *  Resets the signal handlers and sets f_handler1_called to
 *  TRUE and f_handler1_sigval to the signal type code (-1 if
 *  an unsupported sigval).  Then longjmp's using
 *  my_jmp_buf1 and the sigval.
 */
void my_handler2(int sigval)
{
  f_handler2_called = TRUE;
  Asc_SignalRecover(FALSE);
  switch (sigval)
  {
    case SIGFPE:
      f_handler2_sigval = SIGFPE;
      FPRESET;
      break;
    case SIGINT:
      f_handler2_sigval = SIGINT;
      break;
    case SIGSEGV:
      f_handler2_sigval = SIGSEGV;
      break;
    default:
      f_handler2_sigval = -1;
      break;
  }
  longjmp(my_jmp_buf2, sigval);
}


static void test_ascSignal(void)
{
  SigHandlerFn* old_fpe_handler = NULL;
  SigHandlerFn* old_int_handler = NULL;
  SigHandlerFn* old_seg_handler = NULL;
  SigHandlerFn* old_handler;
  volatile int signal1_caught;
  volatile int signal2_caught;
  volatile int signal3_caught;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NO_SIGNAL_TRAPS
  /* no point in testing if the functionality is disabled */
  CU_FAIL("Signal handler manager not enabled.");
#else
  
  old_fpe_handler = signal(SIGFPE, my_handler1);        /* save any pre-existing handlers */
  old_int_handler = signal(SIGINT, my_handler1);
  old_seg_handler = signal(SIGSEGV, my_handler1);

  signal(SIGFPE, my_handler1);                          /* install some pre-existing handlers */
  signal(SIGINT, SIG_DFL);
  signal(SIGSEGV, my_handler2);

  /* Asc_SignalInit(), Asc_SignalDestroy() - not much to test */

  CU_TEST(0 == Asc_SignalInit());                       /* initialize the signal manager */

  old_handler = signal(SIGFPE, SIG_DFL);                /* previously-installed handlers should still be active */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);

  Asc_SignalRecover(TRUE);

  old_handler = signal(SIGFPE, SIG_DFL);                /* handlers should have been reinstalled */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);

  Asc_SignalRecover(TRUE);

  /* test Asc_SignalPush(), Asc_SignalPop() */

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));  /* ok - supported signal, ok func */
  old_handler = signal(SIGFPE, SIG_DFL);
  CU_TEST(Asc_SignalTrap == old_handler);

  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));     /* ok - supported signal, ok func */
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(my_handler1 == old_handler);

  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, NULL));           /* NULL func - should have no effect */
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);                          /* old handler should still be installed */

  CU_TEST(0 != Asc_SignalHandlerPush(SIGILL, Asc_SignalTrap));  /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPush(SIGABRT, my_handler2));    /* unsupported signal type */

  Asc_SignalRecover(TRUE);

  old_handler = signal(SIGFPE, SIG_DFL);                /* handlers should have been reinstalled */
  CU_TEST(Asc_SignalTrap == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler1));
  old_handler = signal(SIGFPE, SIG_DFL);                /* SIGINT should be reset to no handler */
  CU_TEST(Asc_SignalTrap == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  CU_TEST(0 != Asc_SignalHandlerPop(SIGFPE, my_handler1));  /* wrong handler indicated */
  old_handler = signal(SIGFPE, SIG_DFL);                /* so SIGFPE should be not be reset */
  CU_TEST(Asc_SignalTrap == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  old_handler = signal(SIGFPE, SIG_DFL);                /* but Asc_SignalRecover should reset it */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  CU_TEST(0 != Asc_SignalHandlerPop(SIGILL, my_handler1));     /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPop(SIGABRT, Asc_SignalTrap)); /* unsupported signal type */

  old_handler = signal(SIGFPE, SIG_DFL);                /* should be no change in handlers */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  /* test Asc_SignalTrap() */

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, Asc_SignalTrap));

  signal1_caught = FALSE;
  if (0 == setjmp(g_fpe_env)) {                         /* raise & catch a SIGFPE */
    raise(SIGFPE);
  }
  else {
    signal1_caught = TRUE;
    CU_PASS("SIGFPE caught.");
  }
  CU_TEST(TRUE == signal1_caught);

  signal1_caught = FALSE;
  if (0 == setjmp(g_int_env)) {                         /* raise & catch a SIGINT */
    raise(SIGINT);
  }
  else {
    signal1_caught = TRUE;
    CU_PASS("SIGINT caught.");
  }
  CU_TEST(TRUE == signal1_caught);

  signal1_caught = FALSE;
  if (0 == setjmp(g_seg_env)) {                         /* raise & catch a SIGSEGV */
    raise(SIGSEGV);
  }
  else {
    signal1_caught = TRUE;
    CU_PASS("SIGSEGV caught.");
  }
  CU_TEST(TRUE == signal1_caught);

Asc_SignalRecover(TRUE);

  CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPop(SIGSEGV, Asc_SignalTrap));

  old_handler = signal(SIGFPE, SIG_DFL);                /* handlers should be restored at this point */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  /* test typical use with nesting of handlers */

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler1));   /* test for SIGFPE */
  if (0 == setjmp(my_jmp_buf1)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler2));
    if (0 == setjmp(my_jmp_buf2)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));
      if (0 == setjmp(g_fpe_env)) {
         raise(SIGFPE);
      }
      else {
        CU_TEST(f_handler1_called == FALSE);
        CU_TEST(f_handler1_sigval == 0);
        CU_TEST(f_handler2_called == FALSE);
        CU_TEST(f_handler2_sigval == 0);
        signal3_caught = TRUE;
      }
      CU_TEST(FALSE == signal1_caught);
      CU_TEST(FALSE == signal2_caught);
      CU_TEST(TRUE == signal3_caught);
      CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, Asc_SignalTrap));
      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      raise(SIGFPE);
    }
    else {
      CU_TEST(f_handler1_called == FALSE);
      CU_TEST(f_handler1_sigval == 0);
      CU_TEST(f_handler2_called == TRUE);
      CU_TEST(f_handler2_sigval == SIGFPE);
      signal2_caught = TRUE;
    }
    CU_TEST(FALSE == signal1_caught);
    CU_TEST(TRUE == signal2_caught);
    CU_TEST(FALSE == signal3_caught);
    CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, my_handler2));
    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
    raise(SIGFPE);
  }
  else {
    CU_TEST(f_handler1_called == TRUE);
    CU_TEST(f_handler1_sigval == SIGFPE);
    CU_TEST(f_handler2_called == FALSE);
    CU_TEST(f_handler2_sigval == 0);
    signal1_caught = TRUE;
  }
  CU_TEST(TRUE == signal1_caught);
  CU_TEST(FALSE == signal2_caught);
  CU_TEST(FALSE == signal3_caught);
  CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, my_handler1));

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler2));   /* test for SIGINT */
  if (0 == setjmp(my_jmp_buf2)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, Asc_SignalTrap));
    if (0 == setjmp(g_int_env)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));
      if (0 == setjmp(my_jmp_buf1)) {
         raise(SIGINT);
      }
      else {
        CU_TEST(f_handler1_called == TRUE);
        CU_TEST(f_handler1_sigval == SIGINT);
        CU_TEST(f_handler2_called == FALSE);
        CU_TEST(f_handler2_sigval == 0);
        signal3_caught = TRUE;
      }
      CU_TEST(FALSE == signal1_caught);
      CU_TEST(FALSE == signal2_caught);
      CU_TEST(TRUE == signal3_caught);
      CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler1));
      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      raise(SIGINT);
    }
    else {
      CU_TEST(f_handler1_called == FALSE);
      CU_TEST(f_handler1_sigval == 0);
      CU_TEST(f_handler2_called == FALSE);
      CU_TEST(f_handler2_sigval == 0);
      signal2_caught = TRUE;
    }
    CU_TEST(FALSE == signal1_caught);
    CU_TEST(TRUE == signal2_caught);
    CU_TEST(FALSE == signal3_caught);
    CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, Asc_SignalTrap));
    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
    raise(SIGINT);
  }
  else {
    CU_TEST(f_handler1_called == FALSE);
    CU_TEST(f_handler1_sigval == 0);
    CU_TEST(f_handler2_called == TRUE);
    CU_TEST(f_handler2_sigval == SIGINT);
    signal1_caught = TRUE;
  }
  CU_TEST(TRUE == signal1_caught);
  CU_TEST(FALSE == signal2_caught);
  CU_TEST(FALSE == signal3_caught);
  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler2));

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, Asc_SignalTrap));   /* test for SIGSEGV */
  if (0 == setjmp(g_seg_env)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler2));
    if (0 == setjmp(my_jmp_buf2)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler1));
      if (0 == setjmp(my_jmp_buf1)) {
         raise(SIGSEGV);
      }
      else {
        CU_TEST(f_handler1_called == TRUE);
        CU_TEST(f_handler1_sigval == SIGSEGV);
        CU_TEST(f_handler2_called == FALSE);
        CU_TEST(f_handler2_sigval == 0);
        signal3_caught = TRUE;
      }
      CU_TEST(FALSE == signal1_caught);
      CU_TEST(FALSE == signal2_caught);
      CU_TEST(TRUE == signal3_caught);
      CU_TEST(0 == Asc_SignalHandlerPop(SIGSEGV, my_handler1));
      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      raise(SIGSEGV);
    }
    else {
      CU_TEST(f_handler1_called == FALSE);
      CU_TEST(f_handler1_sigval == 0);
      CU_TEST(f_handler2_called == TRUE);
      CU_TEST(f_handler2_sigval == SIGSEGV);
      signal2_caught = TRUE;
    }
    CU_TEST(FALSE == signal1_caught);
    CU_TEST(TRUE == signal2_caught);
    CU_TEST(FALSE == signal3_caught);
    CU_TEST(0 == Asc_SignalHandlerPop(SIGSEGV, my_handler2));
    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
    raise(SIGSEGV);
  }
  else {
    CU_TEST(f_handler1_called == FALSE);
    CU_TEST(f_handler1_sigval == 0);
    CU_TEST(f_handler2_called == FALSE);
    CU_TEST(f_handler2_sigval == 0);
    signal1_caught = TRUE;
  }
  CU_TEST(TRUE == signal1_caught);
  CU_TEST(FALSE == signal2_caught);
  CU_TEST(FALSE == signal3_caught);
  CU_TEST(0 == Asc_SignalHandlerPop(SIGSEGV, Asc_SignalTrap));

  old_handler = signal(SIGFPE, SIG_DFL);                /* handlers should be restored at this point */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  Asc_SignalDestroy();

  old_handler = signal(SIGFPE, SIG_DFL);                /* original handlers should still be in place */
  CU_TEST(my_handler1 == old_handler);
  old_handler = signal(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = signal(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

#endif  /* NO_SIGNAL_TRAPS */

  if (NULL != old_fpe_handler)                /* restore any pre-existing handlers */
    signal(SIGFPE, old_fpe_handler);
  if (NULL != old_int_handler)
    signal(SIGINT, old_int_handler);
  if (NULL != old_seg_handler)
    signal(SIGSEGV, old_seg_handler);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascSignal_test_list[] = {
  {"ascSignal", test_ascSignal},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"utilities_ascSignal", NULL, NULL, ascSignal_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascSignal(void)
{
  return CU_register_suites(suites);
}
