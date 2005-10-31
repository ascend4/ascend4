/*
 *  Test runner for ASCEND base library.
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
#include <string.h>

#include "utilities/ascConfig.h"
#include "compiler/redirectFile.h"
#include "utilities/ascMalloc.h"
#include "printutil.h"

#include "CUnit/Basic.h"
#include "general/test/test_register_general.h"
#include "utilities/test/test_register_utilities.h"
#include "solver/test/test_register_solver.h"

int main(int argc, char* argv[])
{
  CU_BasicRunMode mode = CU_BRM_VERBOSE;
  CU_ErrorAction error_action = CUEA_IGNORE;
  CU_ErrorCode result;
  int print_messages = FALSE;
  int i;

  setvbuf(stdout, NULL, _IONBF, 0);

  for (i=1 ; i<argc ; i++) {
    if (!strcmp("-i", argv[i])) {
      error_action = CUEA_IGNORE;
    }
    else if (0 == strcmp("-f", argv[i])) {
      error_action = CUEA_FAIL;
    }
    else if (0 == strcmp("-A", argv[i])) {
      error_action = CUEA_ABORT;
    }
    else if (0 == strcmp("-s", argv[i])) {
      mode = CU_BRM_SILENT;
    }
    else if (0 == strcmp("-n", argv[i])) {
      mode = CU_BRM_NORMAL;
    }
    else if (0 == strcmp("-v", argv[i])) {
      mode = CU_BRM_VERBOSE;
    }
    else if (0 == strcmp("-d", argv[i])) {
      print_messages = FALSE;
    }
    else if (0 == strcmp("-w", argv[i])) {
      print_messages = TRUE;
    }
    else {
      printf("\nUsage:  test_ascend [options]\n\n"
               "Options:   -i   ignore framework errors [default].\n"
               "           -f   fail on framework error.\n"
               "           -A   abort on framework error.\n\n"
               "           -s   silent mode - no output to screen.\n"
               "           -n   normal mode - standard output to screen.\n"
               "           -v   verbose mode - max output to screen [default].\n\n"
               "           -d   hide ASCEND messages [default].\n"
               "           -w   print ASCEND messages to console.\n\n"
               "           -h   print this message and exit.\n\n");
      return 0;
    }
  }

  /* initialize testing framework */
  result = CU_initialize_registry();
  if (CUE_SUCCESS != result) {
    fprintf(stderr, "\nInitialization of Test Registry failed - Aborting.");
    return result;
  }

  /* register general component */
  result = test_register_general();
  if (CUE_SUCCESS != result) {
    fprintf(stderr, "\nError during registration of general component tests.  "
                    "\nError code = %d (%s).",
                    result, CU_get_error_msg());
    return result;
  }

  /* register utilities component */
  result = test_register_utilities();
  if (CUE_SUCCESS != result) {
    fprintf(stderr, "\nError during registration of utilities component tests.  "
                    "\nError code = %d (%s).",
                    result, CU_get_error_msg());
    return result;
  }

  /* register solver component */
  result = test_register_solver();
  if (CUE_SUCCESS != result) {
    fprintf(stderr, "\nError during registration of solver component tests.  "
                    "\nError code = %d (%s).",
                    result, CU_get_error_msg());
    return result;
  }

  if (TRUE == print_messages) {
    test_enable_printing();
  }

  Asc_RedirectCompilerDefault();  /* direct internal named streams to std streams */
  CU_basic_set_mode(mode);
  CU_set_error_action(error_action);
  result = CU_basic_run_tests();
  CU_cleanup_registry();
  
  if (CU_BRM_VERBOSE == mode) {
    ascshutdown("Testing completed.");    /* shut down memory manager */
  }

  if (TRUE == print_messages) {
    test_disable_printing();
  }

  return result;
}
