/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Include all needed functions from IDA. We place all the includes in this
	single file because #include conventions have changed with successive
	SUNDIALS releases.
*/

#include <sundials/sundials_config.h>

/*
	for cases where the headers don't expose version information, guess 5.x.
*/
#ifndef SUNDIALS_VERSION_MINOR
# define SUNDIALS_VERSION_MINOR 0
#endif
#ifndef SUNDIALS_VERSION_MAJOR
# define SUNDIALS_VERSION_MAJOR 5
#endif

#if SUNDIALS_VERSION_MAJOR >= 5
# if SUNDIALS_VERSION_MAJOR == 5
#  define SUN_PREC_NONE PREC_NONE
#  define SUN_PREC_LEFT PREC_LEFT
#  define SUN_MODIFIED_GS MODIFIED_GS
#  define SUN_CLASSICAL_GS CLASSICAL_GS
# endif
# if SUNDIALS_VERSION_MAJOR >= 7
#  include <sundials/sundials_context.h>
#  include <sundials/sundials_errors.h>
#  include <sundials/sundials_types_deprecated.h>
#  define SUNLS_SUCCESS SUN_SUCCESS
# elif SUNDIALS_VERSION_MAJOR >= 6
#  include <sundials/sundials_context.h>
# endif
# include <nvector/nvector_serial.h>
# include <ida/ida.h>
# include <ida/ida_ls.h>
# include <sunmatrix/sunmatrix_dense.h>
# include <sunlinsol/sunlinsol_dense.h>
# include <sunlinsol/sunlinsol_spgmr.h>
# include <sunlinsol/sunlinsol_spbcgs.h>
# include <sunlinsol/sunlinsol_sptfqmr.h>
# define IDA_MTX_T SUNMatrix
# define ASC_IDA_DENSE_ELEM(A,i,j) SM_ELEMENT_D((A),(i),(j))
# define ASC_SUNDIALS_5PLUS 1
#else
# error "Unsupported SUNDIALS version: ASCEND IDA requires SUNDIALS 5 or newer"
#endif

#ifndef IDA_SUCCESS
# error "Failed to include SUNDIALS IDA header file"
#endif
