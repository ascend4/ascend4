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

#include <ascend/utilities/ascSignal.h>
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <test/common.h>
#include <stdio.h>

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
void my_handler1(int sigval){
  f_handler1_called = TRUE;
  Asc_SignalRecover(FALSE);
  switch (sigval){
    case SIGFPE:
      FPRESET;
    case SIGINT:
    case SIGSEGV:
      f_handler1_sigval = sigval;
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
/* identical to my_handler1 */
void my_handler2(int sigval){
  f_handler2_called = TRUE;
  Asc_SignalRecover(FALSE);
  switch (sigval){
    case SIGFPE:
      FPRESET;
    case SIGINT:
    case SIGSEGV:
      f_handler2_sigval = sigval;
      break;
    default:
      f_handler2_sigval = -1;
      break;
  }
  longjmp(my_jmp_buf2, sigval);
}

static jmp_buf my_jmp_buf3;
static int f_handler3_called;
static int f_handler3_sigval;
/* identical to my_handler1 */
void my_handler3(int sigval){
  f_handler3_called = TRUE;
  Asc_SignalRecover(FALSE);
  switch (sigval){
    case SIGFPE:
      FPRESET;
    case SIGINT:
    case SIGSEGV:
      f_handler3_sigval = sigval;
      break;
    default:
      f_handler3_sigval = -1;
      break;
  }
  longjmp(my_jmp_buf3, sigval);
}

/*----------------------------------------------------------------------------*/

static void test_pushpop(void){
  unsigned long prior_meminuse;
  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NO_SIGNAL_TRAPS
  /* no point in testing if the functionality is disabled */
  CU_FAIL("Signal handler manager not enabled.");
#else

  SigHandlerFn* old_SIGFPE_handler = signal(SIGFPE, my_handler1);
  SigHandlerFn* old_SIGINT_handler = signal(SIGINT, my_handler1);
  SigHandlerFn* old_SIGSEGV_handler = signal(SIGSEGV, my_handler1);

  signal(SIGFPE, my_handler1);                          /* install some pre-existing handlers */
  signal(SIGINT, SIG_DFL);
  signal(SIGSEGV, my_handler2);

  /* Asc_SignalInit(), Asc_SignalDestroy() - not much to test */

  CU_TEST(0 == Asc_SignalInit());                       /* initialize the signal manager */

  /* previously-installed handlers should still be active */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler1);
  CU_TEST(signal(SIGINT, SIG_DFL) == SIG_DFL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);

  Asc_SignalRecover(TRUE);

  /* handlers should have been reinstalled */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler1);
  CU_TEST(signal(SIGINT, SIG_DFL) == SIG_DFL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);

  Asc_SignalRecover(TRUE); /* back to h1, 0, h2 again */

  /*
  In the following, take
  H1 = my_handler1
  H2 = my_handler2
  AS = Asc_SignalTrap
  DF = SIG_DFL
  00 = NULL
  so the initial state of the stacks is
  FPE:H1, INT:00, SEG: H2.
  */

#if 0
  CONSOLE_DEBUG("INITIAL STACKS...");
  Asc_SignalPrintStack(SIGFPE);
  Asc_SignalPrintStack(SIGINT);
  Asc_SignalPrintStack(SIGSEGV);
#endif

  /* test Asc_SignalPush(), Asc_SignalPop() */
  CU_TEST(Asc_SignalStackLength(SIGFPE) == 1);

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap)); /* ok - supported signal, ok func */
  CU_TEST(signal(SIGFPE, SIG_DFL) == Asc_SignalTrap);

  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));    /* ok - supported signal, ok func */
  CU_TEST(signal(SIGINT, SIG_DFL) == my_handler1);

  CU_TEST(-2 == Asc_SignalHandlerPush(SIGSEGV, NULL));          /* NULL func - should have no effect */
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);             /* old handler should still be installed */

  CU_TEST(0 != Asc_SignalHandlerPush(SIGILL, Asc_SignalTrap));  /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPush(SIGABRT, my_handler2));    /* unsupported signal type */

  /* expect FPE:H1:AS, INT:00,H1, SEG:H2 */
  CU_TEST(Asc_SignalStackLength(SIGFPE) == 2);

  Asc_SignalRecover(TRUE);

#if 0
  CONSOLE_DEBUG("UPDATED STACKS...");
  Asc_SignalPrintStack(SIGFPE);
  Asc_SignalPrintStack(SIGINT);
  Asc_SignalPrintStack(SIGSEGV);
#endif

  /* handlers should have been reinstalled */
  CU_TEST(signal(SIGFPE, SIG_DFL) == Asc_SignalTrap);
  CU_TEST(signal(SIGINT, SIG_DFL) == my_handler1);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);
  Asc_SignalRecover(TRUE); /* still at asc, h1, 0 */

  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler1));
  /* SIGINT should be reset to no handler */
  /* expect FPE:H1:AS, INT:00, SEG:H2 */
  CU_TEST(signal(SIGFPE, SIG_DFL) == Asc_SignalTrap);
  CU_TEST(signal(SIGINT, SIG_DFL) == NULL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);
  Asc_SignalRecover(TRUE);

  //CONSOLE_DEBUG("Testing pop of incorrect handler...");
  CU_TEST(0 != Asc_SignalHandlerPop(SIGFPE, my_handler2));  /* wrong handler indicated */
  /* expect FPE:H1, INT:00, SEG:H2, but SIGFPE shoule still be AS. */
  CU_TEST(signal(SIGFPE, SIG_DFL) == Asc_SignalTrap);
  CU_TEST(signal(SIGINT, SIG_DFL) == NULL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);

  Asc_SignalRecover(TRUE);
  /* but Asc_SignalRecover should fix up SIGFPE to H1 */
  /* expect FPE:H1, INT:00, SEG:H2 */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler1);
  CU_TEST(signal(SIGINT, SIG_DFL) == NULL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);
  Asc_SignalRecover(TRUE);

  CU_TEST(0 != Asc_SignalHandlerPop(SIGILL, my_handler1));     /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPop(SIGABRT, Asc_SignalTrap)); /* unsupported signal type */

  /* should be no change in handlers */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler1);
  CU_TEST(signal(SIGINT, SIG_DFL) == NULL);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler2);
  Asc_SignalRecover(TRUE);

  Asc_SignalDestroy();

#define RESTORE(SIG) \
   if(NULL!=old_##SIG##_handler)signal(SIG,old_##SIG##_handler);\
   else signal(SIG,SIG_DFL);

  RESTORE(SIGFPE);
  RESTORE(SIGINT);
  RESTORE(SIGSEGV);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */

}


void test_raise(){
  SigHandlerFn* old_handler;
  volatile int signal1_caught;
  volatile int signal2_caught;
  volatile int signal3_caught;

  unsigned long prior_meminuse;
  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  /* save pre-existing handlers */
  SigHandlerFn* old_SIGFPE_handler = signal(SIGFPE, my_handler3);
  SigHandlerFn* old_SIGINT_handler = signal(SIGINT, my_handler2);
  SigHandlerFn* old_SIGSEGV_handler = signal(SIGSEGV, my_handler1);

  Asc_SignalInit();

  /* test Asc_SignalTrap() */

  //CONSOLE_DEBUG("Testing trapping of signals");
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

  //CONSOLE_DEBUG("Check handler settings after pop");

  /* handlers should be restored at this point */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler3);
  CU_TEST(signal(SIGINT, SIG_DFL) == my_handler2);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler1);
  Asc_SignalRecover(TRUE);

#if 0
  CONSOLE_DEBUG("CURRENT STACKS...");
  Asc_SignalPrintStack(SIGFPE);
  Asc_SignalPrintStack(SIGINT);
  Asc_SignalPrintStack(SIGSEGV);
#endif

#if 0 /* these tests just don't work and probably don't adhere to coding rules
for signal handling. more work required. */
  /* test typical use with nesting of handlers */
  //CONSOLE_DEBUG("Testing typical use of nested handlers");

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  f_handler3_called = FALSE;
  f_handler3_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler1));   /* test for SIGFPE */
  if(0 == setjmp(my_jmp_buf1)){
    CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler2));
    if(0 == setjmp(my_jmp_buf2)){
      CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler3));

      Asc_SignalRecover(TRUE);
      CONSOLE_DEBUG("CURRENT STACKS...");
      Asc_SignalPrintStack(SIGFPE);
      Asc_SignalPrintStack(SIGINT);
      Asc_SignalPrintStack(SIGSEGV);

      if(0 == setjmp(my_jmp_buf3)){
         /* make sure the expected handler is in place... */
         CU_TEST(my_handler3 == signal(SIGFPE, my_handler3));

         /* raise our first SIGFPE... */
         CONSOLE_DEBUG("Raising SIGFPE exception...");
         CU_TEST(0 == raise(SIGSEGV));

         CU_FAIL("shouldn't be here!");
         CONSOLE_DEBUG("got here (didn't want to)");
         //Asc_SignalTrap(SIGFPE);
         //CONSOLE_DEBUG("and here");
      }else{
        /* catching our first SIGFPE, labelled 'signal 3' */ 
        CONSOLE_DEBUG("got here");
        CU_TEST(f_handler1_called == FALSE);
        CU_TEST(f_handler1_sigval == 0);
        CU_TEST(f_handler2_called == FALSE);
        CU_TEST(f_handler2_sigval == 0);
        signal3_caught = TRUE;
      }
      /* fall through: we should have caught 'signal 3' before this */
      CONSOLE_DEBUG("down here");
      CU_TEST(FALSE == signal1_caught);
      CU_TEST(FALSE == signal2_caught);
      CU_TEST(TRUE == signal3_caught);

      CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, my_handler3));

      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      raise(SIGFPE);
    }else{
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
  }else{
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
  if(0 == setjmp(my_jmp_buf2)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, Asc_SignalTrap));
    if(0 == setjmp(g_int_env)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));
      if(0 == setjmp(my_jmp_buf1)) {
         raise(SIGINT);
         CU_FAIL("shouldn't be here!");
      }else{
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
    }else{
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
  }else{
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
  if(0 == setjmp(g_seg_env)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler2));
    if(0 == setjmp(my_jmp_buf2)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler1));
      if(0 == setjmp(my_jmp_buf1)) {
         raise(SIGSEGV);
         CU_FAIL("shouldn't be here!");
      }else{
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
    }else{
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
  }else{
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

  //CONSOLE_DEBUG("Check recovered signals");
#endif

  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler3);
  CU_TEST(signal(SIGINT, SIG_DFL) == my_handler2);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler1);

  Asc_SignalRecover(TRUE);

  Asc_SignalDestroy();

  /* after destruction, handlers should not have been touched */
  CU_TEST(signal(SIGFPE, SIG_DFL) == my_handler3);
  CU_TEST(signal(SIGINT, SIG_DFL) == my_handler2);
  CU_TEST(signal(SIGSEGV, SIG_DFL) == my_handler1);

  /* what does this do, now?? return an error? */
  Asc_SignalRecover(TRUE);

#endif  /* NO_SIGNAL_TRAPS */

  RESTORE(SIGFPE);
  RESTORE(SIGINT);
  RESTORE(SIGSEGV);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(pushpop) \
	T(raise)

REGISTER_TESTS_SIMPLE(utilities_ascSignal, TESTS)

