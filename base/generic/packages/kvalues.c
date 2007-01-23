/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Abbott.
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
	External distillation routines
*//*
	by Kirk Abbott
	Created: July 4, 1994
	Version: $Revision: 1.5 $
	Date last modified: $Date: 1997/07/18 12:20:07 $
*/

#include <utilities/ascConfig.h>

#include <compiler/packages.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <general/list.h>
#include <compiler/sets.h>
#include <compiler/atomvalue.h>
#include <compiler/parentchild.h>
#include <compiler/instquery.h>
#include <compiler/instance_name.h>
#include <compiler/symtab.h>
#include <compiler/extcall.h>
#include <math.h>
#include "kvalues.h"

#define KVALUES_DEBUG 1

#ifndef EXTERNAL_EPSILON
#define EXTERNAL_EPSILON 1.0e-12
#endif

#define N_INPUT_ARGS 3
#define N_OUTPUT_ARGS 1


struct Vpdata {
  char *component;
  double a, b, c;
};

static struct Vpdata VpTable[] = {
  { "benzene", 15.5381, 2032.73, -33.15 },
  { "chloro" , 15.8333, 2477.07, -39.94 },
  { "acetone", 15.8737, 2911.32, -56.51 },
  { "hexane" , 15.3866, 2697.55, -46.78 },
  { "",        0.0,     0.0    , 0.0 }
};

struct KVALUES_problem {
  unsigned long ncomps;
  char **components;
  int ninputs;
  int noutputs;
  double *x;
};

static struct KVALUES_problem *CreateProblem(void)
{
  struct KVALUES_problem *result;
  result = (struct KVALUES_problem *)malloc(sizeof(struct KVALUES_problem));
  result->ncomps = 0;
  result->components = NULL;
  result->ninputs = result->noutputs = 0;
  result->x = NULL;
  return result;
}

static int LookupData(struct Vpdata *vp)
{
  /* char *component; */
  unsigned long c=0L;

  while (strcmp(VpTable[c].component,"")!=0) {
    if (strcmp(VpTable[c].component,vp->component)==0) {
      vp->a = VpTable[c].a;
      vp->b = VpTable[c].b;
      vp->c = VpTable[c].c;
      return 0;
    }
    c++;
  }
  return 1;
}


/*
	The 2nd. argument is the array of mole fractions.
	the length of this list is then the number of components.
*/
static int GetNumberOfComponents(struct gl_list_t *arglist)
{
  int ncomps;
  struct gl_list_t *mole_fracs;
  mole_fracs = (struct gl_list_t *)gl_fetch(arglist,2L);
  if (mole_fracs) {
    ncomps = (int)gl_length(mole_fracs);
    return ncomps;
  }
  else{
    return 0; /* error */
  }
}

/**
	The following code takes a data instance and tries to decode it
	to determine the names of the components used in the model. It will
	then create an array of these names and attach it to the problem.
	The data is expected to be an instance of a model of the form:

	  MODEL kvalues_data;
	     ncomps IS_A integer;
	     ncomps := 3;
	     components[1..ncomps] IS_A symbol;

	     components[1] := 'a';
	     components[2] := 'b';
	     components[3] := 'c';
	  END kvalues_data;

*/
static int GetComponentData(
		struct Instance *data,
	    struct KVALUES_problem *problem
){
  struct InstanceName name;
  struct Instance *child;
  unsigned long pos,c;
  unsigned long nch;
  char *component;
  symchar *s_component = NULL;

  s_component = AddSymbol("components");
  SetInstanceNameStrPtr(name,s_component);
  SetInstanceNameType(name,StrName);

  if (!data) {
    FPRINTF(stderr,"Error: expecting a data instance to be provided\n");
    return 1;
  }
  pos = ChildSearch(data,&name);	/* find array head */
  if (!pos) {
    FPRINTF(stderr,"Error: cannot find instance \"components\"\n");
    return 1;
  }
  child = InstanceChild(data,pos);
  if (InstanceKind(child)!= ARRAY_INT_INST) {
    FPRINTF(stderr,"Error: expecting components to be an array\n");
    return 1;
  }
  nch = NumberChildren(child);
  if ((nch==0) || (nch!=problem->ncomps)) {
    FPRINTF(stderr,"Error: Inconsistent component definitions\n");
    return 1;
  }
  if (InstanceKind(InstanceChild(child,1))!=SYMBOL_ATOM_INST) {
    FPRINTF(stderr,"Error: Expecting an array of symbols\n");
    return 1;
  }

  problem->components = (char **)malloc((int)nch*sizeof(char *));
  for (c=1;c<=nch;c++) {
    component = (char *)GetSymbolAtomValue(InstanceChild(child,c));
    problem->components[c-1] = component; /* just peek at it; dont copy it */
  }
  return 0;
}


static int CheckArgsOK(struct BBoxInterp *interp,
		       struct Instance *data,
		       struct gl_list_t *arglist,
		       struct KVALUES_problem *problem
){
  unsigned long len,ninputs,noutputs;
  int n_components;
  int result;
  (void)interp;

  if (!arglist) {
    FPRINTF(stderr,"External function arguement list does not exist\n");
    return 1;
  }
  len = gl_length(arglist);
  if (!len) {
    FPRINTF(stderr,"No arguements to external function statement\n");
    return 1;
  }
  if ((len!=(N_INPUT_ARGS+N_OUTPUT_ARGS))) {
    FPRINTF(stderr,"Number of arguements does not match\n");
    FPRINTF(stderr,"external function prototype\n");
    return 1;
  }

  ninputs = CountNumberOfArgs(arglist,1,N_INPUT_ARGS);
  noutputs = CountNumberOfArgs(arglist,N_INPUT_ARGS+1,
			       N_INPUT_ARGS+N_OUTPUT_ARGS);
  n_components = GetNumberOfComponents(arglist);

  problem->ninputs = (int)ninputs;
  problem->noutputs = (int)noutputs;
  problem->ncomps = n_components;
  result = GetComponentData(data,problem);	/* get the component names */
  if (result)
    return 1;

  return 0;
}


static void kvalues_final( struct BBoxInterp *interp)
{
  struct KVALUES_problem *problem;
  if (interp && interp->user_data) {
    problem = (struct KVALUES_problem *)interp->user_data;
    if (problem->components) free((char *)problem->components);
    if (problem->x) free((char *)problem->x);
    if (problem->x) free((char *)problem);
  }
  interp->user_data = NULL;
}

/**
	This function is one of the registered functions. It operates in
	one modes first_call, and
	creates a KVALUES_problem and calls a number of routines to check
	the arguments (data and arglist) and to cache the information
	processed away in the KVALUES_problem structure. We then attach
	to the hook.

	In last_call mode, which should never be seen,
	we delegate to final where we cleanup the problem structure:
	the array of compononent string pointers (we dont own the strings).
	the array of doubles.
	the problem structure itself.
*/
int kvalues_preslv(struct BBoxInterp *interp,
		   struct Instance *data,
		   struct gl_list_t *arglist

){
  struct KVALUES_problem *problem;
  int workspace;
  /*struct Instance *arg;
   int nok,c; */


  if (interp->task == bb_first_call) {
    if (interp->user_data!=NULL) { 	/* we have been called before */
      return 0;				/* the problem structure exists */
    }
    else{
      problem = CreateProblem();
      if(CheckArgsOK(interp,data,arglist,problem)) {
	return 1;
      }
      workspace = 	/* T, x[1..n], P, y[1..n], satp[1..n] */
	problem->ninputs + problem->noutputs +
	  problem->ncomps * 1;
      problem->x = (double *)calloc(workspace, sizeof(double));
      interp->user_data = (void *)problem;
      return 0;
    }
  } 
  if (interp->task == bb_last_call) {
	kvalues_final(interp);
  }
  return 0;
}


/*
	This function provides support to kvalues_fex which is one of the
	registered functions. The input variables are T,x[1..ncomps],P.
	The output variables are y[1..n]. We also load up the x vector
	(our copy) with satP[1..ncomps] as proof of concept that we can
	do (re)initialization.
*/

/*
	The name field will be field in vp before the call.
	The rest of the data will be filled the structure
	provided that there are no errors.
*/
static int GetCoefficients(struct Vpdata *vp){
  if (LookupData(vp)==0)
    return 0;
  else
    return 1; /* error in name lookup */
}


static int DoCalculation(struct BBoxInterp *interp,
			 int ninputs, int noutputs,
			 double *inputs,
			 double *outputs
){
  struct KVALUES_problem *problem;
  int c, offset;
  int ncomps;
  double TotP, T;
  double *liq_frac = NULL;
  double *vap_frac = NULL;
  double *SatP = NULL;
  double *tmp;
  struct Vpdata vp;
  int result = 0;

  problem = (struct KVALUES_problem *)interp->user_data;
  ncomps = problem->ncomps;
  assert(ninputs == ncomps+2);
  assert(noutputs == ncomps);
  liq_frac = (double *)calloc(ncomps,sizeof(double));
  vap_frac = (double *)calloc(ncomps,sizeof(double));
  SatP = (double *)calloc(ncomps,sizeof(double));

  T = inputs[0]; offset = 1;
  for(c=0;c<ncomps;c++) {
    liq_frac[c] = inputs[c+offset];
  }
  offset = ncomps+1;
  TotP = inputs[offset];

  for (c=0;c<ncomps;c++) {
    vp.component = problem->components[c];	/* get component name */
    result = GetCoefficients(&vp);		/* get antoines coeffs */
    SatP[c] = exp(vp.a - vp.b/(T + vp.c));	/* calc satP */
    vap_frac[c] = SatP[c] * liq_frac[c] / TotP;
  }

#ifdef KVALUES_DEBUG
  FPRINTF(stdout,"Temperature   T  = %12.8f\n",T);
  FPRINTF(stdout,"TotalPressure P  = %12.8f\n",TotP);
  for(c=0;c<ncomps;c++) {
    FPRINTF(stdout,"x[%d]  = %12.8g\n",c,liq_frac[c]);
  }
  for (c=0;c<ncomps;c++) {
    FPRINTF(stdout,"y[%d]  = %20.8g\n",c,vap_frac[c]);
  }
#endif /* KVALUES_DEBUG */

  /*
   * Save values, copy the information to the output vector
   * and cleanup.
   */
  tmp = problem->x;			/* save the input data */
  *tmp++ = T;
  for (c=0;c<ncomps;c++) {
    *tmp = liq_frac[c];
    tmp++;
  }
  *tmp++ = TotP;

  for (c=0;c<ncomps;c++) {		/* save the output data */
    *tmp = outputs[c] = vap_frac[c];	/* load up the output vector also */
    tmp++;
  }

  for (c=0;c<ncomps;c++) {		/* save the internal data */
    *tmp = SatP[c];
    tmp++;
  }

  free((char *)liq_frac);
  free((char *)vap_frac);
  free((char *)SatP);
  interp->status = calc_all_ok;
  return 0;
}

int kvalues_fex(struct BBoxInterp *interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
  int nok;
  (void)jacobian;
  nok = DoCalculation(interp, ninputs, noutputs, inputs, outputs);
  if (nok)
    return 1;
  else
    return 0;
}

/**
	Registration function
*/
int kvalues_register(void){
  int result;
  double epsilon = 1.0e-14;

  char kvalues_help[] = "This function does a bubble point calculation\
given the mole fractions in the feed, a T and P.";

  result = CreateUserFunctionBlackBox("bubblepnt", kvalues_preslv, kvalues_fex,
					NULL,NULL,kvalues_final,
					N_INPUT_ARGS,N_OUTPUT_ARGS,
					kvalues_help,epsilon);
  return  result;
}

#undef N_INPUT_ARGS
#undef N_OUTPUT_ARGS
