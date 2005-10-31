/*
 *  Assert implementation override for ASCEND unit tests
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
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
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
#if defined(__GNUC__) || defined(__MINGW32_VERSION)
_CRTIMP void __cdecl _assert(const char *cond, const char *file, int line)

#elif defined(_MSC_VER)
_CRTIMP void __cdecl _assert(const char *cond, const char *file, unsigned line)

#elif defined(__BORLANDC__)
#ifdef __cplusplus
namespace std {
#endif
void _RTLENTRY _EXPFUNC _assert(char *cond, char *file, int line)

#else
#error Unrecognized compiler.

#endif
#else    /* !__WIN32__ */
#if defined(__GNUC__)
void __assert_fail (const char *cond, const char *file,
		   unsigned int line, const char *__function)
/*     __THROW __attribute__ ((__noreturn__)) */ 
#else
#error Unrecognized compiler.
#endif  
#endif    /* __WIN32__ */
{
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
#ifdef __cplusplus
}
#endif
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

