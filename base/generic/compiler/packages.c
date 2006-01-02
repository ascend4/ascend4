/*
 *  User Packages
 *  by Kirk Abbott
 *  Created: July 4, 1994
 *  Version: $Revision: 1.14 $
 *  Version control file: $RCSfile: packages.c,v $
 *  Date last modified: $Date: 1998/03/06 15:47:14 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Abbott.
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
 *
 */

/**
	Code to support dynamic and static loading of user packages.

	The default state is to have packages. As such it takes an explicit
	definition of NO_PACKAGES, if packages are not to be handled.
	An explicit definition of STATIC_PACKAGES or DYNAMIC_PACKAGES is also
	required.
*/

#include <math.h>
#include <ctype.h>  /* was compiler/actype.h */
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/extfunc.h"
#include "compiler/extcall.h"
#include "compiler/mathinst.h"
#include "compiler/instance_enum.h"
#include "compiler/instquery.h"
#include "compiler/atomvalue.h"
#include "compiler/find.h"
#include "compiler/relation_type.h"
#include "compiler/relation.h"
#include "compiler/safe.h"
#include "compiler/relation_util.h"
#include "packages/sensitivity.h"
#include "packages/ascFreeAllVars.h"
#include "compiler/module.h"
#include "compiler/packages.h"

/*
	Initialise the slv data structures used when calling external fns
*/
void Init_Slv_Interp(struct Slv_Interp *slv_interp)
{
  if (slv_interp){
    slv_interp->nodestamp = 0;
    slv_interp->status = calc_all_ok;
    slv_interp->user_data = NULL;
    slv_interp->first_call = (unsigned)0;
    slv_interp->last_call = (unsigned)0;
    slv_interp->check_args = (unsigned)0;
    slv_interp->recalculate = (unsigned)0;
    slv_interp->func_eval = (unsigned)0;
    slv_interp->deriv_eval = (unsigned)0;
    slv_interp->single_step = (unsigned)0;
  }
}

/*
	@deprecated, @see packages.h
*/
symchar *MakeArchiveLibraryName(CONST char *prefix)
{
  char *buffer;
  int len;
  symchar *result;

  len = strlen(prefix);
  buffer = (char *)ascmalloc(len+40);

#if defined(sun) || defined(solaris)
  sprintf(buffer,"%s.so.1.0",prefix);
#elif defined(__hpux)
  sprintf(buffer,"%s.sl",prefix);
#elif defined(_SGI_SOURCE)
  sprintf(buffer,"%s.so",prefix);
#elif defined(linux)
  sprintf(buffer,"lib%s.so",prefix); /* changed from .o to .so -- JP */
#else
  sprintf(buffer,"%s.so.1.0",prefix);
#endif

  result = AddSymbol(buffer); /* the main symbol table */
  ascfree(buffer);
  return result;              /* owns the string */
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
  error_reporter(ASC_USER_WARNING,__FILE__,__LINE,"Builtins_Init: DISABLED at compile-time");
#else
  ERROR_REPORTER_DEBUG("Builtins_Init: Loading function asc_free_all_variables\n");
  result = CreateUserFunction("asc_free_all_variables"
				,(ExtEvalFunc *)NULL
			    ,(ExtEvalFunc **)Asc_FreeAllVars
			    ,(ExtEvalFunc **)NULL
			    ,(ExtEvalFunc **)NULL
			    ,1, 0, "Unset 'fixed' flag of all items of type 'solver_var'");
#endif
  return result;
}

/*---------------------------------------------
  DYNAMIC_PACKAGES code only...
*/
# ifdef DYNAMIC_PACKAGES
static char path_var[PATH_MAX];

/**
	Search the archive library path for a file matching the given
	(platform specific, with extension?) library filename.

	@return a pointer to a string space holding the full path
	name of the file to be opened. The returned pointer may be NULL

	@TODO won't work correctly on windows
	@deprecated { see packages.h }
*/
static
char *SearchArchiveLibraryPath(CONST char *name, char *dpath, char *envv)
{
  register char *path,*result;
  register CONST char *t;
  register unsigned length;
  register FILE *f;
  /* error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"Env var for user packages is '%s'\n",envv); */
  error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"Search path for user packages is '%s'\n",getenv(envv));
  if ((path=getenv(envv))==NULL)
    path=dpath;
  while(isspace(*path)) path++;
  while(*path!='\0'){
    if (*path==':') path++;
    else{
      length = 0;
      /* copy next directory into array */
      while((*path!=':')&&(*path!='\0')&&(!isspace(*path)))
        path_var[length++] = *(path++);
      if (path_var[length-1]!='/')
        path_var[length++]='/';
      /* copy file name into array */
      for(t=name;*t!='\0';)
        path_var[length++] = *(t++);
      path_var[length]='\0';
      /* error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"Searching for for '%s' in dir '%s'\n",name, path_var); */
      if ((f= fopen(path_var,"r"))!=NULL){
		result = path_var;
        fclose(f);
        return result;
      }
    }
    while(isspace(*path)) path++;
  }
  return NULL;
}
#endif /* DYNAMIC_PACKAGES */
/*
  END of DYNAMIC_PACKAGES-specific code
  ------------------------------------------*/

int LoadArchiveLibrary(CONST char *name, CONST char *initfunc)
{
#ifdef NO_PACKAGES
  /** avoid compiler warnings on params: */
  (void) name; (void) initfunc;

  error_reporter(ASC_PROG_ERROR,__FILE__,__LINE__,"LoadArchiveLibrary disabled: NO_PACKAGES");
  return 1;

#elif defined(DYNAMIC_PACKAGES)

  int result;
  char *default_path = ".";
  char *env = PATHENVIRONMENTVAR;
  char *full_file_name = NULL;
  extern int Asc_DynamicLoad(CONST char *,CONST char *);

  full_file_name = SearchArchiveLibraryPath(name,default_path,env);
  if (!full_file_name) {
    error_reporter(ASC_USER_ERROR,NULL,0,"The named library '%s' was not found in the search path",name);
    return 1;
  }
  result = Asc_DynamicLoad(full_file_name,initfunc);
  if (result) {
    return 1;
  }
  ERROR_REPORTER_DEBUG("Successfully ran '%s' from dynamic package '%s'\n",initfunc,name);
  return 0;

#elif defined(STATIC_PACKAGES)

  /* avoid compiler warnings on params: */
  (void) name; (void) initfunc;

  error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"LoadArchiveLibrary disabled: STATIC_PACKAGES, no need to load dynamically.");
  return 0;

#else /* unknown flags */

# error "Invalid package linking flags"
  (void) name; (void) initfunc;
  return 1;

#endif
}

/*---------------------------------------------
  STATIC_PACKAGES code only...

  Declare the functions which we are expected to be able to call.
*/
#ifndef NO_PACKAGES
# ifdef STATIC_PACKAGES

/* kvalues.c */
extern int kvalues_preslv(struct Slv_Interp *,struct Instance *, struct gl_list_t *);
extern int kvalues_fex(struct Slv_Interp *, int, int, double *, double *, double *);

/* bisect.c */
extern int do_set_values_eval(struct Slv_Interp *,struct Instance *, struct gl_list_t *);
extern int do_bisection_eval(struct Slv_Interp *,struct Instance *,struct gl_list_t *);

/* sensitivity.c */
extern int do_sensitivity_eval(struct Slv_Interp *,struct Instance *, struct gl_list_t *);

# endif
#endif

#ifdef STATIC_PACKAGES
/**
	Load all statically-linked packages

	@return 0 on success, >0 if any CreateUserFunction calls failed.
*/
static
int StaticPackages_Init(void)
{
  int result = 0;

  char sensitivity_help[] =
    "This function does sensitivity analysis dy/dx. It requires 4 args.\n"
    "The first arg is the name of a reference instance or SELF.\n"
    "The second arg is x, where x is an array of > solver_var\n."
    "The third arg y, where y is an array of > solver_var\n. "
    "The fourth arg is dy/dx which dy_dx[1..n_y][1..n_x].\n";

  result = CreateUserFunction("do_solve",
                              (ExtEvalFunc *)NULL,
			      (ExtEvalFunc **)do_solve_eval,
			      (ExtEvalFunc **)NULL,
			      (ExtEvalFunc **)NULL,
			      2,0,NULL);
  result += CreateUserFunction("do_finite_difference",
                               (ExtEvalFunc *)NULL,
			       (ExtEvalFunc **)do_finite_diff_eval,
			       (ExtEvalFunc **)NULL,
			       (ExtEvalFunc **)NULL,
			       4,0,NULL);
  result += CreateUserFunction("do_sensitivity",
			       (ExtEvalFunc *)NULL,
			       (ExtEvalFunc **)do_sensitivity_eval,
			       (ExtEvalFunc **)NULL,
			       (ExtEvalFunc **)NULL,
			       4,0,sensitivity_help);
  result += CreateUserFunction("do_sensitivity_all",
			       (ExtEvalFunc *)NULL,
			       (ExtEvalFunc **)do_sensitivity_eval_all,
			       (ExtEvalFunc **)NULL,
			       (ExtEvalFunc **)NULL,
			       4,0,"See do_sensitivity for details");

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
  error_reporter(ASC_PROG_NOTE,NULL,0,"AddUserFunctions disabled at compile-time.");
#else

  /* Builtins are always statically linked */
  if (Builtins_Init()) {
      error_reporter(ASC_PROG_WARNING,NULL,0,"Problem in Builtins_Init: Some user functions not created");
  }

# ifdef DYNAMIC_PACKAGES
  /* do nothing */

# elif defined(STATIC_PACKAGES)
#  ifdef __GNUC__
#   warning "STATIC PACKAGES"
#  endif

  /*The following need to be reimplemented but are basically useful as is. */
  if (StaticPackages_Init()) {
      error_reporter(ASC_PROG_WARNING,NULL,0,"Problem in StaticPackages_Init(): Some user functions not created");
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

  /* All these desperately need a typedef in a header someplace */
  int (*init_func) (struct Slv_Interp *,
                    struct Instance *,
                    struct gl_list_t *);

  int (*eval_func)(struct Slv_Interp *,
                   int /* n_inputs */,
                   int  /* n_outputs */,
                   double * /* inputs */,
                   double * /* outputs */,
                   double * /* jacobian */);

  int (*deriv_func)(struct Slv_Interp *,
                   int /* n_inputs */,
                   int /* n_outputs */,
                   double * /* inputs */,
                   double * /* outputs */,
                   double * /* jacobian */);

/*------------------------------
	After this point everything should be ok.
	<-- says who? when? -- JP
*/

  /* Visual C doesn't like this before the func ptr defs. */
  UNUSED_PARAMETER(inst);   

  ext = BlackBoxExtCall(rel);
  arglist = ExternalCallArgList(ext);
  data = ExternalCallDataInstance(ext);
  efunc = ExternalCallExtFunc(ext);
  init_func = GetInitFunc(efunc);
  eval_func = GetValueFunc(efunc);
  deriv_func = GetDerivFunc(efunc);

  if (init_func && eval_func) {

    /* set up the interpreter. */
    Init_Slv_Interp(&slv_interp);
    slv_interp.check_args = (unsigned)1;
    slv_interp.first_call = (unsigned)1;
    slv_interp.last_call = (unsigned)0;
    slv_interp.nodestamp = ExternalCallNodeStamp(ext);
    n_input_args = NumberInputArgs(efunc);
    n_output_args = NumberOutputArgs(efunc);
    ninputs = CountNumberOfArgs(arglist,1,n_input_args);
    noutputs = CountNumberOfArgs(arglist,n_input_args + 1,
				 n_input_args+n_output_args);

    /* Create the work vectors. Load the input vector from the instance tree. */
    inputs = (double *)asccalloc(ninputs,sizeof(double));
    outputs = (double *)asccalloc(ninputs,sizeof(double));
    jacobian = (double *)asccalloc(ninputs*noutputs,sizeof(double));
    LoadInputVector(arglist,inputs,ninputs,n_input_args);

    /*
     * Call the init function.
     */
    nok = (*init_func)(&slv_interp,data,arglist);
    if (nok) goto error;
    /*
     * Call the evaluation function.
     */
    nok = (*eval_func)(&slv_interp,ninputs,noutputs,
		       inputs,outputs,jacobian);
    if (nok) goto error;
    /*
     * Call the derivative routine.
     */
    if (deriv_func) {
      nok = (*deriv_func)(&slv_interp,ninputs,noutputs,
			  inputs,outputs,jacobian);
      if (nok) goto error;
    }
    /*
     * Call the init function to shut down
     */
    slv_interp.first_call = (unsigned)0;
    slv_interp.last_call = (unsigned)1;
    nok = (*init_func)(&slv_interp,data,arglist);
    if (nok) goto error;
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
