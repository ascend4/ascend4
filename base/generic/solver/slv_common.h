/*
 *  SLV Utilities & Structures for ASCEND Solvers
 *  by Ben Allan 1995
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.23 $
 *  Version control file: $RCSfile: slv_common.h,v $
 *  Date last modified: $Date: 1998/04/26 22:48:25 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1995 Benjamin Allan 1995
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  SLV Utilities & Structures for ASCEND Solvers
 *  <pre>
 *  Contents:     slv common utilities and definitions
 *
 *  Authors:      Ben Allan
 *                based on the original slv.h by KW and JZ.
 *
 *  Dates:        01/94 - original version
 *
 *  Description:
 *  General C  utility routines for slv/Slv class interfaces. Abstracted from
 *  slvX.c January 1995. Ben Allan.
 *  slv.h is the header for folks on the ASCEND end, and this is the one for
 *  folks on the Slv math end.
 *  Don't protoize this file for ASCEND types other than mtx, vec, and boolean
 *  real64, and int32.
 *  or we'll have you shot. In particular, not var and rel. People
 *  who aren't supposed to know about var and rel include this.
 *
 *  In particular, this header may be used without knowing about the ASCEND
 *  compiler or any of its annoying insanities so long as you drag out
 *  ascmalloc.
 *  This does commit you to being able to stomach the mtx.h file, however,
 *  even if you choose to ignore the contents of mtx.
 *  Several functions, notably the print suite for rel/var names,
 *  assume you are linking against something that does know about
 *  ASCEND instances unless the SLV_INSTANCES flag is set to FALSE.
 *
 *  The parameters and status struct definitions have been moved here,
 *  being of general interest.
 *
 *  Requires:     #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef slv_common__already_included
#define slv_common__already_included

#undef SLV_INSTANCES
#define SLV_INSTANCES TRUE
/**< SLV_INSTANCES should only be FALSE in a libasc.a free environment */

/**
 * Common data structures for Westerberg derived solvers.
 */
struct slv_output_data {
   FILE *more_important;  /**< NULL ==> no output */
   FILE *less_important;  /**< NULL ==> no output */
};

/**  @todo KHACK THIS SHOULD BE REMOVED */
struct slv_tolerance_data {
   real64 drop;         /**< matrix entry drop tolerance during factorization */
   real64 pivot;        /**< detect pivot too small, of those available */
   real64 singular;     /**< detect matrix numerically singular */
   real64 feasible;     /**< detect equality relations satisfied */
   real64 rootfind;     /**< detect single equality relation satisfied */
   real64 stationary;   /**< detect lagrange stationary */
   real64 termination;  /**< detect progress diminished */
};

/** Solver parameter information. */
struct slv_sub_parameters {
   /* arrays of parametric data */
   int32   *iap;    /**< Array of parametric int32 data. */
   real64  *rap;    /**< Array of parametric real64 data. */
   char*   *cap;    /**< Array of parametric char* data. */
   void*   *vap;    /**< Array of parametric void* data. */
   /* symbolic parameter names */
   char* *ianames;  /**< Symbolic names for iap parameters. */
   char* *ranames;  /**< Symbolic names for rap parameters. */
   char* *canames;  /**< Symbolic names for cap parameters. */
   char* *vanames;  /**< Symbolic names for vap parameters. */
   /* longer explanations of the parameter data */
   char* *iaexpln;  /**< Longer description of iap parameters. */
   char* *raexpln;  /**< Longer description of rap parameters. */
   char* *caexpln;  /**< Longer description of cap parameters. */
   char* *vaexpln;  /**< Longer description of vap parameters. */
   /* lengths of arrays above */
   int32 ilen;      /**< Length of iap, ianames, and iaexpln. */
   int32 rlen;      /**< Length of rap, ranames, and raexpln. */
   int32 clen;      /**< Length of cap, canames, and caexpln. */
   int32 vlen;      /**< Length of vap, vanames, and vaexpln. */
};

struct slv_block_cost {
  int32 size,
        iterations,
        funcs,
        jacs,
        reorder_method;
  double time, 
         resid,
         functime, 
         jactime;
};

/** Integer parameter substructure. */
struct slv_int_parameter {
  int32 value;
  int32 low;
  int32 high;
};

/** Boolean parameter substructure. */
struct slv_boolean_parameter {
  int32 value;
  int32 low;
  int32 high;
};

/** Real parameter substructure. */
struct slv_real_parameter {
  double value;
  double low;
  double high;
};

/** Char parameter substructure. */
struct slv_char_parameter {
  char *value;
  char **argv;
  int32 high;
};

/** Basic solver parameter types. */
enum parm_type {
  int_parm,
  bool_parm,
  real_parm,
  char_parm
};

union parm_arg
{
  char **argv;
  char *argc;   /**< huh? */
  int32 argi;
  int32 argb;
  real64 argr;
};

/** 
 *  Solver parameter structure. 
 *  @todo Shouldn't b be a slv_boolean_parameter?
 */
struct slv_parameter {
  enum parm_type type;    /**< parameter type */
  int32 number;           /**< index in array */
  int32 display;          /**< display page */
  char *name;             /**< scripting short name */
  char *interface_label;  /**< user interface label */
  char *description;      /**< modest help string */
  union {
    struct slv_int_parameter i;
    struct slv_int_parameter b;
    struct slv_real_parameter r;
    struct slv_char_parameter c;
  } info;                 /**< data */
};

/*
 * Macros for parm_arg unions.
 * Sets appropriate member (parm_u) of the union to the
 * value specified (val) and returns (parm_u).
 * (parm_u) should be one of val, lo, or hi.
 * These macros are used in calls to the
 * slv_define_parm function defined below.
 */

#define U_p_int(parm_u,val)     ((((parm_u).argi = (val))), (parm_u))
/**<
 *  Sets the argi of the parm_arg parm_u to val and returns parm_u.
 *  For use in calls to slv_define_parm().
 */
#define U_p_bool(parm_u,val)    ((((parm_u).argb = (val))), (parm_u))
/**<
 *  Sets the argb of the parm_arg parm_u to val and returns parm_u.
 *  For use in calls to slv_define_parm().
 */
#define U_p_real(parm_u,val)    ((((parm_u).argr = (val))), (parm_u))
/**<
 *  Sets the argr of the parm_arg parm_u to val and returns parm_u.
 *  For use in calls to slv_define_parm().
 */
#define U_p_string(parm_u,val)  ((((parm_u).argc = (val))), (parm_u))
/**<
 *  Sets the argc of the parm_arg parm_u to val and returns parm_u.
 *  For use in calls to slv_define_parm().
 */
#define U_p_strings(parm_u,val) ((((parm_u).argv = (val))), (parm_u))
/**<
 *  Sets the argv of the parm_arg parm_u to val and returns parm_u.
 *  For use in calls to slv_define_parm().
 */

#define SLV_IPARM_MACRO(X,P) \
  if (make_macros == 1) {  \
     (X)  = &((P)->parms[(P)->num_parms-1].info.i.value); \
  }
/**<
 *  Macro for defining macros of type integer (IPARM).
 *  See SLV_CPARM_MACRO() for more information.
 */
#define SLV_BPARM_MACRO(X,P) \
  if (make_macros == 1) {  \
     (X)  = &((P)->parms[(P)->num_parms-1].info.b.value); \
  }
/**<
 *  Macro for defining macros of type boolean (BPARM).
 *  See SLV_CPARM_MACRO() for more information.
 */
#define SLV_RPARM_MACRO(X,P) \
  if (make_macros == 1) {  \
     (X)  = &((P)->parms[(P)->num_parms-1].info.r.value); \
  }
/**<
 *  Macro for defining macros of type real (RPARM).
 *  See SLV_CPARM_MACRO() for more information.
 */
#define SLV_CPARM_MACRO(X,P) \
  if (make_macros == 1) {  \
     (X)  = &((P)->parms[(P)->num_parms-1].info.c.value); \
  }
/**<
 * Macro for defining macros of type character (CPARM).
 * To use, send in a name (X) for the macro (in caps by convention)
 * and a slv_parameters_t pointer (P). The name (X) should be
 * defined as an element in an array of void pointers in the
 * module in which the macro is to be used.  This macro uses the
 * current number of registered parameters to link the array of
 * _VOID_ _POINTERS_ to the correct parameters.  If you want to create
 * a macro for a parameter, you should put the appropriate macro
 * creating macro IMEDIATELY after the call to slv_define_parm
 * for that parameter.<br><br>
 * Local int make_macros; must be defined.
 */

/**
 *  Holds the array of parameters and keeps a count of how many it 
 *  contains.  Also holds various other information which should be 
 *  turned into slv_parameters or moved elsewhere
 *  <pre>
 *  Every registered client should have a slv_parameters_t somewhere in it.
 *
 *  The following is a list of parameters (those parameters that can be
 *  modified during solve without calling slv_presolve() are marked with
 *  "$$$").  It should be noted that some solvers may not be conformable
 *  to some of the parameters.  Default values are subject to change via
 *  experimentation.
 *
 *  output.more_important (default stdout):   $$$
 *  output.less_important (default NULL):     $$$
 *     All output from the solver is written to one of these two files
 *     (except bug messages which are written to stderr).  Common values
 *     are NULL (==> no file) and stdout.  The more important messages
 *     go to output.more_important and less important messages go to
 *     output.less_important.  To shut the solver up, set both files to
 *     NULL.
 *
 *  tolerance.drop         (default 1e-16):
 *  tolerance.pivot        (default 0.1):
 *  tolerance.singular     (default 1e-12):
 *  tolerance.feasible     (default 1e-8):
 *  tolerance.rootfind     (default 1e-12):
 *  tolerance.stationary   (default 1e-8):
 *  tolerance.termination  (default 1e-12):
 *     These define the criterion for selecting pivotable relations,
 *     whether the equations are satisfied, if a local minimum has been
 *     found, or if no further reduction in the augmented lagrange merit
 *     phi can be achieved.
 *   - During jacobian reduction, each equation pivot selected must be
 *     at least a certain fraction given by TOLERANCE.PIVOT of the largest
 *     available.
 *     Also, the largest value in the row must exceed TOLERANCE.SINGULAR
 *     in order to be considered independent.
 *   - The absolute value of each unscaled equation residual is compared
 *     with TOLERANCE.FEASIBLE in order to determine convergence of the
 *     equality constraints during Newton iteration.
 *   - The absolute value of each unscaled equation residual is compared
 *     with TOLERANCE.ROOTFIND in order to determine convergence of the
 *     constraint during rootfinding of single equations.
 *   - Detection of a minimum requires the stationary condition of the
 *     lagrange to be less than TOLERANCE.STATIONARY.
 *   - If the directional derivative of phi along the negative gradient
 *     direction using the suggested iteration step length falls below
 *     TOLERANCE.TERMINATION, iteration is ceased.
 *   - TOLERANCE.DROP is the smallest number magnitude to be allowed
 *     in the Jacobian matrix during factorization. Default is optimistic.
 *
 *  time_limit (default 30.0):   $$$
 *     This defines the time limit expressed as cpu seconds per block.
 *     If the solver requires more time than this in any given block,
 *     then it will stop.
 *
 *  iteration_limit (default 100):   $$$
 *     This defines the maximum number of iterations attempted in a given
 *     block.  The solver will stop after this many iterations if it fails
 *     to converge.
 *
 *  factor_option (default 0):
 *     This sets the number of the linear factorization to suggest.
 *     This does not map directly to linsol numbering of any sort.
 *     The map is: 0 <==> RANKI, 1 <==> RANKI_JZ, 2+ <==> ?.
 *     The solver is free to ignore this suggestion.
 *     In fact, the specific solver is free to define the meaning of factor
 *     option depending on what linear packages it can talk to.
 *
 *  partition (default TRUE):
 *     Specifies whether or not the system will be partitioned into blocks
 *     or not.  If not, then the system will be considered as one large
 *     block.
 *
 *  ignore_bounds (default FALSE):
 *     Specifies whether or not bounds will be considered during solving.
 *     WARNING: if this flag is set, there will be no guarantees that the
 *     solution will lie in bounds.  Suggested use might be to set this
 *     flag to TRUE, solve, reset this flag to FALSE, and resolve.
 *     More often than not, in fact, ignore bounds will lead to floating
 *     point exceptions, halting the solution process.
 *
 *  rho (default 1.0):
 *     Used as a scalar pre-multiplier of the penalty term quantified by one
 *     half the two norm of the equality constraint residuals in an
 *     augmented lagrange merit function.
 *
 *  sp.ia/ra/ca/vap (defaults NULL, READ ONLY):
 *     Is a set of pointers to arrays (int/double/(char*)/void*).
 *     The values of the pointers themselves should not be modified,
 *     though the values pointed at may be modified. Note that this is
 *     _direct_ modification and will take effect immediately, not on
 *     the next call to slv_set_parameters. When the engine gets around
 *     to looking at the values in these arrays is engine dependent.
 *     NULL is the expected value for some or all of these array
 *     pointers, depending on the engine. The sizes of these arrays are
 *     specific to each solver's interface. As being of interest (at
 *     compile time) to both the slvI.c file and the GUI/CLUI, the
 *     sizes of the arrays to be pointed to are part of the slvI.h file.
 *     The implementor of each slvI.c should take care to use as much of
 *     the slv_parameters_t as possible before passing data through the
 *     arrays provided in the sub_parameters. This will make for a
 *     minimal amount of work when adding an engine to the GUI/CLUI.
 *     To further aid reusability/sanity preservation, slvI.h should
 *     be appended with proper defines for subscripting these arrays.
 *
 *  sp.i/r/c/vlen (defaults 0, READ ONLY)
 *     lengths of the sub_parameter arrays.
 *
 *  sp.ia/ra/ca/vanames (defaults NULL, READONLY)
 *     symbolic names for the corresponding entries in ia/ra/ca/vap.
 *
 *  sp.ia/ra/ca/vaexpln (defaults NULL, READONLY)
 *     longer explanations for the corresponding entries in ia/ra/ca/vap.
 *
 *  whose (default 0=>slv0, READ ONLY)
 *     This tells where a parameter set came from, since the default
 *     action of slv_get_parameters is to return a copy of slv0's
 *     parameters if the parameters asked for don't exist because
 *     the solver in question wasn't built/linked.
 *  </pre>
 */
typedef struct slv_parameters_structure {
   struct slv_output_data output;
   struct slv_tolerance_data tolerance;
   struct slv_parameter *parms;
   int32 num_parms;
   int32 dynamic_parms;     /**< set 1 if parms is dynamically allocated */

   /* we wish the following were on the way out */
   struct slv_sub_parameters sp;
   int whose;
   int32 ignore_bounds;
   int32 partition;

   /* the following are on the way out */
   double time_limit;     /**< @todo kill */
   double rho;            /**< @todo kill */
   int32 iteration_limit; /**< @todo kill */
   int32 factor_option;   /**< @todo kill */

} slv_parameters_t;

/**
 *  Deallocates any memory allocated durring parameter creation.
 *  if !(p->dynamic_parms), frees strings in p->parms but not p->parms.
 */
extern void slv_destroy_parms(slv_parameters_t *p);

/**
 * <!--  slv_define_parm: Function for adding (defining) a new         -->
 * <!--  parameter in your parameter structure.                        -->
 * <!--  err = slv_define_parm(p,type,interface_name,                  -->
 * <!--                        interface_label,description,            -->
 * <!--                        value, lower_bound,upper_bound,         -->
 * <!--                        page);                                  -->
 *
 * <!--  int32 err                                                     -->
 * <!--  slv_parameters_t *p                                           -->
 * <!--  enum parm_type type                                           -->
 * <!--  char *interface_name                                          -->
 * <!--  char *interface_label                                         -->
 * <!--  char *description                                             -->
 * <!--  union parm_arg value                                          -->
 * <!--  union parm_arg lower_bound                                    -->
 * <!--  union parm_arg upper_bound                                    -->
 * <!--  int32 page                                                    -->
 *
 *  Adds (defines) a new parameter in the parameter structure.
 *  Returns err = -1 if p is NULL or called with unsupported type.
 *  Returns err = number of registered parameters otherwise.
 *  Supported types are int_parm, bool_parm,real_parm, and char_parm.
 *  interface name should be a very short but descriptive name that
 *  the interface can use to identify the parameter. The interface label
 *  should be a short text string to be displayed on the interface.
 *  The description should be a slightly more detailed string to be
 *  displayed upon request a la balloon help.
 *  The value, lower_bound, and upper_bound fields should be filled
 *  using the appropriate parm_arg union macros defined above.
 *  page should indicate the parameter page number that the parameter
 *  is to be displayed on.  By convention page is set to -1 for parameters
 *  which are not to be displayed on the interface.
 */
extern int32 slv_define_parm(slv_parameters_t *p,
                             enum parm_type type,
                             char *interface_name,
                             char *interface_label,
                             char *description,
                             union parm_arg value,
                             union parm_arg lower_bound,
                             union parm_arg upper_bound,
                             int32 page);

/** <!--  PARM VALUES                                                  -->
 * Resetting the value of a parameter can be done directly
 * except for string parameters which must be set with
 * slv_set_char_parameter(stringpointer,charvalue);
 * slv_set_char_parameter does not keep the charvalue string you pass it.
 */
extern void slv_set_char_parameter(char **cptr, char *newvalue);

/*
 * When used together the above structures, functions, and macros
 * allow us to define all of a solver's parameters in one file and
 * notify the interface of these parameters upon startup (dynamic
 * interface construction).  The parameters can be defined in any order.
 * The only bookkeeping needed is associated with the macros.  You must
 * have an array of void pointers large enough for all of the macros
 * you define and you must give each of the macros you define a unique
 * element of this array. Here is an example using a real parameter
 * and a character parameter. (The int and bool are similar to the real).
 *
 * ---------- START EXAMPLE CODE ----------
 *
 * (* these 4 macros can be defined anywhere more or less so long as it
 * is before the calls to slv_define_parm. *)
 * #define REAL_PTR (sys->parm_array[0])
 * #define REAL     ((*(real64 *)REAL_PTR))
 * #define CHAR_PTR (sys->parm_array[1])
 * #define CHAR     ((*(char **)CHAR_PTR))
 *
 * #define PA_SIZE 2
 * struct example {
 *   struct slv_parameters_t p;
 *   void *parm_array[PA_SIZE];
 *   struct slv_parameter padata[PA_SIZE];
 * } e;
 *  ...
 *   e.p.parms = padata;
 *   e.p.dynamic_parms = 0;
 *
 * static char *character_names[] = {
 *    "name_one","name_two"
 * }
 *   (* fill padata with appropriate info *)
 * slv_define_parm(&(e.p), real_parm,
 *                 "r_parm","real parameter" ,
 *                 "this is an example of a real parameter" ,
 *                 U_p_real(val,25),U_p_real(lo,0),U_p_real(hi,100),1);
 *  (* now assign the element of e.parm_array from somewhere in padata *)
 * SLV_RPARM_MACRO(REAL_PTR,parameters);
 *
 *   (* fill padata with appropriate info *)
 * slv_define_parm(&(e.p), char_parm,
 *                 "c_parm", "character parameter",
 *                 "this is an example of a character parameter",
 *                 U_p_string(val,character_names[0]),
 *                 U_p_strings(lo,character_names),
 *                 U_p_int(hi,sizeof(character_names)/sizeof(char *)),1);
 *  (* now assign the element of e.parm_array that matches. *)
 * SLV_CPARM_MACRO(CHAR_PTR,parameters);
 *
 * Resetting the value of a parameter can be done directly
 * except for string parameters which should be set with, for example,
 * slv_set_char_parameter(CHAR_PTR,newvalue);
 * or outside a solver where there is no sys->parm_array:
 *   slv_set_char_parameter(&(p.parms[i].info.c.value),argv[j]);
 *
 * ---------- END OF EXAMPLE CODE ----------
 */

struct slv__block_status_structure {
   int32 number_of;
   int32 current_block;
   int32 current_reordered_block;
   int32 current_size;
   int32 previous_total_size;
   int32 previous_total_size_vars;
   int32 iteration;
   int32 funcs;
   int32 jacs;
   double cpu_elapsed;
   double functime;
   double jactime;
   real64 residual;
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
 *    The interpretation of these data are somewhat up to the coder.
 *
 *  costsize
 *    This is how big the cost array is. It should in general be the
 *    number of blocks in the system plus 1 so that all the unincluded
 *    relations can be billed to the blocks+1th cost if they are
 *    evaluated.
 *  </pre>
 */
typedef struct slv_status_structure {
   uint32 ok : 1;
   uint32 over_defined : 1;
   uint32 under_defined : 1;
   uint32 struct_singular : 1;
   uint32 ready_to_solve : 1;
   uint32 converged : 1;
   uint32 diverged : 1;
   uint32 inconsistent : 1;
   uint32 calc_ok : 1;
   uint32 iteration_limit_exceeded : 1;
   uint32 time_limit_exceeded : 1;
   uint32 panic :1;
   int32 iteration;
   int32 costsize;
   double cpu_elapsed;
   struct slv_block_cost *cost;
   struct slv__block_status_structure block;
} slv_status_t;

/**  A dense vector class of some utility and the functions for it. */
struct vector_data {
   real64       norm2;        /**< 2-norm of vector squared */
   mtx_range_t  *rng;         /**< Pointer to range */
   real64       *vec;         /**< NULL => uninitialized */
   boolean      accurate;     /**< ? is vector currently accurate */
};

/*
 *  vector_data operations.
 *
 *  If we get brave, we will consider replacing the cores of these
 *  routines with blas calls. We aren't just overeager to go mixed
 *  language call nuts yet, however.
 */

extern void slv_zero_vector(struct vector_data *vec);
/**<
 *  <!--  slv_zero_vector(struct vector_data *vec);                    -->
 *  Assign vector entries between vec->rng.low and  vec->rng.high to 0.0.
 */

extern void slv_copy_vector(struct vector_data *vec1,
                            struct vector_data *vec2);
/**<
 *  <!--  slv_copy_vector(struct vector_data *vec1, struct vector_data *vec2); -->
 *  Copy data [vec1->rng.low .. vec1->rng.high] to vec2 starting
 *  at position vec2->rng.low.
 *
 *  @todo  Copyvector could stand to be optimized.
 */

extern real64 slv_inner_product(struct vector_data *vec1,
                                struct vector_data *vec2);
/**<
 *  <!--  slv_inner_product(struct vector_data *vec1, struct vector_data *vec2); -->
 *  Dot [vec1->rng.low .. vec1->rng.high] with vec2 starting at
 *  position vec2->rng.low.
 *
 *  @todo inner_product could stand to be optimized.
 */

extern real64 slv_square_norm(struct vector_data *vec);
/**<
 *  <!--  slv_square_norm(struct vector_data *vec);                    -->
 *  Dot [vec->rng.low .. vec->rng.high] with itself and store the
 *  result in vec->norm2.
 *
 *  @todo square_norm could stand to be optimized.
 */

extern void slv_matrix_product(mtx_matrix_t mtx, 
                               struct vector_data *vec,
                               struct vector_data *prod, 
                               real64 scale, 
                               boolean transpose);
/**<
 *  <!--  slv_matrix_product(mtx,vec,prod,scale,transpose)             -->
 *  <!--  mtx_matrix_t mtx;                                            -->
 *  <!--  struct vector_data *vec,*prod;                               -->
 *  <!--  real64 scale;                                                -->
 *  <!--  boolean transpose;                                           -->
 *
 *  Stores prod := (scale)*(mtx)(vec) or (scale)*(mtx-transpose)(vec).
 *  vec and prod must be completely different.
 *  If (!transpose) vec->vec is assumed indexed by current col and
 *                 prod->vec is indexed by current row of mtx.
 *  If (transpose) vec->vec is assumed indexed by current row and
 *                 prod->vec is indexed by current col of mtx.
 *
 *  @todo mtx_product needs attention - does it go into mtx?
 */

extern void slv_write_vector(FILE *fp, struct vector_data *vec);
/**<
 *  <!--  slv_write_vector(fp,vec)                                     -->
 *  Write the values in the range of the vector to file fp along with
 *  a few other doodads.
 */

/*
 * Misc. BLAS-like functions.
 */

/**
 *  <!--  sum=slv_dot(len, a1, a2);                                    -->
 *  <!--  real64 *a1, *a2, sum;                                        -->
 *  <!--  int32 len;                                                   -->
 *  Dot product of 2 arrays of real64. Loop unrolled.
 *  Takes advantage of identical vectors.
 *
 *  Used inside slv_inner_product, so no need to use specially
 *  if you are using the vector_data type.
 */
extern real64 slv_dot(int32 len, const real64 *a1, const real64 *a2);

/*
 *  General input/output routines
 *  -----------------------------
 */

/**
 *  Takes a file pointer, and if it is null returns a pointer to /dev/null.
 *  If you are in environment that doesn't have something like
 *  /dev/null, you'd better be damn sure your sys->p.output.*_important
 *  is not NULL.
 */
extern FILE *slv_get_output_file(FILE *fp);

/*
 * FILE pointer macros.
 *     fp = MIF(sys)
 *     fp = LIF(sys)
 *     fp = PMIF(sys)
 *     fp = PLIF(sys)
 *     or fprintf(MIF(sys),"stuff",data...);
 *  Use of these is requested on grounds of readability but not required.
 *  MIF and LIF are macros, which means any specific solver interface
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
 */
#define LIF(sys) slv_get_output_file( (sys)->p.output.less_important )
/**< 
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t.
 */
#define PMIF(sys) slv_get_output_file( (sys)->p->output.more_important )
/**< 
 *  Retrieve the "more important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 */
#define PLIF(sys) slv_get_output_file( (sys)->p->output.less_important )
/**< 
 *  Retrieve the "less important" output file for a system.
 *  sys must exist and contain an element p of type slv_parameters_t*.
 */

/*------------------- begin compiler dependent functions -------------------*/
#if SLV_INSTANCES

/**
 *  void slv_print_obj_name(outfile,obj) [1/95: not yet implemented]
 *  void slv_print_rel_name(outfile,sys,rel)
 *  void slv_print_var_name(outfile,sys,var)
 *  void slv_print_logrel_name(outfile,sys,lrel)
 *  void slv_print_dis_name(outfile,sys,dvar)
 *
 *  Prints appropriate name.  If name can't be found by
 *  *_make_name(*), the global index is printed by default.
 *
 *  void slv_print_obj_index(outfile,obj)[1/95: not yet implemented]
 *  void slv_print_rel_sindex(outfile,rel)[1/95: not yet implemented]
 *  void slv_print_var_sindex(outfile,var)[1/95: not yet implemented]
 *  void slv_print_logrel_sindex(outfile,lrel)[1/95: not yet implemented]
 *  void slv_print_dis_sindex(outfile,dvar)[1/95: not yet implemented]
 *
 *  To print the local index of a ***, call slv_print_***_index();
 * 
 *  FILE *outfile;
 *  obj_objective_t obj;
 *  struct rel_relation *rel;
 *  struct var_variable *var;
 *  struct logrel_relation *lrel;
 *  struct dis_discrete *dvar;
 *  slv_system_t sys;
 *
 *  @todo Implement new functions or remove prototypes.
 */
#ifdef NEWSTUFF
extern void slv_print_obj_name(FILE *outfile, obj_objective_t obj);
/**<  Prints the name of obj to outfile. */
#endif
extern void slv_print_rel_name(FILE *outfile,
                               slv_system_t sys,
                               struct rel_relation *rel);
/**<  Prints the name of rel to outfile. */
extern void slv_print_var_name(FILE *outfile,
                               slv_system_t sys,
                               struct var_variable *var);
/**<  Prints the name of var to outfile. */
extern void slv_print_logrel_name(FILE *outfile,
                                  slv_system_t sys,
                                  struct logrel_relation *lrel);
/**<  Prints the name of lrel to outfile. */
extern void slv_print_dis_name(FILE *outfile,
                               slv_system_t sys,
                               struct dis_discrete *dvar);
/**<  Prints the name of dvar to outfile. */

#ifdef NEWSTUFF
extern void slv_print_obj_index(FILE *outfile, obj_objective_t obj);
/**<  Prints the index of obj to outfile. */
#endif
extern void slv_print_rel_sindex(FILE *outfile, struct rel_relation *rel);
/**<  Prints the index of rel to outfile. */
extern void slv_print_var_sindex(FILE *outfile, struct var_variable *var);
/**<  Prints the index of var to outfile. */
extern void slv_print_logrel_sindex(FILE *outfile, struct logrel_relation *lrel);
/**<  Prints the index of lrel to outfile. */
extern void slv_print_dis_sindex(FILE *outfile, struct dis_discrete *dvar);
/**<  Prints the index of dvar to outfile. */

/**
 *  <!--  int slv_direct_solve(server,rel,var,file,epsilon,ignore_bounds,scaled) -->
 *  <!--  struct rel_relation *rel;                                    -->
 *  <!--  struct var_variable *var;                                    -->
 *  <!--  boolean ignore_bounds;                                       -->
 *  <!--  real64 epsilon;                                              -->
 *
 *  Attempt to directly solve the given relation (equality constraint) for
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
 *  unscaled residual according to scaled (no scale <- 0)..
 *  If file != NULL and there are leftover possible solutions, we
 *  will write about them to file.
 */
extern int slv_direct_solve(slv_system_t server,
                            struct rel_relation *rel,
                            struct var_variable *var,
                            FILE *file,
                            real64 epsilon,
                            int ignore_bounds,
                            int scaled);

/**
 *  <!--  int slv_direct_log_solve(server,lrel,dvar,file,perturb,insts)-->
 *  <!--  struct logrel_relation *lrel;                                -->
 *  <!--  struct dis_discrete *dvar;                                   -->
 *
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
 */
extern int slv_direct_log_solve(slv_system_t sys,
                                struct logrel_relation *lrel,
                                struct dis_discrete *dvar,
                                FILE *file,
                                int perturb,
                                struct gl_list_t *insts);

#endif
/*-------------------- END compiler dependent functions --------------------*/

/*
 *  lnkmap functions:
 */

extern int32 **slv_create_lnkmap(int32 m, int32 n, int32 hl, int32 *hi, int32 *hj);
/**<
 *  <!--  map=slv_create_lnkmap(m,n,hl,hi,hj);                         -->
 *  <!--  int32 m, n, hl                                               -->
 *  <!--  int32 *hi, *hj                                               -->
 *  <!--  int32 **map                                                  -->
 *
 *  Builds a row biased mapping array from the hi,hj lists given.
 *  The map returned has the following format:
 *    - map[i] is a vector describing the incidence in row i of the matrix.
 *    - Let vars=map[i], where vars is int32 *.
 *    - vars[0]=number of incidences in the relation.
 *    - For all 0<=k<vars[0]
 *       - vars[2*k+1]= original column index of some var in the eqn.
 *       - vars[2*k+2]= the lnk list index of element(i,vars[2*k+1])
 *
 *  The map should only be deallocated by destroy_lnkmap().
 *  The memory allocation for a lnkmap is done efficiently.<br><br>
 *
 *  These create an odd compressed row mapping, given the hi and hj
 *  subscript vectors. The primary utility of the lnkmap is that
 *  it can be traversed rapidly when one wants to conditionally map a row of
 *  a Harwell style (arbitrarily ordered) link representation
 *  back into another representation where adding elements to a row
 *  is easily done.<br><br>
 *
 *  hi and hj should specify a unique incidence pattern, that is no
 *  duplicate elements are allowed.  Rowindex and colindex refer to 
 *  the data in hi,hj.
 *
 *  @param m  The number of rows expected. The map returned will be this long.
 *  @param n  The number of columns expected.
 *  @param hl The length of hi and hj.
 *  @param hi The eqn indices of a C numbered sparse matrix list.
 *  @param hj The var indices of a C numbered sparse matrix list.
 */

extern int32 **slv_lnkmap_from_mtx(mtx_matrix_t mtx, int32 len, int32 m);
/**<
 *  <!--  map=slv_lnkmap_from_mtx(mtx,len,m)                           -->
 *  <!--  mtx_matrix_t mtx                                             -->
 *  <!--  int32 len, m                                                 -->
 *  <!--  int32 **map                                                  -->
 *
 *  Generates a map from a matrix.
 *  Empty rows and columns are allowed in the matrix.
 *
 *  @param mtx  The matrix to map.
 *  @param m    The number of rows expected. The map returned will be this long.
 *  @param len  The number of nonzeros in mtx.
 *
 *  @see slv_create_lnkmap()
 */

extern void slv_destroy_lnkmap(int32 **map);
/**<
 *  <!--  slv_destroy_lnkmap(map);                                     -->
 *  <!--  int32 **map                                                  -->
 *
 *  Deallocate a map created by slv_create_lnkmap() or slv_destroy_lnkmap().
 *  destroy_lnkmap() will tolerate a NULL map as input.
 */

extern void slv_write_lnkmap(FILE *fp, int m, int32 **map);
/**<
 *  <!--  slv_write_lnkmap(fp,m,map);                                  -->
 *  <!--  FILE *fp                                                     -->
 *  <!--  int32 m (same as was created)                                -->
 *  <!--  int32 **map                                                  -->
 *
 *  Print a link map.
 *  write_lnkmap() will tolerate a NULL map as input.
 */

#endif  /* slv_common__already_included */

