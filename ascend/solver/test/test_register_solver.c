/*
 *  Registration function for the ASCEND solver component.
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

#include "CUnit/CUnit.h"
#include <utilities/ascConfig.h>
#include "test_register_solver.h"

/* #include "test_bnd.h" */
#include "test_slv_common.h"
/* #include "test_slv.h" */

CU_ErrorCode test_register_solver(void)
{
  CU_ErrorCode result = CUE_SUCCESS;

  /* for new tests, add the test registration call to the following sequence: */

  /* solver/bnd.c */
  /* result = test_register_solver_bnd();
  if (CUE_SUCCESS != result)
    return result;
  */

  /* solver/slv_common.c */
  result = test_register_solver_slv_common();
  if (CUE_SUCCESS != result)
    return result;

  /* solver/slv.c */
/*
  result = test_register_solver_slv();
  if (CUE_SUCCESS != result)
    return result;
*/

  return result;
}

