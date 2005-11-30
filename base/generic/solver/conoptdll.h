/*
 *  Definitions Required to use the CONOPT.DLL under Windows
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: conoptdll.h,v $
 *  Date last modified: $Date: 1998/02/26 15:57:57 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

/** @file
 *  Definitions Required to use the CONOPT.DLL under Windows.
 *  A public header file to be distributed.
 *  <pre>
 *  Requires:   #include "utilities/ascConfig.h"
 *  </pre>
 *  @todo Document conoptdll.h.
 */

#ifndef ASC_CONOPTDLL_H
#define ASC_CONOPTDLL_H

#ifndef ASC_ASCCONFIG_H /* only define these if we haven't seen ascConfig.h */
typedef int int32;
typedef double real64;
#endif /* _ASCCONFIG_H */


/* defines and typedefs for user supplied functions */
enum conopt_func_names {
  COIPSZ_ENUM,
  COIRMS_ENUM,
  COIRMU_ENUM,
  COIRC_ENUM,
  COIRR_ENUM,
  COIRNZ_ENUM,
  COIRNU_ENUM,
  COIFDE_ENUM,
  COIFBL_ENUM,
  COISTA_ENUM,
  COIRS_ENUM,
  COIUSZ_ENUM,   /* NOT IMPLEMENTED */
  COIUC_ENUM,    /* NOT IMPLEMENTED */
  COIUR_ENUM,    /* NOT IMPLEMENTED */
  COIUNZ_ENUM,   /* NOT IMPLEMENTED */
  COIUNU_ENUM,   /* NOT IMPLEMENTED */
  COILF_ENUM,
  COIEC_ENUM,
  COIER_ENUM,
  COIENZ_ENUM,
  COIMSG_ENUM,
  COIPRG_ENUM,
  COIORD_ENUM,
  COIORC_ENUM,
  COIBAN_ENUM,
  COISCR_ENUM,
  COICF_ENUM,    /* NOT IMPLEMENTED */
  COIOPT_ENUM
};


#define COIPSZ_VARS (nintgr,ipsz,nreal,rpsz,usrmem)
#define COIPSZ_ARGS (int32 *nintgr,int32 *ipsz,int32 *nreal, \
                     real64 *rpsz,real64 *usrmem)
typedef void COIPSZ_FUNC COIPSZ_ARGS;

#define COIRMS_VARS (lower, curr, upper, vsta, type, rhs, \
                     fv, esta, colsta, rowno, value, nlflag, \
                     n, m, n1, nz, usrmem )
#define COIRMS_ARGS (real64 *lower,real64 *curr,real64 *upper,int32 *vsta, \
               int32 *type,real64 *rhs,real64 *fv,int32 *esta, \
               int32 *colsta,int32 *rowno,real64 *value,int32 *nlflag, \
               int32 *n,int32 *m,int32 *n1,int32 *nz,real64 *usrmem)
typedef void COIRMS_FUNC COIRMS_ARGS;

#define COIRMU_VARS (lower, curr, upper, vsta, type, rhs, \
                     fv, esta, rowno, colno, value, nlflag, \
                     n, m, nz, usrmem )
#define COIRMU_ARGS (real64 *lower,real64 *curr,real64 *upper,int32 *vsta, \
               int32 *type,real64 *rhs,real64 *fv,int32 *esta, \
               int32 *rowno,int32 *colno,real64 *value,int32 *nlflag, \
               int32 *n,int32 *m,int32 *nz,real64 *usrmem)
typedef void COIRMU_FUNC COIRMU_ARGS;

#define COIRC_VARS (lower, curr, upper, vsta, n, usrmem )
#define COIRC_ARGS (real64 *lower,real64 *curr,real64 *upper,int32 *vsta, \
               int32 *n,real64 *usrmem)
typedef void COIRC_FUNC COIRC_ARGS;

#define COIRR_VARS (type, rhs, fv, esta, m, usrmem )
#define COIRR_ARGS (int32 *type,real64 *rhs,real64 *fv,int32 *esta, \
               int32 *m,real64 *usrmem)
typedef void COIRR_FUNC COIRR_ARGS;

#define COIRNZ_VARS (colsta, rowno, value, nlflag, n1, nz, usrmem )
#define COIRNZ_ARGS (int32 *colsta,int32 *rowno,real64 *value, \
               int32 *nlflag,int32 *n1,int32 *nz,real64 *usrmem)
typedef void COIRNZ_FUNC COIRNZ_ARGS;

#define COIRNU_VARS (rowno, colno, value, nlflag, nz, usrmem )
#define COIRNU_ARGS (int32 *rowno,int32 *colno,real64 *value, \
               int32 *nlflag,int32 *nz,real64 *usrmem)
typedef void COIRNU_FUNC COIRNU_ARGS;

#define COIFDE_VARS (x, g, jac, rowno, \
	    jcnm, mode, errcnt, \
	    newpt, n, nj, usrmem)
#define COIFDE_ARGS (real64 *x, real64 *g, real64 *jac, int32 *rowno, \
	    int32 *jcnm, int32 *mode, int32 *errcnt, \
	    int32 *newpt, int32 *n, int32 *nj, real64 *usrmem)
typedef void COIFDE_FUNC COIFDE_ARGS;

#define COIFBL_VARS (x, g, otn, nto, from, to, jac, stcl, \
	    rnum, cnum, nl, strw, llen, indx, mode, errcnt, n, \
	    m, n1, m1, nz, usrmem)
#define COIFBL_ARGS (real64 *x, real64 *g, int32 *otn, int32 *nto, \
      int32 *from, int32 *to, real64 *jac, int32 *stcl, int32 *rnum, \
	    int32 *cnum, int32 *nl, int32 *strw, int32 *llen, int32 indx, \
	    int32 *mode, int32 *errcnt, int32 *n, int32 *m, int32 *n1, \
      int32 *m1, int32 *nz, real64 *usrmem)
typedef void COIFBL_FUNC COIFBL_ARGS;

#define COISTA_VARS (modsta, solsta,iter,objval, usrmem)
#define COISTA_ARGS (int32 *modsta, int32 *solsta, int32 *iter, \
      real64 *objval, real64 *usrmem)
typedef void COISTA_FUNC COISTA_ARGS;

#define COIRS_VARS (xval, xmar, xbas, xsta, \
      yval, ymar, ybas, ysta, n, m, usrmem)
#define COIRS_ARGS (real64 *xval, real64 *xmar, int32 *xbas, \
      int32 *xsta, real64 *yval, real64 *ymar, int32 *ybas, \
      int32 *ysta, int32 *n, int32 *m, real64 *usrmem)
typedef void COIRS_FUNC COIRS_ARGS;

/** @todo: put chapter 6,7 and 8 functions here! */

#define COILF_VARS (flen, fn, usrmem)
#define COILF_ARGS (int32 *flen, char *fn,real64 *usrmem)
typedef void COILF_FUNC COILF_ARGS;

#define COIEC_VARS (colno, msglen, msg, usrmem)
#define COIEC_ARGS (int32 *colno, int32 *msglen, char msg[/*80*/],real64 *usrmem)
typedef void COIEC_FUNC COIEC_ARGS;

#define COIER_VARS (rowno, msglen, msg, usrmem)
#define COIER_ARGS (int32 *rowno, int32 *msglen, char msg[/*80*/],real64 *usrmem)
typedef void COIER_FUNC COIER_ARGS;

#define COIENZ_VARS (colno, rowno, posno, msglen, msg, usrmem)
#define COIENZ_ARGS (int32 *colno, int32 *rowno, int32 *posno, \
      int32 *msglen, char msg[/*80*/],real64 *usrmem)
typedef void COIENZ_FUNC COIENZ_ARGS;

#define COIMSG_VARS (nmsg, smsg, llen, msgv, usrmem)
#define COIMSG_ARGS (int32 *nmsg, int32 *smsg, int32 *llen, \
      char msgv[/*80*15*/],real64 *usrmem)
typedef void COIMSG_FUNC COIMSG_ARGS;

#define COIPRG_VARS (nintgr, intrep, nreal, rl, x, usrmem, finish)
#define COIPRG_ARGS (int32 *nintgr, int32 *intrep, int32 *nreal, \
      real64 *rl, real64 *x, real64 *usrmem, int32 *finish)
typedef void COIPRG_FUNC COIPRG_ARGS;

#define COIORD_VARS (colno, rowno, value, resid, usrmem)
#define COIORD_ARGS (int32 *colno, int32 *rowno, real64 *value, \
      real64 *resid,real64 *usrmem)
typedef void COIORD_FUNC COIORD_ARGS;

#define COIORC_VARS (colno, rowno, value, resid, usrmem)
#define COIORC_ARGS (int32 *colno, int32 *rowno, real64 *value, \
      real64 *resid,real64 *usrmem)
typedef void COIORC_FUNC COIORC_ARGS;

#define COIBAN_VARS (msg, vers)
#define COIBAN_ARGS (char *msg, char *vers)
typedef void COIBAN_FUNC COIBAN_ARGS;

#define COISCR_VARS (msg, len)
#define COISCR_ARGS (char msg[/*80*/], int32 *len)
typedef void COISCR_FUNC COISCR_ARGS;

#define COIOPT_VARS (name, rval, ival, lval, usrmem)
#define COIOPT_ARGS (char *name, real64 *rval, int32 *ival, \
      int32 *lval, real64 *usrmem)
typedef void COIOPT_FUNC COIOPT_ARGS;


/* defines and typedefs for conopt supplied functions */
/* registration functions */
#define REGISTER_CONOPT_FUNCTION_ARGS (enum conopt_func_names name, void *func)
typedef void REGISTER_CONOPT_FUNCTION_FUNC REGISTER_CONOPT_FUNCTION_ARGS;
#define UNREGISTER_CONOPT_FUNCTION_ARGS (enum conopt_func_names name, void *func)
typedef void UNREGISTER_CONOPT_FUNCTION_FUNC UNREGISTER_CONOPT_FUNCTION_ARGS;

/* chapter 2 functions */
#define COICSU_ARGS (int32 *keep,real64 *usrmem)
#define COICRS_ARGS (int32 *keep,real64 *usrmem)
#define COIRM_ARGS ()
#define COICSM_ARGS (int32 *kept,real64 *usrmem,int32 *lwork, \
                    real64 *work,int32 *maxusd,int32 *curusd)
typedef void COICSM_FUNC COICSM_ARGS;
#define COICRM_ARGS (int32 *kept,real64 *usrmem,int32 *lwork, \
                    real64 *work,int32 *maxusd,int32 *curusd)
typedef void COICRM_FUNC COICRM_ARGS;
#define COIMEM_ARGS (int32 *nintgr,int32 *ipsz,int32 *minmem,int32 *estmem)
typedef void COIMEM_FUNC COIMEM_ARGS;
#define COIXIT_ARGS ()

#endif /* _CONOPTDLL_H */

