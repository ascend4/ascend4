/*
 *  UnitsProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.41 $
 *  Version control file: $RCSfile: UnitsProc.c,v $
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

#define ASC_BUILDING_INTERFACE

#include <utilities/config.h>
#ifdef ASC_SIGNAL_TRAPS
# include <utilities/ascSignal.h>
#endif

#include <stdarg.h>
#include <tcl.h>

#include "UnitsProc.h"

#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/dstring.h>

#include <compiler/instance_enum.h>
#include <compiler/cmpfunc.h>
#include <compiler/dimen_io.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/expr_types.h>
#include <compiler/mathinst.h>
#include <compiler/instance_name.h>
#include <compiler/relation_type.h>
#include <compiler/extfunc.h>
#include <compiler/find.h>
#include <compiler/rel_blackbox.h>
#include <compiler/vlist.h>
#include <compiler/relation.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/relation_util.h>

#include <linear/mtx.h>

#include <solver/slv_common.h>
#include <solver/slv_client.h>

#include "old_utils.h"
#include "HelpProc.h"
#include "BrowserQuery.h"
#include "BrowserProc.h"
#include "Qlfdid.h"
#include "Driver.h"
#include "HelpProc.h"
#include "SolverGlobals.h"

#ifndef lint
static CONST char UnitsProcID[] = "$Id: UnitsProc.c,v 1.41 2003/08/23 18:43:08 ballan Exp $";
#endif


/* convenience macros */
#define SNULL (char *)NULL
#define UnitsFindType(s) FindType(AddSymbol(s))

#ifdef ASC_SIGNAL_TRAPS
/** 
	return info when units convert badly. because io may happen within
	another trap context, we have our own return address and return function.
*/
static jmp_buf g_unit_env;
#endif


struct Units * g_base_units[NUM_DIMENS];
struct Units * g_SI_units[NUM_DIMENS];

/*********************INTERNALS ******************************/

static int display_precision = 6;
static char *unit_display_string = NULL;
static int updatefundunitdim;
static Tcl_Interp *unitsinterp;
#define UDS (unit_display_string)
#define UPREC (display_precision)

#define DLSIZE (512L)
/* while there are infinitely many instances and units combos, there are
   only a limited (in practice) number of dimensionalities. DLSIZE should
   be a reasonable upper bound on that limited number.
*/
static struct gl_list_t *DUList; /* association list for display units */

struct DisplayUnit {
  dim_type *d;       /* dimensionality applied to */
  struct Units *u;   /* user set display units */
  struct Units *fu;  /* fundamental display units */
};


static int check_units_set(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[]) /* args ignored but needed*/
{
  static int base_units_set;
  if (!base_units_set) {
     Asc_UnitDefaultBaseUnits(cdata,interp,argc,argv);
     base_units_set = 1;
  }
  return (base_units_set);
}

static
int Unit_CmpDU(CONST struct DisplayUnit *du1, CONST struct DisplayUnit *du2)
{
  return CmpDimen(du1->d,du2->d);
}

static int destroy_DUList() {
  if (DUList) {
    gl_free_and_destroy(DUList);
  }
  return 0;
}

static int check_DU_set()
{
  static int duset = 0 ;
  if (!duset) { /* first time through, init the world */
    dim_type *d;
    register unsigned long c,len = gl_length(g_dimen_list);
    struct DisplayUnit *newDU;
    DUList = gl_create(DLSIZE);
    assert(DUList!=NULL);
    for(c = 1;c <= len;c++) { /*far faster than FindOrAddDU */
      newDU=(struct DisplayUnit *)ascmalloc(sizeof(struct DisplayUnit));
      d = (dim_type *)gl_fetch(g_dimen_list,c);
      newDU->d = d;
      newDU->u = (struct Units *)NULL;
      newDU->fu = (struct Units *)NULL;
      gl_insert_sorted(DUList,(VOIDPTR)newDU,(CmpFunc)Unit_CmpDU);
    }
    duset = 1;
  }
  return (duset);
}

/* push fundy's to bottom of list */
static
int Unit_CmpAtomName(CONST struct TypeDescription *d1,
                     CONST struct TypeDescription *d2)
{
  if (!d1 || !d2 || CheckFundamental(GetName(d1)) ) {
    return 1;
  }
  return CmpSymchar(GetName(d1),GetName(d2));
}

/* small numbers and nulls go to the bottom (high index) of the list */
static
int Unit_CmpConv(CONST struct Units *u1, CONST struct Units *u2)
{
  if (!u1 || !u2 || UnitsConvFactor(u1)<UnitsConvFactor(u2)) {
    return 1;
  }
  if (UnitsConvFactor(u1)==UnitsConvFactor(u2)) {
    if (SCLEN(UnitsDescription(u1)) > SCLEN(UnitsDescription(u2))) {
      return 1;
    } else if (SCLEN(UnitsDescription(u1)) < SCLEN(UnitsDescription(u2))) {
      return -1;
    } else {
      return (CmpSymchar(UnitsDescription(u1),UnitsDescription(u2)));
    }
  } else {
    return -1;
  }
}

/*
 * note that since fractional and irrational exponents are not allowed
 * on dimensioned quantities, we don't have to worry on denominator.
 */
static
void Unit_WriteNumer(Tcl_DString *str, struct fraction frac,
                    CONST char *baseunit, int *CONST p)
{
  char sval[MAXIMUM_NUMERIC_LENGTH];
  if (Numerator(frac)>0) {
    (*p) = 1;
    if (Denominator(frac)==1) {
      if (Numerator(frac)==1) {
        sprintf(sval,"%s*",baseunit);
      } else {
        sprintf(sval,"%s^%d*",baseunit,Numerator(frac));
      }
    } else {
      /* this won't parse, but shouldn't happen anyway */
      sprintf(sval,"%s^(%d/%d)*",baseunit,Numerator(frac),Denominator(frac));
    }
    Tcl_DStringAppend(str,sval,-1);
  }
}

static
void Unit_WriteDenom(Tcl_DString *str, struct fraction frac,
                    CONST char *baseunit, int *CONST p)
{
  char sval[MAXIMUM_NUMERIC_LENGTH];
  if (Numerator(frac)<0) {
    (*p) = 1;
    if (Denominator(frac)==1) {
      if (Numerator(frac)==-1) {
        sprintf(sval,"/%s",baseunit);
      } else {
        sprintf(sval,"/%s^%d",baseunit,-Numerator(frac));
      }
    } else {
      /* this won't parse, but shouldn't happen anyway */
      sprintf(sval,"/%s^(%d/%d)",baseunit,-Numerator(frac),Denominator(frac));
    }
    Tcl_DStringAppend(str,sval,-1);
  }
}

/* return a nicely formatted units string */
static
char *Unit_MakeString(const dim_type *dimp, struct Units * units[NUM_DIMENS])
{
  struct fraction frac;
  Tcl_DString str1, str2;
  char *result;
  int printed = 0,len;
  Tcl_DStringInit(&str1);
  Tcl_DStringInit(&str2);
  if (IsWild(dimp)) {
    Tcl_DStringAppend(&str2,"*",-1);
  } else {
    int i;
    for( i = 0; i<NUM_DIMENS; i++ ) {
       frac = GetDimFraction(*dimp,i);
       Unit_WriteNumer(&str1,frac,SCP(UnitsDescription(units[i])),&printed);
    }
    if (!printed) {
      Tcl_DStringAppend(&str2,"1",-1);
      printed = 1;
    } else {
      /* eat the trailing multiply */
      Tcl_DStringAppend(&str2,
                        Tcl_DStringValue(&str1),
                        (strlen(Tcl_DStringValue(&str1))-1) );
    }
    for( i = 0; i<NUM_DIMENS; i++ ) {
      frac = GetDimFraction(*dimp,i);
      Unit_WriteDenom(&str2,frac,SCP(UnitsDescription(units[i])),&printed);
    }
  }
  len = strlen(Tcl_DStringValue(&str2));
  result = Asc_MakeInitString(len);
  strcpy(result, Tcl_DStringValue(&str2));
  Tcl_DStringFree(&str1);
  Tcl_DStringFree(&str2);
  return result;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void Unit_PrintDU(struct DisplayUnit *du)
{
  if (du==NULL) {
    FPRINTF(stderr,"NULL");
  } else {
    FPRINTF(stderr,"0x%p 0x%p \n",du->d, du->u);
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/*
 * return the DisplayUnit pointer that matches dimp.
 * create it if necessary. failing create, crash.
 */
static struct DisplayUnit *Unit_FindOrAddDU(const dim_type *dimp)
{
  unsigned long ndx;
  struct DisplayUnit dimpDU;
  struct DisplayUnit *newdu;

  dimpDU.d = (dim_type *)dimp;
  check_DU_set();
  ndx = gl_search(DUList,(VOIDPTR)&dimpDU,(CmpFunc)Unit_CmpDU);
  if (ndx==0L) {
    newdu = ASC_NEW(struct DisplayUnit);
    newdu->d = (dim_type *)dimp;
    newdu->u = NULL;
    newdu->fu = NULL;
    gl_insert_sorted(DUList,(VOIDPTR)newdu,(CmpFunc)Unit_CmpDU);
    return newdu;
  }
  return ((struct DisplayUnit *)gl_fetch(DUList,ndx));
}

/*
 * returns display unit matching dimp, creating it in association list if
 * not existing. Will not actually create a Units struct. a NULL return
 * means the unit has been defaulted in the units window.
 */
static struct Units *Unit_DisplayUnits(const dim_type *dimp)
{
  struct DisplayUnit *dimpdu;
  assert(check_DU_set());
  dimpdu = Unit_FindOrAddDU(dimp);
  return (dimpdu->u);
}

/*
 * called when display unit is defaulted. constructs a display string
 * if needed using user set base units and adds to the Units hash table.
 * returns the pointer of the fund units.
 */
static struct Units *Unit_DisplayFund(const dim_type *dimp)
{
  struct DisplayUnit* dimpDU;
  char *newunits = NULL;
  unsigned long pos;
  int ecode;

  assert(check_DU_set());
  dimpDU = Unit_FindOrAddDU(dimp);
  if (dimpDU->fu==NULL) {
    newunits = Unit_MakeString(dimp,g_base_units);
    dimpDU->fu = (struct Units *)FindOrDefineUnits(newunits,&pos,&ecode);
    if (!dimpDU->fu) {
      FPRINTF(stderr,"Error %d, position %ld\n",ecode,pos);
      FPRINTF(stderr,"failed fundstring: {%s} \n",newunits);
    }
    if (newunits) {
      ascfree(newunits);
    }
  }
  return (dimpDU->fu);
}

/*
 * called when display unit is too small. constructs a display string
 * using SI base units and adds to the Units hash table if needed.
 * returns the pointer of the units. This should seldom be called, so
 * we are constructing the string every time rather than expanding
 * the DisplayUnit struct.
 */
static struct Units *Unit_DisplaySI(const dim_type *dimp)
{
  static unsigned long pos;
  static int ecode;
  struct Units *u;
  char *newunits = Unit_MakeString(dimp,g_SI_units);
  u = (struct Units *)FindOrDefineUnits(newunits,&pos,&ecode);
  if (!u) {
    FPRINTF(stderr,"error %d, position %ld\n",ecode,pos);
    FPRINTF(stderr,"SIstring: {%s} \n",newunits);
  }
  if (newunits) {
    ascfree(newunits);
  }
  return u;
}

/*
 * this function checks to see if the fundamental unit being updated
 * needs to be changed in already set displayunits.
 */
static
void Unit_UpdateFundUnits(struct DisplayUnit *du)
{
  dim_type *d;

  if (du==NULL || du->fu==NULL) {
    return;
  }
  d = (dim_type *)UnitsDimensions(du->fu);
  if (Numerator(GetDimFraction(*(dim_type *)UnitsDimensions(du->fu),
                               updatefundunitdim)) != 0) {
    du->fu = NULL;
    Unit_DisplayFund(du->d);
  }
}

#ifdef ASC_SIGNAL_TRAPS
/* un/conversion error handling done in the next 5 procedures */
static
void uunconversion_trap(int sigval)
{
  (void)sigval;   /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"Float error in converting display value to SI value.\n");
  FPRESET;
  longjmp(g_unit_env,SIGFPE);
}
#endif /* ASC_SIGNAL_TRAPS */

/* respects any already active Asc_SignalTrap as we may want unit
 * output during another call which needs trapping.
 * retval is the SI value converted from the units specified by u
 * returns 1 if unhappy, 0 otherwise.
 */
static int Unit_UnconvertReal(double val, struct Units *u, double *retval)
{
  static int status;
  if (!u) {
    return 1;
  }
#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,uunconversion_trap);
  if (setjmp(g_unit_env)==0) {
#endif /* ASC_SIGNAL_TRAPS */
    status = 0;
    *retval = val*UnitsConvFactor(u);
#ifdef ASC_SIGNAL_TRAPS
  } else {
    status = 1;
  }
  Asc_SignalHandlerPop(SIGFPE,uunconversion_trap);
#endif /* ASC_SIGNAL_TRAPS */
  return status;
}

#ifdef ASC_SIGNAL_TRAPS
static
void uconversion_trap(int sigval)
{
  (void)sigval;   /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"Float error in converting SI value to display units.\n");
  FPRESET;
  longjmp(g_unit_env,SIGFPE);
}
#endif

/* respects any already active fp_trap
 * retval is the display value in the units specified by u
 */
static int Unit_ConvertReal(double val, struct Units *u, double *retval)
{
  static int status;
  if (!u) {
    return 1;
  }
#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,uconversion_trap);
  if (setjmp(g_unit_env)==0) {
#endif /* ASC_SIGNAL_TRAPS */
    status = 0;
/* often enough debug
    FPRINTF(stderr,"Conversion: v%.16g f%.16g\n",val,UnitsConvFactor(u));
*/
    *retval = val/UnitsConvFactor(u);
#ifdef ASC_SIGNAL_TRAPS
  } else {
    status = 1;
  }
  Asc_SignalHandlerPop(SIGFPE,uconversion_trap);
#endif /* ASC_SIGNAL_TRAPS */
  return status;
}

/* does integer math yield errors?? */
static int Unit_ConvertInteger(long val, struct Units *u, long *retval)
{
  (void)u;        /* stop gcc whine about unused parameter */

  *retval = val;
  return 0;
}
/* end any pretense of conversion error handling */

static int Unit_PrintUndefined(dim_type *dimp)
{
  char str[MAXIMUM_NUMERIC_LENGTH];
  int len;
  struct Units *du;
  if (IsWild(dimp)) {
    sprintf(str,"UNDEFINED *");
    len = 11; /* should match prior sprintf */
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  if (CmpDimen(dimp,Dimensionless())==0) {
    sprintf(str,"UNDEFINED ");
    len = 10;
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  du = Unit_DisplayUnits(dimp);            /* get units window set */
  if (du==NULL) {                          /* if defaulted, use base units */
    du = Unit_DisplayFund(dimp);
  }
  assert(du!=NULL);
  sprintf(str,"UNDEFINED ");
  len = strlen(str)+SCLEN(UnitsDescription(du));
  UDS = Asc_MakeInitString(len);
  strcat(UDS,str);
  strcat(UDS,SCP(UnitsDescription(du)));
  return 0;
}

static int Unitless_PrintUndefined(void)
{
  UDS = Asc_MakeInitString(9);
  strcat(UDS,"UNDEFINED");
  return 0;
}

/*
 * Assumes UDS is NULL on entry. That's why it's static.
 * Unit_PrintXXXXX are responsible for mallocing UDS, and
 * Asc_UnitValue is responsible for destroying it.
 */
static int Unit_PrintReal(double val, dim_type *dimp)
{
  char str[MAXIMUM_NUMERIC_LENGTH];
  int len;
  double dval;                           /* the printed value */
  struct Units *du;
  if (IsWild(dimp)) {
    sprintf(str,"%.*g *",UPREC,val);
    len = strlen(str);
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  if (CmpDimen(dimp,Dimensionless())==0) {
    sprintf(str,"%.*g",UPREC,val);
    len = strlen(str);
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  du = Unit_DisplayUnits(dimp);  	/* get units window set */
  if (du==NULL) {                          	/* if defaulted, use base units */
    du = Unit_DisplayFund(dimp);
    if (Unit_ConvertReal(val,du,&dval)) {  /* if conversion error, use SI */
      du = Unit_DisplaySI(dimp);
      dval = val;
    }
  } else {
    if (Unit_ConvertReal(val,du,&dval)) {  /* if units window set failed */
      du = Unit_DisplayFund(dimp);         /* get base units */
      if (Unit_ConvertReal(val,du,&dval)) {/* if conversion error, use SI */
        du = Unit_DisplaySI(dimp);
        dval = val;
      }
    }
  }
  assert(du!=NULL);
  sprintf(str,"%.*g ",UPREC,dval);
  len = strlen(str)+SCLEN(UnitsDescription(du));
  UDS = Asc_MakeInitString(len);
  strcat(UDS,str);
  strcat(UDS,SCP(UnitsDescription(du)));
  return 0;
}

/*
 * Assumes UDS is NULL on entry. That's why it's static.
 * Call only from Asc_UnitValue.
 */
static int Unitless_PrintReal(double val, dim_type *dimp, int si)
{
  char str[MAXIMUM_NUMERIC_LENGTH];
  int len;
  double dval;                /* the printed value */
  struct Units *du;
  if (IsWild(dimp) || CmpDimen(dimp,Dimensionless())==0) {
    sprintf(str,"%.*g",UPREC,val);
    len = strlen(str);
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  du = Unit_DisplayUnits(dimp);            /* get units window set */
  if (du==NULL) {                       /* if defaulted, use base units */
    du = Unit_DisplayFund(dimp);
    if (si || Unit_ConvertReal(val,du,&dval)) {
      /* if conversion error or si, use SI */
      dval = val;
    }
  } else {
    if (si || Unit_ConvertReal(val,du,&dval)) {
      /* if si or units window set conversion failed */
      du = Unit_DisplayFund(dimp);         /* get base units */
      if (si || Unit_ConvertReal(val,du,&dval)) {
        /* if conversion error or si, use SI */
        dval = val;
      }
    }
  }
  sprintf(str,"%.*g",UPREC,dval);
  len = strlen(str);
  UDS = Asc_MakeInitString(len);
  strcat(UDS,str);
  return 0;
}

/*
 * Assumes UDS is NULL on entry. That's why it's static.
 * Call only from Asc_UnitValue.
 */
static int Unit_PrintInteger(long val, dim_type *dimp)
{
  char str[81];
  int len;
  long dval;                              /* the printed value */
  struct Units *du;
  if (CmpDimen(dimp,Dimensionless())==0) {
    sprintf(str,"%ld",val);
    len = strlen(str);
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  if (IsWild(dimp)) {
    sprintf(str,"%ld *",val);
    len = strlen(str);
    UDS = Asc_MakeInitString(len);
    strcat(UDS,str);
    return 0;
  }
  du = Unit_DisplayUnits(dimp);              /* get units window set */
  if (du==NULL) {                            /* if defaulted, use base units */
    du = Unit_DisplayFund(dimp);
    if (Unit_ConvertInteger(val,du,&dval)) {  /* if conversion error, use SI */
      du = Unit_DisplaySI(dimp);
      dval = val;
    }
  } else {
    if (Unit_ConvertInteger(val,du,&dval)) {  /* if units window set failed */
      du = Unit_DisplayFund(dimp);            /* get base units */
      if (Unit_ConvertInteger(val,du,&dval)) {/* if conversion error, use SI */
        du = Unit_DisplaySI(dimp);
        dval = val;
      }
    }
  }
  sprintf(str,"%ld ",dval);
  len = strlen(str)+SCLEN(UnitsDescription(du));
  UDS = Asc_MakeInitString(len);
  strcat(UDS,str);
  strcat(UDS,SCP(UnitsDescription(du)));
  return 0;
}

/*
 * This code at the moment is only valid for token relations.
 */
static
dim_type *Unit_FindRelDim(CONST struct Instance *i)
{
  int consistent;
  dim_type dim;
  dim_type *newd;
  CONST struct relation *reln;
  enum Expr_enum reltype;

  reln = GetInstanceRelation(i,&reltype);
  switch (reltype) {
  case e_blackbox:
    if (!IsWild(RelationDim(reln))) {
      return RelationDim(reln);
    }
    if (g_check_dimensions_noisy) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox relation dimensionality waiting on output var dimensionality.");
      /* should come off lhs var of blackbox */
    }
    return (dim_type *)WildDimension();
  case e_glassbox:
  case e_opcode:
    if (g_check_dimensions_noisy) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Dimensionality checking not supported for these relns");
    }
    return (dim_type *)WildDimension();
  case e_token:
    break;
  default:
    Asc_Panic(2, __FUNCTION__, "Type is not a relation type in Unit_FindRelDimen.");
  }

  consistent = asc_check_dimensions(reln,&dim);
  if( !consistent ) {
    if (g_check_dimensions_noisy) {
      FPRINTF(stderr,"An inconsistency was found in ");
      WriteInstanceName(stderr,i,NULL); PRINTF("\n");
    }
    return (dim_type *)WildDimension();
  } else {
    newd = (dim_type *)FindOrAddDimen(&dim);
    if (consistent ==1) {
      SetRelationDim((struct relation *)reln,newd);
    }
    return newd;
  }
}

/*
 * returns TRUE if the instance is NULL or is dimensionally
 * sane. integers are sane (DIMENSIONLESS).
 */
static int IsDimInstance(CONST struct Instance *i)
{
  enum inst_t t;
  if (!i) {
    return 1;
  }
  t = InstanceKind(i);
  return (t== REAL_ATOM_INST ||    	/* non-fundamental real atom */
    t==REAL_INST         ||           	/* a fundamental real instance */
    t==REAL_CONSTANT_INST ||            /* a non fundamental real
                                         * constant instance */
    t==INTEGER_ATOM_INST ||           	/* non-fundamental integer atom */
    t==INTEGER_INST      ||           	/* a fundamental integer instance */
    t==INTEGER_CONSTANT_INST ||         /* a non fundamental integer
                                         * constant instance */
    t==REL_INST                       	/* relation(equality or inequality) */
  );
}

/*
 * Don't call it on a non-dimensional instance or you will get what
 * you deserve: a null pointer back.
 * For my purposes dimensional instances are INT, REAL, REL and their
 * ATOM versions.
 * Happens to not mind integers,since I think integers will have dims
 * eventually, and because it make the browser easy to deal with.
 * This is the broker for access to the Unit_PrintXXXXX routines and
 * is responsible for destroying the UDS.
 */
char *Asc_UnitValue(CONST struct Instance *i)
{
  double dval;
  dim_type *dimp = NULL;
  long ival;
  CONST struct relation *reln;
  enum Expr_enum reltype;

  if (UDS) {
    ascfree(UDS);
    UDS = SNULL;
  }
  if (!i) {
    return SNULL;
  }
  switch (InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case REAL_INST:
    dimp = (dim_type *)RealAtomDims(i);
    if (!AtomAssigned(i)) {
      Unit_PrintUndefined(dimp);
    } else {
      dval = RealAtomValue(i);
      Unit_PrintReal(dval,dimp);
    }
    break;
  case REL_INST:
    reln = GetInstanceRelation(i,&reltype);
    if (reln!=NULL) {
      dval = RelationResidual((struct relation *)reln);
      dimp = Unit_FindRelDim(i);
    } else {
      dval = 0.0;
      dimp = (dim_type *)WildDimension();
    }
    Unit_PrintReal(dval,dimp);
    break;
  case INTEGER_INST:
  case INTEGER_CONSTANT_INST:
  case INTEGER_ATOM_INST:
    /* dimp = IntegerAtomDims(i); Doesnotexist. integer inst don't have dim.*/
    if (!AtomAssigned(i)) {
      Unit_PrintUndefined((dim_type *)Dimensionless());
    } else {
      ival = GetIntegerAtomValue(i);
      Unit_PrintInteger(ival,(dim_type *)Dimensionless());
    }
    break;
  default:
    break;
  }
  return UDS;
}

/* follows exactly the same UDS management protocol as Asc_UnitValue. see
   comments above */
char *Asc_UnitlessValue(CONST struct Instance *i, int si)
{
  double dval;
  dim_type *dimp = NULL;
  long ival;
  enum Expr_enum reltype;
  if (UDS) {
    ascfree(UDS);
    UDS = SNULL;
  }
  if (!i) {
    return SNULL;
  }
  switch (InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case REAL_INST:
    dimp = (dim_type *)RealAtomDims(i);
    if (!AtomAssigned(i)) {
      Unitless_PrintUndefined();
    } else {
      dval = RealAtomValue(i);
      Unitless_PrintReal(dval,dimp,si);
    }
    break;
  case REL_INST:
    if (GetInstanceRelation(i,&reltype)!=NULL) {
      dval = RelationResidual(GetInstanceRelation(i,&reltype));
      dimp = Unit_FindRelDim(i);
    } else {
      dval = 0.0;
      dimp = (dim_type *)WildDimension();
    }
    Unitless_PrintReal(dval,dimp,si);
    break;
  case INTEGER_INST:
  case INTEGER_CONSTANT_INST:
  case INTEGER_ATOM_INST:
    /* dimp = IntegerAtomDims(i); Doesnotexist. integer inst don't have dim.*/
    if (!AtomAssigned(i)) {
      Unit_PrintUndefined((dim_type *)Dimensionless());
    } else {
      ival = GetIntegerAtomValue(i);
      Unit_PrintInteger(ival,(dim_type *)Dimensionless());
    }
    break;
  default:
    break;
  }
  return UDS;
}
/* follows UDS string convention as for Asc_UnitValue. */
char *Asc_UnitString(CONST struct Instance *i, int si)
{
  dim_type *dimp = NULL;
  struct Units *du;
  size_t len;
  enum Expr_enum reltype;
  if (UDS) {
    ascfree(UDS);
    UDS = SNULL;
  }
  if (!i) {
    return SNULL;
  }
  switch (InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case REAL_INST:
    dimp = (dim_type *)RealAtomDims(i);
    break;
  case REL_INST:
    if (GetInstanceRelation(i,&reltype)!=NULL) {
      dimp = Unit_FindRelDim(i);
    } else {
      dimp = (dim_type *)WildDimension();
    }
    break;
  case INTEGER_INST:
  case INTEGER_CONSTANT_INST:
  case INTEGER_ATOM_INST:
    dimp = (dim_type *)Dimensionless();
    break;
  default:
    break;
  }
  if (dimp == NULL) {
    FPRINTF(stderr,
            "WARNING: (Asc_UnitString) Called on undimensional instance\n");
    return UDS;
  }
  if (IsWild(dimp)) {
    UDS = Asc_MakeInitString(1);
    strcat(UDS,"*");
    return UDS;
  }
  if (CmpDimen(dimp,Dimensionless())==0) {
    UDS = Asc_MakeInitString(1);
    sprintf(UDS,"%s","");
    return UDS;
  }
  if (si) {
    du = Unit_DisplaySI(dimp);
  } else {
    du = Unit_DisplayUnits(dimp);    /* get units window set */
    if (du==NULL) {                       /* if defaulted, use base units */
      du = Unit_DisplayFund(dimp);
    }
  }
  len = SCLEN(UnitsDescription(du));
  UDS = Asc_MakeInitString(len);
  strcat(UDS,SCP(UnitsDescription(du)));
  return UDS;
}

/* follows UDS string convention as for Asc_UnitValue. */
char *Asc_UnitDimString(const dim_type *dimp, int si)
{
  struct Units *du;
  size_t len;
  if (UDS) {
    ascfree(UDS);
    UDS = SNULL;
  }
  if (dimp==NULL) {
    return SNULL;
  }
  if (IsWild(dimp)) {
    UDS = Asc_MakeInitString(1);
    strcat(UDS,"*");
    return UDS;
  }
  if (CmpDimen(dimp,Dimensionless())==0) {
    UDS = Asc_MakeInitString(1);
    sprintf(UDS,"%s","");
    return UDS;
  }
  if (si) {
    du = Unit_DisplaySI(dimp);
  } else {
    du = Unit_DisplayUnits(dimp);    /* get units window set */
    if (du==NULL) {                  /* if defaulted, use base units */
      du = Unit_DisplayFund(dimp);
    }
  }
  len = SCLEN(UnitsDescription(du));
  UDS = Asc_MakeInitString(len);
  strcat(UDS,SCP(UnitsDescription(du)));
  return UDS;
}

/* Convert to/from si. if si is TRUE, assumes in is si value and
tries to convert value consistent with units given. If si is FALSE
assumes in is value in units given and tries to convert to si.
*/
int Asc_UnitConvert(struct Units *u, double in, double *op, int si)
{

  if (u==NULL || op == NULL) {
    return 1;
  }
  if (si) {
    if (Unit_ConvertReal(in,u,op)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    if (Unit_UnconvertReal(in,u,op)) {
      return 1;
    } else {
      return 0;
    }
  }
}
int Asc_UnitSetRealAtomValue(CONST struct Instance *i,
                          char *vstr, char *ustr, unsigned depth)
{
  double dval = 0;
  char *ends = SNULL;

  /* check instance validity */
  if (!i || (!(InstanceKind(i)==REAL_INST)
             && !(InstanceKind(i)==REAL_ATOM_INST)
             && !(InstanceKind(i)==REAL_CONSTANT_INST)
             )) {
    return 4;
  }
  /* get number from vstr */
  if (!vstr) {
    return 5;
  }
  dval = strtod(vstr,&ends);
  if (ends==vstr) {
    return 5;
  }

  /* get valid units struct */
  if (ustr==NULL || strlen(ustr)==0 || strcmp(ustr,"*")==0) {
    SetRealAtomValue((struct Instance *)i,dval,depth);
  } else {
    dim_type *dimp = NULL;
    double sival = (double)0.0;
    struct Units *up = (struct Units *)LookupUnits(ustr);

    if (up==NULL) {
      unsigned long pos;
      int ecode;
      up = (struct Units *)FindOrDefineUnits(ustr,&pos,&ecode);
      if (up==NULL) {
        return 1;
      }
    }
    dimp = (dim_type *)UnitsDimensions(up);
    assert(dimp!=NULL /* null dims returned by FindOrDefineUnits */);

    /* assign dimensionality if wild, or check for dim incompatibility. */
    if (IsWild(RealAtomDims(i))) {
      SetRealAtomDims((struct Instance *)i,dimp);
    } else if ( CmpDimen(dimp,RealAtomDims(i)) ) {
        return 2;
    }
    /*  convert to SI, and assign if ok */
    if (Unit_UnconvertReal(dval,up,&sival)) {
      return 3;
    }
    SetRealAtomValue((struct Instance *)i,sival,depth);
  }
  return 0;
}

/* assumes the tcl unitsinterp is set before entry. else does nothing.
*/
static
void Unit_GetUserSet(struct DisplayUnit *du)
{
  if (!unitsinterp) {
    return;
  }
  if (du->u!=NULL && UnitsDescription(du->u)!=NULL) {
    Tcl_AppendElement(unitsinterp,(char *)UnitsDescription(du->u));
  }
}

/********************* END INTERNALS ******************************/

int Asc_UnitDestroyDisplayList(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    return TCL_ERROR;
  }
  destroy_DUList();
  return TCL_OK;
}


int Asc_UnitDefaultBaseUnits(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  static int SIset;
  int i;

  UNUSED_PARAMETER(cdata);
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if (!SIset) {
    g_SI_units[D_MASS]=
      (struct Units *)LookupUnits(UNIT_BASE_MASS);
    g_SI_units[D_QUANTITY]=
      (struct Units *)LookupUnits(UNIT_BASE_QUANTITY);
    g_SI_units[D_LENGTH]=
      (struct Units *)LookupUnits(UNIT_BASE_LENGTH);
    g_SI_units[D_TIME]=
      (struct Units *)LookupUnits(UNIT_BASE_TIME);
    g_SI_units[D_TEMPERATURE]=
      (struct Units *)LookupUnits(UNIT_BASE_TEMPERATURE);
    g_SI_units[D_CURRENCY]=
      (struct Units *)LookupUnits(UNIT_BASE_CURRENCY);
    g_SI_units[D_ELECTRIC_CURRENT]=
      (struct Units *)LookupUnits(UNIT_BASE_ELECTRIC_CURRENT);
    g_SI_units[D_LUMINOUS_INTENSITY]=
      (struct Units *)LookupUnits(UNIT_BASE_LUMINOUS_INTENSITY);
    g_SI_units[D_PLANE_ANGLE]=
      (struct Units *)LookupUnits(UNIT_BASE_PLANE_ANGLE);
    g_SI_units[D_SOLID_ANGLE]=
      (struct Units *)LookupUnits(UNIT_BASE_SOLID_ANGLE);
    SIset = 1;
    for (i = 0;i<NUM_DIMENS;i++) {
      assert(g_SI_units[i]!=NULL);
    }
  }
  for (i = 0;i<NUM_DIMENS;i++) {
    g_base_units[i]=g_SI_units[i];
  }
  return TCL_OK;
}

int Asc_UnitGetBaseUnits(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  int i;
  check_units_set(cdata, interp, argc, argv);
  for (i = 0; i<NUM_DIMENS; i++) {
    if (g_base_units[i]!=NULL) {
      Tcl_AppendElement(interp,(char *)UnitsDescription(g_base_units[i]));
    } else {
      Tcl_AppendElement(interp,(char *)"undefined!");
    }
  }
  return TCL_OK;
}

int Asc_UnitDump(ClientData cdata, Tcl_Interp *interp,
              int argc, CONST84 char *argv[])
{
  int dev,status = TCL_OK, tmpi;
  FILE * fp;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    FPRINTF(stderr,"call is: u_dump <device #> \n");
    Tcl_SetResult(interp, "u_dump <arg> expects 0,1,2 for #.", TCL_STATIC);
    return TCL_ERROR;
  }

  tmpi = 3;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >2) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_dump: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_dump: invalid output dev #", TCL_STATIC);
    return status;
  } else {
    dev = tmpi;
  }

  switch (dev) {
    case 0: fp = stdout;
            break;
    case 1: fp = stderr;
            break;
    case 2: fp = NULL;
            break;
    default : /* should never be here */
            FPRINTF(stderr,"u_dump called with strange i/o option!!\n");
            return TCL_ERROR;
  }

  if (fp==NULL) {
    char a[1024];
    register unsigned long c;
    struct Units *p;
    for(c = 0;c<UNITS_HASH_SIZE;c++) {
      for(p = g_units_hash_table[c];p!=NULL;p = p->next) {
        if ( argc == 3 ) {
          sprintf(a,"%20s %20.15g ",
                  SCP(UnitsDescription(p)),
                  UnitsConvFactor(p));
        } else {
          /* pretty was asked not asked for */
          char *ussi;
          char *dimstr;
          ussi = UnitsStringSI(p);
          dimstr = WriteDimensionString(UnitsDimensions(p));
          sprintf(a,"%p %s %.16g %s %s",
                  (void *)UnitsDimensions(p),
                  SCP(UnitsDescription(p)),
                  UnitsConvFactor(p),
                  UnitsStringSI(p),
                  (dimstr==NULL)?"":dimstr);
          if (dimstr != NULL) {
            ascfree(dimstr);
          }
          ascfree(ussi);
        }

        Tcl_AppendElement(interp,a);
      }
    }
  } else {
    DumpUnits(fp);
  }
  return TCL_OK;
}

int Asc_DimenDump(ClientData cdata, Tcl_Interp *interp,
               int argc, CONST84 char *argv[])
{
  int dev,status = TCL_OK, tmpi;
  FILE * fp;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_dims <device #> \n");
    Tcl_SetResult(interp, "u_dims <arg> expects 0,1,2 for #.", TCL_STATIC);
    return TCL_ERROR;
  }

  tmpi = 3;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >2) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_dims: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_dims: invalid output dev #", TCL_STATIC);
    return status;
  } else {
    dev = tmpi;
  }

  switch (dev) {
    case 0: fp = stdout;
            break;
    case 1: fp = stderr;
            break;
    case 2: fp = NULL;
            break;
    default : /* should never be here */
            FPRINTF(stderr,"u_dims called with strange i/o option!!\n");
           return TCL_ERROR;
  }

  if (!fp) {
    char a[1024];
    register unsigned long c,len = gl_length(g_dimen_list);
    dim_type *d;
    for(c = 1;c<=len;c++) {
      d = (dim_type *)gl_fetch(g_dimen_list,c);
      if (IsWild(d)) {
        sprintf(a,"wild");
      } else {
        sprintf(a,"%s","\0");
      }
      if (CmpDimen(d,Dimensionless())!=0) {
        Asc_BrowWriteDimensions(a,d);
      }
      Tcl_AppendResult(interp," {",a,"}",SNULL);
    }

  } else {
    DumpDimens(fp);
  }
  return TCL_OK;
}

int Asc_DimenRelCheck(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[]) {
  int status,tmpi;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_dim_setverify <0,1>\n");
    Tcl_SetResult(interp, "u_dim_setverify expects a number 0 or 1.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  tmpi = 2;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi>1) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_dim_setverify: value must be 0 or 1");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_dim_setverify: invalid boolean given.",
                  TCL_STATIC);
    return status;
  }
  g_check_dimensions_noisy = tmpi;
  return TCL_OK;
}
int Asc_UnitBaseDimToNum(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  char tmps[4];
  char *c;
  int i;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_dim2num <M,T,L,C,Q,TMP,P,S,E,LUM> \n");
    Tcl_SetResult(interp, "u_dim2num expects 1 argument", TCL_STATIC);
    return TCL_ERROR;
  }
  c = QUIET(argv[1]);
  for( i = 0; i < NUM_DIMENS && strcmp(c,DimName(i)); i++ );
  if( i == NUM_DIMENS ) {
    Tcl_SetResult(interp, "u_dim2num called with unknown base dimension.",
                  TCL_STATIC);
    return TCL_ERROR;
  } else {
    sprintf(tmps,"%d",i);
  }
  Tcl_AppendResult(interp,tmps,SNULL);
  return TCL_OK;
}

int Asc_UnitNumToBaseDim(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  int status = TCL_OK, tmpi;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_num2dim <num> \n");
    Tcl_SetResult(interp, "u_num2dim <arg>", TCL_STATIC);
    return TCL_ERROR;
  }
  tmpi = 100;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi>=NUM_DIMENS) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_num2dim: arg is in range 0 - %d\n",(NUM_DIMENS-1));
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_num2dim: invalid dim #", TCL_STATIC);
    return status;
  }
  Tcl_AppendResult(interp,DimName(tmpi),SNULL);
  return TCL_OK;
}

int Asc_UnitMatchBaseDim(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int status = TCL_OK;
  int tmpi;

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_num2dim <num> \n");
    Tcl_SetResult(interp, "u_num2dim <arg>", TCL_STATIC);
    return TCL_ERROR;
  }
  tmpi = 100;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi>=NUM_DIMENS) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_frombasedim: arg is in range 0 - %d\n",
                   (NUM_DIMENS-1));
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_frombasedim: invalid dim #", TCL_STATIC);
    return status;
  }
  status = Asc_UnitNumToBaseDim(cdata,interp,argc,argv);
  if (status ==TCL_OK) {
    dim_type dim;
    struct gl_list_t *ulist = gl_create(50L);
    register unsigned long c;
    struct Units *p;

    ClearDimensions(&dim);
    ParseDim(&dim,Tcl_GetStringResult(interp));
    Tcl_ResetResult(interp);
    for(c = 0;c<UNITS_HASH_SIZE;c++) {
      for(p = g_units_hash_table[c];p!=NULL;p = p->next) {
        if (CmpDimen(&dim,UnitsDimensions(p))==0) {
          gl_insert_sorted(ulist,(VOIDPTR)p,(CmpFunc)Unit_CmpConv);
        }
      }
    }
    for(c = 1;c<=gl_length(ulist);c++) {
      Tcl_AppendElement(interp,
        (char *)UnitsDescription((struct Units *)gl_fetch(ulist,c)));
    }
    gl_destroy(ulist);
  } else {
    Tcl_AppendResult(interp," called from u_frombasedim",SNULL);
  }
  return status;
}

int Asc_UnitMatchAtomDim(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_fromatomdim <atom_typename> \n");
    Tcl_SetResult(interp, "u_fromatomdim: expects atom type.", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = UnitsFindType(argv[1]);
  if (desc!=NULL && GetBaseType(desc)==real_type) {
    dim_type *dim = (dim_type *)GetRealDimens(desc);
    struct gl_list_t *ulist = gl_create(50L);
    register unsigned long c;
    struct Units *p;

    for(c = 0;c<UNITS_HASH_SIZE;c++) {
      for(p = g_units_hash_table[c];p!=NULL;p = p->next) {
        if (CmpDimen(dim,UnitsDimensions(p))==0) {
          gl_insert_sorted(ulist,(VOIDPTR)p,(CmpFunc)Unit_CmpConv);
        }
      }
    }
    for(c = 1;c<=gl_length(ulist);c++) {
      Tcl_AppendElement(interp,
        (char *)UnitsDescription((struct Units *)gl_fetch(ulist,c)));
    }
    gl_destroy(ulist);
  } else {
    Tcl_SetResult(interp, "u_fromatomdim called with bad real atom name",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_UnitGetAtomList(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct gl_list_t *alist = gl_create(200L);
  struct gl_list_t *dlist = NULL;
  register struct TypeDescription *desc, *rtdesc;
  register dim_type *dim;
  register unsigned long c,len;
  char a[1024];

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(stderr,"call is: u_getdimatoms <no args> \n");
    Tcl_SetResult(interp, "u_getdimatoms: unexpected arg found.", TCL_STATIC);
    return TCL_ERROR;
  }
  rtdesc = UnitsFindType("real");
  assert(rtdesc);
  dlist = DefinitionList();
  if (!dlist) {
    Tcl_SetResult(interp, "u_getdimatoms found no type definitions.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  len = gl_length(dlist);
  for(c = 1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(dlist,c);
    if (desc) {/* MoreRefined doesn't take kindly to null */
      if (MoreRefined(desc,rtdesc)) {
        dim = (dim_type *)GetRealDimens(desc);
        if (CmpDimen(dim,Dimensionless())!=0 && !IsWild(dim)) {
          gl_insert_sorted(alist,(VOIDPTR)desc,(CmpFunc)Unit_CmpAtomName);
        }
      }
    }
  }
  gl_destroy(dlist);
  len = gl_length(alist);
  for(c = 1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(alist,c);
    sprintf(a,"%s ",(char *)SCP(GetName(desc)));
    Asc_BrowWriteDimensions(a,GetRealDimens(desc));
    Tcl_AppendElement(interp,a);
  }
  gl_destroy(alist);
  return TCL_OK;
}

int Asc_UnitChangeBaseUnit(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct Units *up = NULL;
  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_change_baseunit <unit>\n");
    Tcl_SetResult(interp, "u_change_baseunit wants a simple unit arg",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  check_units_set(cdata,interp,argc,argv);
  check_DU_set();
  up = (struct Units *)LookupUnits(argv[1]);
  if (up != NULL) {
    int c;
    for(c = 0;
        ((c < (NUM_DIMENS))
         && CmpDimen(UnitsDimensions(up),UnitsDimensions(g_base_units[c]))!=0);
        c++);
    if (c==(NUM_DIMENS)) {
      Tcl_SetResult(interp,
                    "u_change_baseunit called with non-base dimensioned unit",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    g_base_units[c]=up;
    updatefundunitdim = -1;
    for (c = 0;updatefundunitdim<0 && c<NUM_DIMENS;c++) {
      if (Numerator(GetDimFraction(*(UnitsDimensions(up)),c)) != 0 ) {
        updatefundunitdim = c;
      }
    }
    gl_iterate( DUList, (void (*)(VOIDPTR))Unit_UpdateFundUnits );
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "u_change_baseunit called with unknown unit.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UnitSetUser(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  struct Units *up = NULL;
  struct DisplayUnit *du;
  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_set_user <unit>\n");
    Tcl_SetResult(interp, "u_set_user wants a units string arg", TCL_STATIC);
    return TCL_ERROR;
  }
  check_units_set(cdata,interp,argc,argv);
  check_DU_set();
  up = (struct Units *)LookupUnits(argv[1]);
  if (up ==NULL) {
    unsigned long pos;
    int ecode;
    up = (struct Units *)FindOrDefineUnits(argv[1],&pos,&ecode);
  }
  if (up!=NULL) {
    du = Unit_FindOrAddDU((dim_type *)UnitsDimensions(up));
    du->u = up;
    return TCL_OK;
  }
  Tcl_SetResult(interp, "u_set_user unable to parse the units given.",
                TCL_STATIC);
  return TCL_ERROR;
}

/*
 * return all real atoms and real constants which have the units
 * given.
 */
int Asc_UnitGetAtomsForUnit(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc, *rtdesc, *rcdesc;
  struct Units *up = NULL;
  struct DisplayUnit *du;
  unsigned long c,len;
  struct gl_list_t *dlist, *alist, *blist;
  dim_type *dim;
  unsigned long pos;
  int ecode;

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_get_atoms <unit>\n");
    Tcl_SetResult(interp, "u_get_atoms wants a units string arg", TCL_STATIC);
    return TCL_ERROR;
  }
  check_units_set(cdata,interp,argc,argv);
  check_DU_set();
  up = (struct Units *)LookupUnits(argv[1]);
  if (up == NULL) {
    up = (struct Units *)FindOrDefineUnits(argv[1],&pos,&ecode);
  }
  if (up != NULL) {
    du = Unit_FindOrAddDU((dim_type *)UnitsDimensions(up));
    du->u = up;
  } else {
    Tcl_SetResult(interp, "u_get_atoms unable to parse the units given.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rtdesc = UnitsFindType("real");
  rcdesc = UnitsFindType("real_constant");
  assert(rtdesc);
  assert(rcdesc);
  dlist = DefinitionList();
  if (!dlist) {
    Tcl_SetResult(interp, "u_get_atoms found no type definitions.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  len = gl_length(dlist);
  alist = gl_create(len);
  blist = gl_create(len);
  for(c = 1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(dlist,c);
    if (desc != NULL) {/* MoreRefined doesn't take kindly to null */
      if (MoreRefined(desc,rtdesc)!= NULL) {
        dim = (dim_type *)GetRealDimens(desc);
        if (CmpDimen(dim,du->d) == 0 && !IsWild(dim)) {
          gl_insert_sorted(alist,(VOIDPTR)desc,(CmpFunc)Unit_CmpAtomName);
        }
      } else {
        if (MoreRefined(desc,rcdesc) != NULL) {
          dim = (dim_type *)GetConstantDimens(desc);
          if (dim != NULL && CmpDimen(dim,du->d) == 0 && !IsWild(dim)) {
            gl_insert_sorted(blist,(VOIDPTR)desc,(CmpFunc)Unit_CmpAtomName);
          }
        }
      }
    }
  }
  gl_destroy(dlist);

  len = gl_length(alist);
  for(c = 1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(alist,c);
    Tcl_AppendElement(interp,(char *)SCP(GetName(desc)));
  }
  gl_destroy(alist);

  len = gl_length(blist);
  for(c = 1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(blist,c);
    Tcl_AppendElement(interp,(char *)SCP(GetName(desc)));
  }
  gl_destroy(blist);
  return TCL_OK;
}


int Asc_UnitGetPrec(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(stderr,"call is: u_getprec <no args>\n");
    Tcl_SetResult(interp, "u_getprec expects no arguments.", TCL_STATIC);
    return TCL_ERROR;
  }
  sprintf(buf,"%d",UPREC);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

int Asc_UnitGetCPrec()
{
  return UPREC;
}

int Asc_UnitSetPrec(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  int status,tmpi;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_setprec <number>\n");
    Tcl_SetResult(interp, "u_setprec expects a number 4 to 16.", TCL_STATIC);
    return TCL_ERROR;
  }
  tmpi = 100;
  status = Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<4 || tmpi>16) {
    status = TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(stderr,"u_setprec: Precision must be in range 4 - 16");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_setprec: invalid precision specified.",
                  TCL_STATIC);
    return status;
  }
  UPREC = tmpi;
  return TCL_OK;
}

int Asc_UnitGetUnits(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_get_units <atom_typename> \n");
    Tcl_SetResult(interp, "u_get_units: expects atom type.", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = UnitsFindType(argv[1]);
  if (desc!=NULL && GetBaseType(desc)==real_type) {
    dim_type *dim = (dim_type *)GetRealDimens(desc);
    struct DisplayUnit *du;
    assert(dim!=NULL);
    if (IsWild(dim) || CmpDimen(dim,Dimensionless())==0 ) {
      return TCL_OK;
    }
    du = Unit_FindOrAddDU(dim);
    if (du->u!=NULL) {
      Tcl_AppendResult(interp,UnitsDescription(du->u),SNULL);
      return TCL_OK;
    }
    if (du->fu!=NULL) {
      Tcl_AppendResult(interp,UnitsDescription(du->fu),SNULL);
      return TCL_OK;
    } else {
      struct Units *u = Unit_DisplayFund(dim);
      if (!u) {
        Tcl_SetResult(interp,
                      "u_get_units: unable to define fundamental units!",
                      TCL_STATIC);
        return TCL_ERROR;
      } else {
        Tcl_AppendResult(interp,UnitsDescription(u),SNULL);
        return TCL_OK;
      }
    }
  } else {
    Tcl_SetResult(interp, "u_get_units called with bad real atom name",
                  TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UnitGetUser(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_get_user <atom_typename> \n");
    Tcl_SetResult(interp, "u_get_user: expects atom type.", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = UnitsFindType(argv[1]);
  if (desc!=NULL && GetBaseType(desc)==real_type) {
    dim_type *dim = (dim_type *)GetRealDimens(desc);
    struct Units *u;
    assert(dim!=NULL);
    if (IsWild(dim) || CmpDimen(dim,Dimensionless())==0 ) {
      return TCL_OK;
    }
    u = Unit_DisplayUnits(dim);
    if (u!=NULL) {
      Tcl_AppendResult(interp,UnitsDescription(u),SNULL);
    } else {
      Tcl_SetResult(interp, "default", TCL_STATIC);
    }
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "u_get_user called with bad real atom name",
                  TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UnitGetList(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(stderr,"call is: u_get_list <no args> \n");
    Tcl_SetResult(interp, "u_get_list: expects no arguments.", TCL_STATIC);
    return TCL_ERROR;
  }
  check_DU_set();
  unitsinterp = interp;
  gl_iterate(DUList,(void (*)(VOIDPTR))Unit_GetUserSet);
  return TCL_OK;
}
int Asc_UnitClearUser(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    FPRINTF(stderr,"call is: u_clear_user <atom_typename> \n");
    Tcl_SetResult(interp, "u_clear_user: expects atom type.", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = UnitsFindType(argv[1]);
  if (desc!=NULL && GetBaseType(desc)==real_type) {
    dim_type *dim = (dim_type *)GetRealDimens(desc);
    struct DisplayUnit *du;
    assert(dim!=NULL);
    if (IsWild(dim) || CmpDimen(dim,Dimensionless())==0 ) {
      return TCL_OK;
    }
    du = Unit_FindOrAddDU(dim);
    du->u = (struct Units *)NULL;
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "u_clear_user called with bad real atom name",
                  TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UnitGetVal(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  struct Instance *i;
  int status;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "u_getval expected <qlfdid>", TCL_STATIC);
    return TCL_ERROR;
  }
  status = Asc_QlfdidSearch3(argv[1],0);
  if (status ==0) {
    i = g_search_inst;
  } else {
    Tcl_AppendResult(interp,"u_getval: QlfdidSearchCmd error",
                     argv[1], " not found.",SNULL);
    return TCL_ERROR;
  }
  if (IsDimInstance(i)) {
    Tcl_AppendElement(interp,Asc_UnitValue(i));
  } else {
    Tcl_SetResult(interp, "u_getval called on undimensioned object.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_UnitBrowGetVal(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *i;
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  ASCUSE;

  if ( argc > 2 ) {
    Tcl_SetResult(interp, "u_browgetval [search]", TCL_STATIC);
    return TCL_ERROR;
  }
  if (argc==2) {
    if (strncmp(argv[1],"search",3)!=0) {
      Tcl_AppendResult(interp, "Error: ",argv[0]," incorrect argument",
                       argv[1],(char *) NULL);
      return TCL_ERROR;
    }
    i = g_search_inst;
  } else {
    i = g_curinst;
  }
  if (IsDimInstance(i)) {
    Tcl_AppendElement(interp,Asc_UnitValue(i));
  } else {
    Tcl_SetResult(interp, "u_browgetval called on undimensioned object.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_UnitSlvGetRelVal(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct rel_relation **rp;
  int32 maxrel,relnum;
  int status = TCL_OK;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendElement(interp,"u_slvgetrelval expects solver relation index.");
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(stderr,"u_slvgetrelval called with NULL pointer\n");
    Tcl_AppendElement(interp,"u_slvgetrelval called without slv_system");
    return TCL_ERROR;
  }
  rp = slv_get_solvers_rel_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(stderr,  "NULL relation list found in u_slvgetrelval\n");
    Tcl_AppendElement(interp,"u_slvgetrelval called with null rellist");
    return TCL_ERROR;
  }
  maxrel = (int32)slv_get_num_solvers_rels(g_solvsys_cur);
  status = Tcl_GetInt(interp,argv[1],&relnum);
  if (relnum>=maxrel||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_slvgetrelval: equation requested does not exist",
                  TCL_STATIC);
    FPRINTF(stderr,"u_slvgetrelval: relation index invalid.\n");
    return TCL_ERROR;
  }
  if ( IsDimInstance( rel_instance(rp[relnum]) ) ) {
    Tcl_AppendResult(interp,Asc_UnitValue(rel_instance(rp[relnum])),SNULL);
  } else {
    Tcl_SetResult(interp, "u_slvgetrelval called on wierd object.",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_UnitSlvGetVarVal(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct var_variable **vp;
  int32 maxvar,varnum;
  int status = TCL_OK;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendElement(interp,"u_slvgetvarval expects solver variable index.");
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(stderr,"u_slvgetvarval called with NULL pointer\n");
    Tcl_AppendElement(interp,"u_slvgetvarval called without slv_system");
    return TCL_ERROR;
  }
  vp = slv_get_solvers_var_list(g_solvsys_cur);
  if (!vp) {
    FPRINTF(stderr,  "NULL variable list found in u_slvgetvarval\n");
    Tcl_AppendElement(interp,"u_slvgetvarval called with null varlist");
    return TCL_ERROR;
  }
  maxvar = (int32)slv_get_num_solvers_vars(g_solvsys_cur);
  status = Tcl_GetInt(interp,argv[1],&varnum);
  if (varnum>=maxvar||status ==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_slvgetvarval: variable requested does not exist",
                  TCL_STATIC);
    FPRINTF(stderr,"u_slvgetvarval: variable index invalid.\n");
    return TCL_ERROR;
  }
  if (IsDimInstance(var_instance(vp[varnum]))) {
    Tcl_AppendResult(interp,Asc_UnitValue(var_instance(vp[varnum])),SNULL);
  } else {
    Tcl_SetResult(interp, "u_slvgetrelval called on wierd object.",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_UnitSlvGetObjVal(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct rel_relation **rp;
  int32 maxobj,objnum;
  int status = TCL_OK;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendElement(interp,"u_slvgetobjval expects solver objective index.");
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(stderr,"u_slvgetobjval called with NULL pointer\n");
    Tcl_AppendElement(interp,"u_slvgetobjval called without slv_system");
    return TCL_ERROR;
  }
  rp = slv_get_solvers_obj_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(stderr,  "NULL objective list found in u_slvgetobjval\n");
    Tcl_AppendElement(interp,"u_slvgetobjval called with null objlist");
    return TCL_ERROR;
  }
  maxobj = (int32)slv_get_num_solvers_objs(g_solvsys_cur);
  status = Tcl_GetInt(interp,argv[1],&objnum);
  if (objnum>=maxobj||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "u_slvgetobjval: objective requested does not exist",
                  TCL_STATIC);
    FPRINTF(stderr,"u_slvgetobjval: objective index invalid.\n");
    return TCL_ERROR;
  }
  if (  IsDimInstance( rel_instance(rp[objnum]) )  ) {
    Tcl_AppendResult(interp,Asc_UnitValue(rel_instance(rp[objnum])),SNULL);
  } else {
    Tcl_SetResult(interp, "u_slvgetobjval called on wierd object.",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;



/* OLD CODE HERE TO END
  if ( argc != 2 ) {
    Tcl_SetResult(interp, "u_slvgetobjval takes no args.", TCL_STATIC);
    return TCL_ERROR;
  } */
  /* write code here when exprdim available */
/*  return TCL_OK; */
}


#define LONGHELP(b,ms) ((b)?ms:"")
int Asc_UnitHelpList(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  boolean detail = 1;

  UNUSED_PARAMETER(cdata);

  if ( argc > 2 ) {
    FPRINTF(stderr,"call is: uhelp [s,l] \n");
    Tcl_SetResult(interp, "Too many args to uhelp. Want 0 or 1 args",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 2 ) {
    if (argv[1][0]=='s') {
      detail = 0;
    }
    if (argv[1][0]=='l') {
      detail = 1;
    }
    PRINTF("%-22s%s\n","u_destroy_list",
           LONGHELP(detail,"deallocate display units list"));
    PRINTF("%-22s%s\n","u_setSIdef",
           LONGHELP(detail,"set SI mks as display base units"));
    PRINTF("%-22s%s\n","u_getbasedef",
           LONGHELP(detail,"get current display base units list"));
    PRINTF("%-22s%s\n","u_dump",
           LONGHELP(detail,"dump all global units to out, err or list"));
    PRINTF("%-22s%s\n","u_dims",
           LONGHELP(detail,"dump all global dims to out, err or list"));
    PRINTF("%-22s%s\n","u_dim_setverify",
           LONGHELP(detail,"turn relation dim checking noise on or off"));

    PRINTF("%-22s%s\n","u_num2dim",
           LONGHELP(detail,"return simple dimension corresponding to num"));
    PRINTF("%-22s%s\n","u_dim2num",
           LONGHELP(detail,"return number of a simple dimension"));
    PRINTF("%-22s%s\n","u_frombasedim",
           LONGHELP(detail,"return all unit names matching base dimension"));
    PRINTF("%-22s%s\n","u_fromatomdim",
           LONGHELP(detail,"return unit names matching atom dimension set"));

    PRINTF("%-22s%s\n","u_getdimatoms",
           LONGHELP(detail,"get list of dimensioned atoms"));
    PRINTF("%-22s%s\n","u_get_atoms",
           LONGHELP(detail,"get list of atoms matching units given"));
    PRINTF("%-22s%s\n","u_change_baseunit",
           LONGHELP(detail,"change the display default unit for a dimension"));
    PRINTF("%-22s%s\n","u_getprec",
           LONGHELP(detail,"get current display value precision"));
    PRINTF("%-22s%s\n","u_setprec",
           LONGHELP(detail,"set new display value precision"));

    PRINTF("%-22s%s\n","u_get_units",
           LONGHELP(detail,"get display units of dimensioned atom type"));
    PRINTF("%-22s%s\n","u_set_user",
       LONGHELP(detail,"set user display units in string given"));
    PRINTF("%-22s%s\n","u_get_list",
       LONGHELP(detail,"get all user set display units"));
    PRINTF("%-22s%s\n","u_get_user",
       LONGHELP(detail,"get user set display units of dimensioned atom type"));
    PRINTF("%-22s%s\n","u_clear_user",
       LONGHELP(detail,"unset user set display units of dimensioned atom"));

    PRINTF("%-22s%s\n","u_getval",
           LONGHELP(detail,"get value and units of qlfdid"));
    PRINTF("%-22s%s\n","u_browgetval",
           LONGHELP(detail,"get value and units of g_curinst"));
    PRINTF("%-22s%s\n","u_slvgetrelval",
           LONGHELP(detail,"get value and units of indexed relation resid"));
    PRINTF("%-22s%s\n","u_slvgetvarval",
           LONGHELP(detail,"get value and units of indexed variable"));
    PRINTF("%-22s%s\n","u_slvgetobjval",
           LONGHELP(detail,"get value and units of objective"));

    PRINTF("%-22s%s\n","uhelp",
           LONGHELP(detail,"uhelp s(=names only) l(=this list)."));

    PRINTF("\n");
  }
  if ( argc == 1 ) {
    char * tmps;
    tmps = (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));

    sprintf(tmps,"u_destroy_list");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_setSIdef");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_getbasedef");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_dump");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_dims");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_dim_setverify");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"u_num2dim");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_dim2num");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_frombasedim");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_fromatomdim");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"u_getdimatoms");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_get_atoms");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_change_baseunit");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_getprec");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_setprec");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"u_get_units");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_set_user");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_get_user");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_get_list");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_clear_user");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"u_getval");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_browgetval");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"u_slvgetrelval");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_slvgetvarval");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"u_slvgetobjval");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"uhelp");
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}

