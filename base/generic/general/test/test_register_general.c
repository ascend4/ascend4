/*
 *  Registration function for the ASCEND general component.
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
#include "test_register_general.h"

#include "test_dstring.h"
#include "test_hashpjw.h"
#include "test_list.h"
#include "test_listio.h"
#include "test_pool.h"
#include "test_pretty.h"
#include "test_stack.h"
#include "test_table.h"
#include "test_tm_time.h"

CU_ErrorCode test_register_general(void)
{
  CU_ErrorCode result = CUE_SUCCESS;

  /* for new tests, add the test registration call to the following sequence: */

  /* general/dstring.c */
  result = test_register_general_dstring();
  if (CUE_SUCCESS != result)
    return result;

  /* general/hashpjw.c */
  result = test_register_general_hashpjw();
  if (CUE_SUCCESS != result)
    return result;

  /* general/list.c */
  result = test_register_general_list();
  if (CUE_SUCCESS != result)
    return result;

  /* general/listio.c */
  result = test_register_general_listio();
  if (CUE_SUCCESS != result)
    return result;

  /* general/pool.c */
  result = test_register_general_pool();
  if (CUE_SUCCESS != result)
    return result;

  /* general/pretty.c */
  result = test_register_general_pretty();
  if (CUE_SUCCESS != result)
    return result;

  /* general/stack.c */
  result = test_register_general_stack();
  if (CUE_SUCCESS != result)
    return result;

  /* general/table.c */
  result = test_register_general_table();
  if (CUE_SUCCESS != result)
    return result;

  /* general/tm_time.c */
  result = test_register_general_tm_time();
  if (CUE_SUCCESS != result)
    return result;

  return result;
}

