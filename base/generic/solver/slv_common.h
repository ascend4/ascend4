/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005-2006 Carnegie Mellon University

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
	SLV common utilities & structures for ASCEND solvers.

	Routines in this header are applicable to both the system API (as accessed
	from ASCEND compiler and GUI/CLI) as well as the solver backend (slv3.c, 
	and other solvers, etc)

	This header therefore includes the following:
	  - parameters struct definitions & manipulation routines
	  - status struct definitions & retrieval routines
	  - vector operations
	  - solver print routines
	  - lnkmap support functions

	@see slv_client.h for the routines that a concrete SLV solver will use to access the model.
	@see slv_server.h for the routines that ASCEND uses to run and query the solver.

	@NOTE
	USAGE NOTES:
	slv.h is the header for folks on the ASCEND end, and this is the one for
	folks on the Slv math end.
	Don't protoize this file for ASCEND types other than mtx, vec, and boolean
	real64, and int32 or we'll have you shot. In particular, not var and rel.
	People who aren't supposed to know about var and rel include this.

	In particular, this header may be used without knowing about the ASCEND
	compiler or any of its annoying insanities so long as you drag out
	ascmalloc().
	This does commit you to being able to stomach the mtx.h file, however,
	even if you choose to ignore the contents of mtx.
	Several functions, notably the print suite for rel/var names,
	assume you are linking against something that does know about
	ASCEND instances unless the SLV_INSTANCES flag is set to FALSE.

	The parameters and status struct definitions have been moved here,
	being of general interest.
	@ENDNOTE

	Requires:
	#include <stdio.h>
	#include <utilities/ascConfig.h>
	#include <solver/slv_types.h>
	#include <solver/rel.h>
	#include <solver/logrel.h>
	#include <solver/mtx.h>
	#include <general/list.h>
*/

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
*//*
 	Abstracted from
	slvX.c January 1995. Based on the original slv.h by KW and JZ (01/94), by Ben Allan.
*/

#ifndef ASC_SLV_COMMON_H
#define ASC_SLV_COMMON_H

/**	@addtogroup solver Solver
	@{
*/

#include <utilities/ascConfig.h>

#undef SLV_INSTANCES
#define SLV_INSTANCES TRUE
/**< SLV_INSTANCES should only be FALSE in a libasc.a free environment */

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

/**
	Data structure for solver statistics.
	This is to collect data for the comparison of algorithms.  All solvers
	should have at least one of these, though the interface will check for
	NULL before reading the data.  The interpretation of these data is
	somewhat up to the coder.
 */
struct slv_block_cost {
  int32 size,             /**< How big is the block, in terms of variables? */
        iterations,       /**< How many iterations to convergence/divergence? */
        funcs,            /**< How many function evaluations were made? */
        jacs,             /**< How many jacobian evaluations were made? */
        reorder_method;   /**< Not documented. Up to individual solver? */
  double time,            /**< How much cpu total time elapsed while in the block? */
         resid,           /**< Not documented.  The size of the residual? */
         functime,        /**< Time spent in function evaluations. */
         jactime;         /**< Time spent in jacobian evaluations, stuffing. */
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

int slv_param_int (struct slv_parameters_structure *p, const int index, const SlvParameterInitInt);
int slv_param_bool(struct slv_parameters_structure *p, const int index, const SlvParameterInitBool);
int slv_param_real(struct slv_parameters_structure *p, const int index, const SlvParameterInitReal);
int slv_param_char(struct slv_parameters_structure *p, const int index, const SlvParameterInitChar, char *options[]);

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

	   * All the 'meta' strings are freed, as they are allocated using ascstrdup.
	   * String values and option arrays are

	Only the held memory is freed, not p itself.  Further, if
	(p->dynamic_parms != 0), the strings in p->parms are freed
	but not p->parms itself.  Does nothing if p is NULL.

	@NOTE the above description does not appear to be correct! check the code!

	@param p  The parameter structure to destroy.
 */

/* slv_define_parm() is defined in slv.c */
extern int32 slv_define_parm(slv_parameters_t *p,
                             enum parm_type type,
                             char *interface_name,
                             char *interface_label,
                             char *description,
                             union parm_arg value,
                             union parm_arg lower_bound,
                             union parm_arg upper_bound,
                             int32 page);
/**<
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

/*------------------------------------------------------------------------------
  OVERALL SOLVER STATUS and INDIVIDUAL BLOCK STATUS
*/
/** Solver block status record. */
struct slv__block_status_structure {
   int32 number_of;                 /**< Number of blocks in system. */
   int32 current_block;             /**< Block number of the current block that the
                                         solver is working on.  It is assumed that all
                                         previous blocks have already converged. */
   int32 current_reordered_block;   /**< Number of the block most recently reordered. */
   int32 current_size;              /**< Number of variables/relations in the current block. */
   int32 previous_total_size;       /**< Total size of previous blocks (= number of
                                         variables/relations already converged). */
   int32 previous_total_size_vars;  /**< Not currently implemented. */
   int32 iteration;                 /**< Number of iterations so far in the current block. */
   int32 funcs;                     /**< Number of residuals calculated in the current block. */
   int32 jacs;                      /**< Number of jacobians evaluated in the current block. */
   double cpu_elapsed;              /**< Number of cpu seconds elapsed in the current block. */
   double functime;                 /**< Number of cpu seconds elapsed getting residuals. */
   double jactime;                  /**< Number of cpu seconds elapsed getting jacobians. */
   real64 residual;                 /**< Current residual (RMS value) for the current block. */
};

/**
 *  Solver status flags.
 *  <pre>
 *  The following is a list of statuses and their meanings.  Statuses
 *  cannot be written to, and thus there is no notion of default value.
 *
 *  ok:
 *     Specifies whether or not everything is "ok".  It is a shorthand for
 *     testing all of the other flags.
 *
 *  over_defined:
 *  under_defined:
 *  struct_singular:
 *     Specifies whether the system is over-defined, under-defined, or
 *     structurally singular.  These fields are set by slv_presolve where
 *     the structural analysis is performed.  It should be noted that
 *     over_defined and under_defined are mutually exclusive and both
 *     imply struct_singular, although a system can be structurally
 *     singular without being over-defined or under-defined.
 *
 *  ready_to_solve:
 *     Specifies whether the system is ready to solve.  In other words, is
 *     slv_iterate or slv_solve legal?  This flag is FALSE before
 *     slv_presolve or after the system has converged or the solver has
 *     given up for whatever reason.
 *
 *  converged:
 *     This flag is set whenever the entire system has converged.  The
 *     convergence will be genuine (all relations satisfied within
 *     tolerance, all bounds satisfied, all calculations defined, etc.).
 *
 *  diverged:
 *     This flag is set whenever the solver has truly given up (i.e. given
 *     up for any reason not covered below).
 *
 *  inconsistent:
 *     The solver has concluded unambiguously (e.g. by symbolic
 *     manipulation) that the system is inconsistent.
 *
 *  calc_ok:
 *     Specifies whether or not there were any calculation errors in
 *     computing the residuals at the current point.
 *
 *  iteration_limit_exceeded:
 *     Specifies whether or not the iteration count was exceeded or not.
 *
 *  time_limit_exceeded:
 *     Specifies whether or not the cpu time limit was exceeded.
 *
 *  panic:
 *     Specifies whether or not the user called a halt interactively;
 *
 *  iteration:
 *     Total number of iterations so far.  Total iteration count is reset to
 *     zero whenever slv_presolve or slv_resolve is called.
 *
 *  cpu_elapsed:
 *     Total number of cpu seconds elapsed.  Total cpu time elapsed is reset
 *     to zero whenever slv_presolve or slv_resolve is called.
 *
 *  block.number_of:
 *     Number of blocks in system.
 *
 *  block.current_block:
 *     Block number of the current block that the solver is working on.
 *     It is assumed that all previous blocks have already converged.
 *
 *  block.current_size:
 *     Number of variables/relations in the current block.
 *
 *  block.previous_total_size:
 *     Total size of previous blocks (= number of variables/relations
 *     already converged).
 *
 *  block.iteration:
 *     Number of iterations so far in the current block.
 *
 *  block.functime:
 *     Number of cpu seconds elapsed getting residuals from whereever.
 *
 *  block.jactime:
 *     Number of cpu seconds elapsed getting jacobians from whereever.
 *
 *  block.cpu_elapsed:
 *     Number of cpu seconds elapsed so far in the current block.
 *
 *  block.residual:
 *     Current residual (RMS value) for the current block.
 *
 *  cost (READ ONLY)
 *    This is a pointer to first of an array which is costsize long of
 *    slv_block_cost structures. This is to collect data for the
 *    comparison of algorithms. All solvers should have at least
 *    one of these, though the interface will check for null before
 *    reading the data. The block_cost structure contains:
 *      size       (how big is the block, in terms of variables)
 *      iterations (how many iterations to convergence/divergence)
 *      funcs      (how many function evaluations were made?)
 *      jacs       (how many jacobian evaluations were made?)
 *      time       (how much cpu total time elapsed while in the block?)
 *      functime   (time spent in function evaluations)
 *      jactime    (time spent in jacobian evaluations, stuffing)
 *                 (for those codes where a function evaluation is
 *                  a byproduct of gradient evaluation, the func cost
 *                  will be billed here.)
 *    The interpretation of these data is somewhat up to the coder.
 *
 *  costsize
 *    This is how big the cost array is. It should in general be the
 *    number of blocks in the system plus 1 so that all the unincluded
 *    relations can be billed to the blocks+1th cost if they are
 *    evaluated.
 *  </pre>
 */
typedef struct slv_status_structure {
   uint32 ok : 1;                       /**< If TRUE, everything is ok. */
   uint32 over_defined : 1;             /**< Is system over-defined? */
   uint32 under_defined : 1;            /**< Is system under-defined? */
   uint32 struct_singular : 1;          /**< Is system structurally singular? */
   uint32 ready_to_solve : 1;           /**< Is system ready to solve? */
   uint32 converged : 1;                /**< Has system fully convergeded? */
   uint32 diverged : 1;                 /**< Has system diverged? */
   uint32 inconsistent : 1;             /**< Was system was found to be inconsistent? */
   uint32 calc_ok : 1;                  /**< Were any errors encounted calculating residuals? */
   uint32 iteration_limit_exceeded : 1; /**< Was the iteraction limit exceeded? */
   uint32 time_limit_exceeded : 1;      /**< Was the time limit exceeded? */
   uint32 panic :1;                     /**< Did the user stop the solver interactively? */
   int32 iteration;                     /**< Total number of iterations so far. */
   int32 costsize;                      /**< Number of elements in the cost array. */
   double cpu_elapsed;                  /**< Total elapsed cpu seconds. */
   struct slv_block_cost *cost;         /**< Array of slv_block_cost records. */
   struct slv__block_status_structure block;  /**< Block status information. */
} slv_status_t;

/* vector math stuff moved to mtx_vector.h */

/*------------------------------------------------------------------------------
  GENERAL INPUT/OUTPUT ROUTINES
*/

ASC_DLLSPEC FILE *slv_get_output_file(FILE *fp);
/**<
 *  Checks a file pointer, and if NULL returns a pointer to the nul device.
 *  If you are in environment that doesn't have something like
 *  /dev/null (nul on Windows), you'd better be damn sure your
 *  sys->p.output.*_important are not NULL.
 *
 *  @param fp The file stream to check.
 *  @return fp if it is not NULL, a pointer to the nul device otherwise.
 */

/*
 * FILE pointer macros.
 *     fp = MIF(sys)
 *     fp = LIF(sys)
 *     fp = PMIF(sys)
 *     fp = PLIF(sys)
 *     or fprintf(MIF(sys),"stuff",data...);
 *  Use of these is requested on grounds of readability but not required.
 *  All of these are macros, which means any specific solver interface
 *  to ASCEND can use them, since all interfaces are supposed to
 *  support a parameters structure p somewhere in a larger system
 *  structure (sys) they keep privately.
 *  Use the PMIF or PLIF flavors if the parameters sys->p is a pointer
 *  rather than a in-struct member.
 */
#define MIF(sys) slv_get_output_file( (sys)->p.output.more_important )
/**<
 *  Retrieve the "more important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "more important" output file for sys.
 */
#define LIF(sys) slv_get_output_file( (sys)->p.output.less_important )
/**<
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "less important" output file for sys.
 */
#define PMIF(sys) slv_get_output_file( (sys)->p->output.more_important )
/**<
 *  Retrieve the "more important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "more important" output file for sys.
 */
#define PLIF(sys) slv_get_output_file( (sys)->p->output.less_important )
/**<
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 *
 *  @param sys The slv_system_t to query.
 *  @return A FILE * to the "less important" output file for sys.
 */

/*===============================================================================
  COMPILER-DEPENDENT FUNCTIONS

  The following functions reach into the data structures in the <compiler>
  section of ASCEND. That means that these functions can't be present in a
  fully split-out and general-purpose SLV engine. 

  If you're trying to use SLV to solve systems other that ASCEND models
  therefore, these functions need to be re-implemented for your case.
*/

#if SLV_INSTANCES

#ifdef NEWSTUFF
extern void slv_print_obj_name(FILE *outfile, obj_objective_t obj);
/**<
 *  Not implemented.
 *  Prints the name of obj to outfile.  If obj_make_name() can't
 *  generate a name, the global index is printed instead.
 *  @todo Implement solver/slv_common:slv_print_obj_name() or remove prototype.
 */
#endif
extern void slv_print_rel_name(FILE *outfile,
                               slv_system_t sys,
                               struct rel_relation *rel);
/**<
 *  Prints the name of rel to outfile.  If rel_make_name() can't
 *  generate a name, the global index is printed instead.
 *
 *  @param outfile The stream to receive the output.
 *  @param sys     The solver system.
 *  @param rel     The relation whose name should be printed.
 *  @todo Move solver/slv_common:slv_print_rel_name() to solver/rel.
 */

extern void slv_print_var_name(FILE *outfile,
                               slv_system_t sys,
                               struct var_variable *var);
/**<
 *  Prints the name of var to outfile. If var_make_name() can't
 *  generate a name, the global index is printed instead.
 *
 *  @param outfile The stream to receive the output.
 *  @param sys     The solver system.
 *  @param var     The variable whose name should be printed.
 *  @todo Move solver/slv_common:slv_print_var_name() to solver/var.
 */

extern void slv_print_logrel_name(FILE *outfile,
                                  slv_system_t sys,
                                  struct logrel_relation *lrel);
/**<
 *  Prints the name of lrel to outfile. If logrel_make_name() can't
 *  generate a name, the global index is printed instead.
 *
 *  @param outfile The stream to receive the output.
 *  @param sys     The solver system.
 *  @param lrel    The logical relation whose name should be printed.
 *  @todo Move solver/slv_common:slv_print_logrel_name() to solver/logrel.
 */

extern void slv_print_dis_name(FILE *outfile,
                               slv_system_t sys,
                               struct dis_discrete *dvar);
/**<
 *  Prints the name of dvar to outfile. If dis_make_name() can't
 *  generate a name, the global index is printed instead.
 *
 *  @param outfile The stream to receive the output.
 *  @param sys     The solver system.
 *  @param dvar    The discrete variable whose name should be printed.
 *  @todo Move solver/slv_common:slv_print_dis_name() to solver/discrete.
 */

#ifdef NEWSTUFF
extern void slv_print_obj_index(FILE *outfile, obj_objective_t obj);
/**<
 *  Not implemented.
 *  Prints the index of obj to outfile.
 *  @todo Implement solver/slv_common:slv_print_obj_index() or remove prototype.
 */
#endif
extern void slv_print_rel_sindex(FILE *outfile, struct rel_relation *rel);
/**<
 *  Prints the index of rel to outfile.
 *
 *  @param outfile The stream to receive the output.
 *  @param rel     The relation whose index should be printed.
 *  @todo Move solver/slv_common:slv_print_rel_name() to solver/rel.
 */

extern void slv_print_var_sindex(FILE *outfile, struct var_variable *var);
/**<
 *  Prints the index of var to outfile.
 *
 *  @param outfile The stream to receive the output.
 *  @param var     The variable whose index should be printed.
 *  @todo Move solver/slv_common:slv_print_var_name() to solver/var.
 */

extern void slv_print_logrel_sindex(FILE *outfile, struct logrel_relation *lrel);
/**<
 *  Prints the index of lrel to outfile.
 *
 *  @param outfile The stream to receive the output.
 *  @param lrel    The logical relation whose index should be printed.
 *  @todo Move solver/slv_common:slv_print_logrel_name() to solver/logrel.
 */

extern void slv_print_dis_sindex(FILE *outfile, struct dis_discrete *dvar);
/**<
 *  Prints the index of dvar to outfile.
 *
 *  @param outfile The stream to receive the output.
 *  @param dvar    The discrete variable whose index should be printed.
 *  @todo Move solver/slv_common:slv_print_dis_name() to solver/discrete.
 */

extern int slv_direct_solve(slv_system_t server,
                            struct rel_relation *rel,
                            struct var_variable *var,
                            FILE *file,
                            real64 epsilon,
                            int ignore_bounds,
                            int scaled);
/**<
 *  Attempts to directly solve the given relation (equality constraint) for
 *  the given variable, leaving the others fixed.  Returns an integer
 *  signifying the status as one of the following three:
 *  <pre>
 *     0  ==>  Unable to determine anything.
 *             Not symbolically invertible.
 *     1  ==>  Solution(s) found.
 *             Variable value set to first found if more than one.
 *    -1  ==>  No solution found.
 *             Function invertible, but no solution exists satisfying
 *             var bounds (if active) and the epsilon given.
 *  </pre>
 *  The variable bounds will be upheld, unless ignore_bounds=FALSE.
 *  Residual testing will be against epsilon and either scaled or
 *  unscaled residual according to scaled (no scale -> 0).
 *  If file != NULL and there are leftover possible solutions, we
 *  will write about them to file.
 *
 *  @param server        The slv_system_t (mostly ignored).
 *  @param rel           The relation to attempt to solve.
 *  @param var           The variable for which to solve.
 *  @param file          File stream to receive other possible solutions.
 *  @param epsilon       Tolerance for testing convergence.
 *  @param ignore_bounds If TRUE, ignore bounds on variable.
 *  @param scaled        If TRUE, test scaled residuals against epsilon.
 *  @todo solver/slv_common:slv_direct_solve() should be in solver/relman
 *        or solver/slv3.
 */

extern int slv_direct_log_solve(slv_system_t sys,
                                struct logrel_relation *lrel,
                                struct dis_discrete *dvar,
                                FILE *file,
                                int perturb,
                                struct gl_list_t *instances);
/**<
 *  Attempt to directly solve the given logrelation for the given
 *  discrete variable, leaving the others fixed.  Returns an integer
 *  signifying the status as one of the following three:
 *  <pre>
 *     0  ==>  Unable to determine anything. Bad logrelation or dvar
 *     1  ==>  Solution found.
 *     2  ==>  More than one solution found. It does not modify the value
 *             of dvar. Conflicting.
 *    -1  ==>  No solution found. Inconsistency
 *  </pre>
 *  If file != NULL and there are leftover possible solutions, we
 *  will write about them to file.
 *  The flag perturb and the gl_list are used to change the truth
 *  value of some boundaries. This is sometimes useful in
 *  conditional modeling.
 *
 *  @param sys        The slv_system_t (mostly ignored).
 *  @param lrel       The logical relation to attempt to solve.
 *  @param dvar       The discrete variable for which to solve.
 *  @param file       File stream to receive other possible solutions.
 *  @param perturb    If TRUE, perturbs the truth values if necessary to find the solution.
 *  @param instances  List of instances.
 *  @todo solver/slv_common:slv_direct_log_solve() should be in solver/logrel
 *        or solver/slv9.
 */

#endif
/* === END compiler dependent functions === */

/*------------------------------------------------------------------------------
  LINK-MAP FUNCTIONS
*/
/**
  @TODO what are these all abount? Something about linking permuted rows
  and columns back to the original data? -- JP
*/

ASC_DLLSPEC int32 **slv_create_lnkmap(int32 m, int32 n, int32 hl, int32 *hi, int32 *hj);
/**<
 *  Builds a row-biased mapping array from the hi,hj lists given.
 *  The map returned has the following format:
 *    - map[i] is a vector describing the incidence in row i of the matrix.
 *    - Let vars=map[i], where vars is int32 *.
 *    - vars[0]=number of incidences in the relation.
 *    - For all 0<=k<vars[0]
 *       - vars[2*k+1] = original column index of some var in the eqn.
 *       - vars[2*k+2] = the lnk list index of element(i,vars[2*k+1])
 *
 *  The ordering of column data (i.e. vars[2*k+1]) is implementation-defined
 *  and should not be counted on.  Similarly, the lnk list index (i.e.
 *  vars[2*k+2]) will be a unique number in the range (0..hl-1), but the
 *  exact ordering is implementation-defined.  The map should only be
 *  deallocated by destroy_lnkmap().  The memory allocation for a lnkmap
 *  is done efficiently.<br><br>
 *
 *  These create an odd compressed row mapping, given the hi and hj
 *  subscript vectors.  The primary utility of the lnkmap is that
 *  it can be traversed rapidly when one wants to conditionally map a row of
 *  a Harwell style (arbitrarily ordered) link representation
 *  back into another representation where adding elements to a row
 *  is easily done.<br><br>
 *
 *  hi and hj should specify a unique incidence pattern.  That is, duplicate
 *  (hi, hj) coordinates are not allowed and only 1 of the occurrences will
 *  end up in the map.  hi should contain row indexes all less than m.
 *  hj should contain column indexes all less than n.  If an invalid row/col
 *  index is encountered, NULL is returned.
 *
 *  @param m  The number of rows expected (> highest index in hi).
 *            The map returned will be this long.
 *  @param n  The number of columns expected (> highest index in hj).
 *  @param hl The length of hi and hj.
 *  @param hi The eqn indices of a C numbered sparse matrix list.
 *  @param hj The var indices of a C numbered sparse matrix list.
 *  @return Pointer to the new lnkmap array, or NULL if an error occurred.
 */

ASC_DLLSPEC int32 **slv_lnkmap_from_mtx(mtx_matrix_t mtx, mtx_region_t *region);
/**<
 *  Generates a lnkmap from a region of a matrix.
 *  The length of the map returned will be the order of mtx.  Empty rows
 *  and columns are allowed in the matrix.  Map entries for rows outside
 *  the specified region will be 0 even if the row contains non-zero
 *  elements.  If mtx is NULL, or if the region is invalid for mtx, then
 *  NULL is returned.<br><br>
 *
 *  The map returned has the following format:
 *    - map[i] is a vector describing the incidence in row i of the matrix.
 *    - Let vars=map[i], where vars is int32 *.
 *    - vars[0]=number of non-zeros in the row.
 *    - For all 0<=k<vars[0]
 *       - vars[2*k+1] = original column index of some a non-zero element in the row.
 *       - vars[2*k+2] = the value of the element (i,vars[2*k+1]), cast to int32.
 *
 *  @param mtx    The matrix to map (non-NULL).
 *  @param region The region of the matrix to map (non-NULL).
 *  @return Pointer to the new lnkmap array, or NULL if an error occurred.
 *  @see slv_create_lnkmap() for a more details about lnkmaps.
 */

ASC_DLLSPEC void slv_destroy_lnkmap(int32 **map);
/**<
 *  Deallocate a map created by slv_create_lnkmap() or slv_destroy_lnkmap().
 *  destroy_lnkmap() will tolerate a NULL map as input.
 *
 *  @param map The lnkmap to destroy.
 */

ASC_DLLSPEC void slv_write_lnkmap(FILE *fp, int m, int32 **map);
/**<
 *  Prints a link map to a file.
 *  write_lnkmap() will tolerate a NULL map as input.
 *
 *  @param fp  The file stream to receive the report.
 *  @param m   The number of rows in map to print.
 *  @param map The lnkmap to print.
 */

/* @} */

#endif  /* ASC_SLV_COMMON_H */

