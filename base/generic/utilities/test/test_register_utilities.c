/*
 *  Registration function for the ASCEND utilities component.
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
#include "utilities/ascConfig.h"
#include "test_register_utilities.h"

#include "test_ascDynaLoad.h"
#include "test_ascEnvVar.h"
#include "test_ascMalloc.h"
#include "test_ascPanic.h"
#include "test_ascPrint.h"
#include "test_ascSignal.h"
#include "test_mem.h"
#include "test_readln.h"
#include "test_set.h"

CU_ErrorCode test_register_utilities(void)
{
  CU_ErrorCode result = CUE_SUCCESS;

  /* for new tests, add the test registration call to the following sequence: */

  /* utilites/ascDynaLoad.c */
  result = test_register_utilities_ascDynaLoad();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/ascEnvVar.c */
  result = test_register_utilities_ascEnvVar();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/ascMalloc.c */
  result = test_register_utilities_ascMalloc();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/ascPanic.c */
  result = test_register_utilities_ascPanic();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/ascPrint.c */
  result = test_register_utilities_ascPrint();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/ascSignal.c */
  result = test_register_utilities_ascSignal();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/mem.c */
  result = test_register_utilities_mem();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/readln.c */
  result = test_register_utilities_readln();
  if (CUE_SUCCESS != result)
    return result;

  /* utilites/set.c */
  result = test_register_utilities_set();
  if (CUE_SUCCESS != result)
    return result;

  return result;
}

