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
	Code to support dynamic and static loading of user packages.

	The default state is to have packages. As such it takes an explicit
	definition of NO_PACKAGES, if packages are not to be handled.
	An explicit definition of STATIC_PACKAGES or DYNAMIC_PACKAGES is also
	required.
*//*
	by Kirk Abbott
	Created: July 4, 1994
	Last in CVS: 1.14 ballan 1998/03/06 15:47:14
*/

#if !defined(DYNAMIC_PACKAGES) && !defined(STATIC_PACKAGES) && !defined(NO_PACKAGES)
# error "Package linking option not set!"
#endif

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
#include <compiler/importhandler.h>
#include <utilities/ascPanic.h>
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
#include "relation.h"
#include "safe.h"
#include "relation_util.h"
#include "extfunc.h"
#include <packages/sensitivity.h>
#include <packages/ascFreeAllVars.h>
#include "module.h"
#include "packages.h"

/*
	Initialise the slv data structures used when calling external fns
*/
void Init_Slv_Interp(struct Slv_Interp *slv_interp)
{
  if (slv_interp){
    slv_interp->nodestamp = 0;
    slv_interp->status = calc_all_ok;
    slv_interp->user_data = NULL;
    slv_interp->task = bb_none;
/*
    slv_interp->first_call = (unsigned)0; // gone away
    slv_interp->last_call = (unsigned)0; // gone away
    slv_interp->check_args = (unsigned)0; // gone away
    slv_interp->recalculate = (unsigned)0; // gone away
    slv_interp->func_eval = (unsigned)0; // gone away
    slv_interp->deriv_eval = (unsigned)0; // gone away
    slv_interp->single_step = (unsigned)0; // gone away
*/
  }
}

/*---------------------------------------------
  BUILT-IN PACKAGES...
*/

/**
	Load builtin packages, unless NO_PACKAGES.

	@return 0 if success, 1 if failure.
*/
static
int Builtins_Init(void)
{
  int result = 0;

#ifdef NO_PACKAGES
  ERROR_REPORTER_HERE(ASC_USER_WARNING,"Builtins_Init: DISABLED at compile-time");
#else
  /* ERROR_REPORTER_DEBUG("Loading function asc_free_all_variables\n"); */
  result = CreateUserFunctionMethod("asc_free_all_variables"
		,Asc_FreeAllVars
		,1 /* num of args */
		,"Unset 'fixed' flag of all items of type 'solver_var'" /* help */
		,NULL /* user_data */
  );
#endif
  return result;
}

/* return 0 on success */
int LoadArchiveLibrary(CONST char *partialpath, CONST char *initfunc){

#ifdef DYNAMIC_PACKAGES
	struct FilePath *fp1;
	int result;
	struct ImportHandler *handler=NULL;

	/** 
		@TODO
			* modify SearchArchiveLibraryPath to use the ImportHandler array
			  in each directory in the path.
			* when a file is found, return information about which ImportHandler
			  should be used to open it, then make the call.
	*/

	CONSOLE_DEBUG("Searching for external library '%s'",partialpath);

	importhandler_createlibrary();

	fp1 = importhandler_findinpath(
		partialpath, ASC_DEFAULTPATH, PATHENVIRONMENTVAR,&handler
	);
	if(fp1==NULL){
		CONSOLE_DEBUG("External library '%s' not found",partialpath);
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"External library '%s' not found.",partialpath);
		return 1; /* failure */
	}

	asc_assert(handler!=NULL);
	
	CONSOLE_DEBUG("About to import external library...");
	/* note the import handler will deal with all the initfunc execution, etc etc */
	result = (*(handler->importfn))(fp1,initfunc,partialpath);
	if(result){
		CONSOLE_DEBUG("Error %d when importing external library of type '%s'",result,handler->name);
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Error importing external library '%s'",partialpath);
		ospath_free(fp1);
		return 1;
	}

	ospath_free(fp1);
  	return 0;
#else

	DISUSED_PARAMETER(name); DISUSED_PARAMETER(initfunc);

# if defined(STATIC_PACKAGES)
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"LoadArchiveLibrary disabled: STATIC_PACKAGES, no need to load dynamically.\n");
	return 0;
# elif defined(NO_PACKAGES)
	ERROR_REPORTER_HERE(ASC_PROG_ERROR,"LoadArchiveLibrary disabled: NO_PACKAGES");
	return 1;
# else
#  error "Invalid package linking flags"
# endif

#endif
}

/*---------------------------------------------
  STATIC_PACKAGES code only...

  Declare the functions which we are expected to be able to call.
*/
#ifndef NO_PACKAGES
# ifdef STATIC_PACKAGES

#include <packages/kvalues.h>
#include <packages/bisect.h>
#include <packages/sensitivity.h>

# endif
#endif

#ifdef STATIC_PACKAGES
/**
	Load all statically-linked packages

	@return 0 on success, >0 if any CreateUserFunction calls failed.
*/
static int StaticPackages_Init(void){
  int result = 0;

  result += sensitivity_register();
  result += kvalues_register();

  return result;
}
#endif

/**
	This is a general purpose function that will load whatever user
	functions are required according to the compile-time settings.

	If NO_PACKAGES, nothing will be loaded. If DYNAMIC_PACKAGES, then
	just the builtin packages will be loaded. If STATIC_PACKAGES then
	builtin plus those called in 'StaticPackages_Init' will be loaded.
*/
void AddUserFunctions(void)
{
#ifdef NO_PACKAGES
# ifdef __GNUC__
#  warning "EXTERNAL PACKAGES ARE BEING DISABLED"
# endif
  ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"AddUserFunctions disabled at compile-time.");
#else

  /* Builtins are always statically linked */
  if (Builtins_Init()) {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Problem in Builtins_Init: Some user functions not created");
  }

# ifdef DYNAMIC_PACKAGES
  /* do nothing */

# elif defined(STATIC_PACKAGES)
#  ifdef __GNUC__
#   warning "STATIC PACKAGES"
#  endif

  /*The following need to be reimplemented but are basically useful as is. */
  if (StaticPackages_Init()) {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Problem in StaticPackages_Init(): Some user functions not created");
  }

# endif
#endif
}

/*---------------------------------------
	TESTING FUNCTIONS

	The following functions may be called someone desirous of testing
	an external relation provided as a package. They are here
	for convenience, and should be really in a separate file.
*/

/**
	Get the real values of each struct Instance pointed to in the gl_list
	'arglist' and put it into the 'inputs' array of doubles.

	For example, use this to evaluate the input arguments for a Black Box relation.
*/
static void LoadInputVector(struct gl_list_t *arglist,
			    double *inputs,
			    unsigned ninputs,
			    unsigned long n_input_args
){
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

	This seems to be duplicated over in rel.c as ExtRel_Evaluate_RHS.
*/
int CallBlackBox(struct Instance *inst,
		 CONST struct relation *rel)
{
  struct Instance *data;

  struct Slv_Interp slv_interp;
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
    Init_Slv_Interp(&slv_interp);
/*
    slv_interp.check_args = (unsigned)1;
    slv_interp.first_call = (unsigned)1;
    slv_interp.last_call = (unsigned)0;
*/
    slv_interp.nodestamp = ExternalCallNodeStamp(ext);
    n_input_args = NumberInputArgs(efunc);
    n_output_args = NumberOutputArgs(efunc);
    ninputs = CountNumberOfArgs(arglist,1,n_input_args);
    noutputs = CountNumberOfArgs(arglist,n_input_args + 1,
				 n_input_args+n_output_args);

    /* Create the work vectors. Load the input vector from the instance tree. */
    inputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
    outputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
    jacobian = ASC_NEW_ARRAY_CLEAR(double,ninputs*noutputs);
    LoadInputVector(arglist,inputs,ninputs,n_input_args);

    /*
     * Call the init function.
     */
    slv_interp.task = bb_first_call;
    nok = (*init_func)(&slv_interp,data,arglist);
    if (nok) goto error;
    /*
     * Call the evaluation function.
     */
    slv_interp.task = bb_func_eval;
    nok = (*eval_func)(&slv_interp,ninputs,noutputs,
		       inputs,outputs,jacobian);
    if (nok) goto error;
    /*
     * Call the derivative routine.
     */
    if (deriv_func) {
      slv_interp.task = bb_deriv_eval;
      nok = (*deriv_func)(&slv_interp,ninputs,noutputs,
			  inputs,outputs,jacobian);
      if (nok) goto error;
    }
    /*
     * Call the init function to shut down
     */
    if (final_func) {
/*
      slv_interp.first_call = (unsigned)0;
      slv_interp.last_call = (unsigned)1;
 */
      slv_interp.task = bb_last_call;
      nok = (*final_func)(&slv_interp,data,arglist);
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
  f = ASC_NEW_ARRAY_CLEAR(double,1 + 2*n);
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
