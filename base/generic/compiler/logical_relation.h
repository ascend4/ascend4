/**< 
 *  Logical Relation Data
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: logical_relation.h,v $
 *  Date last modified: $Date: 1997/07/29 15:52:42 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#ifndef __LOGICAL_RELATION_H_SEEN__
#define __LOGICAL_RELATION_H_SEEN__

/**< 
 *  When including logical_relation.h, make sure these files are included:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 */


/**< 
 *  The following logical relation term types should only be accessed through
 *  the operators in logrelation.h.
 */


struct LogRelBVar {
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;		/**< flags for future use */
  unsigned long varnum;
};


struct LogRelBoolean {
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;		/**< flags for future use */
  int bvalue;
};

struct LogRelInteger {
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;		/**< flags for future use */
  int ivalue;
};

/**< operators */
struct LogRelSatisfied {
  enum Expr_enum t;		/**< type of term */
  CONST struct Name *ncond;     /**< Name of relation inside CONDITIONAL */
  unsigned long relnum;         /**< number of relation in rel list */
  double rtol;                  /**< tolerance */
  CONST dim_type *dim;          /**< units of the relation */
  unsigned int flags;		/**< flags for future use */
};


struct LogRelUnary {		/**< unary NOT */
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;		/**< flags for future use */
  struct logrel_term *left;
};

struct LogRelBinary {           /**< binary AND, OR */
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;		/**< flags for future use */
  struct logrel_term *left;
  struct logrel_term *right;
};

/**< token type designed just as struct Instance */
struct logrel_term {
  enum Expr_enum t;		/**< type of term */
  unsigned int flags;
  /**< flags for future use. only valid for operator and bvar terms */
};

/**< 
 *  A union type for sizeof and other sorts.
 *  Not really intended for actual use except in sizeof and array declarations.
 */

union LogRelTermUnion {
  struct logrel_term anon;	/**< anonymous logical relation term type */
  struct LogRelBVar bvar;	/**< boolean vars */
  struct LogRelBoolean bol;     /**< boolean constants */
  struct LogRelInteger intt;    /**< integer for index in set */
  struct LogRelSatisfied sat;	/**< satisfied operator */
  struct LogRelUnary uni;	/**< unary operator NOT */
  struct LogRelBinary bin;	/**< binary operators OR AND */
};

/**< 
 * The different types of relation structures that need to
 * be supported by the relation module.
 */

struct TokenLogRel {
  unsigned long lhs_len, rhs_len;
  union LogRelTermUnion *lhs, *rhs;		/**< postfix arrays */
  struct logrel_term *lhs_term, *rhs_term;	/**< infix trees */
};
/**< 
 *  Under NO CIRCUMSTANCES should you attempt to free any element
 *  of the trees. They share memory with the postfix arrays.
 *  Also you should not dereference the pointers in a  logical
 *  relation except by appropriate operators from the header.
 */
/**< 
 * The struct relation may be shared by multiple LogRelInstances.
 */
struct logrelation {
  struct TokenLogRel token;  /**<   pointer ??   */
  int logresidual;
  int lognominal;
  int logiscond;
  struct gl_list_t *bvars;
  struct gl_list_t *satrels;
  REFCOUNT_T ref_count;
  enum Expr_enum relop;		/**< type of boolean constraint */
};

/**< casts to fix things up, should they really be needed. */
#define LOGA_TERM(i) ((struct logrel_term *)(i))
/**< anonymous term */
#define LOGBV_TERM(i) ((struct LogRelBVar *)(i))
#define LOGBC_TERM(i) ((struct LogRelBoolean *)(i))
#define LOGI_TERM(i) ((struct LogRelInteger *)(i))
#define LOGS_TERM(i) ((struct LogRelSatisfied *)(i))
#define LOGB_TERM(i) ((struct LogRelBinary *)(i))
#define LOGU_TERM(i) ((struct LogRelUnary *)(i))
#define LOGUNION_TERM(i) ((union LogRelTermUnion *)(i))

/**< 
 * The following define is for people who expect each term
 * allocated to be individually deallocated and interchangable
 * to all types of term. It returns a struct relation_term *.
 * Cast as needed if you must.
 * Use of individually allocated terms is a really bad idea!
 */
#define LOGTERM_ALLOC LOGA_TERM(ascmalloc(sizeof(union LogRelTermUnion)))

#endif /**< __LOGICAL_RELATION_H_SEEN__ */


