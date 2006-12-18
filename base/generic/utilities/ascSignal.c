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
	
	10/15/2005  - Changed ascresetneeded() so that any previously
	              registered handlers are reset before return.
	            - Added Asc_SignalRecover() to standard handler
	              Asc_SignalTrap() so handlers are reset if necessary. (JDS)
	12/10/2005  - Changed storage of signal handlers from gl_list's
	              to local arrays.  gl_list's can't legally hold
	              function pointers. (JDS)
	18 Dec 06   - Removed ascresetneeded (moved to SConstruct)
*/

#include <stdio.h>
#include "config.h"
#include "ascConfig.h"

#include <signal.h>
#include <setjmp.h>

#ifdef HAVE_C99FPE
# include <fenv.h>
#endif

#ifdef __WIN32__
# include <process.h>
#else
# include <unistd.h>
#endif

#include "ascMalloc.h"
#include "ascSignal.h"

/*------------------------------------------------------------------------------
  GLOBALS AND FOWARD DECS
*/

/* test buf for initialization */
jmp_buf g_fpe_env;
jmp_buf g_seg_env;
jmp_buf g_int_env;

#ifdef HAVE_C99FPE
fenv_t g_fenv;
#endif

/* for future use */
jmp_buf g_foreign_code_call_env;

static SigHandlerFn **f_fpe_traps = NULL;  /**< array for pushed SIGFPE handlers */
static int f_fpe_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_int_traps = NULL;  /**< array for pushed SIGINT handlers */
static int f_int_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_seg_traps = NULL;  /**< array for pushed SIGSEGV handlers */
static int f_seg_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

#ifdef HAVE_C99FPE
static fenv_t *f_fenv_stack = NULL;
static int f_fenv_stack_top = -1;
#endif

static void initstack (SigHandlerFn **traps, int *stackptr, int sig);
static int pop_trap(SigHandlerFn **tlist, int *stackptr, SigHandlerFn *tp);
static int push_trap(SigHandlerFn **tlist, int *stackptr, SigHandlerFn *tp);
static void reset_trap(int signum, SigHandlerFn **tlist, int tos);

#ifdef HAVE_C99FPE
static int fenv_pop(fenv_t *stack, int *top);
static int fenv_push(fenv_t *stack,int *top);
#endif

/*------------------------------------------------------------------------------
  API FUNCTIONS
*/

/**
	Initialise ASCEND signal handling

	Does not establish any traps, just the structures for maintaining them. 
	Pushes the existing traps, if any, on the bottom of the created stacks.

	@NOTE Cannot be called twice successfully.

	@return 0 if successful, 1 if out of memory, 2 otherwise.
*/
int Asc_SignalInit(void)
{
  /* initialize SIGFPE stack */
  if (f_fpe_traps == NULL) {
    f_fpe_traps = ASC_NEW_ARRAY_CLEAR(SigHandlerFn*,MAX_TRAP_DEPTH);
    if (f_fpe_traps == NULL) {
      return 1;
    }
  }
  f_fpe_top_of_stack = -1;

#ifdef HAVE_C99FPE
  if(f_fenv_stack==NULL){ /* if we haven't already initialised this... */
    f_fenv_stack = ASC_NEW_ARRAY_CLEAR(fenv_t,MAX_TRAP_DEPTH);
	if(f_fenv_stack == NULL) return 1; /* failed to allocate */
  }
  f_fenv_stack_top = -1;
#endif

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

  /* push the old ones if any, on the stack. */
  initstack(f_fpe_traps, &f_fpe_top_of_stack, SIGFPE);
  initstack(f_int_traps, &f_int_top_of_stack, SIGINT);
  initstack(f_seg_traps, &f_seg_top_of_stack, SIGSEGV);

#ifdef HAVE_C99FPE
  CONSOLE_DEBUG("Adding original FPE state to stack (%d)",f_fenv_stack_top);
  fenv_push(f_fenv_stack,&f_fenv_stack_top);
#endif

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

/**
	This function reinstalls all the signal handlers this module
	has been informed of. This should be called after every
	trapped exception and at any other time when the status of
	exception handlers may have become not well defined.
	The most recently pushed handler is installed for each supported
	signal. If nothing on stack, SIG_DFL gets installed.

	@NOTE that if somebody installs a handler without going through
	our push/pop, theirs is liable to be forgotten.
*/
void Asc_SignalRecover(int force){
#ifndef ASC_RESETNEEDED
	if(force){
#endif
		reset_trap(SIGFPE, f_fpe_traps, f_fpe_top_of_stack);
		reset_trap(SIGINT, f_int_traps, f_int_top_of_stack);
		reset_trap(SIGSEGV, f_seg_traps, f_seg_top_of_stack);
#ifndef ASC_RESETNEEDED
	}
#endif
}

int Asc_SignalHandlerPushDefault(int signum){
	return Asc_SignalHandlerPush(signum, &Asc_SignalTrap);
}

/**
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
	  //CONSOLE_DEBUG("PUSH SIGFPE");
      err = push_trap(f_fpe_traps, &f_fpe_top_of_stack, tp);
#ifdef HAVE_C99FPE
      err = fenv_push(f_fenv_stack, &f_fenv_stack_top);
#endif
      break;
    case SIGINT:
	  /* CONSOLE_DEBUG("PUSH SIGINT"); */
      err = push_trap(f_int_traps, &f_int_top_of_stack, tp);
      break;
    case SIGSEGV:
	  /* CONSOLE_DEBUG("PUSH SIGSEGV"); */
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

int Asc_SignalHandlerPopDefault(int signum){
	return Asc_SignalHandlerPop(signum, &Asc_SignalTrap);
}

/* see ascSignal.h */
int Asc_SignalHandlerPop(int signum, SigHandlerFn *tp){
  int err;
  switch (signum) {
  case SIGFPE:
    //CONSOLE_DEBUG("POP SIGFPE");
    err = pop_trap(f_fpe_traps, &f_fpe_top_of_stack, tp);
#ifdef HAVE_C99FPE
	err = fenv_pop(f_fenv_stack, &f_fenv_stack_top);
#endif
    break;
  case SIGINT:
    /* CONSOLE_DEBUG("POP SIGINT"); */
    err = pop_trap(f_int_traps, &f_int_top_of_stack, tp);
    break;
  case SIGSEGV:
    /* CONSOLE_DEBUG("POP SIGSEGV"); */
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
  switch(sigval) {
  case SIGFPE:
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Floating point error caught");
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
}

/*------------------------------------------------------------------------------
  UTILITY FUNCTIONS
*/

/*
	Removed ascresetneeded from here. This is a build-time configuration test
	rather than a runtime test (and causes annoyance when running ASCEND through
	a debugger).

	So far the following seem to need reset trapped signals after
	a longjmp, or unconditionally.
		HPUX cc -Aa -D_HPUX_SOURCE
		Solaris cc
		AIX xlc
		IRIX cc
		Windows

	The following retain the last trap set with or without a call to longjmp
	and so don't need resetting of traps.
		SunOS4 acc
		OSF32 cc
		NetBSD gcc 2.4.5 -ansi
		Linux gcc (i386)
*/

//------------------------------------
// COMMOM STACK ROUTINES (shared by the three different signal handler stacks)

static void initstack(SigHandlerFn **traps, int *stackptr, int sig){
  SigHandlerFn *old;
  old = signal(sig,SIG_DFL);
  if (old != SIG_ERR && old != SIG_DFL) {
    traps[0] = old;
    *stackptr = 0;
    (void)signal(sig,old);
  }
}

static void reset_trap(int signum, SigHandlerFn **tlist, int tos){
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

/**
	Append a pointer to the list given, if the list is not full.
*/
static int push_trap(SigHandlerFn **tlist, int *stackptr, SigHandlerFn *tp){
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

/*------------------------------------------
  FPE ENV STACK
*/

#ifdef HAVE_C99FPE

/** 
	Store current FPU state so that we can reset it later (after we've done
	some stuff)

	return 0 on success 
*/
static int fenv_push(fenv_t *stack,int *top){
	if(*top > MAX_TRAP_DEPTH - 1){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"FPE stack is full");
		return 1;
	}
	if(*top < -1){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"stack top < -1");
		return 2;
	}
	if(stack==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"stack is NULL");
		return 3;
	}
	fenv_t *fe = &stack[++(*top)];
	if(fegetenv(fe)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"unable to get env");
		return 4;
	}
	feenableexcept(FE_DIVBYZERO);
	//CONSOLE_DEBUG("Enabled div-by-zero FPE exception (%d)",*top);
	return 0;
}

/**
	Restore a save FPU state. Return 0 on success.
*/
static int fenv_pop(fenv_t *stack, int *top){
	if(*top < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"FPE stack is empty");
		return 1;
	}
	if(stack==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"stack is NULL");
		return 2;
	}
	fenv_t *fe = &stack[(*top)--];
	if(fesetenv(fe)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"unable to set env");
		return 3;
	}
	//CONSOLE_DEBUG("Restorted FPE state");
	return 0;
}

#endif /* HAVE_C99FPE */

