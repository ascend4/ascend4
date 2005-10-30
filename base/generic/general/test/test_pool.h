/*
 *  Unit test functions for ASCEND: general/pool.c
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

/*  @file
 *  Registration function for unit tests of the ASCEND general/pool.c module.
 *  <pre>
 *  Requires:   #include "CUnit/CUnit.h"
 *              #include "utilities/ascConfig.h"
 *              #include "general/pool.h"
 *  </pre>
 */

#ifndef TEST_POOL_H_SEEN
#define TEST_POOL_H_SEEN

#ifdef __cplusplus
extern "C" {
#endif

CU_ErrorCode test_register_general_pool(void);
/**< 
 *  Registers the unit tests for the ASCEND general/pool module.
 *  Returns a CUnit error code (CUE_SUCCESS if no errors).
 */

#ifdef __cplusplus
}
#endif

#endif  /* TEST_POOL_H_SEEN */
