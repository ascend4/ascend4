/*	ASCEND modelling environment
	Copyright (C) 1997 Benjamin Andrew Allan
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
*//** @file
	Signal handling protocol definitions for ASCEND

	Start of making signal handling in ASCEND
	code somewhat sane. Still needs to somehow
	support the management of jmp_buf's so that
	handlers will longjmp to the right place.

	A better alternative is to make all our code check/return
	status flags based on a variable set by the trap
	and cleared by recipient.
*//*
	by Benjamin Andrew Allan, May 27, 1997
	Last in CVS $Revision: 1.9 $ $Date: 1999/01/19 12:23:20 $ $Author: mthomas $
	
	ChangeLog

	10/15/2005  - Changed ascresetneeded() so that any previously
	              registered handlers are reset before return.
	            - Added Asc_SignalRecover() to standard handler
	              Asc_SignalTrap() so handlers are reset if necessary. (JDS)
	12/10/2005  - Changed storage of signal handlers from gl_list's
	              to local arrays.  gl_list's can't legally hold
	              function pointers. (JDS)
*/

#include <stdio.h>
#include "ascConfig.h"

#ifndef NO_SIGNAL_TRAPS
# include <signal.h>
# include <setjmp.h>
#endif /* NO_SIGNAL_TRAPS*/

#ifdef __WIN32__
# include <process.h>
#else
# include <unistd.h>
#endif

#include "ascMalloc.h"
#include "ascSignal.h"

#if !defined(NO_SIGINT_TRAP) || !defined(NO_SIGSEGV_TRAP)
static jmp_buf f_test_env;    /* for local testing of signal handling */
#endif

#ifndef NO_SIGNAL_TRAPS
/* test buf for initialization */
jmp_buf g_fpe_env;
jmp_buf g_seg_env;
jmp_buf g_int_env;

/* for future use */
jmp_buf g_foreign_code_call_env;

#endif /* NO_SIGNAL_TRAPS*/

static int f_reset_needed = -2;
/* has value 0 or 1 after Init is called.
 * and if Init is called without the value -2 in f_reset_needed,
 * it will fail.
 */
static SigHandlerFn **f_fpe_traps = NULL;  /**< array for pushed SIGFPE handlers */
static int f_fpe_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_int_traps = NULL;  /**< array for pushed SIGINT handlers */
static int f_int_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_seg_traps = NULL;  /**< array for pushed SIGSEGV handlers */
static int f_seg_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

#ifndef NO_SIGSEGV_TRAP
/* function to throw an interrupt. system dependent. */
static int testdooley2(int sig){
  raise(sig);
  return 0;
}
#endif

#if !defined(NO_SIGINT_TRAP) || !defined(NO_SIGSEGV_TRAP)
/* function to catch an interrupt */
static void testcatch(int signum){
  FPRINTF(ASCERR," signal %d caught ",signum);
  if (signum == SIGFPE) {
    FPRESET;
  }
  longjmp(f_test_env, signum);
}
#endif

/*
 * So far the following seem to need reset trapped signals after
 * a longjmp, or unconditionally.
 * HPUX cc -Aa -D_HPUX_SOURCE
 * Solaris cc
 * AIX xlc
 * IRIX cc
 * Windows
 *
 * The following retain the last trap set with or without a call to longjmp
 * and so don't need resetting of traps.
 * SunOS4 acc
 * OSF32 cc
 * NetBSD gcc 2.4.5 -ansi
 */

/** 
	This function tests the signal reseting of compilers using SIGINT.
	It should not be called except when starting a process.

	Side effects:
	 - a line is sent to ASCERR
	 - SIGINT is set to SIG_DFL if no handler was previously registered
	 - SIGFPE may be set to SIG_DFL if no handler was previously registered

	@return 0 for no reset needed, 1 for reset needed, and
	-1 if the test fails (presuming program doesn't exit first.)
*/
static int ascresetneeded(void) {
  static int result = 0;

#if !defined(NO_SIGINT_TRAP) || !defined(NO_SIGSEGV_TRAP)
  SigHandlerFn *lasttrap;
  volatile SigHandlerFn *savedtrap;
  static int c=0;
#endif

#ifndef NO_SIGINT_TRAP

  /* test interrupt */
  savedtrap = signal(SIGINT, testcatch);
  CONSOLE_DEBUG("Testing signal SIGINT (signum = %d) %p\t%p\t", SIGINT, savedtrap, testcatch);
  if (setjmp(f_test_env) == 0) {
    testdooley2(SIGINT);
  } else {
    c++;
  }
  if (c != 1) {
    CONSOLE_DEBUG("SIGINT test failed");
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Signal (SIGINT) test failed. ASCEND unlikely to work on this hardware.");
    result = -1;
  }
  lasttrap = signal(SIGINT, (NULL != savedtrap) ? savedtrap : SIG_DFL);
  CONSOLE_DEBUG("%p",lasttrap);
  if (lasttrap != testcatch) {
    result = 1;
  }

  if (result != 0) {
    return result;
  }

  c = 0;
#else
  CONSOLE_DEBUG("SIGINT trap bypassed: compile-time settings");
#endif

#ifndef NO_SIGSEGV_TRAP
  /* passed interrupt, check fpe */
  savedtrap=signal(SIGFPE, testcatch);
  CONSOLE_DEBUG("Testing signal %d %p\t%p\t",SIGFPE, savedtrap, testcatch);
  if (setjmp(f_test_env)==0) {
    testdooley2(SIGFPE);
  } else {
    c++;
  }
  if (c != 1) {
    CONSOLE_DEBUG("SIGFPE test failed");
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Signal test failed. ASCEND unlikely to work on this hardware.");
    result = -1;
  }
  lasttrap = signal(SIGFPE, (NULL != savedtrap) ? savedtrap : SIG_DFL);
  CONSOLE_DEBUG("%p\n",lasttrap);
  if (lasttrap != testcatch) {
    result = 1;
  }
#else
  CONSOLE_DEBUG("SIGSEGV trap bypassed: compile-time settings.");
#endif

  return result;
}

static void initstack (SigHandlerFn **traps, int *stackptr, int sig){
  SigHandlerFn *old;
  old = signal(sig,SIG_DFL);
  if (old != SIG_ERR && old != SIG_DFL) {
    traps[0] = old;
    *stackptr = 0;
    (void)signal(sig,old);
  }
}

/**
	Initialise ASCEND signal handling

	Does not establish any traps, just the structures for maintaining them. 
	Pushes the existing traps, if any, on the bottom of the created stacks.

	@NOTE Cannot be called twice successfully.

	@return 0 if successful, 1 if out of memory, 2 otherwise.
*/
int Asc_SignalInit(void)
{
  if (f_reset_needed != -2) {
    return 2;
  }

  /* initialize SIGFPE stack */
  if (f_fpe_traps == NULL) {
    f_fpe_traps = ASC_NEW_ARRAY_CLEAR(SigHandlerFn*,MAX_TRAP_DEPTH);
    if (f_fpe_traps == NULL) {
      return 1;
    }
  }
  f_fpe_top_of_stack = -1;

  /* initialize SIGINT stack */
  if (f_int_traps == NULL) {
    f_int_traps = ASC_NEW_ARRAY_CLEAR(SigHandlerFn*,MAX_TRAP_DEPTH);
    if (f_int_traps == NULL) {
      ascfree(f_fpe_traps);
      f_fpe_traps = NULL;
      return 1;
    }
  }
  f_int_top_of_stack = -1;

  /* initialize SIGSEGV stack */
  if (f_seg_traps == NULL) {
    f_seg_traps = ASC_NEW_ARRAY_CLEAR(SigHandlerFn*,MAX_TRAP_DEPTH);
    if (f_seg_traps == NULL) {
      ascfree(f_fpe_traps);
      f_fpe_traps = NULL;
      ascfree(f_int_traps);
      f_int_traps = NULL;
      return 1;
    }
  }
  f_seg_top_of_stack = -1;

#ifndef NO_SIGNAL_TRAPS
  /* push the old ones if any, on the stack. */
  initstack(f_fpe_traps, &f_fpe_top_of_stack, SIGFPE);

# ifndef NO_SIGINT_TRAP
  initstack(f_int_traps, &f_int_top_of_stack, SIGINT);
# endif

# ifndef NO_SIGSEGV_TRAP
  initstack(f_seg_traps, &f_seg_top_of_stack, SIGSEGV);
# endif

  f_reset_needed = ascresetneeded();
  if (f_reset_needed < 0) {
    f_reset_needed = 1;
    return 2;
  }
#endif /* NO_SIGNAL_TRAPS */
  return 0;
}

/**
	Clears and destroys the stacks of signal handlers.
*/
void Asc_SignalDestroy(void)
{
  ascfree(f_fpe_traps);
  ascfree(f_int_traps);
  ascfree(f_seg_traps);
  f_fpe_traps = f_int_traps = f_seg_traps =  NULL;
  f_fpe_top_of_stack = f_int_top_of_stack = f_seg_top_of_stack =  -1;
}

static void reset_trap(int signum, SigHandlerFn **tlist, int tos)
{
  SigHandlerFn *tp;
  if ((tlist != NULL) && (tos >= 0) && (tos < MAX_TRAP_DEPTH)) {
    tp = tlist[tos];
    if (tp != SIG_ERR) {
      (void)signal(signum,tp);
    }
  } else {
    (void)signal(signum,SIG_DFL);
  }
}
/*
	This function reinstalls all the signal handlers this module
	has been informed of. This should be called after every
	trapped exception and at any other time when the status of
	exception handlers may have become not well defined.
	The most recently pushed handler is installed for each supported
	signal. If nothing on stack, SIG_DFL gets installed.

	@NOTE that if somebody installs a handler without going through
	our push/pop, theirs is liable to be forgotten.
*/
void Asc_SignalRecover(int force)
{
  if (force || f_reset_needed > 0) {
#ifndef NO_SIGNAL_TRAPS
    reset_trap(SIGFPE, f_fpe_traps, f_fpe_top_of_stack);
    reset_trap(SIGINT, f_int_traps, f_int_top_of_stack);
    reset_trap(SIGSEGV, f_seg_traps, f_seg_top_of_stack);
#endif /* NO_SIGNAL_TRAPS */
  }
}

/**
	Append a pointer to the list given, if the list is not full.
*/
static int push_trap(SigHandlerFn **tlist, int *stackptr, SigHandlerFn *tp)
{
  if (tlist == NULL) {
    CONSOLE_DEBUG("TLIST IS NULL");
    return -1;
  }
  if (stackptr == NULL) {
    CONSOLE_DEBUG("STACKPTR IS NULL");
    return -1;
  }
  if (tp == NULL) {
    CONSOLE_DEBUG("TP IS NULL");
    return 2;
  }
  if (*stackptr > MAX_TRAP_DEPTH-1) {
    CONSOLE_DEBUG("TLIST LENGTH = CAPACITY");
    return 1;
  }
  ++(*stackptr);
  tlist[*stackptr] = tp;
  return 0;
}

/*
	Add a handler to the stack of signal handlers for the given signal.

	There is a maximum stack limit, so returns 1 if limit exceeded.
	Returns -1 if stack of signal requested does not exist.
	Pushing a NULL handler does NOT change anything at all.
	On a successful return, the handler has been installed and will
	remain installed until a Asc_SignalHandlerPop or another push.
*/
int Asc_SignalHandlerPush(int signum, SigHandlerFn *tp)
{
  int err;
  if (tp == NULL) {
    return 0;
  }
  switch (signum) {
    case SIGFPE:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGFPE");*/
      err = push_trap(f_fpe_traps, &f_fpe_top_of_stack, tp);
      break;
    case SIGINT:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGINT");*/
      err = push_trap(f_int_traps, &f_int_top_of_stack, tp);
      break;
    case SIGSEGV:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGSEGV");*/
      err = push_trap(f_seg_traps, &f_seg_top_of_stack, tp);
      break;
    default:
      return -1;
  }
  if (err != 0) {
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Asc_Signal (%d) stack limit exceeded.",signum);
    return err;
  }
  (void)signal(signum, tp); /* install */
  return 0;
}

/*
	Returns: 0 -ok, 2 NULL list input, 1 empty list input,
	-1 mismatched input tp and stack data.
*/
static int pop_trap(SigHandlerFn **tlist, int *stackptr, SigHandlerFn *tp){
  SigHandlerFn *oldtrap;

  if ((tlist == NULL) || (stackptr == NULL)) {
    return 2;
  }
  if (*stackptr < 0) {
    return 1;
  }
  oldtrap = tlist[*stackptr];
  tlist[*stackptr] = NULL;
  --(*stackptr);
  return (-(oldtrap != tp));
}

int Asc_SignalHandlerPop(int signum, SigHandlerFn *tp){
  int err;
  switch (signum) {
  case SIGFPE:
    /*ERROR_REPORTER_DEBUG("POP SIGFPE");*/
    err = pop_trap(f_fpe_traps, &f_fpe_top_of_stack, tp);
    break;
  case SIGINT:
    /*ERROR_REPORTER_DEBUG("POP SIGINT");*/
    err = pop_trap(f_int_traps, &f_int_top_of_stack, tp);
    break;
  case SIGSEGV:
    /*ERROR_REPORTER_DEBUG("POP SIGSEGV");*/
    err = pop_trap(f_seg_traps, &f_seg_top_of_stack, tp);
    break;
  default:
	CONSOLE_DEBUG("popping invalid signal type (signum = %d)", signum);
    return -1;
  }
  if (err != 0 && tp != NULL) {
	CONSOLE_DEBUG("stack pop mismatch");
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Asc_Signal (%d) stack pop mismatch.",signum);
    return err;
  }
  Asc_SignalRecover(TRUE);
  return 0;
}

void Asc_SignalTrap(int sigval) {
#ifndef NO_SIGNAL_TRAPS
  switch(sigval) {
  case SIGFPE:
	CONSOLE_DEBUG("SIGFPE caught");
    FPRESET;
    longjmp(g_fpe_env,sigval);
    break;
  case SIGINT:
	CONSOLE_DEBUG("SIGINT (Ctrl-C) caught");
    longjmp(g_int_env,sigval);
    break;
  case SIGSEGV:
	CONSOLE_DEBUG("SIGSEGV caught");
    longjmp(g_seg_env,sigval);
    break;
  default:
    CONSOLE_DEBUG("Installed on unexpected signal (sigval = %d).", sigval);
    CONSOLE_DEBUG("Returning ... who knows where :-)");
    break;
  }
  return;
#else
   UNUSED_PARAMETER(sigval);
#endif /* NO_SIGNAL_TRAPS */
}
