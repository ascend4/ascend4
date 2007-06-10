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
	ASCEND instances unless you have #defined SLV_STANDALONE

	The parameters and status struct definitions have been moved here,
	being of general interest.
	@ENDNOTE
*//*
 	Abstracted from
	slvX.c January 1995. Based on the original slv.h by KW and JZ (01/94), by Ben Allan.
*/

#ifndef ASC_SLV_COMMON_H
#define ASC_SLV_COMMON_H

#include <stdio.h>

#include <utilities/ascConfig.h>
#include <general/list.h>

#include <linear/mtx.h>

#include "slv_types.h"
#include "rel.h"
#include "logrel.h"

/**	@addtogroup solver Solver
	@{
*/

/* #define SLV_STANDALONE */
/**< 
	If undefined, we are lying on top of the ASCEND instance hierarchy. 
	If defined, SLV is running self-contained and must have no compiler 
	dependencies in any of its code.

	Normally this should not be defined.
*/

#ifdef SLV_STANDALONE
# error "why SLV_STANDALONE?"
#endif

/*------------------------------------------------------------------------------
  DATA STRUCTURES
*/

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

#ifndef SLV_STANDALONE

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

ASC_DLLSPEC int slv_direct_solve(slv_system_t server,
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

ASC_DLLSPEC int slv_direct_log_solve(slv_system_t sys,
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

