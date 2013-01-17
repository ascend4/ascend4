/*	ASCEND modelling environment
 Copyright (C) 2011 Carnegie Mellon University

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
Common data declarations etc.
*/

#ifndef FPROPS_COMMON_H
#define FPROPS_COMMON_H

#include <stdlib.h>
#include <string.h>

/** Departure function error codes */
typedef enum FpropsError_enum{
	FPROPS_NO_ERROR
	,FPROPS_NUMERIC_ERROR
	,FPROPS_SAT_CVGC_ERROR /* something went wrong when solving saturation state */
	,FPROPS_RANGE_ERROR
	,FPROPS_DATA_ERROR
	,FPROPS_NOT_IMPLEMENTED /* function shell only, the guts aren't there yet */
	,FPROPS_INVALID_REQUEST
	,FPROPS_VALUE_UNDEFINED /* value is physically undefined for the requested case */
    //etc...
} FpropsError;

/** Return macros for fprops_region_XX functions (FIXME use FpropsError instead?) */
#define FPROPS_NON 0
#define FPROPS_SAT 1
#define FPROPS_ERROR -1


#define FPROPS_NEW(TYPE) ((TYPE *)malloc(sizeof(TYPE)))

#define FPROPS_FREE(PTR) free(PTR)

#define FPROPS_NEW_ARRAY(TYPE,SIZE) ((TYPE*)malloc(sizeof(TYPE)*(SIZE)))

#define FPROPS_ARRAY_COPY(DEST,SRC,TYPE,SIZE) \
	memcpy((void *)(DEST),(void *)(SRC),sizeof(TYPE)*(SIZE));

//#ifdef __GNUC__
//# define SQ(X) ({typeof(X) x_ = (X); x_*x_})
//#else
# define SQ(X) ((X)*(X))
//#endif

#endif
