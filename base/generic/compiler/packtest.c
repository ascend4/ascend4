/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	Code to support testing of certain function logic.
	As usual, the test code is itself incorrect.

	The only place this stuff is being called is from interface.c, which is out of date.
*/

#include <math.h>
#include <ctype.h>  /* was compiler/actype.h */

#include <utilities/ascConfig.h>
#include <utilities/config.h> /* NEW */

#ifndef ASC_DEFAULTPATH
# error "Where is ASC_DEFAULTPATH???"
#endif

#include <general/ospath.h>

#include "compiler.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include <general/list.h>
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "extcall.h"
#include "mathinst.h"
#include "instance_enum.h"
#include "instquery.h"
#include "atomvalue.h"
#include "find.h"
#include "relation_type.h"
#include "extfunc.h"
#include "rel_blackbox.h"
#include "vlist.h"
#include "relation.h"
#include "safe.h"
#include "relation_util.h"
#include "extfunc.h"
#include <packages/sensitivity.h>
#include <packages/ascFreeAllVars.h>
#include "module.h"
#include "packages.h"

int something_for_packtest_to_compile =1;

#ifdef TEST_RELOCATE
/*---------------------------------------
	TESTING FUNCTIONS

	The following functions may be called someone desirous of testing
	an external relation provided as a package. 
*/

/**
	What's this do? -- JP
*/
static void LoadInputVector(struct gl_list_t *arglist,
			    double *inputs,
			    unsigned ninputs,
			    unsigned long n_input_args)
{
  struct Instance *inst;
  struct gl_list_t *input_list;
  unsigned long c,len;

  input_list = LinearizeArgList(arglist,1,n_input_args);

  if(!input_list)return;

  len = gl_length(input_list);

  if(len!=ninputs)return; /* somehow we had inconsistent data */

  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(input_list,c);
    inputs[c-1] = RealAtomValue(inst);
  }
  gl_destroy(input_list);
}

/**
	What's a black box, and what's a glass box? -- JP
	See Abbott thesis. - baa
	This function is, of course, a mess.
*/
int CallBlackBox(struct Instance *inst,
		 CONST struct relation *rel)
{
  struct Instance *data;

  struct BBoxInterp interp;
  struct ExternalFunc *efunc;
  struct ExtCallNode *ext;
  struct gl_list_t *arglist;
  unsigned long n_input_args, n_output_args;
  int nok = 0;

  unsigned long ninputs, noutputs;
  double *inputs = NULL, *outputs = NULL;
  double *jacobian = NULL;

  ExtBBoxInitFunc *init_func;
  ExtBBoxInitFunc *final_func;
  ExtBBoxFunc *eval_func;
  ExtBBoxFunc *deriv_func;

  UNUSED_PARAMETER(inst);

  ext = BlackBoxExtCall(rel);
  arglist = ExternalCallArgList(ext);
  data = ExternalCallDataInstance(ext);
  efunc = ExternalCallExtFunc(ext);
  init_func = GetInitFunc(efunc);
  final_func = GetFinalFunc(efunc);
  eval_func = GetValueFunc(efunc);
  deriv_func = GetDerivFunc(efunc);

  if (init_func && eval_func) {

    /* set up the interpreter. */
    Init_BBoxInterp(&interp);
#if 0
    interp.nodestamp = ExternalCallNodeStamp(ext);
#endif
    n_input_args = NumberInputArgs(efunc);
    n_output_args = NumberOutputArgs(efunc);
    ninputs = CountNumberOfArgs(arglist,1,n_input_args);
    noutputs = CountNumberOfArgs(arglist,n_input_args + 1,
				 n_input_args+n_output_args);

    /* Create the work vectors. Load the input vector from the instance tree. */
    inputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
    outputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
    jacobian = (double *)asccalloc(ninputs*noutputs,sizeof(double));
    LoadInputVector(arglist,inputs,ninputs,n_input_args);

    /*
     * Call the init function.
     */
    interp.task = bb_first_call;
    nok = (*init_func)(&interp,data,arglist);
    if (nok) goto error;
    /*
     * Call the evaluation function.
     */
    interp.task = bb_func_eval;
    nok = (*eval_func)(&interp,ninputs,noutputs,
		       inputs,outputs,jacobian);
    if (nok) goto error;
    /*
     * Call the derivative routine.
     */
    if (deriv_func) {
      interp.task = bb_deriv_eval;
      nok = (*deriv_func)(&interp,ninputs,noutputs,
			  inputs,outputs,jacobian);
      if (nok) goto error;
    }
    /*
     * Call the init function to shut down
     */
    if (final_func) {
      interp.task = bb_last_call;
      nok = (*final_func)(&interp,data,arglist);
      if (nok) goto error;
    }
  }
  else{
    FPRINTF(ASCERR,"External function not loaded\n");
    return 1;
  }

 error:
  if (inputs) ascfree((char *)inputs);
  if (outputs) ascfree((char *)outputs);
  if (jacobian) ascfree((char *)outputs);
  if (nok)
    return 1;
  else
    return 0;
}

/**
	When glassbox are registered, they must register a pointer
	to their function jump table. In other words, they must
	register a pointer to an 'array of pointers to functions'.
	This typedef just makes life a little cleaner.

	<-- what typedef?? -- JP
*/
int CallGlassBox(struct Instance *relinst, CONST struct relation *rel)
{
  CONST struct gl_list_t *incidence;
  struct Instance *var;
  struct ExternalFunc *efunc;
  int index;
  long i;
  double *f, *x, *g;
  int m,mode,result;
  int n;

  ExtEvalFunc **evaltable, *eval_func;
  ExtEvalFunc **derivtable, *deriv_func;

  (void) relinst;
  incidence = RelationVarList(rel);
  if (!incidence) {
    FPRINTF(ASCERR,"Incidence list is empty -- nothing to evaluate\n");
    return 0;
  }
  index = GlassBoxRelIndex(rel);
  efunc = GlassBoxExtFunc(rel);
  evaltable = GetValueJumpTable(efunc);
  eval_func = evaltable[index];
  derivtable = GetDerivJumpTable(efunc);
  deriv_func = derivtable[index];

  m = 0;			  /* FIX not sure what this should be !!! */
  n = gl_length(incidence);
  f = (double *)asccalloc((1 + 2*n),sizeof(double));
  x = &f[1];
  g = &f[n+1];

  for (i=0;i<n;i++) {
    var = (struct Instance *)gl_fetch(incidence,i+1);
    x[i] = RealAtomValue(var);
  }
  result = (*eval_func)(&mode,&m,&n,x,NULL,f,g);
  result += (*deriv_func)(&mode,&m,&n,x,NULL,f,g);

  ascfree((char *)f);
  return result;
}

/**
	No idea what this does. It's referenced in 'interface.c' only, so it
	appears to be defunct -- JP
*/
int CallExternalProcs(struct Instance *inst)
{
  CONST struct relation *rel;
  enum Expr_enum reltype;

  if (inst==NULL){
    FPRINTF(ASCERR,"Instance does not exist for callprocs\n");
    return 1;
  }
  if (InstanceKind(inst)!=REL_INST){
    FPRINTF(ASCERR,"Instance is not a relation\n");
    return 1;
  }
  rel = GetInstanceRelation(inst,&reltype);
  if (!rel) {
    FPRINTF(ASCERR,"Relation structure is NULL\n");
    return 1;
  }
  switch (reltype) {
  case e_blackbox:
    return CallBlackBox(inst,rel);
  case e_glassbox:
    return CallGlassBox(inst,rel);
  default:
    FPRINTF(ASCERR,"Invalid relation type in CallExternalProc\n");
    return 1;
  }
}
#endif /* testreloc*/
