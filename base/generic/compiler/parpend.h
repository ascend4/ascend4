/** 
 *  parpend.h
 *  by Ben Allan
 *  Jan 5, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: parpend.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:46 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

/** this file should be resorbed into the parameter list processing
 * code that results when instantiate.c is split up.
 * In the meanwhile...
 */
#ifndef __PARPEND_H_SEEN__
#define __PARPEND_H_SEEN__

enum ppstatus {
  pp_ERR =0,
  pp_ISA,       /** IS_A of simple to be done, from absorbed. */
  pp_ISAARR,    /** IS_A of array to do, from absorbed and
                 * gets converted to asar during processing.
                 */
  pp_ARR,       /** array that's constructed but requires range checking */
  pp_ASGN,      /** assignment to do in absorbed objects */
  pp_ASSC,      /** scalar assignment to check in absorbed objects */
  pp_ASAR,      /** Array to be checked for being completely assigned,
                 * but its subscript range is presumed right.
                 */
  pp_WV,        /** WITH_VALUE to be checked */
  pp_DONE       /** finished statement */
};

struct parpendingentry {
  struct Set *arg;      /** parameter given in user's IS_A statement */
  struct Statement *s;
  struct Instance *inst;
  struct parpendingentry *next;
  enum ppstatus status;
  int argn; /** the psl position if >0,  or -(the absorbed position) if <0 */
  /** argn==0 is an error */
};

/** 
 * allocate a parpending entry.
 */
extern struct parpendingentry *CreatePPE(void);

/** 
 * destroy a parpending entry.
 */
extern void DestroyPPE(struct parpendingentry *);

/** 
 * starts memory recycle. do not call twice before stopping recycle.
 */
extern void ppe_init_pool(void);

/** 
 * stops memory recycle. do not call while ANY parpending are outstanding.
 */
extern void ppe_destroy_pool(void);

/** 
 * write the pool report to ASCERR for the ppe pool.
 */
extern void ppe_report_pool(void);

#endif /** __PARPEND_H_SEEN__ */
