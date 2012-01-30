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
	25 Jan 12   - Add debug capability for contents of stacks (JDP)
*/

#include "ascSignal.h"
#ifdef ASC_SIGNAL_TRAPS

/*-------- this file will do nothing unless ASC_SIGNAL_TRAPS is turned on -----*/

#include <stdio.h>


#include <signal.h>

#ifdef HAVE_C99FPE
# include <fenv.h>
#endif


#ifdef __WIN32__
# include <process.h>
#else
# include <unistd.h>
#endif

#include <ascend/general/ascMalloc.h>
#include "ascSignal.h"
#include <ascend/general/panic.h>

/* #define SIGNAL_DEBUG */

/*------------------------------------------------------------------------------
  GLOBALS AND FOWARD DECS
*/

/* test buf for initialization */
JMP_BUF g_fpe_env;
JMP_BUF g_seg_env;
JMP_BUF g_int_env;

#ifdef HAVE_C99FPE
fenv_t g_fenv;
#endif

/* for future use */
jmp_buf g_foreign_code_call_env;

/**
	Struct to store signal stacks, with support for debugging names and
	location where handlers were pushed.
*/
typedef struct{
	SigHandlerFn *handler;
#ifdef SIGNAL_DEBUG
	char *name;
	char *file;
	int line;
#endif
} SignalStackItem;

typedef struct{
	SignalStackItem fpe_traps[MAX_TRAP_DEPTH];
	int fpe_top;
	SignalStackItem int_traps[MAX_TRAP_DEPTH];
	int int_top;
	SignalStackItem seg_traps[MAX_TRAP_DEPTH];
	int seg_top;
} SignalStacks;

static SignalStacks *f_traps = NULL;

#if 0
static SigHandlerFn **f_fpe_traps = NULL;  /**< array for pushed SIGFPE handlers */
static int f_fpe_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_int_traps = NULL;  /**< array for pushed SIGINT handlers */
static int f_int_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */

static SigHandlerFn **f_seg_traps = NULL;  /**< array for pushed SIGSEGV handlers */
static int f_seg_top_of_stack = -1;     /**< top of SIGFPE stack, -1 for empty */
#endif

#ifdef HAVE_C99FPE
static fenv_t *f_fenv_stack = NULL;
static int f_fenv_stack_top = -1;
#endif

static void initstack(int sig);
static int pop_trap(int signum, SigHandlerFn *tp, char *name, char *file, int line);
static int push_trap(int signum, SigHandlerFn *func, char *name, char *file, int line);
static void reset_trap(int signum);

#ifdef HAVE_C99FPE
static int fenv_pop(fenv_t *stack, int *top);
static int fenv_push(fenv_t *stack,int *top, int excepts);
#endif

#define SIGNAME(SIGNUM) (SIGNUM==SIGFPE?"SIGFPE":(SIGNUM==SIGINT?"SIGINT":(SIGNUM==SIGSEGV?"SIGSEGV":"unknown")))
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
int Asc_SignalInit(void){
  /* initialize SIGFPE stack */

#ifdef SIGNAL_DEBUG
# ifdef ASC_RESETNEEDED
  CONSOLE_DEBUG("Initialising signal stack (with resets needed)");
# else
  CONSOLE_DEBUG("Initialising signal stack (with resets not required)");
# endif
#endif

  if(f_traps == NULL){
    f_traps = ASC_NEW(SignalStacks);
    if(!f_traps){
      return 1;
    }
    f_traps->fpe_top = -1;
    f_traps->int_top = -1;
    f_traps->seg_top = -1;
  }

#ifdef HAVE_C99FPE
  if(f_fenv_stack==NULL){ /* if we haven't already initialised this... */
    f_fenv_stack = ASC_NEW_ARRAY_CLEAR(fenv_t,MAX_TRAP_DEPTH);
	if(f_fenv_stack == NULL){
      return 1; /* failed to allocate */
    }
  }
  f_fenv_stack_top = -1;
#endif

  /* old signals are *not* stored */
  initstack(SIGFPE);
  initstack(SIGINT);
  initstack(SIGSEGV);

#if defined(HAVE_C99FPE)
  CONSOLE_DEBUG("Initialise FPE state to stack (%d)",f_fenv_stack_top);
  fenv_push(f_fenv_stack,&f_fenv_stack_top,0);
#endif 

  return 0;
}

/**
	Clears and destroys the stacks of signal handlers.
*/
void Asc_SignalDestroy(void)
{
  ascfree(f_traps);
#ifdef HAVE_C99FPE
  if(f_fenv_stack){
    ASC_FREE(f_fenv_stack);
    f_fenv_stack = NULL;
  }
#endif
  f_traps = NULL;
#ifdef SIGNAL_DEBUG
  CONSOLE_DEBUG("Destroyed signal stack");
#endif
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
# ifdef SIGNAL_DEBUG
		CONSOLE_DEBUG("Resetting traps");
# endif
		reset_trap(SIGFPE);
		reset_trap(SIGINT);
		reset_trap(SIGSEGV);
#ifndef ASC_RESETNEEDED
	}
#endif
}

/**
	Add a handler to the stack of signal handlers for the given signal.

	There is a maximum stack limit, so returns 1 if limit exceeded.
	Returns -1 if stack of signal requested does not exist.
	Pushing a NULL handler does NOT change anything at all.
	On a successful return, the handler has been installed and will
	remain installed until a Asc_SignalHandlerPop or another push.

	@return 0 on success, -2 if func is NULL, -1 if unsupported signal is given,
	-3 if 'signal' returns SIG_ERR.
*/
int Asc_SignalHandlerPush_impl(int signum, SigHandlerFn *func, char *name
	, char *file, int line
){
  int err;
  if (func == NULL) {
    return -2;
  }

#ifdef SIGNAL_DEBUG
  CONSOLE_DEBUG("Pushing handler %s for signal %s(%d)"
    ,name,SIGNAME(signum),signum
  );
#endif

  err = push_trap(signum, func, name, file, line);

  if(err != 0){
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Error pushing %s to stack (err = %d, signal=%s(%d))."
      ,name, err, SIGNAME(signum), signum
    );
    return err;
  }

#if 0
/* TODO we can try introducing this code to solve the issues of the test_ascSignal
test cases, but really it's possible that our approach isn't really the right
one (and still not sure this fixes things on Windows, anyway). More work required. */
#ifdef __linux__
  struct sigaction new_action, old_action;
  new_action.sa_handler = func;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(signum, NULL, &old_action);
  /* FIXME we should have provision for dealing with signals currently set to SIG_IGN */
  int err1 = sigaction(signum, &new_action, NULL);
  sigset_t sigset;
  sigprocmask(0, NULL, &sigset);
  if(sigismember(&sigset, signum)){
    sigemptyset(&sigset);
    sigaddset(&sigset, signum);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  }
  return err1;
#else
  return SIG_ERR==SIGNAL(signum, func); /* install */
#endif
#else
  return SIG_ERR==SIGNAL(signum, func); /* install */
#endif
}


int Asc_SignalHandlerPop_impl(int signum, SigHandlerFn *tp, char *name
	, char *file, int line
){
  int err;
#ifdef SIGNAL_DEBUG
  CONSOLE_DEBUG("(%s:%d) Popping signal stack for signal %s (%d) (expecting top to be %p '%s')",file,line,SIGNAME(signum),signum,tp,name);
#endif

  err = pop_trap(signum, tp, name, file, line);

  if (err != 0 && tp != NULL) {
#ifdef SIGNAL_DEBUG
	CONSOLE_DEBUG("stack pop mismatch");
#endif
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Asc_Signal (%d) stack pop mismatch.",signum);
    return err;
  }

#if 0
/* see comments above */
#ifdef __linux__
  struct sigaction new_action, old_action;
  new_action.sa_handler = Asc_SignalStackTop(signum);
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(signum, NULL, &old_action);
  /* FIXME we should have provision for dealing with signals currently set to SIG_IGN */
  sigaction(signum, &new_action, NULL);
  sigset_t sigset;
  sigprocmask(0, NULL, &sigset);
  if(sigismember(&sigset, signum)){
    sigemptyset(&sigset);
    sigaddset(&sigset, signum);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  }
  return err;
#else
  SIGNAL(signum,Asc_SignalStackTop(signum));
  return err;
#endif
#else
  SIGNAL(signum,Asc_SignalStackTop(signum));
  return err;
#endif
}

void Asc_SignalTrap(int sigval){
#ifdef SIGNAL_DEBUG
  CONSOLE_DEBUG("Caught signal #%d",sigval);
#endif
  switch(sigval) {
  case SIGFPE:
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Floating point error caught");
    CONSOLE_DEBUG("SIGFPE caught");
#ifdef HAVE_C99FPE
    FPRESET;
#endif
    LONGJMP(g_fpe_env,sigval);
  case SIGINT:
	CONSOLE_DEBUG("SIGINT (Ctrl-C) caught");
    LONGJMP(g_int_env,sigval);
  case SIGSEGV:
#ifdef SIGNAL_DEBUG
    CONSOLE_DEBUG("SIGSEGV caught");
#endif
    LONGJMP(g_seg_env,sigval);
  default:
#ifdef SIGNAL_DEBUG
    CONSOLE_DEBUG("Unrecognised signal %d caught",sigval);
#endif
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Installed on unexpected signal (sigval = %d). Returning (who knows where...)", sigval);
    return;
  }
}

void Asc_SignalPrintStack(int signum){
	if(!f_traps){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Signal handler not initialised!");
		return;
	}
	SignalStackItem *stack;
	int *index;
	switch(signum){
		case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
		case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
		case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", signum);
			return;
	}
	int i = 0;
	const char *signame = (signum==SIGFPE?"SIGFPE":SIGNAME(signum));
	
#define LMAX 4096
	char str[LMAX], *end = str;
	int ch;
	if(*index == -1){
		fprintf(stderr,"%s handler stack: empty\n",signame);
	}else{
		for(i = 0; i <= *index; ++i){
#ifdef SIGNAL_DEBUG
			ch = SNPRINTF(end, LMAX - (end - str), "%s  ", stack[i].name);
#else
			ch = SNPRINTF(end, LMAX - (end - str), "%p  ", stack[i].handler);
#endif
			end += ch;
			if(ch<0)break;
		}
		fprintf(stderr,"%s handler stack: %s\n",signame,str);
	}
}

int Asc_SignalStackLength(int signum){
	int *index;
	switch(signum){
		case SIGFPE: index = &(f_traps->fpe_top); break;
		case SIGINT: index = &(f_traps->int_top); break;
		case SIGSEGV: index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d",signum);
			return 0;
	}
	return *index + 1;
}

SigHandlerFn *Asc_SignalStackTop(int signum){
	if(!f_traps)return NULL;
	SignalStackItem *stack;
	int *index;
	switch(signum){
		case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
		case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
		case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", signum);
			return NULL;
	}
	if(*index < 0)return NULL;
	if(*index >= MAX_TRAP_DEPTH)return NULL;
	return stack[*index].handler;
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

static void initstack(int sig){
	SigHandlerFn *old;
	SignalStackItem *stack;
	int *index;
	switch(sig){
		case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
		case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
		case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", sig);
			return;
	}
	old = SIGNAL(sig, SIG_DFL);
	if(old != SIG_ERR && old != SIG_DFL){
#ifdef SIGNAL_DEBUG
		CONSOLE_DEBUG("Initialising stack for signal %d to %p",sig,old);
#endif
		stack[0].handler = old;
#ifdef SIGNAL_DEBUG
		if(old == SIG_DFL){
			stack[0].name = "SIG_DFL";
		}else{
			stack[0].name = "preexisting";
		}
		stack[0].file = "unknown";
		stack[0].line = 0;
#endif
		*index = 0;
		(void)SIGNAL(sig,old);
	}else{
#ifdef SIGNAL_DEBUG
		CONSOLE_DEBUG("Initialising stack for signal %d as empty",sig);
#endif
		*index = -1;
	}
}

static void reset_trap(int signum){
	SignalStackItem top;
	SigHandlerFn *oldfn;

	if(f_traps){
		SignalStackItem *stack;
		int *index;
		switch(signum){
			case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
			case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
			case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", signum);
				return;
		}
		if(*index >= 0 && *index < MAX_TRAP_DEPTH){
			oldfn = signal(signum,SIG_DFL); (void)oldfn;
			top = stack[*index];
			if(top.handler != SIG_ERR && top.handler != SIG_DFL){
				/* reset the signal, if it's not already set to what we want */
#ifdef SIGNAL_DEBUG
				CONSOLE_DEBUG("Resetting signal %s from %p to %p (%s)"
					,SIGNAME(signum) 
					,oldfn,top.handler,top.name
				);
#endif
				(void)SIGNAL(signum,top.handler);
				return;
			}
		}
#ifdef SIGNAL_DEBUG
		CONSOLE_DEBUG("Resetting %s handler to SIG_DFL (stack empty or invalid)"
			,SIGNAME(signum)
		);
#endif
		(void)SIGNAL(signum,SIG_DFL);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Signal handler not yet initialised! Setting %s handler to SIG_DFL.",SIGNAME(signum));
		(void)SIGNAL(signum,SIG_DFL);
		return;
	}
}

/**
	Append a pointer to the list given, if the list is not full.
*/
static int push_trap(int signum, SigHandlerFn *func, char *name, char *file, int line){
	if(!f_traps){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Signal handler list f_traps not initialised");
		return -1;
	}
	SignalStackItem *stack;
	int *index;
	switch(signum){
		case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
		case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
		case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", signum);
			return -2;
	}

	if (*index > MAX_TRAP_DEPTH-1) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Signal handler stack for %s is full!"
			,SIGNAME(signum)
		);
		return 1;
	}
	++(*index);
	stack[*index].handler = func;
#ifdef SIGNAL_DEBUG
	stack[*index].name = name;
	stack[*index].file = file;
	stack[*index].line = line;
#endif
	return 0;
}


/**
	@return 0 on success, 2 on NULL tlist or stackptr input, 1 on empty stack
	or -1 on mismatched input tp and stack data

	Any non-zero return code leaves the stack as it was.
*/
static int pop_trap(int signum, SigHandlerFn *func, char *name, char *file, int line){
	int err = 0;
	if(!f_traps){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Signal handler list f_traps not initialised");
		return 2;
	}
	SignalStackItem *stack;
	int *index;
	switch(signum){
		case SIGFPE: stack = f_traps->fpe_traps; index = &(f_traps->fpe_top); break;
		case SIGINT: stack = f_traps->int_traps; index = &(f_traps->int_top); break;
		case SIGSEGV: stack = f_traps->seg_traps; index = &(f_traps->seg_top); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid signal code %d", signum);
			return 3;
	}

	if(*index < 0)return 1;
	
	if(stack[*index].handler != func){
#ifdef SIGNAL_DEBUG
		error_reporter(ASC_PROG_ERR,file,line,name,"Request to pop '%s' (%p), but top of stack contains '%s' (%p)!"
			,name,func,stack[*index].name,stack[*index].handler
		);
#else
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Request to pop handler %p, but top of stack contains %p!"
			,func,stack[*index].handler
		);
#endif
		err = 4;
	}
	stack[*index].handler = NULL;
#ifdef SIGNAL_DEBUG
	stack[*index].name = NULL;
	stack[*index].file = NULL;
	stack[*index].line = 0;
#endif
	--(*index);
	return err;
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
static int fenv_push(fenv_t *stack, int *top, int excepts){
#ifdef SIGNAL_DEBUG
	CONSOLE_DEBUG("Pushing FENV flags %d",excepts);
#endif

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
	fesetexceptflag(&g_fenv, excepts);
	//CONSOLE_DEBUG("Enabled div-by-zero FPE exception (%d)",*top);
	return 0;
}

/**
	Restore a saved FPU state. Return 0 on success.
*/
static int fenv_pop(fenv_t *stack, int *top){
#ifdef CONSOLE_DEBUG
	CONSOLE_DEBUG("Popping FENV flags");
#endif
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

#endif /* ASC_SIGNAL_TRAPS */
