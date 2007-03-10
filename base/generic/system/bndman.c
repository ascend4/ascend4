/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//* @file
	Boundary Manipulator Module
*//*
	Created: 04/97
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:13:57 $ $Author: mthomas $
*/

#include "bndman.h"

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>

#include "relman.h"
#include "logrelman.h"
#include "slv_server.h"

#include <math.h>

double bndman_real_eval(struct bnd_boundary *bnd){
  struct rel_relation *rel;
  double res;
  int32 calc_ok;

  if (bnd_kind(bnd)!=e_bnd_rel) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad bnd type");
    return 0.0;
  }
  calc_ok = 1;
  rel = bnd_rel(bnd_real_cond(bnd));
  res = relman_eval(rel,&calc_ok,1);
  CONSOLE_DEBUG("Got res = %f",res);
  if(!calc_ok){
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Error evaluating rel");
  }else{
    CONSOLE_DEBUG("bnd rel eval ok, res = %f", res);
  }
  CONSOLE_DEBUG("Returning res = %f",res);
  return res;
}


int32 bndman_log_eval(struct bnd_boundary *bnd){
  struct logrel_relation *lrel;
  int32 status,res;

  if (bnd_kind(bnd)!=e_bnd_logrel) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad bnd type");
    return 0;
  }

  status = 0;
  lrel = bnd_logrel(bnd_log_cond(bnd));
  res = logrelman_eval(lrel,&status);
  return res;
}


int32 bndman_calc_satisfied(struct bnd_boundary *bnd){
  int32 logres;
  struct rel_relation *rel;
  real64 res,tol;
  boolean rstat;

  switch(bnd_kind(bnd)) {
    case e_bnd_rel:
      rel = bnd_rel(bnd_real_cond(bnd));
      res = bndman_real_eval(bnd); /* force to reset real residual */
      tol = bnd_tolerance(bnd);
      rstat = relman_calc_satisfied(rel,tol);
      if (rstat) {
        return 1;
      } else {
        return 0;
      }
    case e_bnd_logrel:
      logres = bndman_log_eval(bnd); /* force to reset boolean residual */
      return logres;
    default:
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad bnd type");
      return 0;
  }
}


int32 bndman_calc_at_zero(struct bnd_boundary *bnd){
  real64 tol,res;

  if (bnd_kind(bnd)!=e_bnd_rel) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad bnd type");
    return 0;
  }
  tol = bnd_tolerance(bnd);
  res = bndman_real_eval(bnd);

  if (fabs(tol) > fabs(res)) {
    return 1;
  } else {
   return 0;
  }
}



