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
*//**
	@file
	Signal handling protocol definitions for ASCEND.

	This file standardizes the handling of signals because some OS
	reset signals to SIG_DFL when a trap goes off while others
	process the signal but leave the trapping function in place.
	We want the second behavior and this gives it to us.

	This module implements limited support for managing signal handlers.
	This includes:
	  - a standard signal handler - Asc_SignalTrap()
	  - global jmp_buf's for use with Asc_SignalTrap()
	  - functions for managing nested signal handlers
	
	The following signal types are currently supported:
	  - SIGFPE  - floating point exception
	  - SIGINT  - CTRL-C interactive attention request
	  - SIGSEGV - segmentation fault
	
	A simple use of these facilities to trap floating point exceptions
	might be as follows:
	<pre>
	   Asc_SignalInit();
	   Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap);
	   if (setjmp(g_fpe_env)==0) {
	     y = sqrt(x);
	   } else {
	     y = sqrt(-x);
	   }
	   Asc_SignHandlerPop(SIGFPE,Asc_SignalTrap);
	   Asc_SignalDestroy();
	</pre>

	This example uses the built-in signal handler Asc_SignalTrap()
	and the global <code>jmp_buf</code> g_fpe_env.  After initializing
	the signal manager and registering the handler, <code>setjmp</code>
	is used to select normal and exception paths.  The <code>setjmp</code>
	returns 0 when initially called and the sqrt(x) is calculated.  If
	x is negative, a SIGFPE exception occurs and the handler is called.  It
	uses <code>lngjmp</code> and returns to the if statement, and now
	'setjmp' returns non-zero and the <code>else</code> clause is executed.
	Finally, the handler is removed and the signal manager cleaned up.<br><br>
	
	The stack mechanism also allows nested handlers to be registered.  It is
	important to note that nested handlers for the same signal type cannot
	both use Asc_SignalTrap() as the handler.  This is because different
	<code>jmp_buf</code> variables must be used and Asc_SignalTrap() uses
	the same global <code>jmp_buf</code> each time.  However, you can use
	custome <code>jmp_buf</code>'s and handlers:

	<pre>
	   Asc_SignalInit();
	   Asc_SignalHandlerPush(SIGFPE, Asc_SignalTrap);
	   if (setjmp(g_fpe_env) == 0) {
	       y = sqrt(x);
	       Asc_SignalHandlerPush(SIGFPE, my_handler);
	       if (setjmp(my_jmp_buf) == 0) {
	           y = z/x;
	       } else {
	           Asc_Panic(1, NULL, "Div by zero error.");
	       }
	       Asc_SignHandlerPop(SIGFPE, my_handler);
	   } else {
	     y = sqrt(-x);
	   }
	   Asc_SignHandlerPop(SIGFPE,Asc_SignalTrap);
	   Asc_SignalDestroy();
	</pre>
	
	Here, exceptions in the sqrt(x) calculation are handled by the standard
	Asc_SignalTrap(), while the division is handled by my_handler.<br><br>
	
	Avoid mixing use of the signal manager with direct calls to signal().
	Once Asc_SignalInit() has been called, use of signal() directly is likely
	to be lost or to corrupt the managed handlers.<br><br>
	
	Another warning: setjmp is expensive if called inside a fast loop.

	Requires:
	#include "utilities/ascConfig.h"
*//*
	by Benjamin Andrew Allan, May 27, 1997
	Last in CVS: $Revision: 1.6 $ $Date: 1998/01/10 18:00:05 $ $Author: ballan $
*/

#ifndef ASC_ASCSIGNAL_H
#define ASC_ASCSIGNAL_H

#include <signal.h>
#include <setjmp.h>
#include "utilities/ascConfig.h"

#ifdef __WIN32__
#  include <process.h>
#else
#  include <unistd.h>
#endif

#ifdef __WIN32__
# define FPRESET _fpreset()
#else
# define FPRESET (void)0
#endif

typedef void SigHandlerFn(int);
/**< Signature of a signal handling function. */

#define MAX_TRAP_DEPTH 40L
/**< The maximum number of traps that can be nested. */

#define ASC_JMP_INFO
/**< Whether to store additional information before making a setjmp call */

#ifndef ASC_JMP_INFO
# define SETJMP set_jmp
# define LONGJMP longjmp
# define SIGNAL signal
typedef JMP_BUF jmp_buf
#else
# define SETJMP(ENV) (\
		CONSOLE_DEBUG("SETJMP at %s:%d (%s=%p)",__FILE__,__LINE__,#ENV,ENV.jmp)\
		,ENV.filename = __FILE__, ENV.line = __LINE__, ENV.func = __FUNCTION__\
		,ENV.varname = #ENV\
		, setjmp(ENV.jmp)\
	)
# define LONGJMP(ENV,VAL) (\
		CONSOLE_DEBUG("LONGJMP to %s:%d (%s) (%s=%p)",ENV.filename,ENV.line,ENV.func,ENV.varname,ENV.jmp)\
		, longjmp(ENV.jmp, VAL)\
	)
typedef struct{
	jmp_buf jmp;
	const char *filename;
	int line;
	const char *func;
	const char *varname;
} asc_jmp_buf;
#define JMP_BUF asc_jmp_buf
#define SIGNAL(SIG,HANDLER) (CONSOLE_DEBUG("SIGNAL(%d,%s)",SIG,#HANDLER),signal(SIG,HANDLER))
#endif


	
ASC_DLLSPEC(JMP_BUF) g_fpe_env;   /**< Standard signal jmp_buf - floating point error. */
ASC_DLLSPEC(JMP_BUF) g_seg_env;   /**< Standard signal jmp_buf - segmentation fault. */
ASC_DLLSPEC(JMP_BUF) g_int_env;   /**< Standard signal jmp_buf - interactive attention (<CTRL>C). */

#if 0
extern jmp_buf g_foreign_code_call_env;
/**<
	Not currently in use.  Should be when we get to a unified
	standard for signal handling.
	@todo Implement use of g_foreign_code_call_env?
*/
#endif

ASC_DLLSPEC(void ) Asc_SignalTrap(int sigval);
/**<
 *  Standard signal handler.
 *  This is the trap that should be used for most applications in
 *  ASCEND.  It prints a message then calls longjmp(GLOBAL, sigval)
 *  where GLOBAL is one of g_fpe_env, g_seg_env, or g_int_env.
 *  Because the jmp_buf is global, so you can't nest calls to
 *  setjmp where both use this trap function.<br><br>
 *
 *  Trivial Example:
 *  <pre>
 *     Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
 *     if (setjmp(g_fpe_env)==0) {
 *       y = sqrt(x);
 *     } else {
 *       y = sqrt(-x);
 *     }
 *     Asc_SignHandlerPop(SIGFPE,Asc_SignalTrap);
 *
 *  For x < 0 the else is called because setjmp returns nonzero
 *  when the body of the 'if' signals range error.
 *  </pre>
 *  Remember always to use Asc_SignalHandlerPush() and
 *  Asc_SignalHandlerPop(). You can write an alternate function
 *  to use instead of AscSignalTrap() if need be.  The signals
 *  SIGFPE, SIGINT, SIGSEGV are understood.<br><br>
 *
 *  Note - this handler does not reinstall itself.  After an exception,
 *  you need to reinstall the handler (if desired) using
 *  Asc_SignalRecover().
 *
 *  @todo Should utilities/ascSignal.c:Asc_SignalTrap() reinstall itself
 *        after it catches an expection using Asc_SignalRecover()?
 *  @param sigval Holds the signal type code when called during
 *                an exception.
 */

ASC_DLLSPEC(int ) Asc_SignalInit(void);
/**<
 *  Initializes the signal manager.
 *  This should be called before using any of the signal handling
 *  functions in this module.  It initializes the internal stacks
 *  for mangaging signal handlers.  This function does not install
 *  any signal handlers (although any existing handlers are left
 *  in place).  Calling this function more than once will have no
 *  effect and an error code will be returned.<br><br>
 *
 *  @return Returns 0 if successful, 1 if memory could not be
 *          allocated, and 2 if an error occurred.
 */

ASC_DLLSPEC(void ) Asc_SignalDestroy(void);
/**<
 *  Cleans up and destroys the stacks of signal handlers.
 *  It does not change the status of any registered signal handlers
 *  That is, any handlers registered when this function is called
 *  will still be registered.  It is important to call
 *  Asc_SignalHandlerPop() for each occurrence of Asc_SignalHandlerPush()
 *  before calling this function.  Otherwise, any signal handlers
 *  that were installed before Asc_SignalInit() was called will be lost.
 */

ASC_DLLSPEC(void ) Asc_SignalRecover(int force);
/**<
 *  Reinstalls the most recently pushed handler that has been
 *  installed for each supported signal type.  This should be called
 *  after every trapped exception and at any other time when the
 *  status of exception handlers may have become not well-defined.
 *  If no handler has been pushed for a given signal type, SIG_DFL is
 *  installed.  Note that the standard handler function Asc_SignalTrap()
 *  does not call this function.  If you use the standard handler and
 *  you want it reinstalled after an exception, be sure to call this
 *  function after the longjmp return.  This call is not particularly
 *  cheap if it does the reinstallation.<br><br>
 *
 *  This module tests on startup for whether the OS reverts to
 *  SIG_DFL when a trap function is called.  If it does NOT then
 *  this function will simply return unless force != 0. You don't
 *  want to call this function with force == 1 normally after a
 *  caught exception.  However, if you're not sure of the handler
 *  installation status and want to make sure the handlers are
 *  installed, call with force == 1.  Also, gdb or other
 *  debuggers which intercept and screw up signals may require
 *  applying force (manually) to ensure that the signals get
 *  reinstalled.
 *
 *  @param force If non-zero, the most recent handlers are
 *               reinstalled even if not required by the
 *               compiler/platform.
 */

ASC_DLLSPEC(int ) Asc_SignalHandlerPushDefault(int signum);
ASC_DLLSPEC(int ) Asc_SignalHandlerPush(int signum, SigHandlerFn *func);
/**<
 * Adds a handler to the stack of signal handlers for the given signal.
 * There is a maximum stack limit, so returns 1 if limit exceeded.
 * Returns -1 if stack of signal requested does not exist.
 * Pushing a NULL handler func does NOT change anything at all.
 * On a successful return, the handler has been installed and will
 * remain installed until a Asc_SignalHandlerPop() or another push.
 * The handler will remain installed as long as Asc_SignalRecover()
 * is used properly after every exception.
 *
 *  @param signum The signal type that func should handle.
 *  @param func   The signal handler to register for signum signal types.
 *  @return Returns 1 if the stack limit is exceeded, -1 if managing
 *          of signals for the specified signum is not supported or
 *          initialized, or 0 if the function completes successfully.
 *  @todo Shouldn't utilities/ascSignal.c:Asc_SignalHandlerPush() return
 *        an error code on a NULL func?  It seems too easy for someone to
 *        accidentally push a NULL without realizing it, and then later
 *        popping an unintended handler.
 */

ASC_DLLSPEC(int ) Asc_SignalHandlerPopDefault(int signum);
ASC_DLLSPEC(int ) Asc_SignalHandlerPop(int signum, SigHandlerFn *func);
/**<
 *  Removes the last-pushed handler from the stack for signum signal types.
 *  If the removed handler is the same as func, it is uninstalled and
 *  replaced with the handler now at the top of the stack.  If not, non-zero
 *  is returned and you need to call Asc_SignalRecover() to uninstall the
 *  current handler if desired.  Note that the top handler is popped off
 *  the stack whether it matches func or not.  Non-zero is also returned if
 *  the stack is empty.  A side effect is that all managed signal types will
 *  have the registered handlers reinstalled.
 *
 *  @param signum The signal type whose top-most handler should be replaced.
 *  @param func   The handler function that should be at the top of the
 *                stack (and currently installed) for signals of type signum.
 *  @return Returns non-zero if func is not the replaced handler or if
 *          the stack is empty, 0 if the function completed successfully.
 *  @todo Does it make more sense for utilities/ascSignal.c:Asc_SignalHanderPop()
 *        to fail completely if func is not the top-most handler?  It is not
 *        clear why the function should pop the top handler no matter what, but
 *        only call Asc_SignalRecover() if it matches func.
 */

/** Output the contents of the specified stack. For debugging. */
ASC_DLLSPEC(void) Asc_SignalPrintStack(int signum);

/** Return the length of the specified stack. For debugging. */
ASC_DLLSPEC(int) Asc_SignalStackLength(int signum);

/** For debugging.
	@return handler at top of specified stack, or NULL if stack is empty.
*/
ASC_DLLSPEC(SigHandlerFn *) Asc_SignalStackTop(int signum);

#endif  /* ASC_ASCSIGNAL_H */

