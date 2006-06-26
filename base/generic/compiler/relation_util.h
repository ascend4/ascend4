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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Relation utility functions for Ascend.
	This module defines the dimensionality checking and some other
	auxillaries for Ascend.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler/fractions.h"
	#include "compiler/compiler.h"
	#include "compiler/functype.h"
	#include "compiler/safe.h"
	#include "compiler/dimen.h"
	#include "compiler/expr_types.h"
	#include "compiler/relation_type.h"
	#include "compiler/relation_util.h"
*//*
	Version: $Revision: 1.27 $
	Version control file: $RCSfile: relation_util.h,v $
	Date last modified: $Date: 1998/02/05 16:37:48 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_RELATION_UTIL_H
#define ASC_RELATION_UTIL_H

#include <utilities/ascConfig.h>
#include "fractions.h"
#include "compiler.h"
#include "functype.h"
#include "safe.h"
#include "dimen.h"
#include "expr_types.h"
#include "relation_type.h"

ASC_DLLSPEC(int ) g_check_dimensions_noisy;
/**<
 *  If 0, warnings are suppressed.  If 1, warnings are given
 *  from RelationCheckDimensions();
 */

extern int RelationCheckDimensions(struct relation *rel, dim_type *dimens);
/**<
 *  Scans a relation in postfix and collects all dimensional
 *  information by applying each token.  It returns a value of TRUE
 *  only if no real instances or real atom instances with wild
 *  dimensionality and no dimensional inconsistencies were encountered.
 *  If the return value is 2 rather than 1, then the dimensionality
 *  has been determined before.
 *  The address of an allocated dimension type is passed in so that
 *  the dimensions of the relation (or at least what the function
 *  thinks the dimensions ought to be) can be also obtained.
 *
 *  @NOTE THIS ONLY WORKS ON e_token relations and maybe in future e_opcode
 *  relations. rel is assumed to be valid when called. !!!
 *
 *  @NOTE This brings in the asc_check_dimensions function from ascend_utils.
 *  3/96 Ben Allan
 */

ASC_DLLSPEC(enum Expr_enum ) RelationRelop(CONST struct relation *rel);
/**<
 *  Return the type of the relation operator of the relation.
 *  Returns one of the following:
 *  <pre>
 *  	e_equal      equality constraint
 *  	e_notequal   non-equality contraint
 *  	e_less       less than contraint
 *  	e_greater    greater than contraint
 *  	e_lesseq     less than or equal contraint
 *  	e_greatereq  greater than or equal contraint
 *  	e_maximize   objective function to be maximized (rhs is empty)
 *  	e_minimize   objective function to be minimized (rhs is empty)
 *  </pre>
 */

extern unsigned long NumberVariables(CONST struct relation *rel);
/**<
	This will indicate the number of distinct real atoms to which this relation
	points.  This number contains both fixed and non-fixed atoms.
	This routine is smart enough to deal with all the different relation types.
*/

ASC_DLLSPEC(struct Instance *) RelationVariable(CONST struct relation *rel,
                                         unsigned long varnum);
/**<
	This will return the varnum'th variable.
	This routine is smart enough to deal with all the different relation
	types.
*/

/*----------------------------------------------------------------------------
	TOKENRELATION AND OPCODERELATION STUFF

	It is the responsibility of the user of these routines to be aware
	of the type of relation being used. Very little sanity checking is
	done.
*/

ASC_DLLSPEC(unsigned long ) RelationLength(CONST struct relation *rel, int lhs);
/**<
	Returns the number of terms on one side of the equation. If lhs!=0, does this
	for the LHS. If lhs==0, does this for the RHS.
*/

ASC_DLLSPEC(CONST struct relation_term *) RelationTerm(CONST struct relation *rel,
        unsigned long pos,
        int lhs);
/**<
	Returns the term in position POS (base 1) on one side of the equation. 
	If lhs!=0, does this for the LHS. If lhs==0, does this for the RHS.

	@NOTE A bizarre thing about this operator: 1 <= pos <= RelationLength(rel,lhs).
	This is a holdover from gl_list days.
*/

#ifdef NDEBUG
#define NewRelationTerm(r,p,l) \
A_TERM( (l)!=0 ? (&(RTOKEN(r).lhs[(p)])) : (&(RTOKEN(r).rhs[(p)])) )
#else
#define NewRelationTerm(r,p,l) NewRelationTermF((r),(p),(l))
#endif
/**<
	Returns the term in position POS (base 0) on one side of the equation. 
	If lhs!=0, does this for the LHS. If lhs==0, does this for the RHS.

	For this operator: 0 <= p < RelationLength(rel,lhs) as a C array.

	@TODO Once Everybody gets their damn hands off RelationTerm, switch its
	semantics and eliminate one of this pair.

	@param r CONST struct relation*, the relation to query.
	@param p unsigned long, the position to retrieve.
	@param l int, flag for whether to return from the lhs or rhs.
	@return The specified term as a CONST struct relation_term*.
	@see NewRelationTermF()
*/

extern CONST struct relation_term
*NewRelationTermF(CONST struct relation *rel, unsigned long apos, int lhs);
/**<
	Implementation function for NewRelationTerm().  Do not call this
	function directly.
*/

#ifdef NDEBUG
#define RelationSideTerm(rs,p) A_TERM(&((rs)[(p)]))
#else
#define RelationSideTerm(rs,p) RelationSideTermF((rs),(p))
#endif
/**<
	Return the term in position p of the side.
	For this operator: 0 <= p < length of the side.

	@param rs CONST union RelationTermUnion*, the relation to query.
	@param p unsigned long, the position to retrieve.
	@return The specified term as a CONST struct relation_term*.
	@see RelationSideTermF()
*/
extern CONST struct relation_term
*RelationSideTermF(CONST union RelationTermUnion *relside, unsigned long apos);
/**<
	Implementation function for RelationSideTerm().  Do not call this
	function directly - use RelationSideTerm() instead.
*/

#ifdef NDEBUG
#define RelationTermType(rtp) ((rtp)->t)
#else
#define RelationTermType(rtp) RelationTermTypeF(rtp)
#endif
/**<
	Return the type of the relation term.

	@NOTE WARNING: if ALLOCATED_TESTS is active, term must be an allocated term;
	automatic variables will cause an assert() to fail.
	@param rtp CONST struct relation_term*, the term to query.
	@return The type as an enum Expr_enum.
	@see RelationTermTypeF()
*/
ASC_DLLSPEC(enum Expr_enum ) RelationTermTypeF(CONST struct relation_term *term);
/**<
	Implementation function for RelationTermType().  Do not call this
	function directly - use RelationTermType() instead.
*/

ASC_DLLSPEC(unsigned long ) TermVarNumber(CONST struct relation_term *term);
/**<
	@return the index into the relations variable list.
*/

ASC_DLLSPEC(long ) TermInteger(CONST struct relation_term *term);
/**<
	@return the integer value from a e_int type relation term.
*/

ASC_DLLSPEC(double ) TermReal(CONST struct relation_term *term);
/**<
	@return the double value from a e_real type relation term.
*/

extern double TermVariable(CONST struct relation *rel,
                           CONST struct relation_term *term);
/**<
	@return the double value from a e_var type relation term.
*/

ASC_DLLSPEC(CONST dim_type *) TermDimensions(CONST struct relation_term *term);
/**<
	Return the dimensions of a e_real, e_int, or e_zero relation term type.
	(e_int is always Dimensionless(); e_zero is always WildDimension().)
*/

ASC_DLLSPEC(CONST struct Func *) TermFunc(CONST struct relation_term *term);
/**<
 *  Return the function pointer of a function operator.
 */

ASC_DLLSPEC(unsigned long ) RelationDepth(CONST struct relation *rel);
/**<
	Return the depth of stack required to evaluate this relation.
*/

/*------------------------------------------------------------------------
	TOKENRELATION INFIX OPERATIONS
*/

/*
	The four defines following return term pointers.
	struct relation_term *r, *t;
	r = TermUniLeft(t); for example.

	@TODO These should be implemented as functions which assert type
	and revert to macros with NDEBUG.
*/
#define TermUniLeft(t)  ( ((struct RelationUnary *)t) -> left)
#define TermFuncLeft(t) ( ((struct RelationFunc *)t) -> left)
#define TermBinLeft(t)  ( ((struct RelationBinary *)t) -> left)
#define TermBinRight(t) ( ((struct RelationBinary *)t) -> right)

extern struct relation_term *RelationINF_Lhs(CONST struct relation *rel);
/**<
	Returns the lhs of an infix relation. This may be NULL,
	if the relation has not been set for infix scanning.
*/

extern struct relation_term *RelationINF_Rhs(CONST struct relation *rel);
/**<
	Return the rhs of an infix relation. This may be NULL
	if the relation has not been set up for infix scanning, or if
	the relation is an objective relation.
*/

extern int ArgsForRealToken(enum Expr_enum ex);
/**<
	Return the number of args required for a token from a real equation.
*/

/*------------------------------------------------------------------------
	OPCODE RELATION PROCESSING
*/

/**
	@TODO What's that mean?
	@TODO this stuff is not complete 
*/

#define OpCode_Lhs(r)       ((int *)(ROPCODE(r).lhs))
#define OpCode_Rhs(r)       ((int *)(ROPCODE(r).rhs))
#define OpCodeNumberArgs(r) (ROPCODE(r).nargs)
#define OpCodeConstants(r)  ((double *)(ROPCODE(r).constants))

/*------------------------------------------------------------------------
	BLACK BOX RELATION PROCESSING
*/
extern struct ExtCallNode *BlackBoxExtCall(CONST struct relation *rel);
extern int *BlackBoxArgs(CONST struct relation *rel);

#define BlackBoxNumberArgs(r) (RBBOX(r).nargs)

/*------------------------------------------------------------------------
	GLASS BOX STUFF
*/

/*
	These will be called a lot so that they will all be made
	macros. Double check that the same is true for the
	ExternalFunc routines.
*/
extern struct ExternalFunc *GlassBoxExtFunc(CONST struct relation *rel);
extern int GlassBoxRelIndex(CONST struct relation *rel);
extern int *GlassBoxArgs(CONST struct relation *rel);

#define GlassBoxNumberArgs(r) (RGBOX(r).nargs)

/*-----------------------------------------------------------------------------
	GENERAL STUFF FOR RELATIONS
*/

extern CONST struct gl_list_t *RelationVarList(CONST struct relation *r);
/**<
	Returns the unique incident variable list which is owned by the
	relation. *DO NOT MODIFY*. It is for the convenience of those
	desirous of a READ_ONLY look. It is a list of instance pointers, which may
	be NULL.
	All relation types will properly respond to this qurey.
*/

ASC_DLLSPEC(dim_type *) RelationDim(CONST struct relation *rel);
/**<
	Return the derived dimensionality of the relation.
	Defaults to Wild.
*/

ASC_DLLSPEC(int ) SetRelationDim(struct relation *rel, dim_type *d);
/**<
	Set the  dimensionality of the relation. return 0 unless there is a
	problem (rel was null, for instance.)
*/

ASC_DLLSPEC(double) RelationResidual(CONST struct relation *rel);
/**<
	Return the residual of the relation.
*/

extern void SetRelationResidual(struct relation *rel, double value);
/**<
	Set the value of the relation residual.
*/

extern double RelationMultiplier(CONST struct relation *rel);
/**<
	Return the langrage multiplier of the relation. This will have some
	hybrid dimensions that still needs to be decided, as it is a function
	of the objective function(s).
*/

extern void SetRelationMultiplier(struct relation *rel, double value);
/**<
	Set the value of the relation langrage multiplier. This will have some
	hybrid dimensions that still needs to be decided.
*/

ASC_DLLSPEC(int ) RelationIsCond(CONST struct relation *rel);
/**<
	Return the value of the iscond flag of the relation.
	If relation is NULL, returns 0.
*/

extern void SetRelationIsCond(struct relation *rel);
/**<
	Sets the value of the iscond field of the relation to 1
	If relation is NULL, writes error message.
*/

extern double RelationNominal(CONST struct relation *rel);
/**<
	Return the nominal of the relation.
*/

ASC_DLLSPEC(void ) SetRelationNominal(struct relation *rel, double d);
/**<
	Sets the value of the nominal field of the relation to the absolute
	value of d, unless d is 0.0.
*/

ASC_DLLSPEC(double ) CalcRelationNominal(struct Instance *i);
/**<
	Calculate the nominal of a relation.
	Returns 0.0 if something went detectably wrong in the calculation,
	otherwise calculates the absolute value of the maximum affine term
	and returns it, given an instance which is a token relation. Other
	relation types return the value 1.0.
	Does not set the constant stored with the relation.

	When opcode relations are fully supported,
	this function should be made to work for them, too.

	Art contends that the proper nominal for blackbox
	relations is the nominal of the output variable, though this is
	not implemented at present.

	Art further contends that the proper nominal for glassbox relations
	is the 2 norm of its gradient vector after fixed variables are
	removed and free elements have been scaled by the variable nominals.
	It should be noted that the glassbox scaling proposed by this method
	is precisely what the Slv solvers used up to August 1995.
	IMHO the glassbox generated should include code which knows
	how to calculate relation nominals. -- BAA
*/

extern void PrintRelationNominals(struct Instance *i);
/**<
	Perform a visit-instance-tree starting at i and calc/print consts.
	This function doesn't belong here.
*/

extern char *tmpalloc(int nbytes);
/**<
	Temporarily allocates a given number of bytes.  The memory need
	not be freed, but the next call to this function will reuse the
	previous allocation. Memory returned will NOT be zeroed.
	Calling with nbytes==0 will free any memory allocated.
*/

#define tmpalloc_array(nelts,type)  ((type *)tmpalloc((nelts)*sizeof(type)))
/**<
	Creates an array of "nelts" objects, each with type "type".
*/

/*------------------------------------------------------------------------------
	RELATION EVALUATION STUFF

 *   These are for Token equations, though if they can be done
 *   for all equation types that's a plus.
 *   Supercedes the bleeding mess in calc.c, rel.c, relman.c which was
 *   very ugly. -- BAA 5/96
 */

int RelationCalcResidualBinary(CONST struct relation *rel, double *res);
/**<
 * Returns 0 if it calculates a valid residual, 1 if
 * for any reason it cannot. Reasons include:
 * - relation not token relation.
 * - token relation not compiled to binary.
 * - NaN/infinity result.
 * - out of memory.
 * If return is 1, then res will not have been changed.
 * This function may raise SIGFPE it calls external code.
 */

enum safe_err
RelationCalcResidualPostfixSafe(struct Instance *i, double *res);
/**<
 *  Sets *res to the value (leftside - rightside) of the relation.
 *  status != 0 (safe_ok = 0) implies a problem.
 *  This function is slower than RelationCalcResidual() because it does
 *  a lot of range checking AND a floating point trap.
 */

int RelationCalcResidualPostfix(struct Instance *i, double *res);
/**<
 *  Sets *res to the value (leftside - rightside) of the relation.
 *  Uses postfix evaluation.
 *  status != 0 implies a problem.
 *  Notes: This function is a possible source of floating point
 *         exceptions and should not be used during compilation.
 */

/*
 * The following bit flags are used to build up the return
 * status from RelationCalcExceptionsInfix.
 * No provision is made for checking gradient elements --
 * gradients are in principle an all or nothing affair.
 */
#define RCE_BADINPUT	-1      /**< called with non-token relation */
#define RCE_OK          0     /**< no error */
#define RCE_ERR_LHS     0x1   /**< left side evaluation error */
#define RCE_ERR_RHS     0x2   /**< right side evaluation error */
#define RCE_ERR_LHSGRAD 0x4   /**< left side gradient error */
#define RCE_ERR_RHSGRAD 0x8   /**< right side gradient error */
#define RCE_ERR_LHSINF  0x10  /**< left side returns Infinity */
#define RCE_ERR_RHSINF  0x20  /**< right side returns Infinity */
#define RCE_ERR_LHSNAN  0x40  /**< left side returns NaN */
#define RCE_ERR_RHSNAN  0x80  /**< right side returns NaN */

ASC_DLLSPEC(int ) RelationCalcExceptionsInfix(struct Instance *i);
/**<
 *  Uses infix evaluation to check gradient and residual
 *  floating point exceptions.
 *  status != 0 implies a problem.
 *  Notes: This function is a possible source of floating point
 *         exceptions and should not be used during compilation.
 *  This function should not be called except inside the scope of a
 *   Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
 *   Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
 *  pair.
 *  Functions that report exceptions here may still be evaluated
 *  With the Safe relation evaluation routines in all but the most
 *  bizarre circumstances. The Safe results are necessarily approximate.
 *
 * @TODO (bug) At present, gradient checks are not implemented as the code
 *      required is messy. We need to rearrange CalcResidGrad().
 */

int RelationCalcResidualInfix(struct Instance *i, double *res);
/**<
 *  Sets *res to the value (leftside - rightside) of the relation.
 *  Uses infix evaluation.
 *  Non-zero return value implies a problem.
 *  Notes: This function is a possible source of floating point
 *         exceptions and should not be used during compilation.
 */

#define RelationCalcResidual(i,r) RelationCalcResidualPostfix(i,r)
#define RelationCalcResidualSafe(i,r) RelationCalcResidualPostfixSafe(i,r)

int RelationCalcGradient(struct Instance *i, double *grad);
/**<
	This calculates the gradient of the relation df/dx (f = lhs-rhs)
	where x is ALL entries in the relation's var list.
	The var list is a gl_list_t indexed from 1 to length.
	You must provide grad, the space to put the gradient, an array of
	double of length matching the gl_list_t.
	We will stuff df/dx[i] into grad[i-1], where i is the list position
	in the relation's var list.<br><br>
	
	@return Non-zero return value implies a problem
	
	@NOTE This function is a possible source of floating point
    exceptions and should not be used during compilation.
*/

enum safe_err RelationCalcGradientSafe(struct Instance *i, double *grad);
/**<
	This calculates the gradient of the relation df/dx (f = lhs-rhs)
	where x is ALL entries in the relation's var list.
	This function is to RelationCalcGradient as
	RelationCalcResidualSafe is to RelationCalcResidual.
	Non-zero return value implies a problem.
*/

int RelationCalcResidGrad(struct Instance *i, double *res, double *grad);
/**<
	This function combines the Residual and Gradient calls, since these
	may be done together at basically the cost of just one.
	Non-zero return value implies a problem.<br><br>
	
	@NOTE This function is a possible source of floating point exceptions
	and should not be used during compilation.
*/

enum safe_err
RelationCalcResidGradSafe(struct Instance *i, double *res, double *grad);
/**<
	This is the combined Safe version.
	Non-zero return value implies a problem.
*/

/*------------------------------------------------------------------------------
	ROOT FINDING FUNCTIONS (deprecated?)
*/

double *RelationFindRoots(struct Instance *i,
        double lower_bound, double upper_bound,
        double nominal, double tolerance,
        unsigned long *varnum,
        int *able,
        int *nsolns);
/**<
	RelationFindRoot WILL find a root if there is one. It is in charge of
	trying every trick in the book. The user must pass in a pointer to a
	struct relation. We require that the relation be of the type e_token with
	relation->relop = e_equals and we will whine if it is not.  The calling
	function should check able and/or nsolns before accessing the information
	in the soln_list.
	- nsolns < 0 : severe problems, such as var not found; soln_list will be NULL
	- nsolns = 0 : No solution found
	- nsolns > 0 : The soln_status equals the number of roots found
	
	@return NULL if success? 1 for success and 0 for failure?

	@NOTE In general compiler functions return 0 for success but this function 
	returns 1 for success because success = 1 is the convention on the solver 
	side. 

	@TODO (we really should make a system wide convention for return values)

	@NOTE The calling function should NOT free the soln_list.

	@NOTE we should recycle the memory used for glob_rel 
	
	@NOTE This function is NOT thread safe because it uses an internal memory
	recycle.

	@NOTE Before shutting down the system, or as desired, call this as:
	(void) RelationFindRoots(NULL,0,0,0,0,NULL,NULL,NULL);
	in order to free this memory.

	@TODO I think that this function might not really be used, or might only
	be used by old solvers. Is that the case? -- JP
*/

/*-----------------------------------------------------------------------------
	BINTOKEN STUFF
*/

struct gl_list_t *CollectTokenRelationsWithUniqueBINlessShares(
	struct Instance *i, unsigned long maxlen);
/**<
	Collect the token relation 'shares' in i which have not been compiled
	(or at least attempted so) to binary form yet.
	If more than maxlen are found, returns NULL instead. Presumably there
	is some upper limit beyond which you don't want to know the answer to
	this question. If none are found, returns a 0 length list.
	Actually, it is not a share that is collected, but instead any
	one of the relation instances which use the share is collected.
	The list returned should be destroyed by the user (not its content,though).
*/

#endif  /* ASC_RELATION_UTIL_H */
