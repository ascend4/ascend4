/*
 *  Logical Relation utility functions for Ascend
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: logrel_util.h,v $
 *  Date last modified: $Date: 1997/07/29 15:52:48 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
 *
 *  This file is part of the Ascend Interpreter.
 *  The Ascend Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Ascend is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

/** @file
 *  Logical Relation utility functions for Ascend.
 *  <pre>
 *  When #including logrel_util.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "logrelation.h"
 *         #include "instance_enum.h"
 *         #include "fractions.h"
 *         #include "dimen.h"
 *  </pre>
 */

#ifndef __LOGRELATION_UTIL_H_SEEN__
#define __LOGRELATION_UTIL_H_SEEN__

extern enum Expr_enum LogRelRelop(CONST struct logrelation *lrel);
/**<
 *  <!--  enum Expr_enum LogRelRelop(lrel)                             -->
 *  <!--  CONST struct logrelation *lrel;                              -->
 *  Return the type of the logical relation operator of the relation:
 *  - e_boolean_eq	equality constraint
 *  - e_boolean_neq	non-equality contraint
 */

extern unsigned long NumberBoolVars(CONST struct logrelation *lrel);
/**<
 *  <!--  unsigned long NumberBoolVars(lrel)                           -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  This will indicate the number of distinct boolean atoms to which this
 *  logical relation points.  This number contains both fixed and non-fixed
 *  atoms.
 */

extern struct Instance *LogRelBoolVar(CONST struct logrelation *lrel,
                                      unsigned long varnum);
/**<
 *  <!--  struct Instance *LogRelBoolVar(lrel,varnum)                  -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  unsigned long varnum;                                        -->
 *  This will return the varnum'th boolean variable.
 */

extern unsigned long NumberRelations(CONST struct logrelation *lrel);
/**<
 *  <!--  unsigned long NumberRelations(lrel)                          -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  This will indicate the number of distinct relations to which this
 *  logical relation points.  This number contains both conditional
 *  relations and conditional logrelations.
 */

extern struct Instance *LogRelRelation(CONST struct logrelation *lrel,
                                       unsigned long relnum);
/**<
 *  <!--  struct Instance *LogRelRelation(lrel,relnum)                 -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  unsigned long relnum;                                        -->
 *  This will return the relnum'th relation in the satisfied rel list.
 */

/*---------------------------------------------------------------------------
 *  Logical Relation specific code.
 *  Very little sanity checking is done.
 *---------------------------------------------------------------------------
 */

extern unsigned long LogRelLength(CONST struct logrelation *lrel, int lhs);
/**<
 *  <!--  unsigned long LogRelLength(lrel,lhs)                         -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  <!--  int lhs;                                                     -->
 *  If lhs is true, return the number of terms on the left hand side of the
 *  logical relation; otherwise, return the number of terms on the right
 *  hand side of the logical relation.
 */

extern CONST struct logrel_term
*LogRelTerm(CONST struct logrelation *lrel,
            unsigned long pos,
            int lhs);
/**<
 *  <!--  const struct logrel_term *LogRelTerm(lrel,pos,lhs)           -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  <!--  unsigned long pos;                                           -->
 *  <!--  int lhs;                                                     -->
 *  If lhs is true, return the term in position pos of the left hand side;
 *  otherwise, return the term in position pos of the right hand side.
 *  A bizarre thing about this operator: 1<=pos<=LogRelLength(lrel,lhs).
 *  This is a holdover from gllist days.
 */

#ifdef NDEBUG
#define NewLogRelTerm(r,p,l) \
LOGA_TERM( (l)!=0 ? (&((r)->token.lhs[(p)])) : (&((r)->token.rhs[(p)])) )
#else
#define NewLogRelTerm(r,p,l) NewLogRelTermF((r),(p),(l))
#endif
/**<
 *  If lhs is true, return the term in position p of the left hand side;
 *  otherwise, return the term in position p of the right hand side.
 *  For this operator: 0 <= p < LogRelLength(lrel,lhs) as a C array.
 *  Once Everybody gets their damn hands off LogRelTerm(), switch its
 *  semantics and eliminate one of this pair.
 *  @param r const struct logrelation*, logical relation to query.
 *  @param p unsigned long, position to return.
 *  @param l int, TRUE to return from left hand side, FALSE then from rhs.
 *  @return The term as an CONST struct logrel_term*.
 *  @see NewLogRelTermF()
 */
extern CONST struct logrel_term
*NewLogRelTermF(CONST struct logrelation *lrel, unsigned long apos, int lhs);
/**<
 *  <!--  const struct logrel_term *NewLogRelTerm(lrel,apos,lhs)       -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  <!--  unsigned long apos;                                          -->
 *  <!--  int lhs;                                                     -->
 *  <!--  If lhs is true, return the term in position apos of the left hand side; -->
 *  <!--  otherwise, return the term in position pos of the right hand side.      -->
 *  <!--  For this operator: 0 <= apos < LogRelLength(lrel,lhs) as a C array.     -->
 *  <!--  Once Everybody gets their damn hands off LogRelTerm, switch its         -->
 *  <!--  semantics and eliminate one of this pair.                               -->
 *  Implementation function for NewLogRelTerm().  Do not use this
 *  function directly - use NewLogRelTerm() instead.
 */

#ifdef NDEBUG
#define LogRelSideTerm(rs,p) LOGA_TERM(&((rs)[(p)]))
#else
#define LogRelSideTerm(rs,p) LogRelSideTermF((rs),(p))
#endif
/**<
 *  Return the term in position p of the side.
 *  For this operator: 0 <= p < length of the side.
 *  @param rs  CONST union LogRelTermUnion*, side.
 *  @param p   unsigned long, position to return.
 *  @return The term as an CONST struct logrel_term*.
 *  @see LogRelSideTermF()
 */
extern CONST struct logrel_term
*LogRelSideTermF(CONST union LogRelTermUnion *lrelside, unsigned long apos);
/**<
 *  <!--  CONST struct logrel_term *LogRelSideTerm(lrelside,apos)      -->
 *  <!--  CONST union LogRelTermUnion *lrelside                        -->
 *  <!--  unsigned long apos;                                          -->
 *  <!--  Return the term in position apos of the side.                -->
 *  <!--  For this operator: 0 <= apos < length of the side.           -->
 *  Implementation function for LogRelSideTerm().  Do not use this
 *  function directly - use LogRelSideTerm() instead.
 */

#ifdef NDEBUG
#define LogRelTermType(rtp) ((rtp)->t)
#else
#define LogRelTermType(rtp) LogRelTermTypeF(rtp)
#endif
/**<
 *  Return the type of the logical relation term rtp.
 *  WARNING: if ALLOCATED_TESTS is active, rtp must be an allocated term;
 *  automatic variables will cause an assert() to fail.
 *  @param rtp  const struct logrel_term*, the term to query.
 *  @return The type of the term as an enum Expr_enum.
 *  @see LogRelTermTypeF()
 */
extern enum Expr_enum LogRelTermTypeF(CONST struct logrel_term *term);
/**<
 *  <!--  enum Expr_enum LogRelTermType(term)                          -->
 *  <!--  const struct logrel_term *term;                              -->
 *  <!--  return the type of the logical relation term.                -->
 *  <!--  WARNING: if ALLOCATED_TESTS is active, term must be an allocated term; -->
 *  <!--  automatic variables will cause an assert() to fail.          -->
 *  Implementation function for LogRelTermType().  Do not use this
 *  function directly - use LogRelTermType() instead.
 */

extern unsigned long LogTermBoolVarNumber(CONST struct logrel_term *term);
/**<
 *  <!--  unsigned long LogTermBoolVarNumber(term)                     -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the index into the logical relation variable list.
 */

extern int LogTermBoolean(CONST struct logrel_term *term);
/**<
 *  <!--  int LogTermBoolean(term)                                     -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the boolean value from an e_boolean type logical relation term.
 */

extern int LogTermInteger(CONST struct logrel_term *term);
/**<
 *  <!--  int LogTermInteger(term);                                    -->
 *  <!--  CONST struct logrel_term *term;                              -->
 *  Return the integer value from an e_int type logical relation term.
 */

extern int LogTermIntegerBoolValue(CONST struct logrel_term *term);
/**<
 *  <!--  int LogTermInteger(term);                                    -->
 *  <!--  CONST struct logrel_term *term;                              -->
 *  Return a 0-1 value from an e_int type logical relation term.
 */

extern int LogTermBoolVar(CONST struct logrelation *lrel,
                          CONST struct logrel_term *term);
/**<
 *  <!--  int LogTermBoolVar(lrel,term)                                -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the integar value from a e_var type logical relation term.
 */

extern int LogTermSatisfied(CONST struct logrelation *lrel,
                            CONST struct logrel_term *term,
                            int perturb,
                            struct gl_list_t *instances);
/**<
 *  <!--  int LogTermSatisfied(lrel,term);                             -->
 *  <!--  CONST struct logrelation *lrel;                              -->
 *  <!--  CONST struct logrel_term *term;                              -->
 *  <!--  int perturb;                                                 -->
 *  <!--  struct gl_list_t *instances;                                 -->
 *  Return the truth value of a SATISFIED logical relation term.
 *  If perturb, and the instances pointed by the SATISFIED term is included
 *  in the list instances, the truth value of the SATISFIED term is
 *  inverted.
 */

extern CONST struct Name *LogTermSatName(CONST struct logrel_term *term);
/**<
 *  <!--  struct Name *LogTermSatName(term)                            -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the name of the conditional equation specified in an
 *  e_satisfied term
 */

extern unsigned long LogTermSatRelNumber(CONST struct logrel_term *term);
/**<
 *  <!--  unsigned long LogTermSatRelNumber(term)                      -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the index into the logical relation relations list.
 */

extern double LogTermSatTolerance(CONST struct logrel_term *term);
/**< 
 *  <!--  double *LogTermSatTolerance(term)                            -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the real value specified as the tolerance of an e_satisfied term
 */

extern CONST dim_type *LogTermSatDimensions(CONST struct logrel_term *term);
/**< 
 *  <!--  const dim_type *LogTermSatDimensions(term)                   -->
 *  <!--  const struct logrel_term *term;                              -->
 *  Return the dimensions of the real value specified in the tolerance of
 *  an e_satisfied term
 */

extern unsigned long LogRelDepth(CONST struct logrelation *lrel);
/**< 
 *  <!--  unsigned long LogRelDepth(lrel)                              -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  Return the depth of stack required to evaluate this logical relation.
 */

/*---------------------------------------------------------------------------
 *  TokenRelation Infix operations.
 *---------------------------------------------------------------------------
 */

/*
 *  The four defines following return term pointers.
 *  struct logrel_term *r, *t;
 *  r = LogTermUniLeft(t); for example.
 *      These should be implemented as functions which assert type
 *      and revert to macros with NDEBUG.
 */
#define LogTermUniLeft(t)  ( ((struct LogRelUnary *)t) -> left)
/**< 
 *  Retrieve the lhs of a unary logical relation.
 *  Parameter and return values are <code>struct logrel_term*</code>.
 */
#define LogTermBinLeft(t)  ( ((struct LogRelBinary *)t) -> left)
/**< 
 *  Retrieve the lhs of a binary logical relation.
 *  Parameter and return values are <code>struct logrel_term*</code>.
 */
#define LogTermBinRight(t) ( ((struct LogRelBinary *)t) -> right)
/**< 
 *  Retrieve the rhs of a binary logical relation.
 *  Parameter and return values are <code>struct logrel_term*</code>.
 */

extern struct logrel_term *LogRelINF_Lhs(CONST struct logrelation *lrel);
/**< 
 *  Returns the lhs of an infix logical relation. This may be NULL,
 *  if the logical relation has not been set for infix scanning.
 */

extern struct logrel_term *LogRelINF_Rhs(CONST struct logrelation *lrel);
/**< 
 *  Return the rhs of an infix logical relation. This may be NULL
 *  if the logical relation has not been set up for infix scanning.
 */

extern CONST struct gl_list_t *LogRelBoolVarList(CONST struct logrelation *lrel);
/**<
 *  <!--  const struct gl_list_t *LogRelBoolVarList(lr);               -->
 *  <!--  const struct logrelation *lr;                                -->
 *  Returns the unique incident variable list which is owned by the
 *  logical relation. *DO NOT MODIFY*. It is for the convenience of those
 *  desirous of a READ_ONLY look. It is a list of instance pointers,
 *  which maybe NULL.
 */

extern CONST struct gl_list_t *LogRelSatRelList(CONST struct logrelation *lrel);
/**< 
 *  <!--  const struct gl_list_t *LogRelSatRelList(lr);                -->
 *  <!--  const struct logrelation *lr;                                -->
 *  Returns the unique relation list which is owned by the
 *  logical relation. *DO NOT MODIFY*. It is for the convenience of those
 *  desirous of a READ_ONLY look. It is a list of instance pointers,
 *  which maybe NULL.
 */

ASC_DLLSPEC(int ) LogRelResidual(CONST struct logrelation *lrel);
/**< 
 *  <!--  int LogRelResidual(lrel)                                     -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  Return the boolean residual of the logical relation.
 */

extern void SetLogRelResidual(struct logrelation *lrel, int value);
/**< 
 *  <!--  void SetLogRelResidual(lrel,value)                           -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  int value;                                                   -->
 *  Set the value of the logical relation residual.
 */

ASC_DLLSPEC(int ) LogRelIsCond(CONST struct logrelation *lrel);
/**< 
 *  <!--  int LogRelIsCond(lrel)                                       -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  Return the value of the logiscond flag of the logicalrelation.
 */

extern void SetLogRelIsCond(struct logrelation *lrel);
/**< 
 *  <!--  void SetLogRelIsCond(lrel)                                   -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  Sets the value of the logiscond field of the logical relation to 1
 */

extern int LogRelNominal(CONST struct logrelation *lrel);
/**<
 *  <!--  int LogRelNominal(lrel)                                      -->
 *  <!--  const struct logrelation *lrel;                              -->
 *  Return the nominal of the logical relation.
 */

extern void SetLogRelNominal(struct logrelation *lrel, int i);
/**<
 *  <!--  void SetLogRelNominal(lrel,i)                                -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  int i;                                                       -->
 *  Sets the value of the nominal field of the logical relation to the
 *  value of i
 */

extern int FindTolInSatTermOfLogRel(struct Instance *lrelinst,
                                    struct Instance *condinst,
                                    double *tolerance);
/**<
 *  <!--  status = FindTolInSatTermOfLogRel(lrelinst,condinst);        -->
 *  <!--  struct Instance *lrelinst;                                   -->
 *  <!--  struct Instance *condinst;                                   -->
 *  <!--  int status;                                                  -->
 *  <!--  double *tolerance;                                           -->
 *  Look in the logrelation instance lrelinst for a SATISFIED term involving
 *  the instance condinst. When found, it will find the obtain tolerance
 *  specified in the SATISFIED term (if such a value is given) and it will
 *  assign it to the *tolerance. If the condinst is a logrel (mistakenly),
 *  or if the  SATISFIED term does not specify a value for the tolerance,
 *  it will assign the default value DEFTOLERANCE to *tolerance.
 *  status !=0 indicates it did not find a term containing the instance.
 */

/*
 *---------------------------------------------------------------------------
 * Calculation routines
 *---------------------------------------------------------------------------
 */

/*
 *  Calculation of the Logical Residuals of a logical relation :
 *  LogRelCalcResidualPostfix
 *  LogRelCalcResidualInfix
 *  and calculation of the values(s) of a boolean variable which make a
 *  logical relation to be satisfied:
 *  LogRelFindBoolValues
 *
 *  IMPORTANT:
 *  The solution of conditional model implicates the presence of
 *  SATISFIED terms (satisfaction of Boundary conditions)
 *  in some of the logical relations. Because the freedom that the
 *  modeling language gives to the user, sometimes is very difficult
 *  to know if the change of the truth value of a SATISFIED term is
 *  really meaningful to the configuration of the conditional problem.
 *  A boundary may be crossed, but still the boolean dependent on that
 *  condition may not change; furthermore, even a change in that boolean
 *  may not affect the configuration of the problem because a change
 *  of configuration may require the satisfaction of several conditions
 *  simultaneously.
 *  The easier way to test if the crossing of a boundary affects the current
 *  configuration is to solve the logical relations assuming such a
 *  crossing and finding if the configuration of the problem changes.
 *  This task is performed here by using the "perturb" flag and a list
 *  of pointers to the relation instance which constitutes the boundary(ies).
 *  The activation of the perturbation flag causes the calculation
 *  routines to INVERT the truth value of the SATISFIED terms containing
 *  the relation instances pointed by the list of pointers.
 *  The flag and the list of pointers are needed because the SATISFIED term
 *  is not an instance by itself and we have access to it only trough the
 *  logrel instance.
 *  Another option would be to modify the values of the variables incident
 *  in the boundary until the truth value of the SATISFIED changes by itself,
 *  but this would have to be iteratively (i.e. more expensive), since it
 *  would be required to perform  gradient calculations in
 *  order to know how the change of the value of some variable will modify
 *  the residual of the boundary. Other question that we would need to
 *  answer is how many variables should be modified simultaneously,etc.
 *  Notice that this is not necessary because we do not care about a change
 *  in the real values of the variables incident in the boundary (and, as
 *  a matter of fact, we do not want ot modify the current values), we only
 *  want to know what would it happen with the value of logical variables
 *  if the truth value of the SATISFIED term changes.
 *
 *  A third option would be to keep the information about the number
 *  of subregions in the conditional model and the values for the booleans
 *  which make them active. This is unaceptable for problems involving
 *  a big number of boundary conditions, since the problem is combinatorial.
 *
 *  IN GENERAL, ANY USER INTERESTED IN THESE CALCULATION ROUTINES FOR
 *  PURPOSES OF SOLVING PURE LOGIC PROBLEMS (NOT CONTAINING BOUNDARIES),
 *  SHOULD NOT CONSIDER THE
 *  PARAMETERS EXPLAINED ABOVE. IT IS RECOMMENDED THAT HE/SHE SHOULD
 *  USE THE MACRO
 *  LogRelCalcResidual
 *  WHICH CALL THE CALCULATION ROUTINES WITHOUT CONSIDERING THOSE
 *  PARAMETERS.
 */

extern int LogRelCalcResidualPostfix(struct Instance *i,
                                     int *res,
                                     int perturb,
                                     struct gl_list_t *instances);
/**<
 *  <!--  status = int LogRelCalcResidualPostfix(i,res,perturb,instances); -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  int *res;                                                    -->
 *  <!--  int perturb;                                                 -->
 *  <!--  struct gl_list_t *instances;                                 -->
 *  <!--  int status;                                                  -->
 *  Sets *res to the logical residual of the logical relation. The logical
 *  residual is evaluating by comparing the truth values of the two terms
 *  of the logical relation. If the logical relation is an equality, a
 *  residual of 1 (TRUE) means that the two terms has the same truth value,
 *  if they do not, the residual is zero. Similar reasoning applies when
 *  the logical relation is an inequality.
 *  Uses postfix evaluation.
 *  If *perturb !=0, that means that we will invert the truth value of
 *  the SATISFIED terms involving the relation instances pointed by the
 *  gl_list.
 *  status != 0 implies a problem.
 */

extern int LogRelCalcResidualInfix(struct Instance *i,
                                   int *res,
                                   int perturb,
                                   struct gl_list_t *instances);
/**<
 *  <!--  status = LogRelCalcResidualInfix(i,res,perturb,instances);   -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  int *res;                                                    -->
 *  <!--  int perturb;                                                 -->
 *  <!--  struct gl_list_t *instances;                                 -->
 *  <!--  int status;                                                  -->
 *  Sets *res to the logical residual of the logical relation. The logical
 *  residual is evaluating by comparing the truth values of the two terms
 *  of the logical relation. If the logical relation is an equality, a
 *  residual of 1 (TRUE) means that the two terms has the same truth value,
 *  if they do not, the residual is zero. Similar reasoning applies when
 *  the logical relation is an inequality.
 *  Uses infix evaluation.
 *  If *perturb !=0, that means that we will invert the truth value of
 *  the SATISFIED terms involving the relation instances pointed by the
 *  gl_list
 *  status != 0 implies a problem.
 */

extern int *LogRelFindBoolValues(struct Instance *i,
                                 unsigned long *dvarnum,
                                 int *able,
                                 int *nsolns,
                                 int perturb,
                                 struct gl_list_t *instances);
/**<
 *  <!--  soln_list = LogRelFindBoolValues(i,dvarnum,able,nsolns,perturb,instances); -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  long *dvarnum;                                               -->
 *  <!--  int *able;                                                   -->
 *  <!--  int *nsolns;                                                 -->
 *  <!--  int perturb;                                                 -->
 *  <!--  struct gl_list_t *instances;                                 -->
 *  <!--  int *soln_list;                                              -->
 *
 *  LogRelFindBoolValues WILL find a booolean solution if there is one.
 *  The user must pass in a pointer to a
 *  struct logrelation.  The calling function should check able and/or
 *  nsolns before accessing the information in the soln_list.
 *  nsolns < 0 : severe problems, soln_list will be NULL
 *  nsolns = 0 : No solution found
 *  nsolns > 0 : The soln_status equals the number of roots found
 *  nsolns is at most two for logical operations.
 *  The calling function should NOT free the soln_list.
 *  If *perturb !=0, that means that we will invert the truth value of
 *  the SATISFIED terms involving the relation instances pointed by the
 *  gl_list in order to find the boolean values wanted.
 */

#define LogRelCalcResidual(i,r) LogRelCalcResidualPostfix(i,r,0,NULL)
/**< 
 *  Macro for users interested in the calculation of pure logic relations.
 *  Not conditional modeling.
 */

#endif /* __LOGRELATION_UTIL_H_SEEN__ */

