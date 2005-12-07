/*
 *  Signal handling protocol definitions for ASCEND
 *  May 27, 1997
 *  By Benjamin Andrew Allan
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: ascSignal.c,v $
 *  Date last modified: $Date: 1999/01/19 12:23:20 $
 *  Last modified by: $Author: mthomas $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Programming System.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
 *
 *  The Ascend Programming System is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */
/*
 * Start of making signal handling in ASCEND 
 * code somewhat sane. Still needs to somehow
 * support the management of jmp_buf's so that
 * handlers will longjmp to the right place.
 *
 * A better alternative is to make all our code check/return
 * status flags based on a variable set by the trap
 * and cleared by recipient.
 */
/*
 *  ChangeLog
 *
 *  10/15/2005  - Changed ascresetneeded() so that any previously
 *                registered handlers are reset before return.
 *              - Added Asc_SignalRecover() to standard handler
 *                Asc_SignalTrap() so handlers are reset if necessary. (JDS)
 */

#include <stdio.h>
#include "utilities/ascConfig.h"

#ifndef NO_SIGNAL_TRAPS 
# include <signal.h>
# include <setjmp.h>
#endif /* NO_SIGNAL_TRAPS*/

#ifdef __WIN32__
# include <process.h>
#else
# include <unistd.h>
#endif

#include "utilities/ascSignal.h"
#include "general/list.h"

static jmp_buf f_test_env;    /* for local testing of signal handling */

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
static struct gl_list_t *f_fpe_traps = NULL;
static struct gl_list_t *f_int_traps = NULL;
static struct gl_list_t *f_seg_traps = NULL;
/*
 * Batch of globals because we don't want an array of
 * all possible signals, most of which are NULL entries.
 * Each list holds the stack of pointers to signal handlers.
 */

/* function to throw an interrupt. system dependent. */
static int testdooley2(int sig)
{
  raise(sig);

  return 0;
}

/* function to catch an interrupt */
static void testctrlc(int signum)
{
  FPRINTF(ASCERR," signal %d caught ",signum);
  if (signum == SIGFPE) {
    FPRESET;
  }
  longjmp(f_test_env, signum);
}

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
/* This function tests the signal reseting of compilers using SIGINT.
 * It should not be called except when starting a process.
 * Return 0 for no reset needed, 1 for reset needed, and
 * -1 if the test fails (presuming program doesn't exit first.)
 * Side effects:  
 *   - a line is sent to ASCERR
 *   - SIGINT is set to SIG_DFL if no handler was previously registered
 *   - SIGFPE may be set to SIG_DFL if no handler was previously registered
 */
static int ascresetneeded(void) {
  static int c=0;
  static int result;
  SigHandler lasttrap;
  volatile SigHandler savedtrap;

  result = 0;

#ifndef NO_SIGINT_TRAP
  /* test interrupt */
  savedtrap = signal(SIGINT, testctrlc);
  CONSOLE_DEBUG("Testing signal SIGINT (signum = %d) %p\t%p\t", SIGINT, savedtrap, testctrlc);
  if (setjmp(f_test_env) == 0) {
    testdooley2(SIGINT);
  } else {
    c++;
  }
  if (c != 1) {
	CONSOLE_DEBUG("SIGINT test failed");
    error_reporter(ASC_PROG_ERROR,NULL,0,"Signal (SIGINT) test failed. ASCEND unlikely to work on this hardware.");
    result = -1;
  }
  lasttrap = signal(SIGINT, (NULL != savedtrap) ? savedtrap : SIG_DFL);
  CONSOLE_DEBUG("%p",lasttrap);
  if (lasttrap != testctrlc) {
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
  savedtrap=signal(SIGFPE, testctrlc);
  CONSOLE_DEBUG("Testing signal %d %p\t%p\t",SIGFPE, savedtrap, testctrlc);
  if (setjmp(f_test_env)==0) {
    testdooley2(SIGFPE);
  } else {
    c++;
  }
  if (c != 1) {
	CONSOLE_DEBUG("SIGFPE test failed");
    error_reporter(ASC_PROG_ERROR,NULL,0,"Signal test failed. ASCEND unlikely to work on this hardware.");
    result = -1;
  }
  lasttrap = signal(SIGFPE, (NULL != savedtrap) ? savedtrap : SIG_DFL);
  CONSOLE_DEBUG("%p\n",lasttrap);
  if (lasttrap != testctrlc) {
    result = 1;
  }
#else
  CONSOLE_DEBUG("SIGSEGV trap bypassed: compile-time settings\n");
#endif

  return result;
}

static
void initstack (struct gl_list_t *traps, int sig)
{
  SigHandler old;
  old = signal(sig,SIG_DFL);
  if (old != SIG_ERR && old != SIG_DFL) {
    gl_append_ptr(traps,(VOIDPTR)old);
    (void)signal(sig,old);
  }
}
/*
 * Returns 0 if successful, 1 if out of memory, 2 otherwise.
 * Does not establish any traps, just the structures for
 * maintaining them. Pushes the existing traps, if any, on
 * the bottom of the created stacks.
 * Cannot be called twice successfully.
 */
int Asc_SignalInit(void)
{
  if ((f_reset_needed != -2) || (FALSE == gl_pool_initialized())) {
    return 2;
  }
  f_fpe_traps = gl_create(MAX_TRAP_DEPTH);
  f_int_traps = gl_create(MAX_TRAP_DEPTH);
  f_seg_traps = gl_create(MAX_TRAP_DEPTH);
  if (f_fpe_traps == NULL || f_int_traps == NULL || f_seg_traps == NULL) {
    return 1;
  }
#ifndef NO_SIGNAL_TRAPS
  /* push the old ones if any, on the stack. */
  initstack(f_fpe_traps, SIGFPE);

# ifndef NO_SIGINT_TRAP
  initstack(f_int_traps, SIGINT);
# endif

# ifndef NO_SIGSEGV_TRAP
  initstack(f_seg_traps, SIGSEGV);
# endif
  
  f_reset_needed = ascresetneeded();
  if (f_reset_needed < 0) {
    f_reset_needed = 1;
    return 2;
  }
#endif /* NO_SIGNAL_TRAPS */
  return 0;
}

/*
 * clears and destroys the stacks of signal handlers.
 */
void Asc_SignalDestroy(void)
{
  gl_destroy(f_fpe_traps);
  gl_destroy(f_int_traps);
  gl_destroy(f_seg_traps);
  f_fpe_traps = f_int_traps = f_seg_traps =  NULL;
}

static void reset_trap(int signum, struct gl_list_t *tlist)
{
  SigHandler tp;
  if (tlist != NULL && gl_length(tlist) > 0L) {
    tp = (SigHandler)gl_fetch(tlist, gl_length(tlist));
    if (tp != SIG_ERR) {
      (void)signal(signum,tp);
    }
  } else {
    (void)signal(signum,SIG_DFL);
  }
}
/* This function reinstalls all the signal handlers this module
 * has been informed of. This should be called after every
 * trapped exception and at any other time when the status of
 * exception handlers may have become not well defined.
 * The most recently pushed handler is installed for each supported
 * signal. If nothing on stack, SIG_DFL gets installed.
 *_Note that if somebody installs a handler without going through
 * our push/pop, theirs is liable to be forgotten.
 */
void Asc_SignalRecover(int force) {
  if (force || f_reset_needed > 0) {
#ifndef NO_SIGNAL_TRAPS 
    reset_trap(SIGFPE, f_fpe_traps);
    reset_trap(SIGINT, f_int_traps);
    reset_trap(SIGSEGV, f_seg_traps);
#endif /* NO_SIGNAL_TRAPS */
  }
}

/*
 * append a pointer to the list given, if the list is not full.
 */
static int push_trap(struct gl_list_t *tlist, SigHandler tp)
{
  if (tlist == NULL) {
	CONSOLE_DEBUG("TLIST IS NULL");
    return -1;
  }
  if (gl_length(tlist) == gl_capacity(tlist)) {
  	CONSOLE_DEBUG("TLIST LENGTH = CAPACITY");
    return 1;
  }
  gl_append_ptr(tlist,(VOIDPTR)tp);
  return 0;
}

/* 
 * Adds a handler to the stack of signal handlers for the given signal.
 * There is a maximum stack limit, so returns 1 if limit exceeded.
 * Returns -1 if stack of signal requested does not exist.
 * Pushing a NULL handler does NOT change anything at all.
 * On a successful return, the handler has been installed and will
 * remain installed until a Asc_SignalHandlerPop or another push.
 */
int Asc_SignalHandlerPush(int signum, SigHandler tp)
{
  int err;
  if (tp == NULL) {
    return 0;
  }
  switch (signum) {
    case SIGFPE:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGFPE");*/
      err = push_trap(f_fpe_traps,tp);
      break;
    case SIGINT:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGINT");*/
      err = push_trap(f_int_traps,tp);
      break;
    case SIGSEGV:
	  /*ERROR_REPORTER_DEBUG("PUSH SIGSEGV");*/
      err = push_trap(f_seg_traps,tp);
      break;
    default:
      return -1;
  }
  if (err != 0) {
    error_reporter(ASC_PROG_ERROR,__FILE__,__LINE__,"Asc_Signal (%d) stack limit exceeded.",signum);
    return err;
  }
  (void)signal(signum, tp); /* install */
  return 0;
}

/*
 * Returns: 0 -ok, 2 NULL list input, 1 empty list input,
 * -1 mismatched input tp and stack data.
 */
static int pop_trap(struct gl_list_t *tlist, SigHandler tp)
{
  SigHandler oldtrap;

  if (tlist == NULL) {
    return 2;
  } 
  if (gl_length(tlist) == 0) {
    return 1;
  }
  oldtrap = (SigHandler)gl_fetch(tlist,gl_length(tlist));
  gl_delete(tlist,gl_length(tlist),0);
  return (-(oldtrap != tp));
}

int Asc_SignalHandlerPop(int signum, SigHandler tp)
{
  int err;
  switch (signum) {
  case SIGFPE:
    /*ERROR_REPORTER_DEBUG("POP SIGFPE");*/
    err = pop_trap(f_fpe_traps,tp);
    break;
  case SIGINT:
    /*ERROR_REPORTER_DEBUG("POP SIGINT");*/
    err = pop_trap(f_int_traps,tp);
    break;
  case SIGSEGV:
    /*ERROR_REPORTER_DEBUG("POP SIGSEGV");*/
    err = pop_trap(f_seg_traps,tp);
    break;
  default:
	CONSOLE_DEBUG("popping invalid signal type (signum = %d)", signum);
    return -1;
  }
  if (err != 0 && tp != NULL) {
	CONSOLE_DEBUG("stack pop mismatch");
    error_reporter(ASC_PROG_ERROR,__FILE__,__LINE__,"Asc_Signal (%d) stack pop mismatch.",signum);
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
