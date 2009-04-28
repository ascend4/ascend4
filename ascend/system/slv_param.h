/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005-2007 Carnegie Mellon University

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
*//** @file 
	Solver parameters interface
	@seepage solver-parameters
*//*
	This file split out of slv_common.h -- JP, Jan 2007
*/

#ifndef ASC_SLV_PARAM_H
#define ASC_SLV_PARAM_H

/** @page solver-parameters Solver Parameters in ASCEND

	@NOTE There is a new syntax available for setting solver parameters
	that has not yet been documented here. See slv_common.h (~line 280) and
	also solver/ida.c for examples. @ENDNOTE

	When used together the parameter-related structures, functions, and
	macros allow us to define all of a solver's parameters in one file
	and notify the interface of these parameters upon startup (dynamic
	interface construction).  The parameters can be defined in any order.
	The only bookkeeping needed is associated with the macros.  You must
	have an array of void pointers large enough for all of the macros
	you define and you must give each of the macros you define a unique
	element of this array. Here is an example using a real parameter
	and a character parameter. (The int and bool are similar to the real).

	@code

	(* these 4 macros can be defined anywhere more or less so long as it
	is before the calls to slv_define_parm. *)
	#define REAL_PTR (sys->parm_array[0])
	#define REAL     ((*(real64 *)REAL_PTR))
	#define CHAR_PTR (sys->parm_array[1])
	#define CHAR     ((*(char **)CHAR_PTR))

	#define PA_SIZE 2
	struct example {
	  struct slv_parameters_t p;
	  void *parm_array[PA_SIZE];
	  struct slv_parameter padata[PA_SIZE];
	} e;
	 ...
	  e.p.parms = padata;
	  e.p.dynamic_parms = 0;

	static char *character_names[] = {
	   "name_one","name_two"
	}
	  (* fill padata with appropriate info *)
	slv_define_parm(&(e.p), real_parm,
	                "r_parm","real parameter" ,
	                "this is an example of a real parameter" ,
	                U_p_real(val,25),U_p_real(lo,0),U_p_real(hi,100),1);
	 (* now assign the element of e.parm_array from somewhere in padata *)
	SLV_RPARM_MACRO(REAL_PTR,parameters);

	  (* fill padata with appropriate info *)
	slv_define_parm(&(e.p), char_parm,
	                "c_parm", "character parameter",
	                "this is an example of a character parameter",
	                U_p_string(val,character_names[0]),
	                U_p_strings(lo,character_names),
	                U_p_int(hi,sizeof(character_names)/sizeof(char *)),1);
	 (* now assign the element of e.parm_array that matches. *)
	SLV_CPARM_MACRO(CHAR_PTR,parameters);

	Resetting the value of a parameter can be done directly
	except for string parameters which should be set with, for example,
	slv_set_char_parameter(CHAR_PTR,newvalue);
	or outside a solver where there is no sys->parm_array:

	   slv_set_char_parameter(&(p.parms[i].info.c.value),argv[j]);

	@endcode
*/


#include <utilities/config.h>
#include <utilities/ascConfig.h>

/**	@addtogroup system System
	@{
*/


/*------------------------------------------------------------------------------
  DATA STRUCTURES
*/

/** Solver output file informationn. */
struct slv_output_data {
   FILE *more_important;  /**< More significant output to this file stream.  NULL ==> no output. */
   FILE *less_important;  /**< Less significant output to this file stream.  NULL ==> no output. */
};

/**
	Solver tolerance data structure.
	@todo KHACK THIS SHOULD BE REMOVED - solver/slv_common:slv_tolerance_data.
 */
struct slv_tolerance_data {
   real64 drop;         /**< Matrix entry drop tolerance during factorization */
   real64 pivot;        /**< Detect pivot too small, of those available. */
   real64 singular;     /**< Detect matrix numerically singular. */
   real64 feasible;     /**< Detect equality relations satisfied. */
   real64 rootfind;     /**< Detect single equality relation satisfied. */
   real64 stationary;   /**< Detect lagrange stationary. */
   real64 termination;  /**< Detect progress diminished. */
};

/** Solver sub-parameter data structure. */
struct slv_sub_parameters {
   /* arrays of parametric data */
   int32   *iap;      /**< Array of parametric int32 data. */
   real64  *rap;      /**< Array of parametric real64 data. */
   char*   *cap;      /**< Array of parametric char* data. */
   void*   *vap;      /**< Array of parametric void* data. */
   /* symbolic parameter names */
   char* *ianames;    /**< Array of symbolic names for iap parameters. */
   char* *ranames;    /**< Array of symbolic names for rap parameters. */
   char* *canames;    /**< Array of symbolic names for cap parameters. */
   char* *vanames;    /**< Array of symbolic names for vap parameters. */
   /* longer explanations of the parameter data */
   char* *iaexpln;    /**< Array of longer descriptions of iap parameters. */
   char* *raexpln;    /**< Array of longer descriptions of rap parameters. */
   char* *caexpln;    /**< Array of longer descriptions of cap parameters. */
   char* *vaexpln;    /**< Array of longer descriptions of vap parameters. */
   /* lengths of arrays above */
   int32 ilen;        /**< Length of iap, ianames, and iaexpln arrays. */
   int32 rlen;        /**< Length of rap, ranames, and raexpln arrays. */
   int32 clen;        /**< Length of cap, canames, and caexpln arrays. */
   int32 vlen;        /**< Length of vap, vanames, and vaexpln arrays. */
};

/** Integer solver parameter substructure. */
struct slv_int_parameter {
  int32 value;            /**< Value. */
  int32 low;              /**< Lower bound. */
  int32 high;             /**< Upper bound. */
};

/** Boolean solver parameter substructure. */
struct slv_boolean_parameter {
  int32 value;            /**< Value. */
  int32 low;              /**< Lower bound. */
  int32 high;             /**< Upper bound. */
};

/** Real solver parameter substructure. */
struct slv_real_parameter {
  double value;           /**< Value. */
  double low;             /**< Lower bound. */
  double high;            /**< Upper bound. */
};

/** Char solver parameter substructure. */
struct slv_char_parameter {
  char *value;            /**< Selected value. */
  char **argv;            /**< Array of possible values */
  int32 high;             /**< Length of array of possible values. */
};

/** Basic solver parameter types. */
enum parm_type {
  int_parm,                 /**< Integer type. */
  bool_parm,                /**< Boolean type. */
  real_parm,                /**< Real type. */
  char_parm                 /**< Char type. */
};

/** Parameter arguments */
union parm_arg
{
  char **argv;              /**< Strings array argument. */
  char *argc;               /**< Char argument. */
  int32 argi;               /**< Integer argument. */
  int32 argb;               /**< Boolean argument. */
  real64 argr;              /**< Real argument. */
};

/** Solver parameter structure. */
struct slv_parameter {
  enum parm_type type;              /**< Parameter type. */
  int32 number;                     /**< Index in array. */
  int32 display;                    /**< Display page. */
  char *name;                       /**< Scripting short name. */
  char *interface_label;            /**< User interface label. */
  char *description;                /**< Modest help string. */
  union {
    struct slv_int_parameter i;     /**< Integer parameter. */
    struct slv_boolean_parameter b; /**< Boolean parameter. */
    struct slv_real_parameter r;    /**< Real parameter. */
    struct slv_char_parameter c;    /**< Char parameter. */
  } info;                           /**< Data. */
};

/*------------------------------------------------------------------------------
  SOME STRUCTURES FOR SANER INITIALISATION OF PARAMETERS (says I -- JP)
*/

typedef struct{
	const char *codename;
	const char *guiname;
	const int guipagenum;
	const char *description;
} SlvParameterInitMeta;
	
typedef struct{
	const SlvParameterInitMeta meta;
	const int val;
	const int low;
	const int high;
} SlvParameterInitInt;

typedef struct{
	const SlvParameterInitMeta meta;
	const int val;
} SlvParameterInitBool;

typedef struct{
	const SlvParameterInitMeta meta;
	const double val;
	const double low;
	const double high;
} SlvParameterInitReal;

typedef struct{
	const SlvParameterInitMeta meta;
	const char *val;
	/* list of options will be passed in separately; seems not possible to have static array here */
} SlvParameterInitChar;

struct slv_parameters_structure;

ASC_DLLSPEC int slv_param_int (struct slv_parameters_structure *p, const int index, const SlvParameterInitInt);
ASC_DLLSPEC int slv_param_bool(struct slv_parameters_structure *p, const int index, const SlvParameterInitBool);
ASC_DLLSPEC int slv_param_real(struct slv_parameters_structure *p, const int index, const SlvParameterInitReal);
ASC_DLLSPEC int slv_param_char(struct slv_parameters_structure *p, const int index, const SlvParameterInitChar, char *options[]);

/* macros to access values from your solver code 

	Usage example:
		if(SLV_PARAM_BOOL(p,IDA_PARAM_AUTODIFF)){
			// do something
		}
		SLV_PARAM_BOOL(p,IDA_PARAM_AUTODIFF) = FALSE;
*/

/* the first three are read/write */
#define SLV_PARAM_INT(PARAMS,INDEX)  (PARAMS)->parms[INDEX].info.i.value
#define SLV_PARAM_BOOL(PARAMS,INDEX) (PARAMS)->parms[INDEX].info.b.value
#define SLV_PARAM_REAL(PARAMS,INDEX) (PARAMS)->parms[INDEX].info.r.value

#define SLV_PARAM_CHAR(PARAMS,INDEX) (PARAMS)->parms[INDEX].info.c.value
/**<
	@NOTE, don't use this macro to set the value of your string, as it will
	result in memory leaks
*/

/*------------------------------------------------------------------------------
  ACCESSOR MACROS for parm_arg unions

 * Macros for parm_arg unions.
 * Sets appropriate member (parm_u) of the union to the
 * value specified (val) and returns (parm_u).
 * (parm_u) should be one of val, lo, or hi.
 * These macros are used in calls to the
 * slv_define_parm function defined below.
 */

#define U_p_int(parm_u,val)     ((((parm_u).argi = (val))), (parm_u))
/**<
	Sets the argi of parm_arg parm_u to val and returns the parm_u.
	This macro is used for setting integer parm_arg arguments in calls
	to slv_define_parm().  parm_u should be one of { val, lo, hi },
	which correspond to local parm_arg variables that should be used
	in client functions calling slv_define_parm().
 *
	@param parm_u The parm_arg to modify, one of {val, lo, hi}.
	@param val    int, the new value for the parm_arg.
	@return Returns parm_u.
 */
#define U_p_bool(parm_u,val)    ((((parm_u).argb = (val))), (parm_u))
/**<
	Sets the argb of parm_arg parm_u to val and returns the parm_u.
	This macro is used for setting boolean parm_arg arguments in calls
	to slv_define_parm().  parm_u should be one of { val, lo, hi },
	which correspond to local parm_arg variables that should be used
	in client functions calling slv_define_parm().
 *
	@param parm_u The parm_arg to modify, one of {val, lo, hi}.
	@param val    boolean, the new value for the parm_arg.
	@return Returns parm_u.
 */
#define U_p_real(parm_u,val)    ((((parm_u).argr = (val))), (parm_u))
/**<
	Sets the argr of parm_arg parm_u to val and returns the parm_u.
	This macro is used for setting real parm_arg arguments in calls
	to slv_define_parm().  parm_u should be one of { val, lo, hi },
	which correspond to local parm_arg variables that should be used
	in client functions calling slv_define_parm().
 *
	@param parm_u The parm_arg to modify, one of {val, lo, hi}.
	@param val    double, the new value for the parm_arg.
	@return Returns parm_u.
 */
#define U_p_string(parm_u,val)  ((((parm_u).argc = (val))), (parm_u))
/**<
	Sets the argc of parm_arg parm_u to val and returns the parm_u.
	This macro is used for setting string parm_arg arguments in calls
	to slv_define_parm().  parm_u should be one of { val, lo, hi },
	which correspond to local parm_arg variables that should be used
	in client functions calling slv_define_parm().
 *
	@param parm_u The parm_arg to modify, one of {val, lo, hi}.
	@param val    char *, the new value for the parm_arg.
	@return Returns parm_u.
	For use in calls to slv_define_parm().
 */
#define U_p_strings(parm_u,val) ((((parm_u).argv = (val))), (parm_u))
/**<
	Sets the argv of parm_arg parm_u to val and returns the parm_u.
	This macro is used for setting string array parm_arg arguments in
	calls to slv_define_parm().  parm_u should be one of { val, lo, hi },
	which correspond to local parm_arg variables that should be used
	in client functions calling slv_define_parm().
 *
	@param parm_u The parm_arg to modify, one of {val, lo, hi}.
	@param val    char **, the new value for the parm_arg.
	@return Returns parm_u.
	For use in calls to slv_define_parm().
 */

#define SLV_IPARM_MACRO(NAME,slv_parms) \
  if (make_macros == 1) {  \
     (NAME)  = &((slv_parms)->parms[(slv_parms)->num_parms-1].info.i.value); \
  }
/**<
	Macro for defining macros of type integer (IPARM).
	See SLV_CPARM_MACRO() for more information.
 */
#define SLV_BPARM_MACRO(NAME,slv_parms) \
  if (make_macros == 1) {  \
     (NAME)  = &((slv_parms)->parms[(slv_parms)->num_parms-1].info.b.value); \
  }
/**<
	Macro for defining macros of type boolean (BPARM).
	See SLV_CPARM_MACRO() for more information.
 */
#define SLV_RPARM_MACRO(NAME,slv_parms) \
  if (make_macros == 1) {  \
     (NAME)  = &((slv_parms)->parms[(slv_parms)->num_parms-1].info.r.value); \
  }
/**<
	Macro for defining macros of type real (RPARM).
	See SLV_CPARM_MACRO() for more information.
 */
#define SLV_CPARM_MACRO(NAME,slv_parms) \
  if (make_macros == 1) {  \
     (NAME)  = &((slv_parms)->parms[(slv_parms)->num_parms-1].info.c.value); \
  }
/**<
 * Macro for defining macros of type character (CPARM).
 * To use, provide a NAME for the macro (in caps by convention)
 * and a slv_parameters_t pointer (slv_parm). The NAME should be
 * defined as an element in an array of void pointers in the
 * module in which the macro is to be used.  This macro uses the
 * current number of registered parameters to link the array of
 * _VOID_ _POINTERS_ to the correct parameters.  If you want to create
 * a macro for a parameter, you should put the appropriate macro
 * creating macro IMEDIATELY after the call to slv_define_parm
 * for that parameter.<br><br>
 * Local int make_macros; must be defined.
 */

/*------------------------------------------------------------------------------
  SOLVER PARAMETERS STRUCT & METHODS
*/
/**
	Holds the array of parameters and keeps a count of how many it
	contains.  Also holds various other information which should be
	turned into slv_parameters or moved elsewhere
	<pre>
	Every registered client should have a slv_parameters_t somewhere in it.

	The following is a list of parameters (those parameters that can be
	modified during solve without calling slv_presolve() are marked with
	"$$$").  It should be noted that some solvers may not be conformable
	to some of the parameters.  Default values are subject to change via
	experimentation.

	output.more_important (default stdout):   $$$
	output.less_important (default NULL):     $$$
	   All output from the solver is written to one of these two files
	   (except bug messages which are written to stderr).  Common values
	   are NULL (==> no file) and stdout.  The more important messages
	   go to output.more_important and less important messages go to
	   output.less_important.  To shut the solver up, set both files to
	   NULL.

	tolerance.drop         (default 1e-16):
	tolerance.pivot        (default 0.1):
	tolerance.singular     (default 1e-12):
	tolerance.feasible     (default 1e-8):
	tolerance.rootfind     (default 1e-12):
	tolerance.stationary   (default 1e-8):
	tolerance.termination  (default 1e-12):
	   These define the criterion for selecting pivotable relations,
	   whether the equations are satisfied, if a local minimum has been
	   found, or if no further reduction in the augmented lagrange merit
	   phi can be achieved.
	 - During jacobian reduction, each equation pivot selected must be
	   at least a certain fraction given by TOLERANCE.PIVOT of the largest
	   available.
	   Also, the largest value in the row must exceed TOLERANCE.SINGULAR
	   in order to be considered independent.
	 - The absolute value of each unscaled equation residual is compared
	   with TOLERANCE.FEASIBLE in order to determine convergence of the
	   equality constraints during Newton iteration.
	 - The absolute value of each unscaled equation residual is compared
	   with TOLERANCE.ROOTFIND in order to determine convergence of the
	   constraint during rootfinding of single equations.
	 - Detection of a minimum requires the stationary condition of the
	   lagrange to be less than TOLERANCE.STATIONARY.
	 - If the directional derivative of phi along the negative gradient
	   direction using the suggested iteration step length falls below
	   TOLERANCE.TERMINATION, iteration is ceased.
	 - TOLERANCE.DROP is the smallest number magnitude to be allowed
	   in the Jacobian matrix during factorization. Default is optimistic.

	time_limit (default 30.0):   $$$
	   This defines the time limit expressed as cpu seconds per block.
	   If the solver requires more time than this in any given block,
	   then it will stop.

	iteration_limit (default 100):   $$$
	   This defines the maximum number of iterations attempted in a given
	   block.  The solver will stop after this many iterations if it fails
	   to converge.

	factor_option (default 0):
	   This sets the number of the linear factorization to suggest.
	   This does not map directly to linsol numbering of any sort.
	   The map is: 0 <==> RANKI, 1 <==> RANKI_JZ, 2+ <==> ?.
	   The solver is free to ignore this suggestion.
	   In fact, the specific solver is free to define the meaning of factor
	   option depending on what linear packages it can talk to.

	partition (default TRUE):
	   Specifies whether or not the system will be partitioned into blocks
	   or not.  If not, then the system will be considered as one large
	   block.

	ignore_bounds (default FALSE):
	   Specifies whether or not bounds will be considered during solving.
	   WARNING: if this flag is set, there will be no guarantees that the
	   solution will lie in bounds.  Suggested use might be to set this
	   flag to TRUE, solve, reset this flag to FALSE, and resolve.
	   More often than not, in fact, ignore bounds will lead to floating
	   point exceptions, halting the solution process.

	rho (default 1.0):
	   Used as a scalar pre-multiplier of the penalty term quantified by one
	   half the two norm of the equality constraint residuals in an
	   augmented lagrange merit function.

	sp.ia/ra/ca/vap (defaults NULL, READ ONLY):
	   Is a set of pointers to arrays (int/double/(char*)/void*).
	   The values of the pointers themselves should not be modified,
	   though the values pointed at may be modified. Note that this is
	   _direct_ modification and will take effect immediately, not on
	   the next call to slv_set_parameters. When the engine gets around
	   to looking at the values in these arrays is engine dependent.
	   NULL is the expected value for some or all of these array
	   pointers, depending on the engine. The sizes of these arrays are
	   specific to each solver's interface. As being of interest (at
	   compile time) to both the slvI.c file and the GUI/CLUI, the
	   sizes of the arrays to be pointed to are part of the slvI.h file.
	   The implementor of each slvI.c should take care to use as much of
	   the slv_parameters_t as possible before passing data through the
	   arrays provided in the sub_parameters. This will make for a
	   minimal amount of work when adding an engine to the GUI/CLUI.
	   To further aid reusability/sanity preservation, slvI.h should
	   be appended with proper defines for subscripting these arrays.

	sp.i/r/c/vlen (defaults 0, READ ONLY)
	   lengths of the sub_parameter arrays.

	sp.ia/ra/ca/vanames (defaults NULL, READONLY)
	   symbolic names for the corresponding entries in ia/ra/ca/vap.

	sp.ia/ra/ca/vaexpln (defaults NULL, READONLY)
	   longer explanations for the corresponding entries in ia/ra/ca/vap.

	whose (default 0=>slv0, READ ONLY)
	   This tells where a parameter set came from, since the default
	   action of slv_get_parameters is to return a copy of slv0's
	   parameters if the parameters asked for don't exist because
	   the solver in question wasn't built/linked.
	</pre>
 */
typedef struct slv_parameters_structure {
   struct slv_output_data output;       /**< File streams for solver output. */
   struct slv_tolerance_data tolerance; /**< Defince various tolerances for the solver. */
   struct slv_parameter *parms;         /**< Holds the parameters defined for a solver. */
   int32 num_parms;                     /**< The number of parameters in parms. */
   int32 dynamic_parms;                 /**< Set to TRUE if parms is dynamically allocated. */

   /* we wish the following were on the way out */
   struct slv_sub_parameters sp;        /**< Solver sub-parameters. */
   int whose;                           /**< Code for where a parameter set came from. */
   int32 ignore_bounds;                 /**< Set to TRUE to disregard boundary conditions. */
   int32 partition;                     /**< Set to TRUE if system will be partitioned into blocks. */

   /* the following are on the way out */
   double time_limit;     /**< Max cpu seconds per block.                   @todo kill */
   double rho;            /**< Scaler premultiplier of penalty term.        @todo kill */
   int32 iteration_limit; /**< Max number of iterations.                    @todo kill */
   int32 factor_option;   /**< Suggests a number for linear factorization.  @todo kill */

} slv_parameters_t;


/* slv_destroy_parms() is defined in slv.c */
ASC_DLLSPEC void slv_destroy_parms(slv_parameters_t *p);
/**<
	Deallocates any allocated memory held by a parameter structure.

	   * All the 'meta' strings are freed, as they are allocated using ASC_STRDUP.
	   * String values and option arrays are

	Only the held memory is freed, not p itself.  Further, if
	(p->dynamic_parms != 0), the strings in p->parms are freed
	but not p->parms itself.  Does nothing if p is NULL.

	@NOTE the above description does not appear to be correct! check the code!

	@param p  The parameter structure to destroy.
 */

/* slv_define_parm() is defined in slv.c */
ASC_DLLSPEC int32 slv_define_parm(slv_parameters_t *p,
                             enum parm_type type,
                             char *interface_name,
                             char *interface_label,
                             char *description,
                             union parm_arg value,
                             union parm_arg lower_bound,
                             union parm_arg upper_bound,
                             int32 page);
/**<
	@deprecated (use slv_param_bool and similar instead --JP)

	Adds (defines) a new parameter in a parameter structure.
	Use this function to add & define new parameters for a solver.

	@param p                Parameter structure to receive the new parameter.
	@param type             Parameter type: int_parm, bool_parm, real_parm, or char_parm.
	@param interface_name   A very short but descriptive name that the interface
	                        can use to identify the parameter.
	@param interface_label  A short text string to be displayed on the interface.
	@param description      A slightly more detailed string to be displayed
	                        upon request a la balloon help.
	@param value            The value for the parameter, set using one of
	                        the U_p_int() style macros defined above.
	@param lower_bound      The lower bound for the parameter, set using one of
	                        the U_p_int() style macros defined above.
	@param upper_bound      The upper bound for the parameter, set using one of
	                        the U_p_int() style macros defined above.
	@param page             The page of the interface options dialog on which
	                        to display this parameter.  Ranges from 1..max_page_no.
	                        Set to -1 if this parameter is not to be displayed in
	                        the interface.
	@return Returns -1 if p is NULL or called with unsupported type;
	        otherwise returns the number of registered parameters in p.
 */

/* slv_set_char_parameter() is defined in slv.c */
ASC_DLLSPEC void slv_set_char_parameter(char **cptr, CONST char *newvalue);
/**<
	Sets a char parameter value to a new string.
	Resetting the value of a parameter can be done directly except
	for string parameters which must be set with this function.  The
	string newvalue is not kept by the function.<br><br>

	Example:   slv_set_char_parameter(&(p.parms[i].info.c.value),argv[j]);

	@param cptr     Pointer to the char array to set.
	@param newvalue New value for *cptr.
*/

/* @} */

#endif  /* ASC_SLV_PARAM_H */

