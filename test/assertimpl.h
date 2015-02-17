/*	ASCEND modelling environment
	Copyright (C) 2005 Jerry St.Clair
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
	Assert implementation override for ASCEND unit tests.

	These functions override the default assert implementation to
	allow recovery from failed asserts during testing.  When using
	this implementation, a failed assertion sets a global variable
	(g_assert_status) and issues a longjmp() out of the function.

	For example (in test function):
	                                                <pre>
	  g_assert_status = ast_passed;
	  if (0 == setjmp(g_asc_test_env))
	    call_function_containing_assertion();

	  if (ast_passed == g_assert_status)
	    ==> no assertions failed <==
	  else
	    ==> assertion failed <==                   </pre>

	If you desire that assert() reverts to standard behavior
	(issue a message on stderr and abort the program), call
	enable_longjump(FALSE).

	Support is also provided for trapping asc_assert() failures.
	This mechanism works by using a callback from Asc_Panic to
	issue a longjmp back to test code for asc_assert() failures.
	Test code should enable trapping of asc_assert, then use
	the global jmp_buf g_asc_test_env and setjmp() to wrap the
	code being tested.  Functions are provided to check the whether
	an asc_assert() failure has occurred and reset the status.

	For example (in test function):
	<pre>
	  asc_assert_catch(TRUE);           (* enable trapping of asc_assert() *)
	  asc_assert_reset();               (* prepare for a test *)
	  if (0 == setjmp(g_asc_test_env))  (* setjmp using global jmpbuf & call suspect function *)
	    call_function_containing_assertion();

	  if (TRUE == asc_assert_failed())  (* test whether assertion failed *)
	    ==> no assertions failed <==
	  else
	    ==> assertion failed <==
	  asc_assert_catch(FALSE);          (* disable trapping of asc_assert() when done *)
	</pre>
 */

#ifndef ASSERTIMPL_H_SEEN
#define ASSERTIMPL_H_SEEN

/** assert status results. */
enum assert_status_t {
  ast_failed,
  ast_passed
};

/**
 *  Global variable indicating status of last assert().
 *  Will be ast_passed if the assert passed, ast_failed otherwise.
 */
extern enum assert_status_t g_assert_status;

/**
 *  Global jump buffer variable to use for the call to setjmp().
 */
extern jmp_buf g_asc_test_env;

/**
 *  Change the behavior of failed assert()'s.
 *  Pass TRUE to enable alternate behavior using longjmp, FALSE
 *  to use standard behavior.
 */
void enable_assert_longjmp(int TRUE_or_FALSE);

/**
 *  Returns TRUE if an asc_assert() failed since the last call to
 *  asc_assert_reset() or asc_assert_catch(TRUE).
 */
extern int asc_assert_failed(void);

/**
 *  Resets the accounting on asc_assert().
 *  After calling this function, asc_assert_failed() will return FALSE.
 *  It should be called after a caught failed assertion.
 */
extern void asc_assert_reset(void);

/**
 *  Enables or disables trapping of asc_assert() failures.
 *  Pass TRUE to enable catching of failures, or FALSE to disable it.
 *  Note that while catching is enabled, any call to Asc_Panic() with a
 *  status code of ASCERR_ASSERTION_FAILED will be trapped as a failed
 *  exception.  Other status codes will not be trapped.  When an
 *  assertion is trapped, <br><br>
 *
 *  This function registers a callback with Asc_Panic().  If another
 *  callback was already registered, it is replaced until
 *  asc_assert_catch(FALSE) is called.  At that time the original
 *  callback will be restored.<br><br>
 *
 *  If you're playing lots of games with Asc_Panic() callbacks, this
 *  function may mess you up.  For example, if you register a new callback
 *  after calling this function (1) asc_assert() failures will not be
 *  trapped and (2) some previous callback can be restored when
 *  asc_assert_catch(FALSE) is called.
 *
 *  @param TRUE_or_FALSE Pass TRUE to enable trapping of asc_assert
 *                       failures, FALSE to disable.
 */
extern void asc_assert_catch(int TRUE_or_FALSE);

#endif  /* ASSERTIMPL_H_SEEN */
