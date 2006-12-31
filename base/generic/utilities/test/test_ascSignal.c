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
#include <utilities/ascPanic.h>
#include "CUnit/CUnit.h"
#include "test_ascSignal.h"
#include "printutil.h"

#include <signal.h>

static JMP_BUF my_jmp_buf1;

#define MEMUSED(N) CU_TEST(ascmeminuse()==N)

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
  LONGJMP(my_jmp_buf1, sigval);
}

void wrong_handler(int sigval){
	ASC_PANIC("Don't use this one");
}

static JMP_BUF my_jmp_buf2;

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
  LONGJMP(my_jmp_buf2, sigval);
}

static int old_active = 0;
static SigHandlerFn *old_fpe_handler = NULL;
static SigHandlerFn *old_int_handler = NULL;
static SigHandlerFn *old_seg_handler = NULL;

static void store_current_signals(void){
  CU_TEST(old_active==0);
  old_active = 1;
  old_fpe_handler = SIGNAL(SIGFPE, my_handler1);        /* save any pre-existing handlers */
  old_int_handler = SIGNAL(SIGINT, my_handler1);
  old_seg_handler = SIGNAL(SIGSEGV, my_handler1);
}

#define CHECK_SIGNALS_MATCH_STACKS(FPEFN,INTFN,SEGFN) \
	{	SigHandlerFn *oldfpe, *oldint, *oldseg; \
		oldfpe = signal(SIGFPE,SIG_IGN);\
		CU_TEST(oldfpe == FPEFN);\
		CU_TEST(Asc_SignalStackTop(SIGFPE)==FPEFN);\
		oldfpe = signal(SIGFPE,oldfpe);\
	\
		oldint = signal(SIGINT,SIG_IGN);\
		CU_TEST(oldint == INTFN);\
		CU_TEST(Asc_SignalStackTop(SIGINT)==INTFN);\
		oldint = signal(SIGINT,oldint);\
	\
		oldseg = signal(SIGSEGV,SIG_IGN);\
		CU_TEST(oldseg == SEGFN);\
		CU_TEST(Asc_SignalStackTop(SIGSEGV)==SEGFN);\
		oldseg = signal(SIGSEGV,oldseg);\
	}

static void restore_previous_signals(void){
  CU_TEST(old_active==1);
  if (NULL != old_fpe_handler)                /* restore any pre-existing handlers */
    SIGNAL(SIGFPE, old_fpe_handler);
  if (NULL != old_int_handler)
    SIGNAL(SIGINT, old_int_handler);
  if (NULL != old_seg_handler)
    SIGNAL(SIGSEGV, old_seg_handler);
  old_active = 0;
}	

/*----------------------------------------*/

static void test_ascsignal_basic(void){

  SigHandlerFn *old_handler;
  volatile int signal1_caught;

  CONSOLE_DEBUG("SIGFPE = %d",SIGFPE);
  CONSOLE_DEBUG("SIGINT = %d",SIGINT);
  CONSOLE_DEBUG("SIGSEGV= %d",SIGSEGV);

  store_current_signals();

  SIGNAL(SIGFPE, my_handler1);                          /* install some pre-existing handlers */
  SIGNAL(SIGINT, SIG_DFL);
  SIGNAL(SIGSEGV, my_handler2);

  /* Asc_SignalInit(), Asc_SignalDestroy() - not much to test */

  CU_TEST(0 == Asc_SignalInit());                       /* initialize the signal manager */

  CHECK_SIGNALS_MATCH_STACKS(my_handler1, NULL, my_handler2);

  /* mix up the current handlers */
  SIGNAL(SIGFPE, my_handler2);
  SIGNAL(SIGINT, SIG_IGN);
  SIGNAL(SIGSEGV, my_handler1);

  Asc_SignalRecover(TRUE);

  /* check that recovery worked; check that repeated calls don't damage it */
  CHECK_SIGNALS_MATCH_STACKS(my_handler1, NULL, my_handler2);
  CHECK_SIGNALS_MATCH_STACKS(my_handler1, NULL, my_handler2);
  CHECK_SIGNALS_MATCH_STACKS(my_handler1, NULL, my_handler2);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_pushpopint(void){
  SigHandlerFn *old_handler;
  volatile int signal1_caught;

  store_current_signals();

  SIGNAL(SIGINT, SIG_IGN);
  CU_TEST(0==Asc_SignalInit());

  CU_TEST(Asc_SignalStackLength(SIGINT)==1);

  CU_TEST(0==Asc_SignalHandlerPush(SIGINT, my_handler1));     /* ok - supported signal, ok func */
  CU_TEST(Asc_SignalStackLength(SIGINT)==2);

  CONSOLE_DEBUG("Stack should have my_handler1...");
  Asc_SignalPrintStack(SIGINT);

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(old_handler == my_handler1);

  CU_TEST(0 != Asc_SignalHandlerPush(SIGILL, Asc_SignalTrap));  /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPush(SIGABRT, my_handler2));    /* unsupported signal type */
  Asc_SignalRecover(TRUE);
  CU_TEST(Asc_SignalStackLength(SIGINT)==2);

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(old_handler == my_handler1);

  Asc_SignalRecover(TRUE);
  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler1));
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(old_handler == SIG_IGN);

  Asc_SignalRecover(TRUE);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(SIG_IGN == old_handler);

  Asc_SignalRecover(TRUE);

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(SIG_IGN == old_handler);
  Asc_SignalRecover(TRUE);

  CU_TEST(0 != Asc_SignalHandlerPop(SIGILL, my_handler1));     /* unsupported signal type */
  CU_TEST(0 != Asc_SignalHandlerPop(SIGABRT, Asc_SignalTrap)); /* unsupported signal type */

  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(SIG_IGN == old_handler);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_pushpop(void){
  SigHandlerFn *old_handler;
  volatile int signal1_caught;

  CONSOLE_DEBUG("my_handler1 = %p",my_handler1);
  CONSOLE_DEBUG("my_handler2 = %p",my_handler2);
  CONSOLE_DEBUG("Asc_SignalTrap = %p",Asc_SignalTrap);

  store_current_signals();

  /* set the initial handlers */
  SIGNAL(SIGFPE, my_handler1);
  SIGNAL(SIGINT, SIG_IGN);
  SIGNAL(SIGSEGV, my_handler2);

  /* initialise the stack */
  CU_TEST(0 == Asc_SignalInit());

  CHECK_SIGNALS_MATCH_STACKS(my_handler1, SIG_IGN, my_handler2);

  /* check that initial handlers were pushed onto the stack */
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);

  /* push an additional handler onto SIGFPE stack */
  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));

  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_IGN, my_handler2);

  CONSOLE_DEBUG("SIGFPE stack:");
  Asc_SignalPrintStack(SIGFPE);

  /* push an additional handler onto SIGINT stack */
  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));

  /* check the stacks */
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1); /* my_handler2 */
  CU_TEST(Asc_SignalStackLength(SIGINT)==2); /* SIG_IGN, my_handler1 */
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2); /* my_handler1, Asc_SignalTrap */

  CONSOLE_DEBUG("SIGFPE stack:");
  Asc_SignalPrintStack(SIGFPE);

  /* attempt to push a NULL handler should have no effect */
  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, NULL));
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==2);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);

  CONSOLE_DEBUG("SIGFPE stack:");
  Asc_SignalPrintStack(SIGFPE);
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);

  /* attempt to push handler for unsupported signal types should have no effect, should return error */
  CU_TEST(0 != Asc_SignalHandlerPush(SIGILL, Asc_SignalTrap));
  CU_TEST(0 != Asc_SignalHandlerPush(SIGABRT, my_handler2));
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==2);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);
  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, my_handler1, my_handler2);

  /* pop off the SIGINT stack */
  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, my_handler1));

  CONSOLE_DEBUG("SIGFPE stack:");
  Asc_SignalPrintStack(SIGFPE);

  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_IGN, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);

  /* attempt to pop off an incorrect handler */
  CONSOLE_DEBUG("Popping incorrect handler");
  CU_TEST(0 != Asc_SignalHandlerPop(SIGFPE, wrong_handler));

  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_IGN, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);

  /* mess with the SIGFPE handler, then attempt to recover from stacks */
  old_handler = SIGNAL(SIGFPE, SIG_DFL);
  Asc_SignalRecover(TRUE);

  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_IGN, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);

  /* try a couple of unsupported signal types */
  CU_TEST(0 != Asc_SignalHandlerPop(SIGILL, my_handler1));
  CU_TEST(0 != Asc_SignalHandlerPop(SIGABRT, Asc_SignalTrap));

  CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_IGN, my_handler2);
  CU_TEST(Asc_SignalStackLength(SIGSEGV)==1);
  CU_TEST(Asc_SignalStackLength(SIGINT)==1);
  CU_TEST(Asc_SignalStackLength(SIGFPE)==2);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_trap(void){
  SigHandlerFn *old_handler;
  volatile int signal1_caught;

  store_current_signals();

  SIGNAL(SIGFPE,my_handler1);
  SIGNAL(SIGINT,NULL);
  SIGNAL(SIGSEGV,my_handler2);

  CU_TEST(0 == Asc_SignalInit());

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, Asc_SignalTrap));

  signal1_caught = FALSE;
  CONSOLE_DEBUG("Raise and catch a SIGFPE...");
  if (0 == SETJMP(g_fpe_env)){
	CONSOLE_DEBUG("Raising SIGFPE");
    raise(SIGFPE);
	CONSOLE_DEBUG("SHOULDN'T BE HERE");
	CU_FAIL("Shouldn't be here");
  }else{
    signal1_caught = TRUE;
	CONSOLE_DEBUG("Caught SIGFPE");
  }
  CU_TEST(TRUE == signal1_caught);

  signal1_caught = FALSE;
  /* raise & catch a SIGINT */
  if (0 == SETJMP(g_int_env)){
    raise(SIGINT);
  }else{
    signal1_caught = TRUE;
	CONSOLE_DEBUG("Caught SIGINT");
  }
  CU_TEST(TRUE == signal1_caught);

  signal1_caught = FALSE;
  /* raise & catch a SIGSEGV */
  if (0 == SETJMP(g_seg_env)) {
    raise(SIGSEGV);
  }else{
    signal1_caught = TRUE;
	CONSOLE_DEBUG("Caught SIGSEGV");
  }
  CU_TEST(TRUE == signal1_caught);


  Asc_SignalRecover(TRUE);

  CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPop(SIGINT, Asc_SignalTrap));
  CU_TEST(0 == Asc_SignalHandlerPop(SIGSEGV, Asc_SignalTrap));

  old_handler = SIGNAL(SIGFPE, SIG_DFL);                /* handlers should be restored at this point */
  CU_TEST(my_handler1 == old_handler);
  old_handler = SIGNAL(SIGINT, SIG_DFL);
  CU_TEST(NULL == old_handler);
  old_handler = SIGNAL(SIGSEGV, SIG_DFL);
  CU_TEST(my_handler2 == old_handler);
  Asc_SignalRecover(TRUE);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_nestingfpe(void){
  volatile int signal1_caught;
  volatile int signal2_caught;
  volatile int signal3_caught;

  store_current_signals();

  /* set the initial handlers */
  SIGNAL(SIGFPE, SIG_DFL);
  SIGNAL(SIGINT, SIG_DFL);
  SIGNAL(SIGSEGV,SIG_DFL);

  CU_TEST(0 == Asc_SignalInit());

  /* test typical use with nesting of handlers */

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler1));
  if (0 == SETJMP(my_jmp_buf1)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, my_handler2));
    if (0 == SETJMP(my_jmp_buf2)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap));
      if(0 == SETJMP(g_fpe_env)) {
         CHECK_SIGNALS_MATCH_STACKS(Asc_SignalTrap, SIG_DFL, SIG_DFL);
         CU_TEST(0==raise(SIGFPE));
         CONSOLE_DEBUG("...");
		 CU_FAIL("Can't be here! No signal was raised!");
      }else{
		/* do we need to reset traps here? */
		CONSOLE_DEBUG("...");
        CU_TEST(f_handler1_called == FALSE);
        CU_TEST(f_handler1_sigval == 0);
        CU_TEST(f_handler2_called == FALSE);
        CU_TEST(f_handler2_sigval == 0);
        signal3_caught = TRUE;
      }
      CONSOLE_DEBUG("...");
      CU_TEST(FALSE == signal1_caught);
      CU_TEST(FALSE == signal2_caught);
      CU_TEST(TRUE == signal3_caught);
      CU_TEST(0 == Asc_SignalHandlerPop(SIGFPE, Asc_SignalTrap));
      CHECK_SIGNALS_MATCH_STACKS(my_handler2, SIG_DFL, SIG_DFL);
      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      CU_TEST(0==raise(SIGFPE));
	  CU_FAIL("Shouldn't be here");
    }else{
      CONSOLE_DEBUG("...");
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
    CHECK_SIGNALS_MATCH_STACKS(my_handler1, SIG_DFL, SIG_DFL);
    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
    CU_TEST(0==raise(SIGFPE));
    CU_FAIL("Shouldn't be here");
  }else{
    CONSOLE_DEBUG("...");
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
  CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, SIG_DFL);

  CONSOLE_DEBUG("...");

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_nestingint(void){
  volatile int signal1_caught;
  volatile int signal2_caught;
  volatile int signal3_caught;
  SigHandlerFn *old_handler;

  store_current_signals();

  /* set the initial handlers */
  SIGNAL(SIGFPE, SIG_DFL);
  SIGNAL(SIGINT, SIG_DFL);
  SIGNAL(SIGSEGV,SIG_DFL);

  CU_TEST(0 == Asc_SignalInit());

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CONSOLE_DEBUG("my_handler1 = %p",my_handler1);
  CONSOLE_DEBUG("my_handler2 = %p",my_handler2);
  CONSOLE_DEBUG("Asc_SignalTrap = %p",Asc_SignalTrap);

  CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler2));   /* test for SIGINT */
  if (0 == SETJMP(my_jmp_buf2)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, Asc_SignalTrap));
    if (0 == SETJMP(g_int_env)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGINT, my_handler1));
      if (0 == SETJMP(my_jmp_buf1)) {
         CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, my_handler1, SIG_DFL);
	     CONSOLE_DEBUG("Raising to my_handler1");
         CU_TEST(0==raise(SIGINT));
         CONSOLE_DEBUG("SHOULDN'T BE HERE!");
         CU_FAIL("Shouldn't be here!");
      }else{
		CONSOLE_DEBUG("Caught from my_handler1");
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

      /* check that 'Asc_SignalTrap' is now installed for SIGINT */
      CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, Asc_SignalTrap, SIG_DFL);

      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;

	  CONSOLE_DEBUG("Raising to Asc_SignalTrap = %p",Asc_SignalTrap);
      CU_TEST(0==raise(SIGINT));
	  //Asc_SignalTrap(SIGINT);
      CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, Asc_SignalTrap, SIG_DFL);
	  CONSOLE_DEBUG("SHOULDN'T BE HERE!");
	  CU_FAIL("Shouldn't be here!");
    }else{
	  CONSOLE_DEBUG("Caught from Asc_SignalTrap");
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

    CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, my_handler2, SIG_DFL);

    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
	CONSOLE_DEBUG("Raising to my_handler2");
    CU_TEST(0==raise(SIGINT));
    //my_handler2(SIGINT);
	CONSOLE_DEBUG("SHOULDN'T BE HERE!");
    CU_FAIL("Shouldn't be here!");
  }else{
	CONSOLE_DEBUG("Caught from my_handler2");
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

  CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, SIG_DFL);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*----------------------------------------*/

static void test_ascsignal_nestingsegv(void){

  SigHandlerFn *old_handler;
  volatile int signal1_caught;
  volatile int signal2_caught;
  volatile int signal3_caught;

  store_current_signals();

  /* set the initial handlers */
  SIGNAL(SIGFPE, SIG_DFL);
  SIGNAL(SIGINT, SIG_DFL);
  SIGNAL(SIGSEGV,SIG_DFL);

  CU_TEST(0 == Asc_SignalInit());

  f_handler1_called = FALSE;                              /* initialize flags for detecting flow */
  f_handler1_sigval = 0;
  f_handler2_called = FALSE;
  f_handler2_sigval = 0;
  signal1_caught = FALSE;
  signal2_caught = FALSE;
  signal3_caught = FALSE;

  CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, Asc_SignalTrap));   /* test for SIGSEGV */
  if (0 == SETJMP(g_seg_env)) {
    CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler2));
    if (0 == SETJMP(my_jmp_buf2)) {
      CU_TEST(0 == Asc_SignalHandlerPush(SIGSEGV, my_handler1));
      if (0 == SETJMP(my_jmp_buf1)) {
         CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, my_handler1);
         CU_TEST(0==raise(SIGSEGV));
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
      CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, my_handler2);
      f_handler1_called = FALSE;
      f_handler1_sigval = 0;
      f_handler2_called = FALSE;
      f_handler2_sigval = 0;
      signal1_caught = FALSE;
      signal2_caught = FALSE;
      signal3_caught = FALSE;
      CU_TEST(0==raise(SIGSEGV));
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
    CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, Asc_SignalTrap);
    f_handler1_called = FALSE;
    f_handler1_sigval = 0;
    f_handler2_called = FALSE;
    f_handler2_sigval = 0;
    signal1_caught = FALSE;
    signal2_caught = FALSE;
    signal3_caught = FALSE;
    CU_TEST(0==raise(SIGSEGV));
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

  CHECK_SIGNALS_MATCH_STACKS(SIG_DFL, SIG_DFL, SIG_DFL);

  restore_previous_signals();
  Asc_SignalDestroy();
  MEMUSED(0);
}

/*===========================================================================*/
/* Registration information */

#define T(N) {#N,test_ascsignal_##N}
static CU_TestInfo ascSignal_test_list[] = {
	T(basic)
	, T(pushpopint)
	, T(pushpop)
	, T(trap)
	, T(nestingfpe)
	, T(nestingint)
	, T(nestingsegv)
	, CU_TEST_INFO_NULL
};
#undef T

static CU_SuiteInfo suites[] = {
  {"test_utilities_ascSignal", NULL, NULL, ascSignal_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascSignal(void)
{
  return CU_register_suites(suites);
}
