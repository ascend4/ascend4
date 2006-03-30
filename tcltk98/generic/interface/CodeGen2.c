/*
 *  CodeGen2.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: CodeGen2.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
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

#ifndef lint
static CONST char CodeGen2ID[] = "$Id: CodeGen2.c,v 1.22 2003/08/23 18:43:05 ballan Exp $";
#endif


/*
 *                         CodeGeneration 2 Routines
 *                         by Kirk Andre Abbott.
 *                         May 28, 1995.
 *                         Version: $Revision: 1.22 $
 *                         Date last modified: $Date: 2003/08/23 18:43:05 $
 *                         Copyright (C) 1995 Kirk Andre Abbott, CMU.
 */
#include <math.h>
#include <tcl.h>
#include <tk.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/table.h>
#include <general/dstring.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/exprsym.h>
#include <compiler/instance_io.h>
#include <compiler/instance_enum.h>
#include <compiler/relation_io.h>
#include <solver/system.h>
#include <solver/var.h>
#include "Qlfdid.h"
#include "UnitsProc.h"
#include "CodeGen.h"


#undef CG_OFFSET
#define CG_OFFSET 0

struct CGFormat ASCEND_Format = {
  CG_ascend,CG_squarebracket,CG_hat_power,CG_ascend,CG_ascend
};
struct CGFormat GAMS_Format = {
  CG_gams,CG_round,CG_dstar_power,CG_gams,CG_gams,
};
struct CGFormat Math_Format = {
  CG_math,CG_round,CG_func_power,CG_math,CG_math
};

/*
 * Remember to export these symbols from CodeGen.c
 */
extern struct gl_list_t *PreProcessVars(struct Instance *);


static int CmpTypes(CONST struct Instance *i1, CONST struct Instance *i2)
{
  return strcmp(InstanceType(i1),InstanceType(i2));
}

static int CG_Classify(struct Instance *inst)
{
  return 1;
}

static struct gl_list_t *CodeGen2_SortVariables(struct var_variable **vp,
                                                int num)
{
  struct Instance *var_inst;
  struct gl_list_t *variables;
  int c;

  variables = gl_create((unsigned long)num);
  for (c=0;c<num;c++) {
    var_inst = var_instance(vp[c]);
    gl_append_ptr(variables,(char *)var_inst);
  }
  /*
   * Sort the list by type.
   */
  gl_sort(variables,(CmpFunc)CmpTypes);
  return variables;
}

/*
 **********************************************************************
 * This code assumes that the base type that the solver
 * operates upon is a solver_var. This must be changed if/when
 * we switch over to solver_reals or some other solver entry
 * point.
 * REMEMBER TO CHECK THAT WE NEVER GET CALLED WITH A EMPTY
 * LIST AND TO CHECK SINGLE ELEMENT LISTS. !!
 *
 * REMEMBER TO CHECK CHECK CHECK.
 **********************************************************************
 */


static void CodeGen2_WriteImport(FILE *fp, char *file_prefix)
{
  FPRINTF(fp,"IMPORT %s_Init FROM lib%s;\n\n",file_prefix,file_prefix);
}

/*
 **********************************************************************
 * CodeGen_ParseTypes
 *
 * This function takes a string list -- a Tcl list -- of instance
 * names, and returns a gl_list_t of the correspoding instances.
 * If there are any errors, a NULL is returned. We will also return
 * the count of elements that tcl found.
 **********************************************************************
 */

#define TYPE_HASH_SIZE 31

struct Table *CodeGen2_ParseTypes(Tcl_Interp *interp, char *strlist,
                                  int *found_count)
{
  struct Table *table;
  struct TypeDescription *desc;
  struct Instance *i;
  char **argv;		/* the split list of strings */
  int len,c;
  int error = 0;

  if (Tcl_SplitList(interp,strlist,found_count,&argv) != TCL_OK) {
    return NULL;
  }
  if (*found_count==0) {	/* parsed ok, but was a zero length list */
    if (argv !=NULL) {
      Tcl_Free((char *)argv);
    }
    return NULL;
  }
  len = *found_count;
  table = CreateTable((unsigned long)TYPE_HASH_SIZE);
  /*
   * Search for each type in turn. If found in the main
   * table, then add to our local table.
   */
  for (c=0;c<len;c++) {
    desc = FindType(argv[c]);
    if (desc) {
      AddTableData(table,(void *)desc,argv[c]);
    } else {
      FPRINTF(stderr,"Error in finding type %s\n",argv[c]);
      error++;
      break;
    }
  }

  if (error) {
    DestroyTable(table,0);
    if (argv !=NULL) {
      Tcl_Free((char *)argv);
    }
    return NULL;
  } else {
    if (argv !=NULL) {
      Tcl_Free((char *)argv);
    }
    return table;
  }
}

static void CodeGen2_WriteAscendNames(FILE *fp, char *file_prefix,
                                      struct Instance *reference,
                                      struct CGVar *cgvarlist,
                                      int nvars,
                                      struct Table *table)
{
  struct Instance *inst;
  struct TypeDescription *desc;
  CONST char *name;
  int i;

  FPRINTF(fp,"MODEL  %s_names;\n\n",file_prefix);
  FPRINTF(fp,"(*\n");
  for (i=0;i<nvars;i++) {
    inst = Asc_CGVarInstance(&cgvarlist[i]);
    if (table) {
      name = InstanceType(inst);
      desc = (struct TypeDescription *)LookupTableData(table,name);
      if (desc) {
        FPRINTF(fp,"x[%d]\t= ",i);
        WriteInstanceName(fp,inst,reference);
        FPRINTF(fp,";\n");
      }
    } else {
      FPRINTF(fp,"x[%d]\t= ",i);
      WriteInstanceName(fp,inst,reference);
      FPRINTF(fp,";\n");
    }
  }
  FPRINTF(fp,"*)\n\n");
  FPRINTF(fp,"END %s_names;\n",file_prefix);
}


static void CodeGen2_WriteVarDecls(FILE *fp, char *file_prefix,
                                   struct Instance *reference,
                                   struct CGVar *cgvarlist,
                                   int nvars)
{
  CONST struct Instance *inst;
  struct CGVar *cgvar;
  char *type;
  int start = 0;
  int c, written=0;
  unsigned int filter,MASK;

  FPRINTF(fp,"MODEL %s__base REFINES %s;\n\n",file_prefix,file_prefix);
  FPRINTF(fp,"  all_vars, others, slv_var  IS_A    set OF integer;\n");
  FPRINTF(fp,"  all_vars	:= [%d..%d];\n",  start, nvars-1);
  FPRINTF(fp,"  others   	:= [\n\t");

  /*
   * Here we write out the set of variables that qualify as
   * 'others'. We deal with the first var as a special case,
   * so that we can do the commas correctly.
   */
  filter = cgvarlist[0].flags;
  if (filter & CG_SLV_REAL) {
    written = 0;
  } else {
    FPRINTF(fp,"%d",cgvarlist[0].cmplr_index);
  }

  for (c=1;c<nvars;c++) {
    filter = cgvarlist[c].flags;
    if (filter & CG_SLV_REAL) {
      continue;
    }
    FPRINTF(fp,",%d",cgvarlist[c].cmplr_index);
  }
  FPRINTF(fp,"];\n");

  FPRINTF(fp,"  slv_var		:= all_vars - others;\n");
  FPRINTF(fp,"  x_[all_vars]	IS_A	real;\n\n");

  /*
   * Now we write out *all variables present in the model and
   * ARE_THE_SAME with the x[i]'s. E.g.:
   *
   * tray[2].liquid['in'], x[452]	ARE_THE_SAME;
   */
  for (c=0;c<nvars;c++) {
    inst = cgvarlist[c].instance;
    FPRINTF(fp,"  x_[%d],\t",cgvarlist[c].cmplr_index);
    WriteInstanceName(fp,inst,reference);
    FPRINTF(fp,"\t ARE_THE_SAME;\n");
  }
  PUTC('\n',fp);
  FPRINTF(fp,"END %s__base;\n\n",file_prefix);
}


static void CodeGen2_WriteRelDecls(FILE *fp, char *file_prefix,
                                   struct rel_relation **rp, int nrels)
{
  struct Instance *relinst, *inst;
  enum Expr_enum type;
  CONST struct relation *reln;
  struct CGVar *cgvar;
  int n_varsinrel, penultimate, i;
  unsigned long j;
  int relndx;


  FPRINTF(fp,"MODEL %s_plus_relations REFINES %s__base;\n\n",
          file_prefix, file_prefix);

  for (i=0;i<nrels;i++) {
    relndx = rel_index(rp[i]);
    FPRINTF(fp,"  relation_%d: %s(x_[",  relndx, file_prefix);
    relinst = rel_instance(rp[i]);
    reln = GetInstanceRelation(relinst,&type);

    n_varsinrel = (int)NumberVariables(reln);
    penultimate = n_varsinrel - 1;
    if (n_varsinrel) {
      for (j=0;j<n_varsinrel;j++) {
        inst = RelationVariable(reln,j+1);
        cgvar = (struct CGVar *)GetInterfacePtr(inst);
        FPRINTF(fp,"%d",cgvar->cmplr_index);
        if (j!=penultimate) {
          PUTC(',',fp);
        }
      }
    }
    FPRINTF(fp,"] ; %d);\n", relndx);
  }

  PUTC('\n',fp);
  FPRINTF(fp,"END %s_plus_relations;\n\n", file_prefix);
}


/*
 **********************************************************************
 * Patches
 *
 * This is the beginning of some code to do writing out the ascend
 * models associated with glassboxes, properly.
 * By this we mean that sufficient information will be written out so
 * that the relations may be inserted properly at the scope that they
 * came from. We call this a PATCH file.
 *
 **********************************************************************
 */

static struct Instance *FindNonArrayParent(struct Instance *i)
{
  struct Instance *tmp = i;
  while ((tmp) && (InstanceKind(tmp)!=MODEL_INST)) {
    if (NumberParents(tmp)==0) {
      break;
    }
    tmp = InstanceParent(tmp,1L);
  }
  if (InstanceKind(tmp)==MODEL_INST) {
    return tmp;
  } else {
    return NULL;
  }
}

/*
 * We write out a patch definition of the form:
 *
 * 	PATCH new_patch FOR old_type;
 *
 *	   reln['a'] : new__(x['b'], y[1], ... ,a.z; 12) IN a.b.c;
 * 	   other[1] : new__(x['b'], a.b.z; 12);
 *
 *	END new_patch;
 *
 * The first relation includes a statement of the scope. The second
 * does not, indicating that the relation was defined at the level of
 * the definition model. We could be pendantic and write out SELF, but
 * this should do.
 */
static
void CodeGen2_WritePatch(FILE *fp, char *file_prefix,
                         struct Instance *reference,
                         struct rel_relation **rp,
                         int nrels)
{
  struct Instance *relinst, *inst;
  struct Instance *model;
  enum Expr_enum type;
  CONST struct relation *reln;
  int n_varsinrel,i;
  unsigned long j;
  int relndx, count = 0;	/* used for breaking lines */

  if (nrels==0) {
    return;
  }

  FPRINTF(fp,"PATCH %s_patch FOR %s;\n\n",
          file_prefix, InstanceType(reference));


  for (i=0;i<nrels;i++) {
    relndx = rel_index(rp[i]);
    relinst = rel_instance(rp[i]);
    model = FindNonArrayParent(relinst);
    if (!model) {
      FPRINTF(stderr,"Error in finding rel parent -- skipping reln %d",
              relndx);
      continue;
    }
    WriteInstanceName(fp,relinst,model); /* the relation name */
    fputs(" :\n",fp);
    count += FPRINTF(fp,"\t%s(",file_prefix);
    reln = GetInstanceRelation(relinst,&type);

    /*
     * Write out the glassbox relation, with its relation name
     * and variables in the current scope. We deal with the first
     * variable specially to get the commas correct.
     */
    n_varsinrel = (int)NumberVariables(reln);
    if (n_varsinrel) {
      inst = RelationVariable(reln,1);
      count += WriteInstanceName(fp,inst,model);
    }
    for (j=1;j<n_varsinrel;j++) {
      inst = RelationVariable(reln,j+1);
      count += fputs(" ,",fp);
      count += WriteInstanceName(fp,inst,model);
      if (count >= 60) {
        fputs("\t\n",fp);
        count = 0;
      }
    }
    /*
     * Write out the destination instance name.
     */
    if (model!=reference) {
      FPRINTF(fp," ; %d)\n\t  IN  ", relndx);
      WriteInstanceName(fp,model,reference);
    } else {
      FPRINTF(fp," ; %d)", relndx);
    }
    fputs(";\n\n",fp);
    count = 0;
  }

  PUTC('\n',fp);
  FPRINTF(fp,"END %s_patch;\n\n", file_prefix);
}



static void CodeGen2_WriteInits(FILE *fp, char *file_prefix,
                                struct CGVar *cgvarlist,
                                int nvars)
{
  struct Instance *inst;
  char *units = "?";
  double value;
  int start = 0, i;

  /*
   * Write Header.
   */
  FPRINTF(fp,"MODEL %s_plus_init REFINES %s_plus_relations;\n\n",
          file_prefix, file_prefix);
  FPRINTF(fp,"  METHODS\n\n");

  /*
   * Art's beloved clear procedure.
   */
  FPRINTF(fp,"  METHOD clear;\n");
  FPRINTF(fp,"    x_[slv_var].fixed := FALSE;\n");
  FPRINTF(fp,"  END clear;\n");

  /*
   * Specify -- write out the vars that were fixed at the time
   * of codegeneration.
   */
  FPRINTF(fp,"  METHOD specify;\n");
  for (i=0;i<nvars;i++) {
    if (Asc_CGVarFixed(&cgvarlist[i])) {
      assert(cgvarlist[i].flags & CG_SLV_REAL);
      FPRINTF(fp,"    x_[%d].fixed := TRUE;\n", cgvarlist[i].cmplr_index);
    }
  }
  FPRINTF(fp,"  END specify;\n");

  /*
   * Values -- write out the values for all variables, along
   * with the appropriate units. FIX units later. For the moment
   * just write out wild values.
   */
  FPRINTF(fp,"  METHOD values;\n");
  for (i=0;i<nvars;i++) {
    inst = Asc_CGVarInstance(&cgvarlist[i]);
    value = RealAtomValue(inst);
    FPRINTF(fp,"    x_[%d] := %12.8g {%s};\n",
            cgvarlist[i].cmplr_index, value, units);
  }
  FPRINTF(fp,"  END values;\n");


  /*
   * Write Footer.
   */
  PUTC('\n',fp);
  FPRINTF(fp,"END %s_plus_init;\n\n",
          file_prefix);
}



/*
 * NOTE: Once we call set up, we have taken control of the
 * interface_ptrs associated with the variables. * We *must* *not*
 * use any of the struct var_variable *routines, but should use our CGVar
 * routines instead. This is critical. Shutdown will yield the
 * interface_ptr back to the previous owner.
 */


void Asc_CodeGenWriteAscendFile(slv_system_t sys,
                             FILE *fp, char *file_prefix,
                             int gradients,
                             char *typelist)
{
  struct Instance *root, *inst;
  struct rel_relation **rp;
  struct CGVar *cgvarlist, *cgvar;
  struct gl_list_t *list = NULL;
  struct Table *table = NULL;
  int nvars, nsolvervars, nrels;
  int result, found;

  nsolvervars = slv_get_num_master_vars(sys);	/* to get an idea of how big */
  if (nsolvervars==0) {
    return;
  }
  nrels = slv_get_num_master_rels(sys);
  rp = slv_get_master_rel_list(sys);

  root = g_solvinst_cur;		/* see interface1.h */
  list = PreProcessVars(root);
  nvars = (int)gl_length(list);
  assert(nvars>=nsolvervars);

  cgvarlist = Asc_CodeGenSetUpVariables3(list);
  result = CodeGen_SetupCodeGen(sys, cgvarlist,nvars, NULL,0, NULL,0, NULL,0,
                                NULL,NULL);
  if (result) {
    goto error;
  }

  CodeGen2_WriteImport(fp, file_prefix);
  table = CodeGen2_ParseTypes(interp, typelist, &found);

  CodeGen2_WriteAscendNames(fp, file_prefix, root, cgvarlist, nvars,
                            table);
  CodeGen2_WritePatch(fp, file_prefix, root, rp, nrels);
  /*
   * FIX FIX -- peel these out into another function.
   * CodeGen2_WriteVarDecls(fp, file_prefix, root, cgvarlist, nvars);
   * CodeGen2_WriteRelDecls(fp, file_prefix, rp, nrels);
   * CodeGen2_WriteInits(fp, file_prefix, cgvarlist, nvars);
   */
 error:
  if (list) {
    gl_destroy(list);
  }
  if (table) {
    DestroyTable(table,0);
  }
  Asc_CodeGenShutDown();
}



/*
 *******************************************************************
 * These are the routines for doing code generation for the
 * MathematicaTM file format. It serves more for debugging purposes
 * than anything else. This way we can submit the generated functions
 * for evaluation by mathematica, and check them against what we
 * compute. More important though, is that we can get mathematica to
 * compute the gradients symbolically and numerically and back check
 * our gradients against them.
 * Although some of the functions are *very* similar to the c_format,
 * I have opted to duplicate the code, so as not to burden the
 * c_format which is expected to be used most.
 *******************************************************************
 */
int CodeGen__WriteMathFuncs(FILE *fp, struct rel_relation *rel)
{
  struct Instance *rel_inst, *var_inst;
  RelationINF *r;
  enum Expr_enum type;
  Term *lhs,*rhs;
  int nchars;

  rel_inst = rel_instance(rel);
  r = (RelationINF *)GetInstanceRelation(rel_inst,&type); /* rel structure */
  if (type!=e_token) {
    FPRINTF(stderr,"rel type not supported in CodeGen_WriteMathFuncs\n");
    return;
  }

  FPRINTF(fp,"f%d = ",rel_index(rel)); 		/* write label */
  if ((lhs = RelationINF_Lhs(r))!=NULL) {		/* get lhs */
    nchars = CodeGen_WriteSide(fp,lhs,r,&Math_Format,0);	/* write lhs */
  }
  if ((rhs = RelationINF_Rhs(r))!=NULL) {		/* get rhs */
    FPRINTF(fp," - (");
    nchars = CodeGen_WriteSide(fp,rhs,r,&Math_Format,nchars); /* write rhs */
    PUTC(')',fp);
  }
  FPRINTF(fp,"\n");
  return 0;
}


int CodeGen__WriteMathGrads(FILE *fp, struct rel_relation *rel,
                            int *ndx_offset)
{
  /*
   * !!! Remember to remove all assumptions about vars being synonmous
   * with struct var_variable *'s !!!
   */
  struct Instance *rel_inst, *var_inst;
  struct CGVar *cgvar;
  RelationINF *r, *derivative;
  enum Expr_enum type;
  Term *lhs,*rhs;
  int a_index;
  int nchars;
  unsigned long n_varsinrel,j;

  a_index = *ndx_offset;

  rel_inst = rel_instance(rel);
  r = (RelationINF *)GetInstanceRelation(rel_inst,&type);
  if (type!=e_token) {
    FPRINTF(stderr,"rel type not supported in CodeGen_WriteMathFuncs\n");
    return;
  }

  n_varsinrel = NumberVariables(r);
  for (j=1;j<=n_varsinrel;j++) {
    var_inst = RelationVariable(r,j);
    if (solver_var(var_inst)) {
      cgvar = Asc_CGInstanceVar(var_inst);
      FPRINTF(fp,"g%d = D[f%d,x%d];\n",		/* MathTM instr. */
              ++a_index, rel_index(rel),cgvar->index);
      derivative = RelDeriveSloppy(r,j,CG_Classify);
      FPRINTF(fp,"myg%d = ",a_index);
      if ((lhs = RelationINF_Lhs(derivative))!=NULL) { /* write lhs */
        nchars = CodeGen_WriteSide(fp,lhs,derivative,&Math_Format,0);
      }
      if ((rhs = RelationINF_Rhs(derivative))!=NULL) { /* write rhs */
        FPRINTF(fp," - (");
        nchars = CodeGen_WriteSide(fp,rhs,derivative,&Math_Format,nchars);
        PUTC(')',fp);
      }
      FPRINTF(fp,"\n");
      RelDestroySloppy(derivative);
    }
  }
  *ndx_offset = a_index;	/* set up for return */
  return 0;
}

int CodeGen__WriteGamsFuncs(FILE *fp, struct rel_relation *rel)
{
  struct Instance *rel_inst, *var_inst;
  RelationINF *r;
  enum Expr_enum type;
  Term *lhs,*rhs;
  int nchars;
  double rel_nom;

  rel_inst = rel_instance(rel);
  r = (RelationINF *)GetInstanceRelation(rel_inst,&type); /* rel structure */
  /* this assumes RelationINF and struct relation are the same */
  rel_nom = rel_nominal(rel);
  if (rel_nom < 0.00001) {
      rel_nom = 1.0;
  }
  if (type!=e_token) {
    FPRINTF(stderr,"rel type not supported in CodeGen_WriteGamsFuncs\n");
    return;
  }
  if ((lhs = RelationINF_Lhs(r))!=NULL) {		/* get lhs */
    FPRINTF(fp,"(");
    nchars = CodeGen_WriteSide(fp,lhs,r,&GAMS_Format,0);	/* write lhs */
    FPRINTF(fp,")/%.8g", rel_nom);
    if ((rhs = RelationINF_Rhs(r))==NULL) {
        return 0;     /* if relation is objective function, job is done here */
    }
  }
  switch(RelationRelop(r)) {
  case e_less:
  case e_lesseq:
    FPRINTF(fp," =l= ");
    break;
  case e_equal:
    FPRINTF(fp," =e= ");
    break;
  case e_greater:
  case e_greatereq:
    FPRINTF(fp," =g= ");
    break;
  default:
    FPRINTF(fp,"ERROR");
  }
  if ((rhs = RelationINF_Rhs(r))!=NULL) {		/* get rhs */
    FPRINTF(fp,"(");
    nchars = CodeGen_WriteSide(fp,rhs,r,&GAMS_Format,nchars); /* write rhs */
   FPRINTF(fp,")/%.8g", rel_nom);
  }
  FPRINTF(fp,";\n");
  return 0;
}


void CodeGen_WriteGamsFile(slv_system_t sys,
                           FILE *fp, char *file_prefix, int gradients)
{
  struct rel_relation **rp, **op;
  expr_t obj;
  struct CGVar *vp, var;
  struct Instance *instance;
  int num_rels, num_vars, num_objs;
  int i,offset;
  double value;
  int index;
  int binaries_present = 0;
  int result, count;
  real64 val_tmp;
  char *objs=NULL;
  char *lhs=NULL, *rhs=NULL;
  int nchars;
  RelationINF *r;
  struct TypeDescription *type;
  enum type_kind binary = boolean_type;
  FILE *fp2;
  char *filename2 = NULL;

  filename2 = (char *)ascmalloc((strlen(file_prefix)+6)*sizeof(char));
  sprintf(filename2,"%s.names",file_prefix);
  fp2 = fopen(filename2,"w");

  (void)CodeGen_SetupCodeGen(sys, NULL,0, NULL,0, NULL,0, NULL,0,
                               NULL,NULL);
  offset = CG_OFFSET-1;
  num_rels = g_cgdata.rels.num_rels;
  rp = g_cgdata.rels.rel_list;

  num_objs = g_cgdata.objs.num_objs;
  op = g_cgdata.objs.obj_list;

  num_vars = g_cgdata.vars.num_vars;
  vp = g_cgdata.vars.var_list;

  FPRINTF(fp,"$Title Ascend Generated GAMS Model");
  FPRINTF(fp,"$offsymlist\n");
  FPRINTF(fp,"$offsymxref\n");
  FPRINTF(fp,"option limrow = 0;\n");
  FPRINTF(fp,"option limcol = 0;\n");
  FPRINTF(fp,"$inlinecom /* */\n\n");

  FPRINTF(fp,"variables\n");

  for (i=0;i<num_vars;i++) {
    index = vp[i].index;
    instance = vp[i].instance;
    FPRINTF(fp,"   x%d\t/* ", index);
    WriteInstanceName(fp,instance,NULL);
    FPRINTF(fp," */\n");
  }
  for (i=0;i<num_objs;i++) {
    index = rel_index(op[i]);
    FPRINTF(fp,"   alpha%d\t/* objective variable */\n", index);
  }
  FPRINTF(fp,"   ;\n\n");
  for (i=0;i<num_vars;i++) {
    index = vp[i].index;
    instance = vp[i].instance;
    type = InstanceTypeDesc(instance);
    if (strcmp(type->name,"solver_binary") == 0) {
        FPRINTF(fp,"binary variable   x%d;\n", index);
        binaries_present = 1;
    }
    val_tmp = ((var_lower_bound(instance) < -1e08)
               ? -1e08
               : var_lower_bound(instance));
    FPRINTF(fp,"   x%d.lo = %16.8g;\n", index, val_tmp);

    val_tmp = ((var_upper_bound(instance) > 1e08)
               ? 1e08
               : var_upper_bound(instance));
    FPRINTF(fp,"   x%d.up = %16.8g;\n", index, val_tmp);

    val_tmp = (var_value(instance) > 1e09) ? 1e09 : var_value(instance);
    if (val_tmp < 0.00000001 && val_tmp > -0.00000001) {val_tmp = 0.0;}
    FPRINTF(fp,"   x%d.l = %16.8g;\n", index, val_tmp);

    if (var_fixed(instance)) {
      FPRINTF(fp,"   x%d.fx = %16.8g;\n", index,val_tmp);
    }
    if (fabs(val_tmp) > 0.01) {
      FPRINTF(fp,"   x%d.scale = %16.8g;\n", index, fabs(val_tmp));
    }
  }

  FPRINTF(fp,"equations \n");
  for (i=0;i<num_rels;i++) {
      index = rel_index(rp[i]);
      FPRINTF(fp,"     rel_%d\n",index);
  }
  for (i=0;i<num_objs;i++) {
      index = rel_index(op[i]);
      FPRINTF(fp,"     obj_%d\n",index);
  }
  FPRINTF(fp,";\n");
  for (i=0;i<num_rels;i++) {
      index = rel_index(rp[i]);         /* write label */
      FPRINTF(fp,"rel_%d..     ",index);
      CodeGen__WriteGamsFuncs(fp,rp[i]);
  }

  FPRINTF(fp,"\n");
  for (i=0;i<num_objs;i++) {
      index = rel_index(op[i]);          /* write label */
      if (rel_obj_negate_flag(op[i])) {  /* objective is a maximization */
          FPRINTF(fp,"obj_%d..     alpha%d =g= -(",index,index);
      } else {                           /* objective is a minimization */
          FPRINTF(fp,"obj_%d..     alpha%d =g= (",index,index);
      }
      CodeGen__WriteGamsFuncs(fp,op[i]);
      FPRINTF(fp,");\n");
  }

  FPRINTF(fp,"model test1 using /\n");
  for (i=0;i<num_rels;i++) {
      if (rel_included(rp[i]) && rel_active(rp[i])) {
          index = rel_index(rp[i]);         /* write label */
          FPRINTF(fp,"rel_%d\n",index);
      }
  }
  FPRINTF(fp,"obj_0\n");
  FPRINTF(fp,"          /;\n");
  if (!binaries_present) {
      FPRINTF(fp,"option nlp = conopt;\n");
      FPRINTF(fp,"test1.OPTFILE = 1;\n");
      FPRINTF(fp,"test1.SCALEOPT = 2;\n");
      FPRINTF(fp,"solve test1 using nlp minimizing  alpha0;\n");
  } else {
      FPRINTF(fp,"solve test1 using minlp minimizing  alpha0;\n");
  }
  FPRINTF(fp,"file out /%s.gms_val/;\n",file_prefix);
  FPRINTF(fp,"put out;\n");
  FPRINTF(fp,"put\n");
  for (i=0;i<num_vars;i++) {
      index = vp[i].index;
      instance = vp[i].instance;
      FPRINTF(fp2,"qassgn3 {");
      WriteInstanceName(fp2,instance,NULL);
      FPRINTF(fp2,"} \n");
      FPRINTF(fp,"x%d.l:26:18/\n", index);
  }
  FPRINTF(fp,"put //;\n");
  fclose(fp);
  fclose(fp2);
Asc_CodeGenShutDown();
}


void CodeGen_WriteMathFile(slv_system_t sys,
                           FILE *fp, char *file_prefix, int gradients)
{
  struct rel_relation **rp;
  struct CGVar *vp, var;
  struct Instance *instance;
  int num_rels, num_vars;
  int i,offset;
  double value;
  int index;
  int result;

  (void)CodeGen_SetupCodeGen(sys, NULL,0, NULL,0, NULL,0, NULL,0,
                               NULL,NULL);
  offset = CG_OFFSET-1;
  num_rels = g_cgdata.rels.num_rels;
  rp = g_cgdata.rels.rel_list;

  /*
   * Write out some header stuff.
   */
  FPRINTF(fp,"Clear[\"x*\",\"f*\",\"g*\",\"myg*\"]\n");
  FPRINTF(fp,"dummy >> result.out\n\n");

  /*
   * Write the functions, our gradients and some gradient
   * instructions.
   */
  if (gradients) {
    PrepareDerivatives(1,1,1000);
  }

  for (i=0;i<num_rels;i++) {
    CodeGen__WriteMathFuncs(fp,rp[i]);
    if (gradients) {
      CodeGen__WriteMathGrads(fp,rp[i],&offset);
    }
  }

  if (gradients) {
    ShutDownDerivatives();
  }

  /*
   * Write out the values of the variables.
   */
  num_vars = g_cgdata.vars.num_vars;
  vp = g_cgdata.vars.var_list;
  for (i=0;i<num_vars;i++) {
    index = vp[i].index;
    instance = vp[i].instance;
    value = var_value(instance);
    FPRINTF(fp,"x%d := %12.8f;\n",index, value);
  }

  /*
   * Finally write out the instructions for the calculated
   * values. The maximum number of incidences should now
   * be sitting in offset. (- 1)
   */
  for (i=0;i<offset;i++) {
    FPRINTF(fp,"g%d >>> result.out;\n",i);
    FPRINTF(fp,"myg%d >>> result.out;\n",i);
  }
  Asc_CodeGenShutDown();
}

static FILE *SetUpMainFilePtr(char *filename,
                              struct CGFormat *format)
{
  FILE *fp;
  switch(format->main_format) {
  case CG_gams:
    sprintf(filename,"%s.gms",filename);
    break;
  case CG_ascend:
    sprintf(filename,"%s.patch",filename);
    break;
  case CG_math:
    sprintf(filename,"%s.m",filename);
    break;
  case CG_linear:
    sprintf(filename,"%s.xsys",filename);
    break;
  }

  fp = fopen(filename,"w");
  return fp;
}
/* The following code provides another entry point
 * for generating gams code.  This allows gams users
 * to generate code only specifying the output filename.
 */

int Asc_CodeGenGamsCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  FILE *fp;
  int result = 0;
  char *filename = NULL;
  struct CGFormat format;
  slv_system_t sys = g_solvsys_cur;
  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __codegen_gams filename", (char *)NULL);
    return TCL_ERROR;
  }

  filename = (char *)ascmalloc((strlen(argv[1])+8)*sizeof(char));
  strcpy(filename,argv[1]);
  result = CodeGen_CheckSystem(interp,sys);
  if (result) {
    return TCL_ERROR;
  }

  format = GAMS_Format;

  /*
   * Set up the file pointer based on the format.
   */
  fp = SetUpMainFilePtr(filename,&format);
  if (!fp) {
    Tcl_SetResult(interp,
                  "__codegen_general file open failed. system not written.",
                  TCL_STATIC);
    result = TCL_ERROR;
    goto error;
  }

  /*
   * Generate the code. Each of these formats sets up whatever
   * support structures that are necessary. They will likewise
   * destroy these structures themselves.
   */

  CodeGen_WriteGamsFile(sys,fp,argv[1],0);

 error:
  if (filename) {
    ascfree(filename);
  }
  if (fp) {
    fclose(fp);
  }
  return (result!=0) ? TCL_ERROR : TCL_OK;
}


/*
 * This does some generic code generation schemes.
 * ?grad?nograd? refers to whether gradient code should be
 * generated.
 * ?format? at the moment is one of linear,ascend,math,gams.
 * type is a list of types which may be used to do some filtering
 * of the information that is written out.
 */
int Asc_CodeGenGeneralCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  FILE *fp;
  int do_gradients=0;
  int result = 0;
  char *filename = NULL;
  struct CGFormat format;
  slv_system_t sys = g_solvsys_cur;

  if ( argc != 5 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __codegen_general filename ?grad?nograd? ?format?",
                     "types", (char *)NULL);
    return TCL_ERROR;
  }

  filename = (char *)ascmalloc((strlen(argv[1])+16)*sizeof(char));
  strcpy(filename,argv[1]);
  result = CodeGen_CheckSystem(interp,sys);
  if (result) {
    return TCL_ERROR;
  }

  /*
   * Check gradient args.
   */
  if (strncmp(argv[2],"gradients",4)==0) {
    do_gradients = 1;
  }

  /*
   * Check format args; Default is Math_Format
   */
  if (strncmp(argv[3],"math",4)==0) {
    format = Math_Format;
  } else if (strncmp(argv[3],"gams",4)==0) {
    format = GAMS_Format;
  } else if (strncmp(argv[3],"ascend",4)==0) {
    format = ASCEND_Format;
  } else {
    format = Math_Format;
  }

  /*
   * Set up the file pointer based on the format.
   */
  fp = SetUpMainFilePtr(filename,&format);
  if (!fp) {
    Tcl_SetResult(interp,
                  "__codegen_general file open failed. system not written.",
                  TCL_STATIC);
    result = TCL_ERROR;
    goto error;
  }

  /*
   * Generate the code. Each of these formats sets up whatever
   * support structures that are necessary. They will likewise
   * destroy these structures themselves.
   */
  switch(format.main_format) {
  case CG_c:
    Tcl_SetResult(interp,
                  "wrong format: Use \"__codegen_c\" instead", TCL_STATIC);
    break;
  case CG_math:
    CodeGen_WriteMathFile(sys,fp,argv[1],do_gradients);
    break;
  case CG_ascend:
    Asc_CodeGenWriteAscendFile(sys,fp,argv[1],do_gradients,argv[4]);
    break;
  case CG_gams:
    CodeGen_WriteGamsFile(sys,fp,argv[1],do_gradients);
    break;
  case CG_linear:
    FPRINTF(stderr,"Code generation file formats not yet supported\n");
    break;
  default:
    FPRINTF(stderr,"Unknown code generation file format\n");
    break;
  }

 error:
  if (filename) {
    ascfree(filename);
  }
  if (fp) {
    fclose(fp);
  }
  return (result!=0) ? TCL_ERROR : TCL_OK;
}


/*
 * Some temporary stuff.
 */

struct TypeData {
  CONST char *type;
  int written;
};

static void Collect__Models(struct Instance *inst,
                          void *data)
{
  struct gl_list_t *list = (struct gl_list_t *)data;

  if (inst) {
    switch (InstanceKind(inst)) {
    case MODEL_INST:
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      gl_append_ptr(list,(char *)inst);
      break;
    default:
      break;
    }
  }
}

static struct gl_list_t *CollectModels(struct Instance *inst)
{
  struct gl_list_t *list;
  list = gl_create(40L);
  VisitInstanceTreeTwo(inst,Collect__Models,1,0,(void *)list);
  return list;
}

struct Table *MakeTypeTable(struct Instance *inst,
                            struct gl_list_t *list)
{
  CONST struct Instance *model;
  struct TypeDescription *type;
  struct Table *table;
  struct TypeData *tdata;
  CONST char *typename;
  unsigned long len, c;

  /*
   * Create a list and a table. Visit the tree *bottom up*
   * and collect all models and array instances.
   */
  table = CreateTable(31L);

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    model = (CONST struct Instance *)gl_fetch(list,c);
    type = InstanceTypeDesc(model);
    typename = InstanceType(model);
    if (!typename) {
      continue;
    }
    tdata = (struct TypeData *)LookupTableData(table,typename);
    if (tdata) {
      continue;
    } else {
      tdata = (struct TypeData *)ascmalloc(sizeof(struct TypeData));
      tdata->written = 0;
      tdata->type = typename;
      AddTableData(table,(void *)tdata,typename);
      FPRINTF(stderr,"Added a new type --> %s\n",typename);
    }
  }
  return table;
}

static void PrintTypes(void *dataptr, void *fileptr)
{
  FILE *fp = (FILE *)fileptr;
  struct TypeData *data = (struct TypeData *)dataptr;
  CONST char *name;
  int written;

  if (data && data->written==0) {
    FPRINTF(fp,"Type --> %s\n",data->type);
    data->written++;
  }
}

int Asc_CodeGenTypesCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  FILE *fp;
  struct CGFormat format;
  struct Table *table;
  struct gl_list_t *list;
  char *filename;
  int result = TCL_OK;

  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __codegen_types filename",
                     (char *)NULL);
    return TCL_ERROR;
  }

  format = ASCEND_Format;
  filename = (char *)ascmalloc((strlen(argv[1])+8)*sizeof(char));
  filename = strcpy(filename,argv[1]);
  fp = SetUpMainFilePtr(filename,&format);

  list = CollectModels(g_curinst);
  if (list) {
    table = MakeTypeTable(g_curinst,list);
    TableApplyAllTwo(table,PrintTypes,fp);
  } else {
    list = NULL;
    table = NULL;
  }

 error:
  if (fp) {
    fclose(fp);
  }
  if (list) {
    gl_destroy(list);
  }
  if (table) {
    DestroyTable(table,1);
  }
  return result;
}

/*
 * The above is some temporary stuff which must
 * be deleted.
 */


/*
 * The following read and write functions are a rather
 * speedy option to the read and write values functions
 * in the browser.  These functions read and write values
 * based on the variable index numbers in the solver.
 * This can be dangerous as NO checking is performed
 * (other than checking the number of vars and the
 * number of values to read are the same).
 * YOU are responsible for making sure you are reading
 * values into the correct instance!
*/
void CodeGen_Write_Values_Fast(slv_system_t sys,
                           FILE *fp, char *file_prefix,
                               char *output_type)
{
    struct CGVar *vp;
    struct Instance *instance;
    int i, num_vars, index;
    real64 (*proc)(struct var_variable *);
    boolean (*boolean_proc)(struct var_variable *);
    if (strncmp(output_type,"value",3)==0) {
        proc = var_value;
    } else if (strncmp(output_type,"nominal",3)==0) {
        proc = var_nominal;
    } else if (strncmp(output_type,"lower_bound",3)==0) {
        proc = var_lower_bound;
    } else if (strncmp(output_type,"upper_bound",3)==0) {
        proc = var_upper_bound;
    } else if (strncmp(output_type,"fixed",3)==0) {
        boolean_proc = var_fixed;
    } else {
        FPRINTF(stderr,"must specify output type to be value, "
                " nominal, lower_bound, upper_bound, or fixed\n");
        return;
    }
    CodeGen_SetupCodeGen(sys, NULL,0, NULL,0, NULL,0, NULL,0, NULL,NULL);
    num_vars = g_cgdata.vars.num_vars;
    vp = g_cgdata.vars.var_list;
    FPRINTF(fp,"%i\n",num_vars);
    if (strncmp(output_type,"fixed",3)==0) {
        for (i=0;i<num_vars;i++) {
            index = vp[i].index;
            instance = vp[i].instance;
            FPRINTF(fp,"%i\n",boolean_proc(instance));
        }
    } else {
        for (i=0;i<num_vars;i++) {
            index = vp[i].index;
            instance = vp[i].instance;
            FPRINTF(fp,"%16.8g\n",proc(instance));
        }
    }
    Asc_CodeGenShutDown();
}

void CodeGen_Read_Values_Fast(slv_system_t sys,
                           FILE *fp, char *file_prefix,
                              char *input_type)
{
    struct CGVar *vp;
    struct Instance *instance;
    int i, num_vars, index, check;
    real64 val_tmp;
    int val_tmp_int;
    boolean val_tmp_boolean;
    char *buffer;

    void (*proc)(struct var_variable *,real64);
    void (*boolean_proc)(struct var_variable *,boolean);
    if (strncmp(input_type,"value",3)==0) {
        proc = var_set_value;
    } else if (strncmp(input_type,"nominal",3)==0) {
        proc = var_set_nominal;
    } else if (strncmp(input_type,"lower_bound",3)==0) {
        proc = var_set_lower_bound;
    } else if (strncmp(input_type,"upper_bound",3)==0) {
        proc = var_set_upper_bound;
    } else if (strncmp(input_type,"fixed",3)==0) {
        boolean_proc = var_set_fixed;
    } else {
        FPRINTF(stderr,"must specify input type to be value,"
                " nominal, lower_bound, upper_bound, or fixed\n");
        return;
    }

    CodeGen_SetupCodeGen(sys, NULL,0, NULL,0, NULL,0, NULL,0,
                               NULL,NULL);

    num_vars = g_cgdata.vars.num_vars;
    vp = g_cgdata.vars.var_list;

    buffer = (char *)ascmalloc(16*sizeof(char));
    fgets(buffer,16,fp);
    check = atoi(buffer);
    if ( check == num_vars) {
        if (strncmp(input_type,"fixed",3)==0) {
            for (i=0;i<num_vars;i++) {
                index = vp[i].index;
                instance = vp[i].instance;
                fgets(buffer,10,fp);
                val_tmp_int = atoi(buffer);
                if (val_tmp_int) {
                    val_tmp_boolean = 1;
                } else {
                    val_tmp_boolean = 0;
                }
                boolean_proc(instance,val_tmp_boolean);
            }
        } else {
            for (i=0;i<num_vars;i++) {
                index = vp[i].index;
                instance = vp[i].instance;
                fgets(buffer,16,fp);
                val_tmp = atof(buffer);
                proc(instance,val_tmp);
            }
        }
    } else {
      FPRINTF(stderr,
              "Number of elements in input file and solver are not equal.\n");
      FPRINTF(stderr,"File not read.\n");
      FPRINTF(stderr,"file length = %i, num_vars = %i\n",check,num_vars);
    }

    Asc_CodeGenShutDown();
}

int Asc_CodeGenWriteCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  FILE *fp;
  int result = 0;
  char *filename = NULL;
  struct CGFormat format;
  slv_system_t sys = g_solvsys_cur;
  if ( argc != 3 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __codegen_write filename output_parameter",
                     (char *)NULL);
    return TCL_ERROR;
  }

  filename = (char *)ascmalloc((strlen(argv[1])+8)*sizeof(char));
  strcpy(filename,argv[1]);
  result = CodeGen_CheckSystem(interp,sys);
  if (result) {
    return TCL_ERROR;
  }

  /*
   * Set up the file pointer
   */

  if (strncmp(argv[2],"value",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"val");
  } else if (strncmp(argv[2],"nominal",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"nom");
  } else if (strncmp(argv[2],"lower_bound",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"low");
  } else if (strncmp(argv[2],"upper_bound",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"up");
  } else if (strncmp(argv[2],"fixed",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"fix");
  } else {
      Tcl_SetResult(interp,
                    "Must specify output type to be value,"
                    " nominal, lower_bound, upper_bound, or fixed."
                    TCL_VOLATILE);
      if (filename) {
        ascfree(filename);
      }
      return TCL_ERROR;
  }

  fp = fopen(filename,"w");
  if (!fp) {
    Tcl_SetResult(interp,
                  "__codegen_write file open failed. system not written.",
                  TCL_STATIC);
    result = TCL_ERROR;
    goto error;
  }

  CodeGen_Write_Values_Fast(sys,fp,argv[1],argv[2]);

 error:
  if (filename) {
    ascfree(filename);
  }
  if (fp) {
    fclose(fp);
  }
  return (result!=0) ? TCL_ERROR : TCL_OK;
}

int Asc_CodeGenReadCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  FILE *fp;
  int result = 0;
  char *filename = NULL;
  struct CGFormat format;
  slv_system_t sys = g_solvsys_cur;
  if ( argc != 3 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __codegen_read filename input_parameter",
                     (char *)NULL);
    return TCL_ERROR;
  }

  filename = (char *)ascmalloc((strlen(argv[1])+8)*sizeof(char));
  strcpy(filename,argv[1]);
  result = CodeGen_CheckSystem(interp,sys);
  if (result) {
    return TCL_ERROR;
  }

  /*
   * Set up the file pointer
   */

  if (strncmp(argv[2],"value",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"val");
  } else if (strncmp(argv[2],"nominal",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"nom");
  } else if (strncmp(argv[2],"lower_bound",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"low");
  } else if (strncmp(argv[2],"upper_bound",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"up");
  } else if (strncmp(argv[2],"fixed",3)==0) {
      sprintf(filename,"%s.fast%s",filename,"fix");
  } else {
      Tcl_SetResult(interp,
                    "Must specify input type to be value,"
                    " nominal, lower_bound, upper_bound, or fixed.",
                    TCL_VOLATILE);
      if (filename) {
        ascfree(filename);
      }
      return TCL_ERROR;
  }

  fp = fopen(filename,"r");
  if (!fp) {
    Tcl_SetResult(interp,
                  "__codegen_read file open failed. system not written.",
                  TCL_STATIC);
    result = TCL_ERROR;
    goto error;
  }

  CodeGen_Read_Values_Fast(sys,fp,argv[1],argv[2]);

 error:
  if (filename) {
    ascfree(filename);
  }
  if (fp) {
    fclose(fp);
  }
  return ((result!=0) ? TCL_ERROR : TCL_OK);
}

