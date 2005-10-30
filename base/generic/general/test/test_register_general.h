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

/*  @file
 *  Test registration function for the ASCEND general component.
 *  <pre>
 *  Requires:  #include "CUnit/CUnit.h"
 *  </pre>
 */

#ifndef TEST_REGISTER_GENERAL_H_SEEN
#define TEST_REGISTER_GENERAL_H_SEEN

#ifdef __cplusplus
extern "C" {
#endif

CU_ErrorCode test_register_general(void);
/**< 
 *  Registers all tests for the ASCEND general component.
 *  Returns a CUnit error code (CUE_SUCCESS if no errors).
 */

#endif  /* TEST_REGISTER_GENERAL_H_SEEN */
