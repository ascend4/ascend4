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
*//**
	@file
	External Functions Module.

        This module implements the ExternalFunc structure referenced by
        black and glass box structures and external methods.
        The ExternalFunc structure stores the number of input and
        output parameters (formal)
        as well as 'help' string and 'name' string' for each of these 'calls'.

	This header also provides functions for ExternalFunc library maintenance.
	This allows ASCEND to maintain a list of the ExternalFunc requests derived
	from statements in the model(s). When compilation completes, I suppose
	it should be possible to alert the user about any external functions
	that were not able to be resolved.

	@todo Complete documentation of compiler/extfunc.h.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler/instance_enum.h"
	#include "general/list.h"
	#include "compiler/compiler.h"
*//*
	by Kirk Andre Abbott and Ben Allan
	Created: July 4, 1994.
	Version: $Revision: 1.5 $
	Version control file: $RCSfile: extfunc.h,v $
	Date last modified: $Date: 1997/07/18 12:29:30 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_EXTFUNC_H
#define ASC_EXTFUNC_H

#include <utilities/ascConfig.h>
#include "relation_util.h"

/*------------------------------------------------------------------------------
	type definitions and forward decls
*/

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


/** values a blackbox (or ?) can report when returning. */
enum Calc_status {
  calc_converged,
  calc_diverged,
  calc_fp_error,
  calc_incorrect_args,
  calc_error,
  calc_all_ok
};

/**
	Things that a blackbox can be asked to do.

	@NOTE Rhetorical question: Why do we want this? See comments in Slv_Interp.
*/
enum Request_type {
  bb_none,        /**< do nothing. should never be sent. */
  bb_first_call,  /**< will be given when the initial function pointer is called. */
  bb_last_call,   /**< will be given when the final function pointer is called. */
  bb_check_args,  /**< do any argument checking of the variables, data */
  bb_recalculate, /**< the caller thinks the input may have changed: recalc if reqd */
  bb_func_eval,   /**< the caller needs the residual function pointer. */
  bb_deriv_eval,  /**< the caller needs the deriv function pointer. */
  bb_hess_eval,   /**< the caller needs the hessian function pointer. */
  bb_single_step  /**< the caller would like one step toward the solution;
		usually this is meaningless and should be answered with calc_diverged. */
};

/**
	Each blackbox equation may show up more than once in a simulation
	if models repeat in the structure. For each occurence of the
	blackbox, a unique BBoxInterp object is given when the set of
	corresponding relations is created.
	It is used for the blackbox to communicate to the rest of the system.
	If the blackbox retains internal state between evaluation calls,
	it should store this state in the user_data pointer.
 */
struct BBoxInterp {
  /** status is set by blackbox calls before returning. */
  enum Calc_status status;

  /** user_data is set by the blackbox if it has any persistent state
     during calls to initial and final functions given in
     CreateUserFunctionBlackBox.
   */
  void *user_data;

  /** What the caller wants done on a given call.

      As black boxes are represented with 5 function pointers,
      one might think this is not needed. Providing the 'task' here allows
      one to implement only one function and have it handle all types of
      calls. It also facilitates cases where there is checking rather than
      evaluation.

      @NOTE Problem? Don't init functions and evaluation functions have
      different signatures?
  */
  enum Request_type task;
  /* if someone still needs a nodestamp, we could go back to
     defining one, but it should really be inside user_data as
     ascend doesn't need it. */

};

typedef int ExtBBoxInitFunc(struct BBoxInterp *interp,
                            struct Instance *data,
                            struct gl_list_t *arglist);
/**<
	Note Well: References to ascend instances must not be
	cached in the user_data of interp, as instances may move
	in the dynamically typed ASCEND world.
	@param interp is the structure that all subsequent calls to
		evaluation functions will include.
	@param data is the DATA instance from the ascend model.
	@param arglist is the input and output list of instance lists,
		where each element corresponds to a formal argument.
		Elements 1:n_formal_inputs are the inputs and the next
		n_formal_outputs elements are the outputs.
	@return 0 if ok, nonzero if some error makes the blackbox impossible.

*/

typedef void ExtBBoxFinalFunc(struct BBoxInterp *interp);
/**< @param interp same as called in init and evaluate.
	This is the opportunity to deallocate user_data as the
	instance is being destroyed.
*/

/**
	External black box equations are of the block form
	y_out = f(x_in). This block expands to N_outputs equations
	of the form y_out[i] = f_i(x_in), where the functional details
	of f are assumed to be smooth enough but otherwise totally hidden
	and x_in, y_out are non-overlapping sets of variables.
	The compiler will warn if overlap is detected.
	Normal D.O.F. solver analysis applies in most cases.

	Note that solvers are not psychic; if this blackbox is embedded
	in a larger model such that some of y_out are inconsistently
	fixed variables, the odds of convergence are small.
	Cleverer solvers may issue a warning.

	@param interp the control information is exchanged in interp;
		 interp->task should be consulted.
	@param ninputs the length of the inputs, xi_in.
	@param noutputs, the length of the outputs, y_out.

	@param jacobian, the partial derivative df/dx, where
	each row is df[i]/dx[j] over each j for the y_out[i] of
	matching index. The jacobian array is 1-D, row major, i.e.
	df[i]/dx[j] -> jacobian[i*ninputs+j].

	@TODO this one may need splitting/rework for hessian.
*/
typedef int ExtBBoxFunc(struct BBoxInterp *interp,
        int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian);

struct BlackBoxExternalFunc {
  ExtBBoxInitFunc *initial; /**< called after instance construction */
  ExtBBoxFunc *value; /**< relation residual function. */
  ExtBBoxFunc *deriv; /**< relation gradient function (see jacobian)*/
  ExtBBoxFunc *deriv2; /**< relation hessian function. */
  ExtBBoxFinalFunc *final; /**< cleanup function called at instance destruction. */
  double inputTolerance; /**< largest change in an input variable
			that is allowable without recalculating. */
};


/**
	Function pointer (type) to implement an external method on a particular
	instance

	@param context the instance on which the method is run.
		context may also appear explicitly in the arg list as SELF.
	@param args Each element of args is a list of instances; each
		name in the ascend-language argument list is expanded to a list
		(which may contain 0 or more Instances) and appended to args.
	@return ???
*/
typedef int ExtMethodRun(struct Instance *context, struct gl_list_t *args, void *user_data);

/**
	Setup/teardown, if any needed, for a particular instance.

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

/**
	Destroy function (note comments for ExtMethodInit).

	This function deallocated user_data for a particular external method.
	In the case of external python script methods, for example, this will perform
	Py_DECREF on the external script, so that python can unload it.

	Couldn't see a way to do this without adding back this function here. -- JP

	@return 0 on success, else error code.
*/

typedef int ExtMethodDestroyFn( void *user_data);


struct MethodExternalFunc {
  ExtMethodRun *run; /**< the method invoked. */
  void *user_data; /**< I'd anticipate that this would be a function pointer
		implemented in an external scripting language. Should only be accessed
		from inside the 'run' function! -- JP */
  ExtMethodDestroyFn *destroyfn;
};

struct ExternalFunc {
  enum ExternalFuncType etype;
  CONST char *name; /**< a string we own. */
  CONST char *help; /**< a string we own. */
  unsigned long n_inputs; /**< expected # of formal inputs. */
  unsigned long n_outputs; /**< expected # of formal outputs. */
  union {
	struct GlassBoxExternalFunc glass;
	struct BlackBoxExternalFunc black;
	struct MethodExternalFunc method;
  } u;
};

/*------------------------------------------------------------------------------
  REGISTRATION / LOOKUP FUNCTIONS
*/


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

	@return 1 if an element is added to ExternalFunctionLibrary Table,
		or 0 if no addition is made.
*/

ASC_DLLSPEC struct ExternalFunc *LookupExtFunc(CONST char *funcname);
/**<
	Returns the external function having the given name, or NULL if
	not found.
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


extern void PrintExtFuncLibrary(FILE *f);
/**<
	Prints the contents of the external function library to the given
	file. The file must be opened for writing.
*/

ASC_DLLSPEC char *WriteExtFuncLibraryString(void);
/**<
	Returns a string of formatted information about the external functions
	defined. the string looks like "{{name1} {help1}} {{name2} {help2}} "
	The string may be empty/NULL if there are no external functions loaded.
*/

/**
	This provides a way for other code to visit the external function list
*/
ASC_DLLSPEC void TraverseExtFuncLibrary(void (*)(void *,void *),void *secondparam);


/** fetch the required formal input count for glass, black, or method. */
ASC_DLLSPEC unsigned long NumberInputArgs(CONST struct ExternalFunc *efunc);

/** fetch the required formal output count for glass, black, or method. */
ASC_DLLSPEC unsigned long NumberOutputArgs(CONST struct ExternalFunc *efunc);


ASC_DLLSPEC CONST char*ExternalFuncName(CONST struct ExternalFunc *efunc);
/**<
	Returns the name of an external function.
*/

/*------------------------------------------------------------------------------
  EXTERNAL METHOD STUFF
*/

ASC_DLLSPEC int CreateUserFunctionMethod(CONST char *name
		,ExtMethodRun *run
		,CONST long n_args
		,CONST char *help
		,void *user_data
		,ExtMethodDestroyFn *destroyfn
);
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
 *
 * Note on blackbox integration with nonlinear solvers:
 * The basic idea is that the blackbox has inputs x and outputs y
 * that are all variables to the ascend model. Some of these may
 * be fixed variables, but this is of no consequence to the blackbox
 * routine unless one of the fixed variables is fixed outside the
 * bounds of feasible input to the box. In the newton solution
 * process there are three sets of values involved, x (inputs),
 * yhat (the outputs as computed by the box), and y (the proposed
 * values of the outputs in the model which may not match yhat
 * until the entire model is converged. The many equations of the
 * blackbox are hidden and represented by the reduced set of
 * equations. In mathematical form, an array of equations:
 * y = yhat(x); (ascend form bbrel: bboxname(inputs, outputs, data);)
 * where yhat(x) is computed as bboxname(x,yhat).
 * The bbox may produce the reduced gradient or be finite differenced
 * to get dyhat/dx partial derivatives.
 * The residual then, obviously, is yhat - y and the gradient is
 * (in matrix form) I-dyhat/dx for the reduced equations.
 *
 *  @param name Name of the function being added (or updated).
 *  @param run Pointer to the method.
 *  @param n_args number of arguments expected as input, or -1 if any number is allowed.
 *  @param help a string for human consumption.
 *  @return Returns 0 if the function was successfully added,
 *          non-zero otherwise.
 */

/** Fetch method run function. */
extern ExtMethodRun *GetExtMethodRun(struct ExternalFunc *efunc);
extern void *GetExtMethodUserData(struct ExternalFunc *efunc);

/*------------------------------------------------------------------------------
  BLACK BOX STUFF
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
extern ExtBBoxFinalFunc *GetFinalFunc(struct ExternalFunc *efunc);
/** Fetch black inputTolerance. */
extern double GetValueFuncTolerance(struct ExternalFunc *efunc);


ASC_DLLSPEC int CreateUserFunctionBlackBox(CONST char *name,
		ExtBBoxInitFunc *init,
		ExtBBoxFunc *value,
		ExtBBoxFunc *deriv,
		ExtBBoxFunc *deriv2,
		ExtBBoxFinalFunc *final,
		CONST unsigned long n_inputs,
		CONST unsigned long n_outputs,
		CONST char *help,
		double inputTolerance
);
/**<
	Adds an external function to the ASCEND system.
	The name of the function is looked up.  If it already exists, the
	information will be updated.  If the name was not found in the
	external function library, then an external function node will be
	created and added to the external function library.  We make a
	*copy* of the help string if it is provided.  We also make a copy
	of the name.  Anyone desirous of ASCEND knowing about their
	functions must use this protocol.

        Note on blackbox integration with nonlinear solvers:
        The basic idea is that the blackbox has inputs x and outputs y
        that are all variables to the ascend model. Some of these may
        be fixed variables, but this is of no consequence to the blackbox
        routine unless one of the fixed variables is fixed outside the
        bounds of feasible input to the box. In the newton solution
        process there are three sets of values involved, x (inputs),
        yhat (the outputs as computed by the box), and y (the proposed
        values of the outputs in the model which may not match yhat
        until the entire model is converged. The many equations of the
        blackbox are hidden and represented by the reduced set of
        equations. In mathematical form, an array of equations:
        y = yhat(x); (ascend form bbrel: bboxname(inputs, outputs, data);)
        where yhat(x) is computed as bboxname(x,yhat).
        The bbox may produce the reduced gradient or be finite differenced
        to get dyhat/dx partial derivatives.
        The residual then, obviously, is yhat - y and the gradient is
        (in matrix form) I-dyhat/dx for the reduced equations.

	@param name Name of the function being added (or updated).
	@param init Pointer to initialisation function, or DefaultExtBBoxInitFunc if none.
	@param final Pointer to shutdown function, or DefaultExtBBoxFinalFunc if none.
	@param value  evaluation function pointers, or NULL if none.
	@param deriv first partial derivative functions, or NULL if none.
	@param deriv2 second derivative functions, or NULL if none.
	@param inputTolerance maximum change in any of the inputs that is
               allowable without recomputing the outputs.
		0.0 is conservative, or specify a larger number if the
		outputs are only mildly sensitive to small changes in inputs.
	@return Returns 0 if the function was successfully added,
	        non-zero otherwise.
*/


ASC_DLLSPEC int DefaultExtBBoxInitFunc(struct BBoxInterp *interp,
                            struct Instance *data,
                            struct gl_list_t *arglist);
/**< Default init code for black boxes.
If the user does not supply an init function (possibly because they have
no per-instance data to manage), they should pass this function
instead to CreateUserFunctionBlackBox.
*/

ASC_DLLSPEC int ErrorExtBBoxValueFunc(
		struct BBoxInterp *interp,
		int ninputs,
		int noutputs,
		double *inputs,
		double *outputs,
		double *jacobian
);
/**< Default residual code for black boxes. NOT VERY INTERESTING.
If the user does not supply a value function (possibly because they have
no brain) they will get this. It whines. always returns -1.
*/

ASC_DLLSPEC int DefaultExtBBoxFuncDerivFD(
		struct BBoxInterp *interp,
		int ninputs,
		int noutputs,
		double *inputs,
		double *outputs,
		double *jacobian
);
/**< Default finite-differencing code for blackboxes.
If the user does not supply a derivative function they wrote,
they must supply this derivative function instead.
John Pye claims to have filled this in.
*/

ASC_DLLSPEC int DefaultExtBBoxFuncDeriv2FD(
		struct BBoxInterp *interp,
		int ninputs,
		int noutputs,
		double *inputs,
		double *outputs,
		double *jacobian
);
/**< Currently a pipe dream. returns an error.  */

ASC_DLLSPEC void DefaultExtBBoxFinalFunc(struct BBoxInterp *interp);
/**< Default finalize code for black boxes.
If the user does not supply a final function (possibly because they have
no per-instance data to manage), they should pass this function
instead to CreateUserFunctionBlackBox.
*/

/*-----------------------------------------------------------------------------
  GLASS BOX STUFF
*/

ASC_DLLSPEC int CreateUserFunctionGlassBox(CONST char *name,
		ExtEvalFunc *init,
		ExtEvalFunc **value,
		ExtEvalFunc **deriv,
		ExtEvalFunc **deriv2,
		ExtEvalFunc *final,
		CONST unsigned long n_inputs, CONST unsigned long n_outputs,
		CONST char *help
);
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

#endif /* ASC_EXTFUNC_H */
