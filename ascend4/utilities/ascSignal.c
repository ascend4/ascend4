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

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include "utilities/ascConfig.h"
#ifdef __WIN32__
#include <process.h>
#else
#include <unistd.h>
#endif
#include "utilities/ascSignal.h"
#include "general/list.h"


#define TRAPPTR(a) void (*(a))(int)
/*
 * This macro defines a pointer to a signal handler named a.
 */
#define TPCAST void (*)(int)
/*
 * This macro can be used like  tp = (TPCAST)foo;
 */

static jmp_buf g_test_env;
/* test buf for initialization */
jmp_buf g_fpe_env;
jmp_buf g_seg_env;
jmp_buf g_int_env;

/* for future use */
jmp_buf g_foreign_code_call_env;

static int g_reset_needed = -2;
/* has value 0 or 1 after Init is called.
 * and if Init is called without the value -2 in g_reset_needed,
 * it will fail.
 */
static struct gl_list_t *g_fpe_traps = NULL;
static struct gl_list_t *g_int_traps = NULL;
static struct gl_list_t *g_seg_traps = NULL;
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
  (void) signum;
  FPRINTF(ASCERR," signal %d caught ",signum);
  if (signum == SIGFPE) {
    FPRESET;
  }
  longjmp(g_test_env,signum);
}

/*
 * So far the following seem to need reset trapped signals after
 * a longjmp, or unconditionally.
 * HPUX cc -Aa -D_HPUX_SOURCE 
 * Solaris cc
 * AIX xlc
 * IRIX cc
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
 * Side effects: SIGINT is left in the SIG_DFL state, and
 * a line is sent to ASCERR.
 */
static int ascresetneeded(void) {
  static int c=0;
  static int result;
  TRAPPTR(lasttrap);

  result = 0;

  /* test interrupt */
  lasttrap=signal(SIGINT,testctrlc);
  PRINTF("Testing signal %d %p\t%p\t",SIGINT,lasttrap,testctrlc);
  if (setjmp(g_test_env)==0) {
    testdooley2(SIGINT);
  } else {
    c++;
  }
  if (c != 1) {
    PRINTF("Signal test failed. ASCEND unlikely to work on this hardware.\n");
    result = -1;
  }
  lasttrap = signal(SIGINT,SIG_DFL);
  PRINTF("%p\n",lasttrap);
  if (lasttrap != testctrlc) {
    result = 1;
  }

  if (result != 0) {
    return result;
  }

  c = 0;
  /* passed interrupt, check fpe */
  lasttrap=signal(SIGFPE,testctrlc);
  PRINTF("Testing signal %d %p\t%p\t",SIGFPE,lasttrap,testctrlc);
  if (setjmp(g_test_env)==0) {
    testdooley2(SIGFPE);
  } else {
    c++;
  }
  if (c != 1) {
    PRINTF("Signal test failed. ASCEND unlikely to work on this hardware.\n");
    result = -1;
  }
  lasttrap = signal(SIGFPE,SIG_DFL);
  PRINTF("%p\n",lasttrap);
  if (lasttrap != testctrlc) {
    result = 1;
  }

  return result;
}

static
void initstack (struct gl_list_t *traps, int sig)
{
  TRAPPTR(old);
  old = signal(sig,SIG_DFL);
  if (old != SIG_ERR && old != SIG_DFL) {
    gl_append_ptr(traps,(VOIDPTR)old);
    signal(sig,old);
  }
}
/*
 * Returns 0 if successful, 1 if out of memory, 2 otherwyse.
 * Does not establish any traps, just the structures for
 * maintaining them. Pushes the existing traps, if any, on
 * the bottom of the created stacks.
 * Cannot be called twice successfully.
 */
int Asc_SignalInit(void)
{
  if (g_reset_needed != -2) {
    return 2;
  }
  g_fpe_traps = gl_create(MAX_TRAP_DEPTH);
  g_int_traps = gl_create(MAX_TRAP_DEPTH);
  g_seg_traps = gl_create(MAX_TRAP_DEPTH);
  /* push the old ones if any, on the stack. */
  initstack(g_fpe_traps,SIGFPE);
  initstack(g_int_traps,SIGINT);
  initstack(g_seg_traps,SIGSEGV);
  
  g_reset_needed = ascresetneeded();
  if (g_reset_needed < 0) {
    g_reset_needed = 1;
    return 2;
  }
  if (g_fpe_traps == NULL || g_int_traps == NULL || g_seg_traps == NULL) {
    return 1;
  }
  return 0;
}

/*
 * clears and destroys the stacks of signal handlers.
 */
void Asc_SignalDestroy(void)
{
  gl_destroy(g_fpe_traps);
  gl_destroy(g_int_traps);
  gl_destroy(g_seg_traps);
  g_fpe_traps = g_int_traps = g_seg_traps =  NULL;
}

static void reset_trap(int signum,struct gl_list_t *tlist)
{
  TRAPPTR(tp);
  if (tlist != NULL && gl_length(tlist) > 0L) {
    tp = (TPCAST)gl_fetch(tlist, gl_length(tlist));
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
  if (force || g_reset_needed > 0) {
    reset_trap(SIGFPE,g_fpe_traps);
    reset_trap(SIGINT,g_int_traps);
    reset_trap(SIGSEGV,g_seg_traps);
  }
}

/*
 * append a pointer to the list given, if the list is not full.
 */
static int push_trap(struct gl_list_t *tlist, TRAPPTR(tp))
{
  if (tlist == NULL) {
    return -1;
  }
  if (gl_length(tlist) == gl_capacity(tlist)) {
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
int Asc_SignalHandlerPush(int signum, TRAPPTR(tp))
{
  int err;
  if (tp==NULL) {
    return 0;
  }
  switch (signum) {
  case SIGFPE:
    err = push_trap(g_fpe_traps,tp);
    break;
  case SIGINT:
    err = push_trap(g_int_traps,tp);
    break;
  case SIGSEGV:
    err = push_trap(g_seg_traps,tp);
    break;
  default:
    return -1;
  }
  if (err != 0) {
    FPRINTF(ASCERR,"Asc_Signal (%d) stack limit exceeded.\n",signum);
    return err;
  }
  (void)signal(signum,tp); /* install */
  return 0;
}

/*
 * Returns: 0 -ok, 2 NULL list input, 1 empty list input,
 * -1 mismatched input tp and stack data.
 */
static int pop_trap(struct gl_list_t *tlist, TRAPPTR(tp))
{
  TRAPPTR(oldtrap);

  if (tlist == NULL) {
    return 2;
  } 
  if (gl_length(tlist) == 0) {
    return 1;
  }
  oldtrap = (TPCAST)gl_fetch(tlist,gl_length(tlist));
  gl_delete(tlist,gl_length(tlist),0);
  return (-(oldtrap != tp));
}

int Asc_SignalHandlerPop(int signum, TRAPPTR(tp))
{
  int err;
  switch (signum) {
  case SIGFPE:
    err = pop_trap(g_fpe_traps,tp);
    break;
  case SIGINT:
    err = pop_trap(g_int_traps,tp);
    break;
  case SIGSEGV:
    err = pop_trap(g_seg_traps,tp);
    break;
  default:
    return -1;
  }
  if (err != 0 && tp != NULL) {
    FPRINTF(ASCERR,"Asc_Signal (%d) stack pop mismatch.\n",signum);
    return err;
  }
  Asc_SignalRecover(0);
  return 0;
}

void Asc_SignalTrap(int sigval) {
  switch(sigval) {
  case SIGFPE:
#ifndef __WIN32__
    FPRINTF(ASCERR,"Asc_SignalTrap: SIGFPE caught\n");
#endif
    FPRESET;
    longjmp(g_fpe_env,sigval);
    break;
  case SIGINT:
    FPRINTF(ASCERR,"Asc_SignalTrap: SIGINT (Ctrl-C) caught\n");
    longjmp(g_int_env,sigval);
    break;
  case SIGSEGV:
    FPRINTF(ASCERR,"Asc_SignalTrap: SIGSEGV (bad address) caught\n");
    longjmp(g_seg_env,sigval);
    break;
  default:
    FPRINTF(ASCERR,"Asc_SignalTrap: Installed on unknown signal %d\n",sigval);
    FPRINTF(ASCERR,"Asc_SignalTrap: Returning ... who knows where.");
    break;
  }
  return;
}
