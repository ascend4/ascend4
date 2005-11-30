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
 *         #include "compiler.h"
 *  </pre>
 *  @todo Complete documentation of compiler/extfunc.h.
 */

#ifndef ASC_EXTFUNC_H
#define ASC_EXTFUNC_H

/**
	ExtEvalFunc type is a function pointer.

	@param mode ???
	@param m
	@param n
	@param x
	@param u
	@param f
	@param g
*/
typedef int ExtEvalFunc(int *mode, int *m, unsigned long *n,
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
  ExtEvalFunc **value;
  ExtEvalFunc **deriv;
  ExtEvalFunc **deriv2;
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

typedef int ExtBBoxFunc(struct Slv_Interp *,
   int ninputs, int noutputs,
   double *inputs, double *outputs, double *jacobian);

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


extern struct ExternalFunc *CreateExternalFunc(CONST char *name);
/**<
 *  Creates and returns and ExternalFunc node, with all attributes
 *  initialized to 0 or NULL.
 */

extern int AddExternalFunc(struct ExternalFunc *efunc, int force);
/**< 
 *  Given an external function node, add it to the external function
 *  library. We look up the external function before adding it to the
 *  library. If force is zero and the function exists then nothing
 *  is done and 0 is returned. If force is true, then the old entry is
 *  removed and the new one is added; 1 is returned. If the name is not
 *  found then the information is added to the library.
 */

extern struct ExternalFunc *LookupExtFunc(CONST char *funcname);
/**< 
 *  Returns the external function corresponding to the name, or NULL if
 *  not found.
 */


extern int CreateUserFunction(CONST char *name,
                              ExtEvalFunc *init,
                              ExtEvalFunc **value,
                              ExtEvalFunc **deriv,
                              ExtEvalFunc **deriv2,
                              CONST unsigned long n_inputs,
                              CONST unsigned long n_outputs,
                              CONST char *help);
/**<
	@param name name of the function being added (or updated)
	@param init initialisation function
	@param value location where pointer to evaluation function can be found???
	@param deriv location where pointer to first partial derivative function will be returned???
	@param deriv2 location where pointer to second derivative function can be found???

	This function is used to add external functions to the ASCEND system. The name of the function is looked up. If it already exists, the information will be updated. If the name was not found in the external function library, then an external function node will be created and added to the external function library. We make a *copy* of the help string if it is provided. We also make a copy of the name. Anyone desirous of ASCEND knowing about there functions must use this protocol.
*/

extern struct ExternalFunc *RemoveExternalFunc(char *name);
/**< 
 *  Given the name of an external function will remove it from the
 *  External function library.
 */

extern void DestroyExternalFunc(struct ExternalFunc *name);
/**< 
 *  Destroys an external function, but does *not* remove it from the
 *  library. Use the RemoveExternalFunc library first to retrieve the
 *  information, then call this function.
 */

extern int (*GetInitFunc(struct ExternalFunc *efunc))(/* */);
extern ExtBBoxFunc *GetValueFunc(struct ExternalFunc *efunc);
extern ExtBBoxFunc *GetDerivFunc(struct ExternalFunc *efunc);
extern ExtBBoxFunc *GetDeriv2Func(struct ExternalFunc *efunc);

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

