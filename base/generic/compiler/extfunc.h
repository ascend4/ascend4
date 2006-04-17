/*
	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
	Copyright (C) 2006 Benjamin Allan
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
*//*
	by Kirk Andre Abbott and Ben Allan
	Created: July 4, 1994.
	Version: $Revision: 1.5 $
	Version control file: $RCSfile: extfunc.h,v $
	Date last modified: $Date: 1997/07/18 12:29:30 $
	Last modified by: $Author: mthomas $
*/

/** @file
	External Functions Module.
	<pre>
	When #including extfunc.h, make sure these files are #included first:
	       #include "utilities/ascConfig.h"
	       #include "compiler/instance_enum.h"
	       #include "general/list.h"
	       #include "compiler/compiler.h"
	</pre>
	@todo Complete documentation of compiler/extfunc.h.
 */

#ifndef ASC_EXTFUNC_H
#define ASC_EXTFUNC_H

/**
	ExtEvalFunc type is a function pointer.
	@see rootfind.c:51

	@param mode 'to pass to the eval function' (?)
	@param m    relation index
	@param n    variable index
	@param x    'x' vector (?)
	@param u    'u' vector (?)
	@param f    vector of residuals
	@param g    vector of gradients
*/
typedef int ExtEvalFunc(int *mode, int *m, int *n,
   double *x, double *u, double *f, double *g);

/**
	ExtProcFunc type is a function pointer.

	@param mode
	@param m
	@param n
	@param x
	@param u
	@param f
	@param g
*/
typedef int ExtProcFunc(int *mode, int *m, unsigned long *n,
   double *x, double *u, double *f, double *g);


enum Calc_status {
  calc_converged, calc_diverged, calc_fp_error, calc_incorrect_args,
  calc_error, calc_all_ok
};

/**
	Each blackbox equation may show up more than once in a simulation
	if models repeat in the structure. For each occurence of the
	blackbox, a unique Slv_Interp object is given when the set of
	corresponding relations is created.
	It is used for the blackbox to communicate to the rest of the system.
	If the blackbox retains internal state between evaluation calls,
	it should store this state in the user_data pointer.
 */
struct Slv_Interp {
  /** unique identifier tied to instance tree. */
  int nodestamp;

  /** status is set by evaluation calls before returning. */
  enum Calc_status status;

  /** user_data is set by the external library if it has any persistent state
     during calls to ExtBBoxInitFunc initial and final given in
     CreateUserFunctionBlackBox.
   */
  void *user_data;

  /** will be true when the initial function pointer is called. */
  unsigned first_call  :1;

  /** will be true when the final function pointer is called. */
  unsigned last_call   :1;

  /** If check_args, blackbox should do any argument checking of the variables, data. */
  unsigned check_args  :1;

  /** If recalculate, the caller thinks the input may have changed. */
  unsigned recalculate :1;

  /** If func_eval, the caller is using the residual function pointer. */
  unsigned func_eval   :1;

  /** If deriv_eval, the caller is using the deriv function pointer. */
  unsigned deriv_eval  :1;

  /** If hess_eval, the caller is using the hessian function pointer. */
  unsigned hess_eval   :1;

  /** If single_step, the caller would like one step toward the solution;
     usually this is meaningless and should be answered with calc_diverged. */
  unsigned single_step :1;
};

/// Setup/teardown, if any needed, for a particular instance.
/**
	We don't actually support this method anywhere right now, as
	we're not sure what it can logically be used for that the
	init function in dlopening shouldn't be doing.
	In principal, we could add and cache a client-data pointer
	in each instance so that the external method may be stateful.
	Presently, the external methods must be clever to do that
	on their own or must use ascend instances for state instead.
	@param context the instance on which the method may be run.
*/
typedef int ExtMethodInit( struct Instance *context);

///	Run method a particular instance.
/**
	@param context the instance on which the method is run.
	  context may also appear explicitly in the arg list as SELF.
	@param args Each element of args is a list of instances; each
	name in the ascend-language argument list is expanded to a list
	(which may contain 0 or more Instances) and appended to args.
*/
typedef int ExtMethodRun( struct Instance *context, struct gl_list_t *args);

typedef int ExtBBoxInitFunc(struct Slv_Interp *,
                            struct Instance *,
                            struct gl_list_t *);

/** @TODO this one may need splitting/rework for hessian */
typedef int ExtBBoxFunc(struct Slv_Interp *,
                        int ninputs,
                        int noutputs,
                        double *inputs,
                        double *outputs,
                        double *jacobian);


/**<
	This is an enum to clarify and make type-safer the
	the variation of external functions circa 1995.
	Blackboxes might be callable from methods as well (@TODO), but
	this is dependent on their code registration to setup.
*/
enum ExternalFuncType {
	efunc_ERR = 0, /**< err value (Traps old mode errors too) */
	efunc_BlackBox = 10, /**< remainder of struct is blackbox */
	efunc_GlassBox = 20, /**< remainder of struct is glassbox */
	efunc_Method = 30 /**< remainder of struct is method */
};

struct GlassBoxExternalFunc {
  ExtEvalFunc *initial;
  ExtEvalFunc **value; /**< array of relation residual functions. */
  ExtEvalFunc **deriv; /**< array of relation gradient functions. */
  ExtEvalFunc **deriv2; /**< array of relation hessian functions. */
  ExtEvalFunc *final; /**< cleanup function. */
};

struct BlackBoxExternalFunc {
  ExtBBoxInitFunc *initial;
  ExtBBoxFunc *value; /**< relation residual function. */
  ExtBBoxFunc *deriv; /**< relation gradient function. */
  ExtBBoxFunc *deriv2; /**< relation hessian function. */
  ExtBBoxFunc *final; /**< cleanup function. */
};

struct MethodExternalFunc {
  ExtMethodRun *run; /**< the method invoked. */
#if 0 /* have no use for these currently. */
  ExtMethodInit *initial; /**< allowed to be null if not needed. */
  ExtMethodInit *final; /**< allowed to be null if not needed. */
#endif
};

struct ExternalFunc {
  enum ExternalFuncType etype;
  CONST char *name; /**< a string we own. */
  CONST char *help; /**< a string we own. */
  unsigned long n_inputs; /**< expected # of inputs. */
  unsigned long n_outputs; /**< expected # of outputs. */
  union {
	struct GlassBoxExternalFunc glass;
	struct BlackBoxExternalFunc black;
	struct MethodExternalFunc method;
  } u;
};


/** retired struct. */
struct RetiredExternalFunc {
  CONST char *name; /**< a string we own. */
  char *help; /**< a string we own. */
  ExtEvalFunc *init;
  ExtEvalFunc **value; /**< array of relation residual functions. */
  ExtEvalFunc **deriv; /**< array of relation gradient functions. */
  ExtEvalFunc **deriv2; /**< array of relation hessian functions. */
  unsigned long n_inputs;
  unsigned long n_outputs;
};


extern void InitExternalFuncLibrary(void);
/**<
	The main external functions library initialization routine. This
	function must be called before all others.
*/

extern void DestroyExtFuncLibrary(void);
/**<
	Destroys the external function library and deallocates all the
	information associated with it.
*/


extern int AddExternalFunc(struct ExternalFunc *efunc, int force);
/**<
	Adds an external function node to the external function library.
	We look up the external function before adding it to the
	library.  If force is zero and the function exists then nothing
	is done and 0 is returned.  If force is true, then the old entry is
	removed and the new one is added; 1 is returned.  If the name is not
	found then the information is added to the library.
*/

extern struct ExternalFunc *LookupExtFunc(CONST char *funcname);
/**<
	Returns the external function having the given name, or NULL if
	not found.
*/

typedef int (*CreateUserFunction_fptr_t)(CONST char *name,
                              ExtEvalFunc *init,
                              ExtEvalFunc **value,
                              ExtEvalFunc **deriv,
                              ExtEvalFunc **deriv2,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);

extern int DLEXPORT CreateUserFunctionBlackBox(CONST char *name,
                              ExtBBoxInitFunc *init,
                              ExtBBoxFunc *value,
                              ExtBBoxFunc *deriv,
                              ExtBBoxFunc *deriv2,
                              ExtBBoxFunc *final,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);
/**<
	Adds an external function to the ASCEND system.
	The name of the function is looked up.  If it already exists, the
	information will be updated.  If the name was not found in the
	external function library, then an external function node will be
	created and added to the external function library.  We make a
	*copy* of the help string if it is provided.  We also make a copy
	of the name.  Anyone desirous of ASCEND knowing about their
	functions must use this protocol.

	@param name Name of the function being added (or updated).
	@param init Pointer to initialisation function, or NULL if none.
	@param final Pointer to shutdown function. May be same as init.
	@param value  evaluation function pointers, or NULL if none.
	@param deriv first partial derivative functions, or NULL if none.
	@param deriv2 second derivative functions, or NULL if none.
	@return Returns 0 if the function was successfully added,
	        non-zero otherwise.
*/

extern int DLEXPORT CreateUserFunctionGlassBox(CONST char *name,
                              ExtEvalFunc *init,
                              ExtEvalFunc **value,
                              ExtEvalFunc **deriv,
                              ExtEvalFunc **deriv2,
                              ExtEvalFunc *final,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);
/**<
	Adds an external function to the ASCEND system.
	The name of the function is looked up.  If it already exists, the
	information will be updated.  If the name was not found in the
	external function library, then an external function node will be
	created and added to the external function library.  We make a
	*copy* of the help string if it is provided.  We also make a copy
	of the name.  Anyone desirous of ASCEND knowing about their
	functions must use this protocol.

	@param name Name of the function being added (or updated).
	@param init Pointer to initialisation function, or NULL if none.
	@param value array of evaluation function pointers,
	             or NULL if none.
	@param deriv array of first partial
	             derivative functions, or NULL if none.
	@param deriv2 array of second derivative
	              functions, or NULL if none.
	@return Returns 0 if the function was successfully added,
	        non-zero otherwise.
*/

extern int DLEXPORT CreateUserFunctionMethod(CONST char *name,
                             /*  ExtMethodInit *initial, */
                              ExtMethodRun *run,
                             /*  ExtMethodInit *final, */
                              CONST long n_args,
                              CONST char *help);
/**<
 *  Adds an external method call to the ASCEND system.
 *  The name of the function is looked up.  If it already exists, the
 *  information will be updated.  If the name was not found in the
 *  external function library, then an external function node will be
 *  created and added to the external function library.  We make a
 *  *copy* of the help string if it is provided.  We also make a copy
 *  of the name.  Anyone desirous of ASCEND knowing about their
 *  external methods must use this protocol.
 *
 *  @param name Name of the function being added (or updated).
 *  @param initial Pointer to initialisation function, or NULL if none.
 *  @param run Pointer to the method.
 *  @param final Pointer to cleanup function, or NULL if none.
 *  @param n_args number of arguments expected as input, or -1 if any number is allowed.
 *  @return Returns 0 if the function was successfully added,
 *          non-zero otherwise.
 */

extern struct ExternalFunc *RemoveExternalFunc(char *name);
/**<
	Removes the external function having the given name from the
	External function library.
*/

extern void DestroyExternalFunc(struct ExternalFunc *name);
/**<
	Destroys an external function, but does *not* remove it from the
	library. Use the RemoveExternalFunc library first to retrieve the
	information, then call this function.
*/

/** Fetch black initialization function. */
extern ExtBBoxInitFunc *GetInitFunc(struct ExternalFunc *efunc);
/** Fetch black residual function. */
extern ExtBBoxFunc *GetValueFunc(struct ExternalFunc *efunc);
/** Fetch black sensitivity gradient function. */
extern ExtBBoxFunc *GetDerivFunc(struct ExternalFunc *efunc);
/** Fetch black hessian function. */
extern ExtBBoxFunc *GetDeriv2Func(struct ExternalFunc *efunc);
/** Fetch black cleanup function. */
extern ExtBBoxInitFunc *GetFinalFunc(struct ExternalFunc *efunc);


/** Fetch glass initialization function. */
extern ExtEvalFunc *GetGlassBoxInit(struct ExternalFunc *efunc);
/** Get glass box residual function array. */
extern ExtEvalFunc **GetValueJumpTable(struct ExternalFunc *efunc);
/** Get glass box gradient function array. */
extern ExtEvalFunc **GetDerivJumpTable(struct ExternalFunc *efunc);
/** Get glass box hessian function array. */
extern ExtEvalFunc **GetDeriv2JumpTable(struct ExternalFunc *efunc);
/** Fetch black initialization function. */
extern ExtEvalFunc *GetGlassBoxFinal(struct ExternalFunc *efunc);

/** Fetch method run function. */
extern ExtMethodRun *GetExtMethodRun(struct ExternalFunc *efunc);


extern CONST char *ExternalFuncName(CONST struct ExternalFunc *efunc);
/**<
	Returns the name of an external function.
*/

/** fetch the required input count for glass, black, or method. */
extern unsigned long NumberInputArgs(CONST struct ExternalFunc *efunc);
/** fetch the required output count for glass, black, or method. */
extern unsigned long NumberOutputArgs(CONST struct ExternalFunc *efunc);


extern void PrintExtFuncLibrary(FILE *f);
/**<
	Prints the contents of the external function library to the given
	file. The file must be opened for writing.
*/

extern char *WriteExtFuncLibraryString(void);
/**<
	Returns a string of formatted information about the external functions
	defined. the string looks like "{{name1} {help1}} {{name2} {help2}} "
	The string may be empty/NULL if there are no external functions loaded.
*/

/**
	This provides a way for other code to visit the external function list
*/
extern void TraverseExtFuncLibrary(void (*)(void *,void *),void *secondparam);

#endif /* ASC_EXTFUNC_H */
