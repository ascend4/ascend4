/**< 
 *  Signal handling protocol definitions for ASCEND
 *  May 27, 1997
 *  By Benjamin Andrew Allan
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascSignal.h,v $
 *  Date last modified: $Date: 1998/01/10 18:00:05 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Programming System.
 *  This file standardizes the handling of signals because some OS
 *  reset signals to SIG_DFL when a trap goes off while others
 *  process the signal but leave the trapping function in place.
 *  We want the second behavior and this gives it to us.
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
#include <signal.h>
#include <setjmp.h>
#include "utilities/ascConfig.h"
#ifdef __WIN32__
#include <process.h>
#else
#include <unistd.h>
#endif
#include "general/list.h"

#ifndef lint
static CONST char ascSignalRCS[] = "$Id: ascSignal.h,v 1.6 1998/01/10 18:00:05 ballan Exp $";
#endif

#define TRAPPTR(a) void (*(a))(int)
/**< 
 * This macro defines a pointer to a signal handler named a.
 * Substitute your variable name and
 * TRAPPTR(yourname); is a function pointer declaration.
 * typedef instead?
 */

#define TPTYPE void (*)(int)
/**< 
 * This macro can be used like  tp = (TPTYPE)foo;
 * or in function prototypes to indicate a signal handling
 * function pointer is the argument.
 */

#define MAX_TRAP_DEPTH 40L
/**< 
 * This is the maximum number of traps that can be nested.
 * with push and pop below.
 */

extern jmp_buf g_fpe_env;
extern jmp_buf g_seg_env;
extern jmp_buf g_int_env;
/**< 
 * standard signals
 */

extern jmp_buf g_foreign_code_call_env;
/**< 
 * not currently in use, but should be when we get to a unified
 * standard for signal handling.
 */

extern void Asc_SignalTrap(int);
/**< 
 * Asc_SignalTrap(sigval);
 * This is the trap that should be used for most applications in
 * ASCEND. It calls longjmp(g_SIG_env,sigval) 
 * where SIG is one of fpe, seg, int as listed above.
 * Note that g_SIG_env is global, so you can't nest calls to
 * setjmp where both use this trap function.
 *
 * Trivial Example:
 *
 *   Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
 *   if (setjmp(g_fpe_env)==0) {
 *     y = sqrt(x);
 *   } else {
 *     y = sqrt(-x);
 *   }
 *   Asc_SignHandlerPop(SIGFPE,Asc_SignalTrap); 
 *
 *  For x < 0 the else is called because setjmp returns nonzero
 *  when the body of the 'if' signals range error.
 *
 *  This is one of the really odd things in C; if x > 0
 *  then sqrt is ok and setjmp returns 1. 
 *  Remember always to use push and pop. You can
 *  write an alternate function to use instead of
 *  AscSignalTrap if need be.
 *  SIGFPE, SIGINT, SIGSEGV are understood.
 *
 * Warning: setjmp is expensive if called inside a fast loop.
 */

extern int Asc_SignalInit(void);
/**< int err = Asc_SignalInit();
 * Returns 0 if successful, 1 if out of memory, 2 otherwyse.
 * Does not establish any traps, just the stacks for
 * maintaining them. Cannot be called twice successfully.
 * Currently we support stacks for the signals:
 * SIGFPE, SIGINT, SIGSEGV.
 */

extern void Asc_SignalDestroy(void);
/**< Asc_SignalDestroy();
 * Clears and destroys the stacks of signal handlers.
 */

extern void Asc_SignalRecover(int);
/**< 
 * Asc_SignalRecover(force);
 * This function reinstalls the most recent signal handlers this module
 * has been informed of. This should be called after every
 * trapped exception and at any other time when the status of
 * exception handlers may have become not well defined.
 * The most recently pushed handler is installed for each supported
 * signal. This call is not particularly cheap if it does the
 * reinstallation.
 *
 * This module tests on startup for whether the OS reverts to
 * SIG_DFL when a trap function is called. If it does NOT then
 * this function will simply return unless force != 0. You don't
 * want to call this function with force == 1 normally, but when
 * using gdb or another debugger which intercepts and screws up
 * signals, applying force (manually) ensures that the signals
 * get reinstalled.
 */

extern int Asc_SignalHandlerPush(int, TPTYPE);
/**< 
 * err =  Asc_SignalHandlerPush(signum, tp)
 * Adds a handler to the stack of signal handlers for the given signal.
 * There is a maximum stack limit, so returns 1 if limit exceeded.
 * Returns -1 if stack of signal requested does not exist.
 * Pushing a NULL handler tp does NOT change anything at all.
 * On a successful return, the handler has been installed and will
 * remain installed until a Asc_SignalHandlerPop or another push.
 * (Remain installed part is TRUE only if Asc_SignalRecover is used
 * properly after every longjmp.)
 */

extern int Asc_SignalHandlerPop(int, TPTYPE);
/**< 
 * err = Asc_SignalHandlerPop(signum, tp);
 * Pops the last trap off the stack of signum trap functions and
 * installs the new last trap. Sideeffects: reinstalls all other
 * currently pushed traps, also.
 * If called with tp != NULL, checks that the trap popped is the
 * same as tp and returns nonzero if it is not.
 * If stack is empty, returns nonzero.
 */

