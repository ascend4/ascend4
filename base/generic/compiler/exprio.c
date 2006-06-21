/*
 *  Expression Output Routines
 *  by Tom Epperly
 *  Version: $Revision: 1.14 $
 *  Version control file: $RCSfile: exprio.c,v $
 *  Date last modified: $Date: 1998/04/10 23:25:42 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/dstring.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "func.h"
#include "instance_enum.h"
#include <general/list.h>
#include "extfunc.h"
#include "exprs.h"
#include "exprio.h"
#include "nameio.h"
#include "setio.h"

#ifndef lint
static CONST char ExprIOId[] = "$Id: exprio.c,v 1.14 1998/04/10 23:25:42 ballan Exp $";
#endif

CONST char *ExprEnumName(CONST enum Expr_enum t)
{
  switch (t) {
  case e_nop:
    return "e_nop";
  case e_undefined:
    return "e_undefined";
  case e_glassbox:
    return "e_glassbox";
  case e_blackbox:
    return "e_blackbox";
  case e_opcode:
    return "e_opcode";
  case e_token:
    return "e_token";
  case e_zero:
    return "e_zero";
  case e_real:
    return "e_real";
  case e_int:
    return "e_int";
  case e_var:
    return "e_var";
  case e_uminus:
    return "e_uminus";
  case e_func:
    return "e_func";
  case e_plus:
    return "e_plus";
  case e_minus:
    return "e_minus";
  case e_times:
    return "e_times";
  case e_divide:
    return "e_divide";
  case e_power:
    return "e_power";
  case e_ipower:
    return "e_ipower";
  case e_notequal:
    return "e_notequal";
  case e_equal:
    return "e_equal";
  case e_less:
    return "e_less";
  case e_greater:
    return "e_greater";
  case e_lesseq:
    return "e_lesseq";
  case e_greatereq:
    return "e_greatereq";
  case e_maximize:
    return "e_maximize";
  case e_minimize:
    return "e_minimize";
  case e_boolean:
    return "e_boolean";
  case e_or:
    return "e_or";
  case e_and:
    return "e_and";
  case e_not:
    return "e_not";
  case e_subexpr:
    return "e_subexpr";
  case e_const:
    return "e_const";
  case e_par:
    return "e_par";
  case e_sum:
    return "e_sum";
  case e_prod:
    return "e_prod";
  case e_symbol:
    return "e_symbol";
  case e_set:
    return "e_set";
  case e_in:
    return "e_in";
  case e_st:
    return "e_st";
  case e_card:
    return "e_card";
  case e_choice:
    return "e_choice";
  case e_union:
    return "e_union";
  case e_inter:
    return "e_inter";
  case e_qstring:
    return "e_qstring";
  default:
    return "UNKNOWN_TYPE";
  }
}

void WriteExprNode(FILE *f, CONST struct Expr *e)
{
  AssertMemory(e);
  switch(ExprType(e)) {
  case e_uminus:
    PUTC('-',f);
    break;
  case e_plus:
    PUTC('+',f);
    break;
  case e_minus:
    PUTC('-',f);
    break;
  case e_times:
    PUTC('*',f);
    break;
  case e_divide:
    PUTC('/',f);
    break;
  case e_power:
    PUTC('^',f);
    break;
  case e_boolean:
    switch (ExprBValue(e)) {
    case 0:
      FPRINTF(f,"FALSE");
      break;
    case 2:
      FPRINTF(f,"ANY");
      break;
    default:
      FPRINTF(f,"TRUE");
      break;
    }
    break;
  case e_and:
    FPRINTF(f,"AND");
    break;
  case e_or:
    FPRINTF(f,"OR");
    break;
  case e_not:
    FPRINTF(f,"NOT");
    break;
  case e_equal:
    PUTC('=',f);
    break;
  case e_notequal:
    FPRINTF(f,"<>");
    break;
  case e_boolean_eq:
    FPRINTF(f,"==");
    break;
  case e_boolean_neq:
    FPRINTF(f,"!=");
    break;
  case e_less:
    PUTC('<',f);
    break;
  case e_greater:
    PUTC('>',f);
    break;
  case e_lesseq:
    FPRINTF(f,"<=");
    break;
  case e_greatereq:
    FPRINTF(f,">=");
    break;
  case e_st:
    PUTC('|',f);
    break;
  case e_in:
    FPRINTF(f,"IN");
    break;
  case e_var:
    WriteName(f,ExprName(e));
    break;
  case e_int:
    FPRINTF(f,"%ld",ExprIValue(e));
    break;
  case e_zero:
    FPRINTF(f,"0");
    break;
  case e_real:
    FPRINTF(f,"%g",ExprRValue(e));
    break;
  case e_set:
    PUTC('[',f);
    WriteSet(f,ExprSValue(e));
    PUTC(']',f);
    break;
  case e_symbol:
    FPRINTF(f,"'%s'",SCP(ExprSymValue(e)));
    break;
  case e_qstring:
    FPRINTF(f,"\"%s\"",ExprQStrValue(e));
    break;
  case e_card:
    FPRINTF(f,"CARD[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_choice:
    FPRINTF(f,"CHOICE[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_sum:
    FPRINTF(f,"SUM[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_prod:
    FPRINTF(f,"PROD[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_union:
    FPRINTF(f,"UNION[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_inter:
    FPRINTF(f,"INTERSECTION[");
    WriteSet(f,ExprBuiltinSet(e));
    PUTC(']',f);
    break;
  case e_minimize:
    FPRINTF(f,"MINIMIZE");
    break;
  case e_maximize:
    FPRINTF(f,"MAXIMIZE");
    break;
  case e_func:
    FPRINTF(f,FuncName(ExprFunc(e)));
    break;
  case e_satisfied:
    FPRINTF(f,"SATISFIED(");
    WriteName(f,SatisfiedExprName(e));
    if (SatisfiedExprRValue(e)!=DBL_MAX){
      FPRINTF(f,",");
      FPRINTF(f,"%g",SatisfiedExprRValue(e));
    }
    PUTC(')',f);
    break;
  default:
    FPRINTF(f,"<term>");
    break;
  }
}


void WriteExpr(FILE *f, CONST struct Expr *e)
{
  while (e!=NULL) {
    AssertMemory(e);
    WriteExprNode(f,e);
    e = NextExpr(e);
    if (e!=NULL) {
      PUTC(' ',f);
    }
  }
  return;
}

/*
 * A ridiculous piece of code to convert an expr back into
 * a string. This version is in postfix, as it was read in.
 * The temporary buffer is used to collect known small strings
 * which then get appended to the dynamic string. These cases
 * break rather than return. The longer strings get appended
 * immeadiately, and return.
 */
void WriteExprNode2Str(Asc_DString *dstring, CONST struct Expr *e)
{
  char tmp[64];

  AssertMemory(e);
  switch(ExprType(e)) {
  case e_uminus:
    strcpy(tmp,"-");
    break;
  case e_plus:
    strcpy(tmp,"+");
    break;
  case e_minus:
    strcpy(tmp,"-");
    break;
  case e_times:
    strcpy(tmp,"*");
    break;
  case e_divide:
    strcpy(tmp,"/");
    break;
  case e_power:
    strcpy(tmp,"^");
    break;
  case e_ipower:
    strcpy(tmp,"^");
    break;
  case e_boolean:
    ExprBValue(e) ? strcpy(tmp,"TRUE") : strcpy(tmp,"FALSE");
    break;
  case e_and:
    strcpy(tmp,"AND");
    break;
  case e_or:
    strcpy(tmp,"OR");
    break;
  case e_not:
    strcpy(tmp,"NOT");
    break;
  case e_equal:
    strcpy(tmp,"=");
    break;
  case e_notequal:
    strcpy(tmp,"<>");
    break;
  case e_less:
    strcpy(tmp,"<");
    break;
  case e_greater:
    strcpy(tmp,">");
    break;
  case e_lesseq:
    strcpy(tmp,"<=");
    break;
  case e_greatereq:
    strcpy(tmp,">=");
    break;
  case e_st:
    strcpy(tmp,"/");
    break;
  case e_in:
    strcpy(tmp,"IN");
    break;
  case e_var:
    WriteName2Str(dstring,ExprName(e));
    return;
  case e_int:
    sprintf(tmp,"%ld",ExprIValue(e));
    break;
  case e_zero:
    sprintf(tmp,"0");
    break;
  case e_real:
    sprintf(tmp,"%g",ExprRValue(e));
    break;
  case e_set:
    Asc_DStringAppend(dstring,"[",-1);
    WriteSet2Str(dstring,ExprSValue(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_symbol:
    sprintf(tmp,"'%s'",SCP(ExprSymValue(e)));
    break;
  case e_qstring:
    sprintf(tmp,"\"%s\"",ExprQStrValue(e));
    break;
  case e_card:
    Asc_DStringAppend(dstring,"CARD[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_choice:
    Asc_DStringAppend(dstring,"CHOICE[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_sum:
    Asc_DStringAppend(dstring,"SUM[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_prod:
    Asc_DStringAppend(dstring,"PROD[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_union:
    Asc_DStringAppend(dstring,"UNION[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_inter:
    Asc_DStringAppend(dstring,"INTERSECTION[",-1);
    WriteSet2Str(dstring,ExprBuiltinSet(e));
    Asc_DStringAppend(dstring,"]",-1);
    return;
  case e_minimize:
    strcpy(tmp,"MINIMIZE");
    break;
  case e_maximize:
    strcpy(tmp,"MAXIMIZE");
    break;
  case e_func:
    strcpy(tmp,FuncName(ExprFunc(e)));
    break;
  default:
    strcpy(tmp,"<term>");
    break;
  }
  Asc_DStringAppend(dstring,tmp,-1);
}

void WriteExpr2Str(Asc_DString *dstring, CONST struct Expr *e)
{
  while (e!=NULL) {
    AssertMemory(e);
    WriteExprNode2Str(dstring,e);
    e = NextExpr(e);
    if (e!=NULL) {
      Asc_DStringAppend(dstring," ",-1);
    }
  }
  return;
}









