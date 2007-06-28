/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Abbott.
	Copyright (C) 1997 Benjamin Allan
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
	User package management.

	Users of this header should not have to include anything
	except instance_enum.h and list.h and whatever headers those
	may require.
*//*
	by Ben Allan & Kirk Abbott
	Created: July 4, 1994
	Version: $Revision: 1.12 $
	Version control file: $RCSfile: packages.h,v $
	Date last modified: $Date: 1998/05/18 16:36:45 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_PACKAGES_H
#define ASC_PACKAGES_H

/**	@addtogroup compiler Compiler
	@{
*/

/**
	Have included the follow here to simplify instructions for people 
	writing their own external packages:
*/
#include <utilities/ascConfig.h>
#include "instance_enum.h"
#include <general/list.h>
#include "extfunc.h"

#ifndef ASC_CLIENTDATA_DECLARED
# ifdef __STDC__
   typedef void *ascClientData;
# else
   typedef int *ascClientData;
# endif
# define ASC_CLIENTDATA_DECLARED
#endif

/** 
	these are for external clients to return to us when they have
	an error or other procedural message to report about flow of
	control.
*/
#define ASC_OK          0
#define ASC_ERROR       1
#define ASC_RETURN      2
#define ASC_BREAK       3
#define ASC_CONTINUE    4

typedef int (Asc_MethodProc) (ascClientData, CONST char *,
                              struct Instance *, struct gl_list_t *);

typedef int (Asc_MethodDelete) (ascClientData, CONST char *, Asc_MethodProc);

/** 
	Clients should use the operators below to access this enum and
	associated structure.
*/
enum argItemEnum {
  argError,         /**< should never be seen by client */
  /* the first 3 characters of the comment lines are the
   * equivalent of C conversion characters in printf when
   * specifying the arguments desired in the parsestring.
   */
  argOldFileId,     /**< OF  name of an existing file (char *) */
  argNewFileId,     /**< NF  name of new file (char *) */
  argUnkFileId,     /**< UF  name of file, unknown status (char *) */
  argFloat,         /**<  f  maps ASCEND integer/real types to a float value */
  argDouble,        /**<  g  maps ASCEND integer/real types to a double value */
  argLong,          /**<  ld maps ASCEND integer/boolean to long value. */
  argDoublePtr,     /**< pg  maps ASCEND reals to a double * */
  argLongPtr,       /**< pld maps ASCEND integers to long * */
  argInt,           /**<  d  maps ASCEND integer/boolean to int value. */
  argBoolean,       /**<  b  maps ASCEND boolean or integer to 0,1 value */
  argString,        /**<  s  maps ASCEND symbol types to a C const char * */
  argSetString,     /**<  ss maps ASCEND set types into a C const char * */
  argNameString,    /**<  n  maps ASCEND Name given into a C const char * */
  argNameList,      /**<  N  expand ASCEND Name given into a C const char * */
  argTypeString,    /**<  T  maps ASCEND type name given into a C const char * */
  argInst,          /**<  i  instance pointer may be any kind */
  argInstReal,      /**<  R  instance pointer must be REAL*INST */
  argInstRealList,  /**< LR  flattens array of real insts into a gl_list */
  argInstRealArray, /**< AR  instance pointer must be an array of reals */
  argDoubleConst,   /**< CR  pointer to new double value for unassigned const */
  argInstInt,       /**<  I  instance pointer must be INTEGER*INST */
  argInstIntList,   /**< LI  flattens array of integer insts into a gl_list */
  argInstIntArray,  /**< AI  instance pointer must be an array of integers */
  argIntConst,      /**< CI  pointer to new long value for unassigned const */
  argInstBool,      /**<  B  instance must be BOOLEAN_[ATOM/CONSTANT]_INST */
  argInstBoolList,  /**< LB  flattens array of boolean insts into a gl_list */
  argInstBoolArray, /**< AB  instance pointer must be an array of boolean */
  argBoolConst,     /**< CB  pointer to new 0/1 value for unassigned const */
  argInstSym,       /**<  S  instance must be SYMBOL_[ATOM/CONSTANT]_INST */
  argInstSymList,   /**< LS  flattens array of symbol insts into a gl_list */
  argInstSymArray,  /**< AS  instance pointer must be an array of symbols */
  argSymConst,      /**< CS  pointer to new char * value for unassigned const */
  argInstSet,       /**<  SS instance must be SET_[ATOM/CONSTANT]_INST */
  argInstSetList,   /**< LSS flattens array of set insts into a gl_list */
  argInstSetArray,  /**< ASS instance pointer must be an array of sets */
  argISetConst,     /**< CSS pointer to gl_list of longs for unassigned set */
  argESetConst,     /**< DSS pointer to gl_list of char * for unassigned set */
  argInstRel,       /**<  r  instance must be RELATION_INST */
  argInstRelList,   /**< Lr  flattens array of relation insts into a gl_list */
  argInstRelArray,  /**< Ar  instance pointer must be an array of relations */
  argInstLRel,      /**<  lr instance must be LOGREL_INST */
  argInstLRelList,  /**< Llr flattens array of logrelations into a gl_list */
  argInstLRelArray, /**< Alr instance pointer must be an array of logrel */
  argInstModel,     /**<  M  MODEL instance pointer */
  argInstModelList, /**< LM  flattens array of MODEL insts into a gl_list */
  argInstModelArray,/**< AM  flattens array of MODEL insts into a gl_list */
  argProcName,      /**<  P  maps name of a method/call to a CONST char * */
  argExpr,          /**<  V  maps a reference to an ASCEND Expr into lval */
  argSLit           /**<  Q  a C string literal is expected. */
  /* probably need to add type Q for double quoted text */
};

struct argItem {
  enum argItemEnum kind;
  int depth;    /**< For Array and list arguments, sets the number of
                 * subscripts expected. If 0, any number is allowed.
                 * Internal use only.
                 */
  union {
    double dval;        /**< double value */
    long lival;         /**< long int value */
    double dptr;        /**< double pointer */
    long liptr;         /**< long int pointer */
    int ival;           /**< int value */
    int bval;           /**< boolean value */
    CONST char *cval;   /**< const char * from an ascend set/symbol */
    CONST char *nval;   /**< string form of the ascend name given */
    CONST char *fval;   /**< file id string */
    CONST struct gl_list_t *lval; /**< list according to enum */
    struct Instance *i; /**< instance according to enum */
  } u;                  /**< union of possible values */
  short args;           /**< 0 usually. 1 if item is ... at the end. Internal use. */
  short exact;          /**< 0 usually. 1 if type specifier to be match. 2 if exactly */
  symchar *type_name;    /**< typeidentifier from the symbol table */
};

#define Asc_argItemKind(aip)        ((aip)->kind)
#define Asc_argItemDepth(aip)       ((aip)->depth)
#define Asc_argItemInstance(aip)    ((aip)->u.i)
#define Asc_argItemDoublePtr(aip)   ((aip)->u.dptr)
#define Asc_argItemLongPtr(aip)     ((aip)->u.liptr)
#define Asc_argItemDouble(aip)      ((aip)->u.dval)
#define Asc_argItemLongVal(aip)     ((aip)->u.lival)
#define Asc_argItemIntVal(aip)      ((aip)->u.ival)
#define Asc_argItemBoolVal(aip)     ((aip)->u.bval)
#define Asc_argItemStringVal(aip)   ((aip)->u.cval)
#define Asc_argItemNameString(aip)  ((aip)->u.nval)
#define Asc_argItemTypeName(aip)    ((aip)->u.nval)
#define Asc_argItemFileString(aip)  ((aip)->u.fval)
#define Asc_argItemListValue(aip)   ((aip)->u.lval)
/**<
 * All of the data in or referenced directly by an argItem
 * should not be changed or destroyed by the user proc.
 * The only exception is that the value pointed to by argItemDoublePtr
 * or argItemLongPtr items may be changed.
 */

/* Asc_AddUserMethod is not used and has been removed from this header. We are
	using CreateUserFunctionMethod instead. */

/* Asc_DeleteUserMethod was not being used and has been removed from this
	header. Not sure what we are using instead, actually. */

/** Look up the brief help text for a named external method. */
extern CONST char *Asc_UserMethodDescription(char *mname);
/**<
	@return description oneliner help string for a named method, or NULL if mname unknown.
	@param mname name of the method
*/


/**	Loop up detailed help text for a named external method. */
extern CONST char *Asc_UserMethodDetails(char *mname);
/**<
	@return long descriptive help text for the named method, or NULL if mname unknown.
	@param mname name of the method
*/


/** Get a list of all defined user methods in the user packages library. */
extern struct gl_list_t *Asc_UserMethodsDefined(void);
/**< 
	@return list of const char* pointers where registered method names are stored.

 	The caller should gl_destroy(mlist) when done with looking at it.
*/

/** Give usage info for a named user method */
extern int Asc_UserMethodArguments(FILE *fp, char *mname);
/**< 
	@param fp file to which the output should be written
	@param mname method name for which details are required

	Writes a synopsis of the arguments required, based on information
	derived from the parse-string to fp.
*/

/*-------------------------------------------------------
	This whole header is junk below here. Temporarily, it is functioning
	extern * junk. We need to reimplement some and scrap the rest.
*/

/**	@file packages.h
	Interface to external packages
	@see @ref packagespage "Packages"
*/

/** @page packagespage

This file implements an interface to external packages. These external packages may be invoked from the procedural section, say for doing external calculations, querying a database, and writing/ reading values from the instance tree. They may also be used in the declarative section for providing relations to augment those in the declarative section. These may be full models/ solvers capable of solving themselves or smarter models/matrix routines capable of operating in a single step mode. [This overloading has caused a protocol that serves neither very well.] The rest of the discussion here is based on packages in the procedural section, and packages in the declarative section that will solve themselves to completion at each major iteration of the solution scheme in which they are imbedded.

@section procedural Procedural section

The packages provided in the procedural section follow the 'foreign pointer' concept; i.e., they will be handed a pointer to a reference instance, and a list of list of arguments. They need to provide only the number of input arguements, and may freely manipulate the instance tree and whatever way they feel. The only *mandatory* function must respect the following function prototype:

@code
int (*proc) (struct BBoxInterp *interp
		,struct Instance *reference
		,struct gl_list_t *arglist
);
@endcode

An optional help string may be provided. We will *make a copy* of this string, if non-null. When invoked this package must do whatever arg checking that is necessary, do its thing, and return a nonzero error result if there are any errors.

@section declarative Declarative section

More information is need from the packages in the declarative section that provide relations. In particular a *presolve/init* routine must be provided, which the compiler/solver will invoke to make sure that all is ok. This package when registering itself has to provide: n_input_args -- the number of input args on its arglist. n_output_args -- the number of output args on its arglist. In addition to the presolve/init routine, the following functions for doing function and jacobian evaluations have to be provided. The deriv (jacobian evaluation routine) and higher order derivative routines may be left NULL, and finite difference via repeated function calls will be done. The calling protocal for all functions is as follows:

@code
int (*init) (struct BBoxInterp *interp,
              struct Instance *model_data,
              struct gl_list_t *arglist);

int (*value) (struct BBoxInterp *interp,
              int ninputs, int noutputs,
       double *inputs, double *outputs,
       double *jacobian);

int (*deriv) (struct BBoxInterp *interp,
              int ninputs, int noutputs,
       double *inputs, double *outputs,
       double *jacobian);
@endcode

deriv2 and any higher order derivatives that come along will follow same calling protocol as for value and deriv.

@note The arguement model_data is provided for external packages that need to get additional information from the instance tree so as to do there computations. This information, should ideally be retrieved during exection of the init function and cached away.

@section interpreter The Interpreter

The interpreter structure is the means of communication between external packages and the ASCEND instance tree and/or solvers. It is particularly necessary in the case of packages in the declarative section. The state of the bit flags must be monitored and appropriate action taken. In the case of an error, a nonzero result must be returned from all functions, and the reason indicated by writing to the interp->status field. At the moment 1 wild hook is provided for the convenience of users. A user may attach an object to this hook and it will be passed around to each of its function calls. As an example consider the following code fragment.

@code
int init(struct BBoxInterp *interp
	, struct Instance *data
    ,struct gl_list_t *arglist)
{
	struct my_solve_system *system;
	double *my_inputs;
	if(interp->firstcall){
		system = my_presolve(data,arglist);

		if (system!=NULL) {
			my_inputs = MakeVectorFromList(arglist);
			system->inputs = my_inputs;
			interp->user_data = (void *)system;
		}else{
			interp->status = calc__error;
			return 1;
		}
	}else{
		destroy_sys((my_solve_system *)interp->user_data);
		interp->user_data = NULL;
	}
	interp->status = calc_all_ok;
	return 0;
}
@endcode

The arglist and all references to instances have been intentionally removed from the value (function evaluation) and deriv* routines so as to allow ease of interfacing to existing standalone packages.
*/

/** Reset the interpreter to its initial state. */
extern void Reset_BBoxInterp(struct BBoxInterp *interp);
/*
	@deprecated { Needs revising @see packages.h }
*/

/**  Register all statically-linked user packages */
extern void AddUserFunctions(void);
/**< 
	@deprecated { Needs revising @see packages.h }

	If the compiler-time flag STATIC_PACKAGES was set, this function should load any statically linked packages into the user packages library. If the flag was not set, it should do nothing.
*/

#ifdef TEST_RELOCATE
/** Calculate an external relation */
extern int CallExternalProcs(struct Instance *i);
/**<
	@deprecated { Needs revising @see packages.h }

	This function given a handle to a relation instance which represents an external relation, will attempt to invoke it and write the results to stdout.
*/
#endif

extern int package_load(CONST char *partialpath, CONST char *initfunc);
/**< 
	Generalised loading of external libraries. This allows for the possibility of 
	libraries being either external shared libraries (DLL or SO files) or external scripts
	of types registered using importhandler_add().

	@param name the short name of the library, as well as partial directory path, optionally
	@param initfunc name of the registration function, or NULL.
	@return 0 if success, 1 if failure.

	@note
	This function was previously deprecated but now it is positively encouraged!

	If the registration function name is not specified, it will be auto-generated by the importhandler's importfn.

	In the case of external DLL/SO files, the registration function must be visible in your DLL/SO and have the funciton prototype:
	
	@code void Routine_register(void); @endcode
*/

extern void Init_BBoxInterp(struct BBoxInterp *interp);
/**<
	@deprecated { Needs revising @see packages.h }

	Gets the interpreter back to a 'clean' state. The default settings are guaranteed to be as follows:
	  @li nodestamp = 0;
	  @li status = calc_all_ok;
	  @li first_call  = (unsigned)0;
	  @li last_call   = (unsigned)0;
	  @li check_args = (unsigned)0;
	  @li recalculate = (unsigned)0;
	  @li deriv_eval = (unsigned)0;
	  @li func_eval = (unsigned)0;
*/

/* @} */

#endif /* ASC_PACKAGES_H */

