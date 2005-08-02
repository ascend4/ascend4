/*
 *  External Relations Cache for solvers.
 *  by Kirk A. Abbott
 *  Created: 8/10/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: extrel.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:16 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1994 Kirk Abbott
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/** @file
 *  External Relations Cache for solvers.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "var.h"
 *                #include "rel.h"
 *                #include "relation.h"
 *                #include "instance_enum.h"
 *  </pre>
 *  @todo This header is marked for removal.
 */

#ifndef __EXTREL_H_SEEN__
#define __EXTREL_H_SEEN__

#if 0 /* going away */
extern double g_external_tolerance;

extern struct ExtRelCache *CreateExtRelCache(struct ExtCallNode *);
extern struct ExtRelCache *CreateCacheFromInstance(struct Instance *);
extern void ExtRel_DestroyCache(struct ExtRelCache *);

extern int ExtRel_PreSolve(struct ExtRelCache *cache, int setup);
extern real64 ExtRel_Evaluate_RHS(struct rel_relation *);
extern real64 ExtRel_Evaluate_LHS(struct rel_relation *);
extern real64 ExtRel_Diffs_RHS(struct rel_relation *, var_filter_t *,
                               int32, mtx_matrix_t);
extern real64 ExtRel_Diffs_LHS(struct rel_relation *, var_filter_t *,
                               int32, mtx_matrix_t);
#endif
#endif /* __EXTREL_H_SEEN__ */

