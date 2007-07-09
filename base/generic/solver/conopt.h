/*	ASCEND modelling environment
	Copyright (C) 2006, 2007 Carnegie Mellon University

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

/**	@addtogroup solver Solver
	@{
*/

#include <utilities/config.h>
#include <utilities/ascConfig.h>

#ifdef ASC_WITH_CONOPT  /* code used if CONOPT is available */

/**
 * Macros defined because of the different convention of Fortran from C about
 * the use of an index in arrays (starting from zero or from one).
 */
#define F2C(x) x - 1
#define C2F(x) x + 1

/*
	Default bound limit for CONOPT
*/
#define CONOPT_BOUNDLIMIT 3.1e9 

/**
 * Parameter required for CONOPT subroutines
 */
#define NINTGR 3
#define MAX_INT 20000
#define MAX_REAL 10e300

/* What is our calling convention? */

#if !defined(_WIN32)
# define FNAME_LCASE_DECOR
#endif

#ifdef ASC_LINKED_CONOPT
/*----------------------------------------
  LINKED CONOPT
*/
# include <conopt.h>
#else
/*----------------------------------------
  DLOPENED CONOPT
*/

ASC_DLLSPEC int asc_conopt_load();

# define CONOPT_DISABLE_FN_DECLS
# include <conopt.h>
/*
	This is a list of the functions that we're going to be using from CONOPT.
	Using this list, we can automate the process of reading the function
	pointers from the DLL
*/

# define INTINT (int*cntvect,int*v)
# define INTINT1 (cntvect,v)
# define INTDOUBLE (int*cntvect,double*v)
# define INTDOUBLE1 (cntvect,v)

/**
	This is a compressed list describing the function calls in the CONOPT
	API. The first parameter in each 'D(...)' is the name we will be using to
	access the function. The second is the parameter declaration. The third is a
	parameter list that can be used to pass on the parameter to another function.
	The final one is the text that is appended to the symbol name in the
	case of the windows DLL, eg "_COIDEF_SIZE@0". That's a kludge because
	the DLL contains decorated export symbols.
*/
# define CONOPT_FNS(D,X) \
	D( COIDEF_Size      , ()            , ()         ,"@0") X \
	D( COIDEF_Ini       , ( int*cntvect), (cntvect)  ,"@4") X \
	D( COIDEF_NumVar    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_NumCon    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_NumNZ     , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_NumNlNz   , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_Base      , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_OptDir    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ObjCon    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ObjVar    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ItLim     , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ErrLim    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_IniStat   , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_FVincLin  , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_FVforAll  , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_DebugFV   , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_MaxSup    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_Square    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_EmptyRow  , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_EmptyCol  , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_Num2D     , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_Debug2D   , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_DisCont   , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_StdOut    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ClearM    , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_2DPerturb , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_NDual     , INTINT        , INTINT1    ,"@8") X \
	D( COIDEF_ResLim    , INTDOUBLE     , INTDOUBLE1 ,"@8") X \
	D( COIDEF_WorkSpace , INTDOUBLE     , INTDOUBLE1 ,"@8") X \
	D( COIDEF_WorkFactor, INTDOUBLE     , INTDOUBLE1 ,"@8") X \
	D( COIDEF_ReadMatrix, (int*cntvect, COI_READMATRIX f), (cntvect,f)   ,"@8") X \
	D( COIDEF_FDEval    , (int*cntvect, COI_FDEVAL     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Status    , (int*cntvect, COI_STATUS     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Solution  , (int*cntvect, COI_SOLUTION   f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Message   , (int*cntvect, COI_MESSAGE    f), (cntvect,f)   ,"@8") X \
	D( COIDEF_ErrMsg    , (int*cntvect, COI_ERRMSG     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Progress  , (int*cntvect, COI_PROGRESS   f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Optfile   , (int*cntvect, COI_OPTFILE    f), (cntvect,f)   ,"@8") X \
	D( COIDEF_Option    , (int*cntvect, COI_OPTION     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_TriOrd    , (int*cntvect, COI_TRIORD     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_FDInterval, (int*cntvect, COI_FDINTERVAL f), (cntvect,f)   ,"@8") X \
	D( COIDEF_2DDir     , (int*cntvect, COI_2DDIR      f), (cntvect,f)   ,"@8") X \
	D( COIDEF_2DDirLag  , (int*cntvect, COI_2DDIRLAG   f), (cntvect,f)   ,"@8") X \
	D( COIDEF_2DLagr    , (int*cntvect, COI_2DLAGR     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_SRFile    , (int*cntvect, COI_SRFILE     f), (cntvect,f)   ,"@8") X \
	D( COIDEF_DualBnd   , (int*cntvect, COI_DUALBND    f), (cntvect,f)   ,"@8") X \
	D( COIDEF_UsrMem    , INTDOUBLE     , INTDOUBLE1                     ,"@8") X \
	D( COIDEF_WorkMem   , (int*cntvect, double*v, int*v2), (cntvect,v,v2),"@12")X \
	D( COIGET_MaxUsed   , (int*cntvect) , (cntvect)                      ,"@4") X \
	D( COIGET_CurUsed   , (int*cntvect) , (cntvect)                      ,"@4") X \
	D( COI_Solve        , (int*cntvect) , (cntvect)                      ,"@4") X \
	D( COI_MemEst       , (int*cntvect,double*v,double*v2),(cntvect,v,v2),"@12")X \
	D( COI_Version      , (float*v, char*c, int i), (v,c,i)              ,"@12")

/*
	Declare local functions to hook into the DLL
*/
# define FN_PTR_HDR(T,A,V,L) \
	ASC_DLLSPEC int COI_CALL T A;
# define SPACE

CONOPT_FNS(FN_PTR_HDR,SPACE)

# undef FN_PTR_HDR
# undef SPACE

# undef INTINT
# undef INTINT1
# undef INTDOUBLE
# undef INTDOUBLE1

#endif

/* either static or dlopened, this macro should now be defined */
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

/* the symbols are exported because they are used in teh solvers, which are dlopened. */

ASC_DLLSPEC int COI_CALL asc_conopt_message( int* SMSG, int* DMSG, int* NMSG, int* LLEN
		,double* USRMEM, char* MSGV, int MSGLEN
);

ASC_DLLSPEC int COI_CALL asc_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
);

ASC_DLLSPEC int COI_CALL asc_conopt_status(int* MODSTA, int* SOLSTA
		, int* ITER, double* OBJVAL, double* USRMEM
);

ASC_DLLSPEC int COI_CALL asc_conopt_solution( double* XVAL, double* XMAR, int* XBAS
		, int* XSTA, double* YVAL, double* YMAR, int* YBAS, int* YSTA
		, int* N, int* M, double* USRMEM
);

ASC_DLLSPEC int COI_CALL asc_conopt_progress( int* LEN_INT, int* INT, int* LEN_RL
		, double* RL, double* X, double* USRMEM
);

#endif /* if ASC_WITH_CONOPT */

/* @} */

#endif /* ASC_CONOPT_H */

