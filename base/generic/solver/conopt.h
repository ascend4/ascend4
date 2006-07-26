/*	ASCEND modelling environment
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
	Header file that in turn includes <conopt.h>.
	
	This file exists in order to pass the correct FNAME_* parameter to
	<conopt.h> and to permit wrapping of conopt routines for use in
	a dlopened implementation.
*//*
	By John Pye
	Based on conopt.h by Vicente Rico Ramirez (created 05/97)
*/

#ifndef ASC_CONOPT_H
#define ASC_CONOPT_H

#include <utilities/config.h>
#include <utilities/ascConfig.h>

#ifdef ASC_WITH_CONOPT  /* code used if CONOPT is available */

/**
 * Macros defined because of the different convention of Fortran from C about
 * the use of an index in arrays (starting from zero or from one).
 */
#define F2C(x) x - 1
#define C2F(x) x + 1

/**
 * Parameter required for CONOPT subroutines
 */
#define NINTGR 3
#define MAX_INT 20000
#define MAX_REAL 10e300

/* What is our calling convention? */

#if defined(__GNUC__) || defined(sun) || defined(__alpha) || defined(__sgi)
# define FNAME_LCASE_DECOR
#elif defined(__WIN32__)
# define FNAME_UCASE_NODECOR
#else
# define FNAME_LCASE_NODECOR
#endif

#include <conopt.h>

#ifndef COIDEF_Size
# error "Where is COIDEF_Size?"
#endif

/**
	Data structure for dealing with CONOPT
	Do we still need this?
*/
struct conopt_data {
  int *cntvect; /* CONOPT's 'control vector' */

  int n;                  /**< Number of columns. */
  int m;                  /**< Number of rows. */
  int nz;                 /**< Number of nonzeros. */
  int nlnz;               /**< Number of nonlinear nonzeros */
  int base;               /**< base of arrays, 1=fortran style */
  int optdir;             /**< optimisation direction */
  int objcon;             /**< objective constraint */ 

  int32 maxrow;             /**< Number of elements in densest row. */
  int32 modsta;             /**< Model status. */
  int32 solsta;             /**< Solver status. */
  int32 iter;               /**< Number of conopt iterations. */
  real64 obj;               /**< Objective value. */

  real64 *work;             /**< Work space. */
  int32 minmem;             /**< Minimum memory suggested by conopt. */
  int32 estmem;             /**< Estimated memory suggested by conopt. */
  int32 lwork;              /**< Size of allocated workspace. */
  int32 nintgr;             /**< Size of problem size vector. */
/*  int32 ipsz[NINTGR]; */      /**< Problem size vector. */ 

  int32 kept;               /**< If 1 can call warm conopt restart. */

  int32 optimized;          /**< Has conopt been called? */
  int32 maxusd;             /**< Maximum work space used. */
  int32 curusd;             /**< Current work space used. */
  int32 opt_count;          /**< Count of calls to coiopt. */
  int32 progress_count;     /**< Count of calls to coiprg. */
};

/**
 * Pointer to the previous structure
 */
typedef struct conopt_function_pointers *conopt_pointers;

/* reporting functions (derived from 'std.c' in CONOPT examples) */

int COI_CALL asc_conopt_message( int* SMSG, int* DMSG, int* NMSG, int* LLEN
		,double* USRMEM, char* MSGV, int MSGLEN
);

int COI_CALL asc_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
);

int COI_CALL asc_conopt_status(int* MODSTA, int* SOLSTA
		, int* ITER, double* OBJVAL, double* USRMEM
);

int COI_CALL asc_conopt_solution( double* XVAL, double* XMAR, int* XBAS
		, int* XSTA, double* YVAL, double* YMAR, int* YBAS, int* YSTA
		, int* N, int* M, double* USRMEM
);

int COI_CALL asc_conopt_progress( int* LEN_INT, int* INT, int* LEN_RL
		, double* RL, double* X, double* USRMEM 
);

#endif /* if ASC_WITH_CONOPT */

#endif /* ASC_CONOPT_H */

