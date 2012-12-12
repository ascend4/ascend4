/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg, Thomas Guthrie Weidner Epperly
	Copyright (C) 1993 Joseph James Zaher
	Copyright (C) 1993, 1994 Benjamin Andrew Allan, Joseph James Zaher
	Copyright (C) 1996 Benjamin Andrew Allan, Kenneth Tyner
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Reverse Automatic Differentiation (AD) routines for ASCEND
	This module defines structures and routines for Exact
	Automatic Differentiation support in Ascend.

	The ideas for methods and structures have been adapted from the book:

	  Andreas Griewank, "Evaluating Derivatives - Principles and Techniques of
	  Algorithmic Differentiation", SIAM, 2000.
*//*
	Author: Mahesh Narayanamurthi
	Written as part of GSOC 2009.
	http://ascendwiki.cheme.cmu.edu/User:Mnm87
*/

#ifndef ASC_REVERSE_AD_H
#define ASC_REVERSE_AD_H

/**	@addtogroup compiler_revad Compiler Reverse Automatic Differentiation
	@{
*/

#include <ascend/general/dstring.h>
#include <ascend/general/platform.h>
#include "expr_types.h"
#include "relation_type.h"


#define MAX_TAPE_COUNT 1


/* ---- Temporarily Needed Structure ---*/

/*----- data types -----*/

/** Description: This structure is used for calculations up to second
	order derivatives. This structure is from the book 
	"Evaluating Derivatives - Principles and Techniques of Algorithmic 
	Differentiation by Andreas Griewank"
 */
typedef struct Doublet_struct{
	double val;
	double dot;
} Doublet;

/**
	This structure is used for maintaining Trace information 
	of all the operations performed during function evaluation. It is 
	later used for adjoint calculation during the return sweep.
	This structure idea has been adapted from the book 
	"Evaluating Derivatives - Principles and Techniques of Algorithmic 
	Differentiation by Andreas Griewank"
	and modified suitably.

	@NOTE The 'next' pointer is different from what is used by griewank, 
	because here we are using a singly linked list instead of a fixed-size
	array of Elements. @ENDNOTE
 */
typedef struct Element_struct{
	Doublet val;
	Doublet bar;
	enum Expr_enum expr_type;   /**< Information about the type of operation is to be recorded */
	const struct Func *fxnptr;
	unsigned long sindex;                 /**< Contains information to identify the variable */
	struct Element_struct *arg1;
	struct Element_struct *arg2;
	struct Element_struct *next; /**< Variable length trace information */
	struct Element_struct *prev;
} Element;


/**
	This structure is used for maintaining pointer to trace 
	information of all the operations performed during function evaluation.

	This structure is from the book 
	Andreas Griewank, "Evaluating Derivatives - Principles and Techniques of
	Algorithmic Differentiation", SIAM, 2000.
 */
typedef struct Redouble_struct{
	Element *ref;
} Redouble;


/*
	FIXME List of tapes...?
*/
typedef struct TapeList_struct{
	Element *tape[MAX_TAPE_COUNT];
	unsigned active;
} TapeList;

/* --------- externally-useful functions provided by this module ------------ */


/**
	This function combines the Residual and Gradient Calculations, since these
	may be done together at basically the cost of just one.
	Non-zero return value implies a problem.

	This function employs the Reverse Mode of Automatic Differentiation.

	@NOTE This function is a possible source of floating point exceptions
	and should not be used during compilation.

	@return FIXME
*/
ASC_DLLSPEC Element* RelationEvaluateResidualGradientRev(CONST struct relation *r,
		double *residual,
		double *gradient,
  		int second_deriv
);

/** 
	Safe-mode evaluation of residual and gradient by Reverse AD.
	This function does not result in floating point (isnan, isinf) errors.

	@return FIXME
*/
ASC_DLLSPEC Element* RelationEvaluateResidualGradientRevSafe(CONST struct relation *r,
		double *residual,
		double *gradient,
  		int second_deriv,
		enum safe_err *serr);
		
/**------------- Second Derivative Calculations-------------*/

/**
	This function calculates the second derivative
*/
/**<
	This is the main function which calculates the second derivatives of 
	a relation r.
	@param r is the relation whose 2nd derivative is desired
	@param deriv2nd  is the pointer to the list of 2nd derivatives
	@param var_index is the index of the variable wrt which the second derivatives are calculated.
	@param hessian_calc is the boolean value indicating whether its an Hessian evaluation (Optimization)
	@param tape is the tape on which gradient information is prerecorded
	@return not significant yet
 */
ASC_DLLSPEC int RelationEvaluateSecondDeriv(CONST struct relation *r,
											double *deriv2nd,
		   									unsigned long var_index,
		   									int hessian_calc,
											Element* tape);


/**<
	This is the main function which calculates the second derivatives of 
	a relation r.
	@param r is the relation whose 2nd derivative is desired
	@param deriv2nd  is the pointer to the list of 2nd derivatives
	@param var_index is the index of the variable wrt which the second derivatives are calculated.
	@param hessian_calc is the boolean value indicating whether its an Hessian evaluation (Optimization)
	@param tape is the tape on which gradient information is prerecorded
	@return not significant yet

	Safe Version
 */
ASC_DLLSPEC int RelationEvaluateSecondDerivSafe(CONST struct relation *r,
												double *deriv2nd,
												unsigned long var_index,
												int hessian_calc,
												Element* tape,
  												enum safe_err *serr);
  												

/**---------------Hessians Evaluations --------------------*/
/**<
	This is the main routine which calculated the hessian matrix of the relation r
	@param r is the relation whose hessian matrix is to be calculated
	@param hess_mtx is the pointer to the 2 dimensional lower triangular hessian matrix
	@param dimension is the dimension of the hessian matrix
	@return not significant yet 
*/
ASC_DLLSPEC int RelationEvaluateHessianMtx(CONST struct relation *r,
											hessian_mtx *hess_mtx,
		   									unsigned long dimension);
  											
  											
/**<
	This is the main routine which calculated the hessian matrix of the relation r
	@param r is the relation whose hessian matrix is to be calculated
	@param hess_mtx is the pointer to the 2 dimensional lower triangular hessian matrix
	@param dimension is the dimension of the hessian matrix
	@return not significant yet 

	Safe Version
 */
ASC_DLLSPEC int RelationEvaluateHessianMtxSafe(CONST struct relation *r,
												hessian_mtx *hess_mtx,
												unsigned long dimension,
												enum safe_err *serr);
												
/**------------------------------------------------------ */
/* @} */

#endif  /* ASC_REVERSE_AD_H */

