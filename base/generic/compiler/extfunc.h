/*
 *  External Functions Module
 *  by Kirk Andre Abbott
 *  Created: July 4, 1994.
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: extfunc.h,v $
 *  Date last modified: $Date: 1997/07/18 12:29:30 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  External Functions Module.
 *  <pre>
 *  When #including extfunc.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler/instance_enum.h"
 *         #include "general/list.h"
 *         #include "compiler/compiler.h"
 *  </pre>
 *  @todo Complete documentation of compiler/extfunc.h.
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


struct ExternalFunc {
  CONST char *name;
  unsigned long n_inputs;
  unsigned long n_outputs;
  char *help;
  ExtEvalFunc *init;
  ExtEvalFunc **value; /**< array of relation residual functions. */
  ExtEvalFunc **deriv; /**< array of relation gradient functions. */
  ExtEvalFunc **deriv2; /**< array of relation hessian functions. */
};

enum Calc_status {
  calc_converged, calc_diverged, calc_fp_error, calc_incorrect_args,
  calc_error, calc_all_ok
};

struct Slv_Interp {
  int nodestamp;
  enum Calc_status status;
  void *user_data;
  unsigned first_call  :1;
  unsigned last_call   :1;
  unsigned check_args  :1;
  unsigned recalculate :1;
  unsigned deriv_eval  :1;
  unsigned func_eval   :1;
  unsigned single_step  :1;
};

typedef int ExtBBoxInitFunc(struct Slv_Interp *,
                            struct Instance *,
                            struct gl_list_t *);

typedef int ExtBBoxFunc(struct Slv_Interp *,
                        int ninputs,
                        int noutputs,
                        double *inputs,
                        double *outputs,
                        double *jacobian);

extern void InitExternalFuncLibrary(void);
/**<
 *  The main external functions library initialization routine. This
 *  function must be called before all others.
 */

extern void DestroyExtFuncLibrary(void);
/**<
 *  Destroys the external function library and deallocates all the
 *  information associated with it.
 */


#ifdef THIS_IS_AN_UNUSED_FUNCTION
extern struct ExternalFunc *CreateExternalFunc(CONST char *name);
/**<
 *  Creates a new ExternalFunc node having the specified name.
 *  All other attributes are initialized to 0 or NULL.  There is
 *  no checking for the validity or uniqueness of name.
 *
 *  @param name The name for the new ExternalFunc.
 *  @return A pointer to the new ExternalFunc.
 */
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

extern int AddExternalFunc(struct ExternalFunc *efunc, int force);
/**<
 *  Adds an external function node to the external function library.
 *  We look up the external function before adding it to the
 *  library.  If force is zero and the function exists then nothing
 *  is done and 0 is returned.  If force is true, then the old entry is
 *  removed and the new one is added; 1 is returned.  If the name is not
 *  found then the information is added to the library.
 */

extern struct ExternalFunc *LookupExtFunc(CONST char *funcname);
/**<
 *  Returns the external function having the given name, or NULL if
 *  not found.
 */

typedef int (*CreateUserFunction_fptr_t)(CONST char *name,
                              ExtEvalFunc *init,
                              ExtEvalFunc **value,
                              ExtEvalFunc **deriv,
                              ExtEvalFunc **deriv2,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);

extern int CreateUserFunction(CONST char *name,
                              ExtEvalFunc *init,
                              ExtEvalFunc **value,
                              ExtEvalFunc **deriv,
                              ExtEvalFunc **deriv2,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);
/**<
 *  Adds an external function to the ASCEND system.
 *  The name of the function is looked up.  If it already exists, the
 *  information will be updated.  If the name was not found in the
 *  external function library, then an external function node will be
 *  created and added to the external function library.  We make a
 *  *copy* of the help string if it is provided.  We also make a copy
 *  of the name.  Anyone desirous of ASCEND knowing about their
 *  functions must use this protocol.
 *
 *  @param name Name of the function being added (or updated).
 *  @param init Pointer to initialisation function, or NULL if none.
 *  @param value Pointer to a function pointer to the evaluation function,
 *               or NULL if none.
 *  @param deriv Pointer to a function pointer to the first partial
 *               derivative function, or NULL if none.
 *  @param deriv2 Pointer to a function pointer to the second derivative
 *                function, or NULL if none.
 *  @return Returns 0 if the function was successfully added,
 *          non-zero otherwise.
 *
 *  @todo compiler/extfunc:CreateUserFunction() is broken.  The current
 *        implementation wants to treat value, deriv, and deriv2 as BOTH
 *        function pointers and arrays of function pointers.  We need to
 *        decide which they are or else provide a mechanism supporting dual
 *        roles.  This could be a union in ExternalFunc explicitly allowing
 *        them to be both.  This would necessitate 2 CreateUserFunction()
 *        varieties - 1 taking (ExtEvalFunc *) and 1 taking (ExtEvalFunc **)
 *        to allow the different types of ExternalFunc's to be set up.
 */

extern struct ExternalFunc *RemoveExternalFunc(char *name);
/**<
 *  Removes the external function having the given name from the
 *  External function library.
 */

extern void DestroyExternalFunc(struct ExternalFunc *name);
/**<
 *  Destroys an external function, but does *not* remove it from the
 *  library. Use the RemoveExternalFunc library first to retrieve the
 *  information, then call this function.
 */

#if 0 
/* don't know where the hell the return type came from.
will check.*/ 
extern ExtEvalFunc *GetInitFunc(struct ExternalFunc *efunc);
#endif
extern ExtBBoxInitFunc *GetInitFunc(struct ExternalFunc *efunc);

/* black box stuff */

extern ExtBBoxFunc *GetValueFunc(struct ExternalFunc *efunc);
extern ExtBBoxFunc *GetDerivFunc(struct ExternalFunc *efunc);
extern ExtBBoxFunc *GetDeriv2Func(struct ExternalFunc *efunc);

/* glass box stuff */

extern ExtEvalFunc **GetValueJumpTable(struct ExternalFunc *efunc);
extern ExtEvalFunc **GetDerivJumpTable(struct ExternalFunc *efunc);
extern ExtEvalFunc **GetDeriv2JumpTable(struct ExternalFunc *efunc);

extern CONST char *ExternalFuncName(CONST struct ExternalFunc *efunc);
/**<
 *  Returns the name of an external function.
 */

extern unsigned long NumberInputArgs(CONST struct ExternalFunc *efunc);
extern unsigned long NumberOutputArgs(CONST struct ExternalFunc *efunc);


extern void PrintExtFuncLibrary(FILE *f);
/**<
 *  Prints the contents of the external function library to the given
 *  file. The file must be opened for writing.
 */

extern char *WriteExtFuncLibraryString(void);
/**<
 * Returns a string of formatted information about the external functions
 * defined. the string looks like "{{name1} {help1}} {{name2} {help2}} "
 * The string may be empty/NULL if there are no external functions loaded.
 */

/**
	This provides a way for other code to visit the external function list
*/
extern void TraverseExtFuncLibrary(void (*)(void *,void *),void *secondparam);

#endif /* ASC_EXTFUNC_H */

