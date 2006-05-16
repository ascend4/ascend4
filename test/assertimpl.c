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
	Assert implementation override for ASCEND unit tests
*/

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>

#include "assertimpl.h"

enum assert_status_t g_assert_status = ast_passed;
jmp_buf g_asc_test_env;
static int f_use_longjump = FALSE;

void enable_assert_longjmp(int TRUE_or_FALSE)
{
  f_use_longjump = TRUE_or_FALSE;
}

/* Override implementation of assert using the signature of the relevant compiler */
#ifdef __WIN32__
#  if defined(__GNUC__) || defined(__MINGW32_VERSION)
_CRTIMP void __cdecl _assert(const char *cond, const char *file, int line)
{

#  elif defined(_MSC_VER)
#    if _MSC_VER < 1400   /* Visual C versions below 14 */
_CRTIMP void __cdecl _assert(const char *cond, const char *file, unsigned line)
{
#    else
void __cdecl _wassert(__in_z const wchar_t * cond, __in_z const wchar_t *file, __in unsigned line)
{
#    endif  /* Visual C version */

#  elif defined(__BORLANDC__)
#    ifdef __cplusplus
namespace std {
#    endif
void _RTLENTRY _EXPFUNC _assert(char *cond, char *file, int line)
{

#  else
#    error Unrecognized Windows compiler.

#  endif
#else    /* !__WIN32__ */
#  if defined(__GNUC__)
void __assert_fail (const char *cond, const char *file,
		   unsigned int line, const char *__function)
/*     __THROW __attribute__ ((__noreturn__)) */
{
  UNUSED_PARAMETER(__function);

#  else
#    error Unrecognized compiler.
#  endif
#endif    /* __WIN32__ */

  g_assert_status = ast_failed;
  if (TRUE == f_use_longjump) {
    longjmp(g_asc_test_env, -1);
  }
  else {
    fprintf(stderr, "\nAssertion failed: %s, file %s, line %d\n", cond, file, line);
    abort();
  }
}

#if defined(__BORLANDC__)
#  ifdef __cplusplus
}
#  endif
#endif

static int f_asc_assert_failed = FALSE;

static int assert_callback(int status)
{
  if (ASCERR_ASSERTION_FAILED == status) {
    f_asc_assert_failed = TRUE;
    longjmp(g_asc_test_env, -1);
  }
  return -1;
}

int asc_assert_failed()
{
  return f_asc_assert_failed;
}

void asc_assert_reset()
{
  f_asc_assert_failed = FALSE;
}

void asc_assert_catch(int TRUE_or_FALSE)
{
  static PanicCallbackFunc old_callback = NULL;

  if (FALSE == TRUE_or_FALSE) {
    (void)Asc_PanicSetCallback(old_callback);
    Asc_PanicDisplayMessageBox(TRUE);
    old_callback = NULL;
    f_asc_assert_failed = FALSE;
  }
  else {
    old_callback = Asc_PanicSetCallback(assert_callback);
    Asc_PanicDisplayMessageBox(FALSE);
    f_asc_assert_failed = FALSE;
  }
}

