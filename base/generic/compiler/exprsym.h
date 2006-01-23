/*
 *  Symbolic Expression Manipulation
 *  by Kirk Abbott
 *  Created: Novermber 21, 1994
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: exprsym.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:23 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND compiler.
 *
 *  Copyright (C) 1994,1995 Kirk Andre Abbott.
 *  Copyright (C) 2006 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Symbolic Expression Manipulation
 *  <pre>
 *  When #including .h, make sure these files are #included first:
 *         #utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 *
 *  The user is reminded that the author of these routines
 *  could not be bothered with dimensionality, so don't expect
 *  much in the way of output that dimensionally checks or
 *  can be converted to real values in non-SI units unless the
 *  input was correct.
 *
 *  The user is also reminded that this code does not deal well
 *  with e_zero. e_zero should not exist in good models in any
 *  case.
 */

#ifndef ASC_EXPRSYM_H
#define ASC_EXPRSYM_H

/**
 * Until we decide whether to let the postfix and
 * infix data structures be shared. we will use these
 * typedefs.
 */
typedef struct Func Func;

typedef struct relation_term Term;
/**< note, so now Term has to be treated like A_TERM. */

typedef struct relation RelationINF;	/**< infix relation */

#define K_TERM(i) ((Term *)(i))
/**< Cast the i back to Term */

extern Term *TermSimplify(Term *term);
/**<
 *  Attempts term simplification. Later different levels of simplification
 *  will be made a feature.
 */

extern Term *Derivative(Term *term, unsigned long wrt,
                        int (*filter)(struct Instance *));
/**<
 *  The low level routine which actually does the symbolic differentiation
 *  with sub epxression simplification/elimination. In general not a safe
 *  place to start as use is made of a free store which has to be set up
 *  before this funcion may be called.
 */

extern void PrepareDerivatives(int setup, int n_buffers, int buffer_length);
/**<
 *  Call this function before and after doing symbolic derivatives.
 *  If setup is true, a free store of terms will be set up, with the
 *  specified number of buffers and buffer length. I am now using 2
 *  buffers of length 5000.
 *  If set up is false the memory allocated in the previous call to set up
 *  will be deallocated.
 */

#define ShutDownDerivatives() PrepareDerivatives(0,0,0)
/**<
 *  Deallocate memory allocated in the previous call to PrepareDerivatives().
 */

extern Term *TermDerivative(Term *term, unsigned long wrt,
                            int (*filter)(struct Instance *) );
/**<
 *  TermDerivative is the function that is used by RelationDerivative
 *  to generate the derivatives. Again it is perhaps more efficient
 *  to call RelationDerivative.
 */

extern RelationINF *RelDerivative(RelationINF *rel, unsigned long wrt,
                                  int (*filter)(struct Instance *));
/**<
 *  Given a infix relation, a index into its variable list and a function
 *  filter used to classify REAL_ATOM_INSTANCES as variables,parmaters or
 *  constants (or for that matter whatever the user pleases), this function
 *  will return a relation which is the symbolic derivative of the relation,
 *  with respect to the given variable. The relation *belongs* to the caller.
 *  The variale list will be updated to represent the new incidence after
 *  differentiation.
 */

extern void RelDestroySloppy(RelationINF *rel);
/**< 
 *  This function is to be used to deallocate a relation that was returned
 *  as a result of a call to RelDeriveSloppy.
 *  <pre>
 *  Example usage:
 *      ( .... )
 *      RelationINF *rel,*deriv;
 *      unsigned long wrt = 3;
 *      rel = (...)
 *      PrepareDerivates(1,3,500);
 *      deriv = RelDeriveSloppy(rel,wrt,NULL);
 *      WriteRelationInfix(stderr,deriv);
 *      RelDestroySloppy(deriv);
 *      ShutDownDerivatives();
 *      ( .... )
 *      return;
 *  </pre>
 */

extern RelationINF *RelDeriveSloppy(RelationINF *rel, unsigned long wrt,
                                    int (*filter)(struct Instance *));
/**<
 *  Given a infix relation, a index into its variable list and a function
 *  filter used to classify REAL_ATOM_INSTANCES as variables,parmaters or
 *  constants (or for that matter whatever the user pleases), this function
 *  will return a relation which is the symbolic derivative of the relation,
 *  with respect to the given variable.<br><br>
 *
 *  NOTE 1:<br>
 *  This function is provided for the benefit of users, who would like
 *  access to symbolic derivatives of a TRANSIENT nature. By this
 *  I mean that the derivative is going to be evaluated, written out etc,
 *  and then discarded. It makes certain shortcuts in the interest of speed
 *  For example, the returned variable list does not relect the fact
 *  that incidence may have been lost due to the process of doing the
 *  derivatives; but is still a valid list as differentiation can reduce
 *  incidence but not increase it.<br><br>
 *
 *  NOTE 2:<br>
 *  The relation structure that is returned *belongs* to the user.
 *  The variable list associated with the relation *belongs* to the user.
 *  The terms that make up the relation *do not belong* to the user.
 *  In fact *DO NOT deallocate any of these structures yourself. Use the
 *  above function RelDestroySloppy. This will deallocate the necessary
 *  structures. The term lists are the property of the freestore that was
 *  set up in PrepareDerivatives and will be handled by that store on
 *  Shutdown.
 */

#endif /* ASC_EXPRSYM_H */

