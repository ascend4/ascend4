#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <compiler/packages.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <general/list.h>
#include <compiler/sets.h>

#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/extcall.h>
#include "bisect.h"

static int CheckArgTypes(struct gl_list_t *branch){
  struct Instance *i;
  enum inst_t kind;
  unsigned long len,c;

  len = gl_length(branch);
  if (!len)
    return 1;
  for (c=1;c<=len;c++) {
    i = (struct Instance *)gl_fetch(branch,c);
    if (i) {
      kind = InstanceKind(i);
      if ((kind!=REAL_ATOM_INST)&&(kind!=REAL_INST)) {
	FPRINTF(stderr,"Incorrect types of arguements given\n");
	return 1;
      }
    } else {
      FPRINTF(stderr,"NULL arguements given\n");
      return 1;
    }
  }
  return 0; /* all should be ok */
}

static int CheckArgVector(struct gl_list_t *branch){
  if (!branch) {
    FPRINTF(stderr,"Empty arglists given\n");
    return 1;
  }
  if (CheckArgTypes(branch))
    return 1;
  return 0;
}

/**
	This function expects 3 arguements; the calling protocol
	from ASCEND it expects to be invoked as:

	set_values(x1:array of generic_reals,
	           x2:array of generic_reals,
	           m: generic_real);

	The dimension of both x1, and x2 are expected to be the same;

*/
static int CheckArgs_SetValues(struct gl_list_t *arglist){
  struct gl_list_t *branch;
  unsigned long len, dim1, dim2;
  char *error_msg = "all ok";

  if (!arglist) {
    error_msg = "No arguement list given";
    goto error;
  }
  len = gl_length(arglist);
  if (len!=3) {
    error_msg = "Incorrect # args given, 3 were expected";
    goto error;
  }

  branch = (struct gl_list_t *)gl_fetch(arglist,1);
  if (CheckArgVector(branch))
    return 1;
  dim1 = gl_length(branch);

  branch = (struct gl_list_t *)gl_fetch(arglist,2);
  if (CheckArgVector(branch))
    return 1;
  dim2 = gl_length(branch);

  if (dim1!=dim2) {
    error_msg = "Inconsistent dimensions for the input vectors";
    goto error;
  }

  /* check the multiplying term.
   */
  branch = (struct gl_list_t *)gl_fetch(arglist,3);
  if (!branch) {
    error_msg = "No multiplier term given";
    goto error;
  }

  dim2 = gl_length(branch);
  if (dim2!=1) {
    error_msg = "A single multiplying term was expected; more than 1 given";
    goto error;
  }
  return 0; /* if here all should be ok */

 error:
  FPRINTF(stderr,"%s\n",error_msg);
  return 1;
}


int do_set_values_eval( struct Instance *i, struct gl_list_t *arglist, void *userdata){
  unsigned long dimension,c;
  struct gl_list_t *inputs, *outputs, *branch;
  double value,multiplier,calculated;
  int result;

  result = CheckArgs_SetValues(arglist);
  if (result)
    return 1;
  inputs = (struct gl_list_t *)gl_fetch(arglist,1);
  outputs = (struct gl_list_t *)gl_fetch(arglist,2);
  dimension = gl_length(inputs);
  branch = (struct gl_list_t *)gl_fetch(arglist,3);
  multiplier = RealAtomValue(gl_fetch(branch,1));

  for (c=1;c<=dimension;c++) {
    value = RealAtomValue(gl_fetch(inputs,c));
    i = (struct Instance *)gl_fetch(outputs,c);
    calculated = value*multiplier;
    SetRealAtomValue(i,calculated,(unsigned)0);
    FPRINTF(stdout,"value %g, multiplier %g, newvalue %g\n",
	           value, multiplier,calculated);
  }
  return 0;
}


/**
	This function expects 3 arguements; the calling protocol
	from ASCEND it expects to be invoked as:

	set_values(x1:array of generic_reals,
	          x2:array of generic_reals,
	           y:array of generic_reals);

	The dimension of x1, x2 and y are expected to be the same;

*/
static int CheckArgs_Bisection(struct gl_list_t *arglist){
  struct gl_list_t *branch;
  unsigned long len,c;
  unsigned long dim1=0, dim2=0;
  char *error_msg = "all ok";

  if (!arglist) {
    error_msg = "No arguement list given";
    goto error;
  }
  len = gl_length(arglist);
  if (len!=3) {
    error_msg = "Incorrect # args given, 3 were expected";
    goto error;
  }

  for (c=1;c<=len;c++) {
    branch = (struct gl_list_t *)gl_fetch(arglist,c);
    if (CheckArgVector(branch))
      return 1;
    if (dim1==0) { /* get the dimension of the first vector */
      dim1 = dim2 = gl_length(branch);
    } else {
      dim2 = gl_length(branch);
      if (dim1!=dim2) {
	error_msg = "Inconsistent dimensions given";
	goto error;
      }
    }
  }
  return 0;

 error:
  FPRINTF(stderr,"%s\n",error_msg);
  return 1;
}

int do_bisection_eval( struct Instance *i, struct gl_list_t *arglist, void *userdata){
  unsigned long dimension,c;
  struct gl_list_t *vector1, *vector2, *outputs;
  double value1, value2, calculated;
  int result;

  result = CheckArgs_Bisection(arglist);
  if (result)
    return 1;
  vector1 = (struct gl_list_t *)gl_fetch(arglist,1);
  vector2 = (struct gl_list_t *)gl_fetch(arglist,2);
  outputs = (struct gl_list_t *)gl_fetch(arglist,3);
  dimension = gl_length(vector1); /* get the dimension of the vectors */

  for (c=1;c<=dimension;c++) {
    value1 = RealAtomValue(gl_fetch(vector1,c));
    value2 = RealAtomValue(gl_fetch(vector2,c));
    i = (struct Instance *)gl_fetch(outputs,c);
    calculated = (value1 + value2)/2.0;
    SetRealAtomValue(i,calculated,(unsigned)0);
  }
  return 0;
}

int Bisection (void){

  char set_values_help[] =
    "This function accepts 3 args, The first 2 arg vectors of equal\n"
    "length. The second is a multiplier to be applied to each element\n"
	"of the first vector to yield the second vector.\n"
	"Example: do_set_values(x[1..n], y[1..n], multiplier[1..n]).\n";

  char bisection_help[] =
	"This function accepts 3 args, each of which must be vectors.\n"
	"It will bisect find the midpoint by bisection.\n"
	"Example: do_bisection(x[1..n],x_par[1..n], y[1..n]). \n";

  int result;
  result = CreateUserFunctionMethod("do_set_values",
			      do_set_values_eval,
			      3,set_values_help,NULL,NULL);
  result += CreateUserFunctionMethod("do_bisection",
			      do_bisection_eval,
			       3,bisection_help,NULL,NULL);
  return result;
}

