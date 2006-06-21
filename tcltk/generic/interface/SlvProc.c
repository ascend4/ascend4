/*
 *  SlvProc.c
 *  by Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: SlvProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#include <math.h>
#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>		/* needed? */
#include <compiler/instance_enum.h>
#include <solver/slv_types.h>
#include <solver/mtx.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/discrete.h>
#include <solver/conditional.h>
#include <solver/logrel.h>
#include <solver/bnd.h>
#include <solver/slv_common.h>
#include <solver/linsol.h>
#include <solver/linsolqr.h>
#include <solver/slv_client.h>
#include "HelpProc.h"
#include "SolverGlobals.h"
#include "Commands.h" /* for registration function */
#include "SlvProc.h"

#ifndef lint
static CONST char SlvProcID[] = "$Id: SlvProc.c,v 1.6 2003/08/23 18:43:08 ballan Exp $";
#endif


#ifndef MAXIMUM_STRING_LENGTH
#define MAXIMUM_STRING_LENGTH 256
#endif


/* hellaciously bad assumption */
#define MAXIMUM_ID_LENGTH 80


/** monitor stuff           *******************************************/

/*
 * cast a pointer to monitor.
 */
#define SMC(cdata) ((struct SlvMonitor *)cdata)

/* the usual free and reassign operator */
#undef free_unless_null
#define free_unless_null(p) if ((p)!=NULL) { ascfree(p); } (p)=NULL

/*
 * a bunch of bit values for the updated field of the struct below.
 */
#define MON_ALLCLEAR	0x0
#define MON_VARCHANGE	0x1
#define MON_RELCHANGE	0x2
#define MON_VARSPEED	0x4
#define MON_RELLOG	0x8

#define MONALLSET(u) (u) = \
  (MON_VARCHANGE | MON_RELCHANGE | MON_VARSPEED | MON_RELLOG)

#define MONCLEAR(u,bit) (u) &= (~(bit))

/*
 * our basic data package.
 * Functions using the package should pay attention to the
 * update flag and if it is set, clear it while doing the appropriate
 * action.
 */
struct SlvMonitor {
  char *interface_id;	/* a constant symbolic handle for this object */
  real64 *lastrelres;	/* last scaled real relation residual */
  real64 *lastvarval;	/* last scaled variable value */
  real64 *lastvardel;	/* last scaled variable delta */
  unsigned int sys_id;	/* slv system unique identifier */
  int32 nrels;		/* size of lastrelres */
  int32 nvars;		/* size of lastvarval,lastvardel */
  int ulx, uly;		/* upper left corner of plot region */
  int w, h;		/* WxH plot region size, pixels. */
  int updated;		/* geometry change or wholesale data change */
  real64 varmax;	/* largest plottable value of any scaled variable */
  real64 relmax;	/* largest plottable value of any scaled residual */
  real64 relmin;	/* smallest plottable value of any scaled residual */
};




/*********************************************************************/

/*
 *  Analyze Stuff. Everything below here is independent
 *  of everything above and should be in a separate file.
 *  Maybe a ascSolverQuery.[ch]
 */

/*
 * This is the beginning of some analysis routines.
 * For upper bounds, lower bounds, nominals, scaling
 * langrange multipliers, residuals etc.
 */

#define SMALL_NUMBER 1e-12

enum Bounds_Enum {
  b_close, b_far, b_equal,
  b_invalid,
  b_lower, b_upper, b_nominal, b_othervalue,
  b_residual
};

static enum Bounds_Enum CloseToBound(real64 value, real64 bound,
                                     real64 tolerance,
                                     enum Bounds_Enum bounds_type,
                                     int relative_check)
{
  double error;

  switch(bounds_type) {
  case b_lower:
    if (value <= bound-SMALL_NUMBER) {
      return b_invalid;
    }
    break;
  case b_upper:
    if (value >= bound+SMALL_NUMBER) {
      return b_invalid;
    }
    break;
  case b_nominal:
  case b_residual:
  case b_othervalue:
    break;
  default:
    return b_invalid;
  }
  /*
   * The rest of this code does not care about
   * the type of bound that we are checking.
   */
  if ((fabs(value) < SMALL_NUMBER) || (relative_check==0)) {
    error = fabs(value - bound);
  } else {			/* do relative_check all other cases */
    error = fabs((value - bound)/value);
  }
  if (error <= tolerance) {
    return b_close;
  } else {
    return b_far;
  }
}


/*
 *
 * This is the analyze routine.
 * We also look for relations.
 * We extract the necessary values, and pass them on to the
 * analyze routine.
 * If b_type==b_othervalue we should do the
 * comparison. For example, one could be checking the value of a
 * lower_bound against some other value. HOWEVER, if we have
 * b_type!=b_othervalue, we just simply ignore it.
 *
 */


static void DoVarAnalyze(Tcl_Interp *interp,
                         struct var_variable **v,
                         unsigned long low, unsigned long high,
                         enum Bounds_Enum b_type,
                         real64 tolerance,
                         real64 othervalue,	/* an arbitrary value */
                         int relative_check)
{
  enum Bounds_Enum b_result;		/* result of the query */
  real64 value =0,checkvalue =0;
  unsigned long c;
  char tmp[80];

  for (c=low; c<=high; c++) {
    switch(b_type) {
      case b_nominal:
        value = var_value(v[c]);
        checkvalue = var_nominal(v[c]);
        break;
      case b_lower:
        value = var_value(v[c]);
        checkvalue = var_lower_bound(v[c]);
        break;
      case b_upper:
        value = var_value(v[c]);
        checkvalue = var_upper_bound(v[c]);
        break;
      case b_othervalue:
        value = var_value(v[c]);
        checkvalue = othervalue;
        break;
      default:
        b_result = b_invalid;	/* should not be here */
        break;
    }
    b_result = CloseToBound(value,checkvalue,tolerance,
                               b_type,relative_check);
    if (b_result==b_close) {
      sprintf(tmp,"%lu b_close",c);
      Tcl_AppendElement(interp,tmp);
    }
  }
}



static void DoRelAnalyze(Tcl_Interp *interp,
                         struct rel_relation **r,
                         unsigned long low, unsigned long high,
                         enum Bounds_Enum b_type,
                         real64 tolerance,
                         real64 othervalue,
                         int relative_check)
{
  enum Bounds_Enum b_result;
  real64 value,checkvalue;
  unsigned long c;
  char tmp[80];

  UNUSED_PARAMETER(othervalue);

  for (c=low; c<=high; c++) {
    if (b_type==b_residual) {
      value = rel_residual(r[c]);
      checkvalue = 0.0;		/* see if the residual is zero */
      b_result = CloseToBound(value,checkvalue,tolerance,
                              b_type,relative_check);
      if (b_result==b_close) {
        sprintf(tmp,"%lu b_close",c);
        Tcl_AppendElement(interp,tmp);
      }
    } else {
      b_result = b_invalid;
    }
  }
}



/*
 * Here is the version of the implementation.
 * Usage: __var_analyze low high \
 *		scaling?lower?upper?other tolerance rel?abs <otherval>.
 * Usage: __rel_analyze low high \
 *		residual?other tolerance rel?abs <otherval>.
 */



int Asc_VarAnalyzeCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct var_variable **vp;
  unsigned long maxvar;

  unsigned long low,high;	        /* range checking */
  enum Bounds_Enum b_type;		/* query type */
  real64 tolerance = 0;			/* tolerance value */
  real64 othervalue = 0;		/* a check an arbitrary value
                                         * which is not a child attribute.*/
  int relative_check = 1;		/* what type of check;
                                         * relative is the default */

  UNUSED_PARAMETER(cdata);

  if ( g_solvsys_cur == NULL ) {
    FPRINTF(stderr,  "Asc_VarAnalyzeCmd called with NULL pointer\n");
    Tcl_SetResult(interp, "Asc_VarAnalyzeCmd called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  vp=slv_get_solvers_var_list(g_solvsys_cur);
  maxvar = (unsigned long) slv_get_num_solvers_vars(g_solvsys_cur);

  if ( argc < 6 ) {
    Tcl_AppendResult(interp,"wrong # args: Usage :",
                    "\" __var_analyze\" low high \n",
                    "scaling?lower?upper?other tolerance rel?abs <othervalue>",
                     (char *)NULL);
    return TCL_ERROR;
  }

  low = atol(argv[1]);
  high = atol(argv[2]);
  if (!((low>0)&&(high>0)&&(high<=maxvar))) {
    Tcl_SetResult(interp, "Invalid index ranges in __var_analyze", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * We should now have a valid solver_var description.
   * Now go for the analysis type. Lagrange multipliers
   * could be added here, for relations as well as say
   * relation residuals.
   */
  if (strncmp(argv[3],"scaling",3)==0) {
    b_type = b_nominal;
  } else if (strncmp(argv[3],"lower",3)==0) {
    b_type = b_lower;
  } else if (strncmp(argv[3],"upper",3)==0) {
    b_type = b_upper;
  } else if (strncmp(argv[3],"other",3)==0) {
    b_type = b_othervalue;
    if ( argc != 7 ) {
      Tcl_AppendResult(interp,"A \"other value\" analysis requires an ",
                       " additional arg which is the comparison value",
                       (char *)NULL);
      return TCL_ERROR;
    } else {
      othervalue = atof(argv[6]);
    }
  } else {
    Tcl_SetResult(interp, "Invalid analyze type requested\n", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * We should now have a valid analyze query. The last 2 things
   * that we need are a relative or absolute check and a tolerance.
   */

  tolerance = atof(argv[4]);
  if (strncmp(argv[5],"relative",3)==0) {
    relative_check = 1;
  } else {
    relative_check = 0;
  }

  DoVarAnalyze(interp,vp,
            low,high,
            b_type,
            tolerance,
            othervalue,
            relative_check);
  return TCL_OK;
}


int Asc_RelAnalyzeCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  struct rel_relation **rp;
  unsigned long maxrel;

  unsigned long low,high;	        /* range checking */
  enum Bounds_Enum b_type;		/* query type */
  double tolerance = 0;			/* tolerance value */
  double othervalue = 0;		/* a check an arbitrary value
                                         * which is not a child attribute.*/
  int relative_check = 1;		/* what type of check;
                                         * relative is the default */

  UNUSED_PARAMETER(cdata);

  if ( g_solvsys_cur == NULL ) {
    FPRINTF(stderr,  "Asc_RelAnalyzeCmd called with NULL pointer\n");
    Tcl_SetResult(interp, "Asc_RelAnalyzeCmd called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  maxrel = (unsigned long) slv_get_num_solvers_rels(g_solvsys_cur);

  if ( argc < 6 ) {
    Tcl_AppendResult(interp,"wrong # args: Usage :",
                     "\" __rel_analyze\" low high\n",
                     "residual?other tolerance rel?abs <othervalue>",
                     (char *)NULL);
    return TCL_ERROR;
  }

  low = atol(argv[1]);
  high = atol(argv[2]);
  if (!((low>0)&&(high>0)&&(high<=maxrel))) {
    Tcl_SetResult(interp, "Invalid index ranges in __rel_analyze", TCL_STATIC);
    return TCL_ERROR;
  }

  if (strncmp(argv[3],"residual",3)==0) {
    b_type = b_residual;
  } else if (strncmp(argv[3],"other",3)==0) {
    b_type = b_othervalue;
    if ( argc != 7 ) {
      Tcl_AppendResult(interp,"A \"other value\" analysis requires an ",
                       " additional arg which is the comparison value",
                       (char *)NULL);
      return TCL_ERROR;
    } else {
      othervalue = atof(argv[6]);
    }
  } else {
    Tcl_SetResult(interp, "Invalid rel analyze type requested\n", TCL_STATIC);
    return TCL_ERROR;
  }

  tolerance = atof(argv[4]);
  if (strncmp(argv[5],"relative",3)==0) {
    relative_check = 1;
  } else {
    relative_check = 0;
  }

  DoRelAnalyze(interp,rp,low,high,b_type,tolerance,othervalue,relative_check);
  return TCL_OK;
}


/*
 * start of a solver monitor.
 * These functions are to be generic to every NLP client, so that
 * we DO NOT NEED to depend on block or mtx information.
 */

static
void MonDestroy(struct SlvMonitor *m)
{
  if (m==NULL) {
    return;
  }
  free_unless_null(m->interface_id);
  free_unless_null(m->lastrelres);
  free_unless_null(m->lastvarval);
  free_unless_null(m->lastvardel);
  m->sys_id = m->nrels = m->nvars = 0;
  m->updated = 103050301;
  ascfree(m);
}

/*
 * Updates the values of vars and rels, if the sys
 * id or sizes may have changed, possibly becoming non-NULL.
 * Returns 0 if ok, or 1 if insufficient memory.
 * If the system identifier is the same and its master list
 * sizes have not changed, this function does nothing.
 */
static
int MonUpdateData(struct SlvMonitor *m, slv_system_t sys)
{
  struct rel_relation **rp;
  struct var_variable **vp;
  int len,i;
  if (m==NULL) {
    return 1;
  }
  if (sys==NULL) {
    free_unless_null(m->lastrelres);
    free_unless_null(m->lastvarval);
    free_unless_null(m->lastvardel);
    m->sys_id = m->nrels = m->nvars = 0;
    return 0;
  }
  /* handle monitoring new system */
  if (m->sys_id != slv_serial_id(sys)) {
    m->sys_id = slv_serial_id(sys);

    free_unless_null(m->lastrelres);
    len = m->nrels = slv_get_num_master_rels(sys);
    if (len > 0) {
      rp = slv_get_master_rel_list(sys);
      m->lastrelres = ASC_NEW_ARRAY(real64,len);
      if (m->lastrelres == NULL) {
        return 1;
      }
      for (i = 0; i < len; i++) {
        m->lastrelres[i] = rel_residual(rp[i]);
      }
      MONALLSET(m->updated);
    }

    free_unless_null(m->lastvarval);
    free_unless_null(m->lastvardel);
    len = m->nvars = slv_get_num_master_vars(sys);
    if (len > 0) {
      vp = slv_get_master_var_list(sys);
      m->lastvarval = ASC_NEW_ARRAY(real64,len);
      if (m->lastvarval == NULL) {
        return 1;
      }
      m->lastvardel = ASC_NEW_ARRAY(real64,len);
      if (m->lastvardel == NULL) {
        return 1;
      }
      for (i = 0; i < len; i++) {
        m->lastvarval[i] = var_value(vp[i]);
        m->lastvardel[i] = 0.0;
      }
      MONALLSET(m->updated);
    }

    return 0;
  }

  /* if, oddly, the rel list changed size, fix up */
  len = slv_get_num_master_rels(sys);
  if (len > m->nrels) {
    free_unless_null(m->lastrelres);
    m->nrels = len;
    m->lastrelres = ASC_NEW_ARRAY(real64,len);
    if (m->lastrelres == NULL) {
      return 1;
    }
    /* update the residual data */
    rp = slv_get_master_rel_list(sys);
    for (i = 0; i < len; i++) {
      m->lastrelres[i] = rel_residual(rp[i]);
    }
    MONALLSET(m->updated);
  }

  /* if, oddly, the var list changed size, fix up */
  len = slv_get_num_master_vars(sys);
  if (len > m->nvars) {
    free_unless_null(m->lastvarval);
    free_unless_null(m->lastvardel);
    m->nvars = len;
    m->lastvarval = ASC_NEW_ARRAY(real64,len);
    if (m->lastvarval == NULL) {
      return 1;
    }
    m->lastvardel = ASC_NEW_ARRAY(real64,len);
    if (m->lastvardel == NULL) {
      return 1;
    }
    /* update the value data */
    vp = slv_get_master_var_list(sys);
    for (i = 0; i < len; i++) {
      m->lastvarval[i] = var_value(vp[i]);
      m->lastvardel[i] = 0.0;
    }
    MONALLSET(m->updated);
  }

  return 0;
}

/*
 * if geometry has changed or newness OTHERWISE detected,
 * updates values. Returns 0 if ok, 1 if check cannot be
 * completed properly.
 * If failure, interp has a message appended.
 */
static
int MonUpdateCheck(struct SlvMonitor *m, Tcl_Interp *interp,
                int argc, CONST84 char *argv[], slv_system_t sys)
{
  (void)argc;
  if (MonUpdateData(m,sys)) {
    Tcl_AppendResult(interp,argv[0],": malloc failure",(char *)NULL);
    return 1;
  }
  return 0;
}

/*
 * s change var 	return scaled values that changed
 * s change rel		return scaled residuals that changed
 * returns a tcl list whose elements are the changed values
 * with master list indices {index value}
 * values are scaled by nominals, or 1 if nominal is 0.
 *
 * This function may raise a floating point exception. The
 * caller should check the exception global and discard the
 * result of the function if exception occured.
 * The cause of such exceptions is division of a large value
 * by a small one.
 */
static
int MonChange(struct SlvMonitor *m, Tcl_Interp *interp,
              int argc, CONST84 char *argv[], slv_system_t sys)
{
  int i,len,do_all=0;
  struct var_variable **vp;
  struct rel_relation **rp;
  real64 tmpd,limit1,limit2,sign;
  real64 *list;
  char buf[40];
  assert(m!=NULL && interp!=NULL && sys != NULL && argc >= 3);

  if (argc != 4) {
    Tcl_AppendResult(interp,"need option to ",argv[0]," ",argv[1]," ",
        argv[2],(char *)NULL);
    return TCL_ERROR;
  }

  if (MonUpdateCheck(m,interp,argc,argv,sys)) {
    return TCL_ERROR;
  }

  switch (argv[3][0]) {
  case 'v':
    if (m->updated & MON_VARCHANGE) {
      MONCLEAR(m->updated,MON_VARCHANGE);
      do_all = 1;
    }
    vp = slv_get_master_var_list(sys);
    list = m->lastvarval;
    limit2 = fabs(m->varmax);
    len = m->nvars;
    for (i = 0; i < len; i++) {
      tmpd = var_nominal(vp[i]);
      if (tmpd == 0.0) {
        tmpd = 1;
      }
      tmpd = var_value(vp[i])/tmpd;
      if (tmpd > limit2) {
        tmpd = limit2;
      } else {
        if (tmpd < -limit2) {
          tmpd = (-limit2);
        }
      }
      if (do_all || tmpd != list[i]) {
        list[i] = tmpd;
        sprintf(buf,"%d %.18g",i,tmpd); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
    }
    return  TCL_OK;
  case 'r':
    if (m->updated & MON_RELCHANGE) {
      MONCLEAR(m->updated,MON_RELCHANGE);
      do_all = 1;
    }
    rp = slv_get_master_rel_list(sys);
    list = m->lastrelres;
    limit1 = m->relmin;
    limit2 = m->relmax;
    len = m->nrels;
    for (i = 0; i < len; i++) {
      tmpd = rel_nominal(rp[i]);
      if (tmpd == 0.0) {
        tmpd = 1;
      }
      /* record sign of residual and do work on abs value */
      tmpd = rel_residual(rp[i])/tmpd;
      sign = (tmpd <0) ? ((tmpd = -tmpd),-1.0) : 1.0;
      if (tmpd > limit2) {
        tmpd = limit2;
      } else {
        if (tmpd < limit1) {
          tmpd = limit1;
        }
      }
      tmpd *= sign;
      if (do_all || tmpd != list[i]) {
        list[i] = tmpd;
        sprintf(buf,"%d %.18g",i,tmpd); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
    }
    return  TCL_OK;
  default:
    /* whine about bad arg to change */
    Tcl_AppendResult(interp,"unknown option to ",argv[0]," ",argv[1]," ",
        argv[2],": ",argv[3],(char *)NULL);
    return TCL_ERROR;
  }
}

/*
 * s geometry w h x y rmin rmax vmax; sets conversion parameters for plotdata
 * turns on all need update, presumably because this is called with a window
 * size or range change that will require new drawing.
 */
static
int MonGeometry(struct SlvMonitor *m, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  /* parses an int arg n that should look like s and assigns v */
#define CSTI(n,s,v) \
    status = Tcl_GetInt(interp,argv[(n)],&i); \
    if (status != TCL_OK) { \
       Tcl_ResetResult(interp); \
       Tcl_AppendResult(interp,"error parsing ",argv[(n)]," as ",(s), \
           (char *)NULL); \
       return TCL_ERROR; \
    } (v) = i;

  /* parses an real64 arg n that should look like s and assigns v */
#define CSTD(n,s,v) \
    status = Tcl_GetDouble(interp,argv[(n)],&x); \
    if (status != TCL_OK) { \
       Tcl_ResetResult(interp); \
       Tcl_AppendResult(interp,"error parsing ",argv[(n)]," as ",(s), \
           (char *)NULL); \
       return TCL_ERROR; \
    } (v) = fabs(x);

  int status;
  int i;
  real64 x;

  assert(m!=NULL);
  if (argc !=10) {
    Tcl_AppendResult(interp,argv[0]," ",argv[1]," ", argv[2], " requires ",
        "4 ints and 3 reals: width height x y minresidual maxresidual maxvar",
        (char *)NULL);
    return TCL_ERROR;
  }
  CSTI(3,"width",m->w);
  CSTI(4,"height",m->h);
  CSTI(5,"x",m->ulx);
  CSTI(6,"y",m->uly);
  m->w = abs(m->w);
  m->h = abs(m->h);

  /* parse positive real limits */
  CSTD(7,"minresidual",m->relmin);
  CSTD(8,"maxresidual",m->relmax);
  CSTD(9,"maxvar",m->varmax);
  /* swap rel range limits if user is a git. */
  if (m->relmax < m->relmin) {
    x = m->relmin;
    m->relmin = m->relmax;
    m->relmax = x;
  }
  /* enforce display of at least 1 order of magnitude */
  if (m->relmax < 10*m->relmin) {
    m->relmax = 10*m->relmin;
  }
  MONALLSET(m->updated);
  return TCL_OK;
#undef CSTI
#undef CSTD
}

/*
 * s plotdata value 	return plot info for scaled values that changed
 * s plotdata speed 	return plot info for scaled rates of value change
 * s plotdata residual	return plot info for scaled residuals that changed
 *
 * Each option returns a list of {x y index} for changed values of the
 * variables or relations. The x,y are coordinates at which a point
 * should be plotted based on a transformation derived from whxy info
 * last obtained by the Geometry command of the monitor.
 * The transformation may specify the same coordinate for more than
 * one relation or variable.
 * If this function raises a floating point exception, as it may,
 * the result should be discarded.
 */
static
int MonPlotData (struct SlvMonitor *m, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[], slv_system_t sys)
{
  int i,do_all=0;
  struct var_variable **vp;
  struct rel_relation **rp;
  real64 tmpd,limit1,limit2,sign,delta,maxlog,minlog,rlog;
  int px,py,width,halfheight,x,y,nvars,nrels,center;
  real64 *list;
  char buf[40];

  assert(m!=NULL && interp!=NULL && sys != NULL && argc >= 3);

  if (argc != 4) {
    Tcl_AppendResult(interp,"need option to ",argv[0]," ",argv[1]," ",
        argv[2],(char *)NULL);
    return TCL_ERROR;
  }
  if (MonUpdateCheck(m,interp,argc,argv,sys)) {
    return TCL_ERROR;
  }

  x = m->ulx;
  y = m->uly;
  width = m->w;
  halfheight = m->h/2;
  center = y + halfheight;

  switch (argv[3][0]) {
  case 'v':
  /* plot variables as scaled between bounds +/-varmax.
   * the variables real bounds do not figure in this picture.
   */
    vp = slv_get_master_var_list(sys);
    if (m->updated & MON_VARCHANGE) {
      MONCLEAR(m->updated,MON_VARCHANGE);
      do_all = 1;
    }
    nvars = (int)m->nvars;
    list = m->lastvarval;
    limit2 = fabs(m->varmax);
    for (i = 0; i < nvars; i++) {
      tmpd = var_nominal(vp[i]);
      if (tmpd == 0.0) {
        tmpd = 1;
      }
      tmpd = var_value(vp[i])/tmpd;
      if (tmpd > limit2) {
        tmpd = limit2;
      } else {
        if (tmpd < -limit2) {
          tmpd = (-limit2);
        }
      }
      if (do_all || tmpd != list[i]) {
        list[i] = tmpd;
        px = x + (i*width)/nvars;
        py = center - (int)((tmpd*halfheight)/limit2);
        sprintf(buf,"%d %d %d",px,py,i); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
    }
    return TCL_OK;
  case 's':
  /* plot variable changes in the scaled space where varmax is the largest
   * plottable scaled change.
   */
    vp = slv_get_master_var_list(sys);
    list = m->lastvarval;
    limit2 = fabs(m->varmax);
    nvars = (int)m->nvars;
    if (m->updated & MON_VARSPEED) {
      MONCLEAR(m->updated,MON_VARSPEED);
      /* all speeds are 0 because we have no good history. update data. */
      for (i = 0; i < nvars; i++) {
        tmpd = var_nominal(vp[i]);
        if (tmpd == 0.0) {
          tmpd = 1;
        }
        tmpd = var_value(vp[i])/tmpd;
        if (tmpd > limit2) {
          tmpd = limit2;
        } else {
          if (tmpd < -limit2) {
            tmpd = (-limit2);
          }
        }
        list[i] = tmpd;
        m->lastvardel[i] = 0.0;
        px = x + (i*width)/nvars;
        sprintf(buf,"%d %d %d",px,center,i); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
      return TCL_OK;
    }
    /* using the last scaled values, calculate all deltas.
     * if delta is not the same as last delta, return data for it.
     * We calculate everything, but we don't return excess stuff for
     * plotting.
     */
    for (i = 0; i < nvars; i++) {
      tmpd = var_nominal(vp[i]);
      if (tmpd == 0.0) {
        tmpd = 1;
      }
      tmpd = var_value(vp[i])/tmpd;
      if (tmpd > limit2) {
        tmpd = limit2;
      } else {
        if (tmpd < -limit2) {
          tmpd = (-limit2);
        }
      }
      delta = tmpd-list[i];
      list[i] = tmpd;
      if (m->lastvardel[i] != delta) {
        m->lastvardel[i] = delta;
        px = x + (i*width)/nvars;
        py = center - (int)((delta*halfheight)/limit2);
        sprintf(buf,"%d %d %d",px,py,i); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
    }
    return TCL_OK;
  case 'r':
  /* log representation of scaled residuals */
    rp = slv_get_master_rel_list(sys);
    if (m->updated & MON_RELLOG) {
      MONCLEAR(m->updated,MON_RELLOG);
      do_all = 1;
    }
    list = m->lastrelres;
    limit1 = m->relmin;
    limit2 = m->relmax;
    maxlog = log10(limit2);
    minlog = log10(limit1);
    delta = maxlog-minlog;
    nrels = (int)m->nrels;
    for (i = 0; i < nrels; i++) {
      tmpd = rel_nominal(rp[i]);
      if (tmpd == 0.0) {
        tmpd = 1;
      }
      /* record sign of residual and do work on abs value */
      tmpd = rel_residual(rp[i])/tmpd;
      sign = (tmpd <0) ? ((tmpd = -tmpd),-1.0) : 1.0;
      if (tmpd > limit2) {
        tmpd = limit2;
      } else {
        if (tmpd < limit1) {
          tmpd = limit1;
        }
      }
      rlog = log10(tmpd);
      tmpd *= sign;
      if (do_all || tmpd != list[i]) {
        list[i] = tmpd;
        px = x + (width*i)/nrels;
        py = center - (int)(sign*halfheight*((rlog-minlog)/delta));
        sprintf(buf,"%d %d %d",px,py,i); /* do faster with tcl objects */
        Tcl_AppendElement(interp,buf);
      }
    }
    return  TCL_OK;
  default:
    /* whine about bad arg */
    Tcl_AppendResult(interp,"unknown option to ",argv[0]," ",argv[1]," ",
        argv[2],": ",argv[3],(char *)NULL);
    return TCL_ERROR;
  }
}

/*
 * Distributor to the options of a monitor command.
 * destroy		destroy monitor
 * s change var 	return scaled values that changed
 * s change rel		return scaled residuals that changed
 * s geometry w h x y rmin rmax vmax; sets conversion parameters for plotdata
 * s plotdata value 	return plot info for scaled values that changed
 * s plotdata speed 	return plot info for scaled rates of value change
 * s plotdata residual	return plot info for scaled residuals that changed
 *
 * The change and plotdata subcommands return information only for those
 * relations or variables that changed since we last recorded their values
 * or we last changed geometry in the case of plotdata.
 *
 * In the description above, the leading s is the symbolic name of a
 * slv_system. At present the value of s is ignored and g_solvsys_cur
 * is assumed. The solver needs to define symbolic handles and a lookup
 * function for us to interpret s properly.
 *
 * Most of the internals of this function and downstream use of
 * the data it returns in the interpreter could be much faster
 * if reimplemented in the tcl Object functions when those functions
 * have stabilized in 8.1.
 *
 * This function visits the entire var or rel master list and should not
 * be called at all on small blocks in large systems.
 */
static
int SolveMonitor(ClientData cdata,Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
  slv_system_t sys;
  char command[80];
  int i;

  ASCUSE;

  if (cdata==NULL) {
    /* what the? */
    return TCL_ERROR;
  }

  if (argc<2) {
    /* do usage thing */
    return TCL_ERROR;
  }
  if (argc < 3) {
    /* must be destroy */
    if (argv[1][0]=='d') {
      sprintf(command,"rename %s {}",SMC(cdata)->interface_id);
      return Tcl_GlobalEval(interp,command);
    }
    Tcl_AppendResult(interp,argv[0],": unknown option ",argv[1],(char *)NULL);
    return TCL_ERROR;
  }

  sys = g_solvsys_cur;
  /* this needs to be generalized to lookup sys based on argv[1].*/

  switch (argv[2][0]) {
  case 'c':
    if (sys==NULL) {
      return TCL_OK;
    }
    return MonChange(SMC(cdata),interp,argc,argv,sys);
  case 'g':
    return MonGeometry(SMC(cdata),interp,argc,argv);
  case 'p':
    if (sys==NULL) {
      return TCL_OK;
    }
    return MonPlotData(SMC(cdata),interp,argc,argv,sys);
  default:
    break;
  }
  Tcl_AppendResult(interp,argv[0],": unknown option",(char *)NULL);
  for (i=1; i<argc; i++) {
    Tcl_AppendResult(interp," ",argv[i],(char *)NULL);
  }
  return TCL_ERROR;
}

STDHLF(Asc_SolveMonitorCmd,(Asc_SolveMonitorCmdHL1,
                            Asc_SolveMonitorCmdHL2,
                            Asc_SolveMonitorCmdHL3,
                            HLFSTOP));

static
STDHLF(SolveMonitor,(SolveMonitorHL1,
                     SolveMonitorHL2,
                     SolveMonitorHL3,
                     SolveMonitorHL4,
                     SolveMonitorHL5,
                     SolveMonitorHL6,
                     SolveMonitorHL7,
                     SolveMonitorHL8,
                     HLFSTOP));
/*
 * Creates a monitor and returns its symbolic handle.
 * Multiple monitors can exist and are manipulated by their
 * symbolic handles.
 * A monitor may be used on a series of unrelated slv_system_t.
 * Currently, this function gets its slv_system_t from g_solvsys_cur,
 * but it should be changed to take a slvsys interface id when
 * the solver interface is changed to work by name.
 */
int Asc_SolveMonitorCmd(ClientData cdata,Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct SlvMonitor *result;
  static unsigned int nextid = 1;

  ASCUSE;
  if (argc!=1) {
    Tcl_AppendResult(interp,argv[0],": no arguments allowed yet",(char *)NULL);
    return TCL_ERROR;
  }

  result = SMC(asccalloc(1,sizeof(struct SlvMonitor)));
  if (result==NULL) {
    Tcl_AppendResult(interp,argv[0],": insufficient memory",(char *)NULL);
    return TCL_ERROR;
  }
  result->interface_id = (char *)ascmalloc(20+strlen(Asc_SolveMonitorCmdHN));
  if (result->interface_id==NULL) {
    Tcl_AppendResult(interp,argv[0],": insufficient memory",(char *)NULL);
    MonDestroy(result);
    return TCL_ERROR;
  }
  sprintf(result->interface_id,"%s%u",Asc_SolveMonitorCmdHN,nextid++);
  if (MonUpdateData(result,g_solvsys_cur)) {
    Tcl_AppendResult(interp,argv[0],result->interface_id,
        ": insufficient memory",(char *)NULL);
    MonDestroy(result);
    return TCL_ERROR;
  }

  result->w = result->h = 1;
  MONALLSET(result->updated);

  Asc_AddCommand(interp, result->interface_id, SolveMonitor,
    (ClientData)result, (Tcl_CmdDeleteProc *)MonDestroy, "solver-monitor",
    SolveMonitorHU, SolveMonitorHS, SolveMonitorHLF);

  Tcl_AppendResult(interp,result->interface_id,(char *)NULL);
  return TCL_OK;
}

