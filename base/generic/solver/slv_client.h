/**< 
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.35 $
 *  Version control file: $RCSfile: slv_client.h,v $
 *  Date last modified: $Date: 1998/04/25 13:01:56 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1996 Benjamin Andrew Allan
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

/**< 
 *  SLV general interface for solver clients, etc.
 *  By Ben Allan
 *
 *  Copyright(C) Karl Westerberg, Joseph Zaher, Benjamin Allan 1994
 *  Copyright(C) Benjamin Andrew Allan 1996
 *
 *  ASCEND (the language) exists to separate, when desirable, the 
 *  formulation of a mathematical problem (numeric) from the solution of
 *  the that problem. ASCEND (the interface) exists to give the user as
 *  much (or as little) control over the compilation and solution of their
 *  problem as they want.
 *
 *  The problems expressible in the language cannot (and indeed should not)
 *  be reduced to a single formulation if the solutions are to be 
 *  implemented in a robust, efficient, and user controllable manner.
 *  All but one of the solving engines attached to ASCEND must inevitably
 *  be hamstrung if we insist that they all fit in the same interface shoebox.
 *  Witness the minos/lsode implementations in the Pascal-version. The
 *  alternative is to make all engines talk through an interface defined
 *  by the intersection of the engines' individual interfaces. This 
 *  alternative is unacceptable from a software engineering point of view.
 *
 *  This portion of the interface, then, has the task of making every
 *  engine conform to a minimum set of semantics (thus enabling the GUI/
 *  CLUI to support the user wanting very little control over a host of
 *  powerful solving engines) while giving the power hungry user access
 *  to those parameters specific to a given engine.
 *  The minimum semantics chosen, due mostly to convenience and the biases
 *  of the developers, are those of slv0 with the provision of a set of
 *  arrays for the passing of auxillary, or 'sub', parameters to each
 *  solver.
 *  The data structures we desire to have common to all the solvers are
 *  kept in slv_common.h (or .c).
 *  The functions the UI are kept here.
 */

/**< 
 *  Contents:     Solver module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *                Benjamin Allan
 *
 *  Dates:        06/90 - original version
 *                01/94 - modified tolerances, eliminated var_to_name
 *                        and rel_to_name function pointers, and
 *                        expanded status report
 *                04/94 - expanded scope of slv0 to perform optimization
 *                05/94 - added counting routines to count variables,
 *                        boundaries, and relations which pass some
 *                        specified filter
 *                10/94 - added stubs for OPT and QRSlv
 *                1/95  - moved status and parameters definitions to
 *                        slv_common.h. BAA
 *                02/96 - added stubs for NGSlv. KTH
 *                06/96 - split into client and server headers.
 *                0/97 - added stubs for CONOPT. KTH
 *  Description:  The inputs to any solver consist of a formulation of
 *                the problem to solve along with a set of parameters to
 *                allow user control of the solution process.  The
 *                general formulation is given below (for non-discrete
 *                problems only):
 *
 *                         min F(x,u,c)
 *
 *                   s.t.  h(x,u,c) = 0
 *                         r(x,u,c) = 0
 *                         g(x,u,c) >= 0
 *                         b(x,u,c) >= 0
 *
 *                A variable list consists of fixed (c), independent (u),
 *                dependent variables (x), and unattached variables (q).
 *                A relation list consists of unconditional (or global)
 *                equality constraints (h), conditional equality
 *                constraints (r), and inequality constraints (g), each of
 *                type struct rel_relation *.  The conditional equalities are
 *                isolated from the global equalities because they are only
 *                defined for certain ranges in values of the variables,
 *                expressed through a set of inequality boundary relations
 *                (b), each of type bnd_boundary_t which may or may not
 *                be satisfied at any given point.  An objective function
 *                (F) is used to provide a criteria with which to
 *                optimize the values of the independent variables.
 *
 *                The objective function is a relation (LHS only)
 *                struct rel_relation * and may be set (its default is NULL)
 *                using slv_set_obj_relation.  The variable, boundary, and
 *                relation lists are pointer lists of
 *                struct var_variable * and struct rel_relation * and
 *                are expected to be NULL terminated.  This means that for
 *                10 variables to be placed in a list, for example, 11
 *                elements must exist in order to reserve the last pointer
 *                to be NULL.  These lists ARE REQUIRED to be set. 
 *
 *                The additional set of inputs are the slv_parameters.
 *                These can specify stopping conditions or the amount of
 *                information displayed during solving, for example.
 *                Monitoring of the solution process is done using the
 *                status report for solvers that iterate. 
 *                More details are given with the
 *                respective declarations below.
 */

#ifndef slv_client_h__already_included
#define slv_client_h__already_included

/**< requires #include "base.h" */
/**< requires #include "instance_enum.h" */
/**< requires #include "var.h" */
/**< requires #include "rel.h" */
/**< requires #include "discrete.h" */
/**< requires #include "conditional.h" */
/**< requires #include "logrel.h" */
/**< requires #include "bnd.h" */
/**< requires #include "linsol.h" */
/**< requires #include "linsolqr.h" */
/**< requires #include "slv_common.h" */
/**< requires #include "slv_types.h" */

/***  ! ! We are going through a solver API definition restructuring.
 ! !  The appearance of ! ! in the header means the code in question
 ! !  has, or is about to have, a change in its meaning or is code that
 ! !  is new and replaces some or all the functionality of an old
 ! !  function definition. Basically, expect to have to read ! ! sections
 ! !  carefully and maybe patch calls dependent on them.
 **/
/***********************************************************************\
  Right, so we're going to have a client-server, object-oriented, 
  open-architecture system designed to handle multiple clients in a
  single-thread process. Furthermore, the clients will NOT have to
  know anything at all about the ASCEND IV compiler hidden out back
  some place -- in fact our compiler may not BE out back, it may be
  on another machine or swapped to disk or whatever.

  That's the ideal. In most applications of ASCEND, particularly the
  interactive one, the compiler is never very far away. Isolating the
  compiler data completely (meaning no looking back at it for anything)
  would mean recreating all the relations (be they tokens, calls to
  C code, or any kind) in a separate process. This is unacceptable from
  a memory conservation point of view until RAM comes down to ~$1/MByte,
  especially if ASCEND is to run on PCs any time soon.

  What we really have then is a slv_system_t made up of variables and
  relations and hiding all the compiler details from the clients.
  Clients will operate directly on the slv_system_t only through real
  C functions and not through macros so we can hide all the nasty
  details about the compiler. Variables and relations only make
  sense in the context of a slv_system_t, so the var_variable type
  and the rel_relation type in this C code sometimes require that
  the system pointer be provided when asking for certain properties
  or services.

  Q: What is a variable?
  A: The question is ambiguous. In ASCEND we have the notion of a
     solver_var ATOM type that includes bounding values, a scaling
     value, and other properties. These are not the only real-number
     based items that can occur, however. For purposes of the CLIENT
     application programming interface (API) we collect all the real-
     based objects we can find and map them all to struct var_variable.
     See var.h for details. We sort them into lists as follows:
       - vars.  These are solver_var that are in an objective or relation.
       - pars.  These are solver_par appearing parametrically.
       - unattached.  These don't appear in relation or objective, and
                      they may be solver_var or solver_par or neither.
     We keep 2 versions of each list: one for ourselves which is READ-
     ONLY for clients and allows us to do many things efficiently, and
     another for clients that clients may rearrange (or even delete)
     as suits their needs. The former version is called a master list,
     and the latter version is called a solvers list.

   Q: What is a relation?
   A: At present a relation in ASCEND is either an objective function
      (a one-sided relation) or a constraining equation. We have a
      variety of relation implementations within ASCEND, but all any
      client needs to know about relations can be found in the rel.h
      file. We keep master and client lists of relations as well.
      We provide a variety of interesting services with relations:
         residual and gradient calculations
         symbolic inversion (where possible)
         numeric root finding
         scaling based on symbolic arguments
         symbolic determination of linearity
      and we expect to add others as they occur to us or you suggest
      them.

   Q: What else is a slv_system_t?
   A: It's has a host of interesting properties.
   1) One slv_system_t (system, hereafter) can only be used by one
      -----------------         registered           -------------
      client at a time, but if your client is an unregistered manager
      of several subclients (for example an NLP code and and MILP code)
      then you can pass it back and forth to those registered clients to solve
      an MINLP. (Note: we haven't done this ourselves yet.)
      Any number of unregistered clients may share a system, but they
      must take responsibility for not stepping on each other or the
      registered client. Registration will be explained further below.
   2) From any given ASCEND type definitions, the master lists in the 
      system will be ordered identically across all invocations of
      ASCEND on any hardware that we are aware of. This property is
      derived from the way we compile instances and create systems.
      This is helpful in benchmarking and other applications.
   3) We have a number of standard clients (registered and not)
      you can use on a the system to precondition it in some way for
      your client:
        - Degrees of freedom analysis.
        - Problem decomposition.
        - Reordering of vars and rels for good factorization.
        - Solution of square nonlinear systems.
        - Generation of MPS code for popular MILP solvers.
        - Generation of GAMS code for an old, slow compiler of an
          extremely awkward modeling language that does happen to 
          have a lot of really good optimizers connected.


  Future work:
  Short term --
  We expect to construct a client that takes the partitioned
  problem and hands off the blocks in sequence to one or more
  solvers designed to handle only 1 block.

  Long term --
  We anticipate that the structure laid out so far is capable of
  expansion (probably by intermediate clients which add additional
  semantic content) to provide standardized (mtx and harwellian)
  sparse matrix support and memory management for codes that don't
  care to think about such things directly.

\***********************************************************************/

typedef void *SlvClientToken;
/**< A pointer that is meaningful to a registered client.
 * Each call that requires a client response will include this
 * token so that we can have multiple copies of a particular
 * client simultaneously. Clients shouldn't have to use any
 * global variables to save their state information -- they
 * should put such info with their token.
 */
/**< NOTE TO present (6/96) developers: SlvClientToken is an alias for
 * all the old slv*_system_t pointers. cast it to be the type you
 * want.
 */

struct slv_reorder_data {
  int partition;
  int basis_selection;
  int block_reordering;
  /**< other parameters here. convert to enums. */
};

typedef struct dof_data_structure {
  mtx_block_t blocks;		/**< block structure determined by analyze */
  int32 structural_rank;	/**< length of output assignment */
  int32 n_rows;			/**< total included equations */
  int32 n_cols;			/**< total free and incident solver variables */
  int32 n_fixed;		/**< total fixed solver variables */
  int32 n_unincluded;		/**< total unincluded equations */
  struct slv_reorder_data reorder;
} dof_t;

#define slv_number_of_solvers g_SlvNumberOfRegisteredClients
extern int g_SlvNumberOfRegisteredClients;

/**< The number of solver's that have ever registered.
 * Once a solver is registered, we keep track of its name,
 * a number which is the order it was registered in, and
 * the functions it defines.
 */
#define SLVMAXCLIENTS 100
/**< 
 * The maximum number of clients that ever can be registered.
 * Limit is arbitrary. Note that not all clients of slv_system_t
 * should register, just those that purport to be solve engines
 * and the like.
 */

/**< type declarations for registered client functions */
/**< We will explain all these later in this file someday soon. */
typedef SlvClientToken (SlvClientCreateF) (slv_system_t,int *);
typedef int (SlvClientDestroyF) (slv_system_t,SlvClientToken);
typedef int (SlvClientEligibleF) (slv_system_t);
typedef int32 (SlvGetDefParamsF) (slv_system_t,SlvClientToken,slv_parameters_t *);
typedef void (SlvGetParamsF) (slv_system_t, SlvClientToken, slv_parameters_t *);
typedef void (SlvSetParamsF) (slv_system_t, SlvClientToken, slv_parameters_t *);
typedef void (SlvGetStatusF) (slv_system_t, SlvClientToken, slv_status_t *);
typedef linsol_system_t (SlvGetLinsolF)(slv_system_t, SlvClientToken);
typedef linsolqr_system_t (SlvGetLinSysF)(slv_system_t, SlvClientToken);
typedef mtx_matrix_t (SlvGetSysMtxF)(slv_system_t, SlvClientToken);
typedef void (SlvDumpInfoF)(slv_system_t, SlvClientToken,int);
typedef void (SlvSolveF)(slv_system_t, SlvClientToken);

typedef struct slv_registration_data {
  int number; /**< we set number AFTER the client registration returns 0 */
  /**< client sets all the rest, starting with a symbolic name */
  const char *name;
  /**< Required functions */
  SlvClientCreateF *ccreate;
  SlvClientDestroyF *cdestroy;
  SlvClientEligibleF *celigible;
  SlvGetDefParamsF	*getdefparam;
  SlvGetParamsF	*getparam;
  SlvSetParamsF	*setparam;
  SlvGetStatusF	*getstatus;
  SlvSolveF	*solve;
  /**< Functions we really want, but can live without if your solver is old
   * and klunky. Your solver may not 'look good' in an interactive environment,
   * but then those nasty batch codes seldom do anyway.
   * Redesign you're blinkin' batch code.
   */
  SlvSolveF	*presolve;
  SlvSolveF	*iterate;
  SlvSolveF	*resolve;
  /**< Strictly Optional Functions */
  SlvGetLinsolF	*getlinsol;
  SlvGetLinSysF	*getlinsys;
  SlvGetSysMtxF	*getsysmtx;
  SlvDumpInfoF	*dumpinternals;
} SlvFunctionsT;


typedef int (SlvRegistration)(SlvFunctionsT *);
/**< 
 ***  status = YourRegistrationFunction(our_sft);
 ***  SlvFunctionsT *our_sft;
 ***  int status;
 ***
 ***  We assume a nonzero return value means you don't register successfully.
 ***  Your function is probably part of an entire file that bridges
 ***  between our headers and your solve engine back end.
 ***  You must provide a function in your solver bridge which conforms with
 ***  the prototype above (SlvRegistration). 
 ***  Your function should fill in all the required and as many of the
 ***  optional slots in the SlvFunctions pointer you are passed as it can. 
 ***  (note: do not fill in number -- that is ours.)
 ***  If you register, but do not fill in a slot we will not accidently
 ***  call a bogus function. We will instead tell the user that an
 ***  incompetent solver was registered.
 **/

extern int slv_register_client(SlvRegistration, char *, char *);
/**< 
 *** Examples:
 *** slv_register_client(slv0_register,NULL,NULL);
 *** slv_register_client(NULL,"yourregisterfuncname","yourbinaryname");
 ***
 *** Call this function with the SlvRegistration function
 *** from your solver or with 2 strings, but not both.
 *** The 2 char strings will be used in dynamically loading
 *** a solver. 
 ***
 *** Returns 0 if registration succeeds, nonzero OTHERWISE.
 ***
 *** Note: the second syntax is NOT YET IMPLEMENTED.
 **/

extern const char *slv_solver_name(int);
/**< 
 ***  There may in general be more than one solver.  The solvers will be
 ***  numbered from 0 to slv_number_of_solvers-1.  Not all the solvers may
 ***  be present in a given installation of ASCEND as some are proprietary
 ***  (MINOS, for example).
 ***  Solvers not yet registered will not have names. Each registered
 ***  client must have a unique name if user interfaces are to be happy,
 ***  though we suppose an interface could make a unique identifier out
 ***  of name-number pair.
 **/

extern int Solv_C_CheckHalt_Flag;
extern int Solv_C_CheckHalt();
/**< 
 ***  Function to check for variable ascSolvStatVect(menubreak) ="1"   
 ***  Returns 1 if true or if variable not found in global context,
 ***  else returns 0.
 ***  Solvers which do not have a real slv_iterate function should
 ***  use this in the functions that call on the ASCEND data structure
 ***  and should stop and restart their time clocks around the call.
 ***
 ***  This is the present hack to deal with signals, particularly
 ***  SIGINT. It needs to be changed along with the front end
 ***  signal handling in the solver and scripting codes.
 **/

extern unsigned int slv_serial_id(slv_system_t);
/**< 
 ***  id =slv_serial_id(sys);
 ***  slv_system_t sys;
 ***  unsigned id;
 ***
 ***  Returns the id number of the slv_system_t. The id is unique
 ***  within the life of the program.
 **/

extern dof_t *slv_get_dofdata(slv_system_t);
extern dof_t *slv_get_log_dofdata(slv_system_t);
/**< 
 ***  d=slv_get_dofdata(server);
 ***  d=slv_get_log_dofdata(server);
 ***  slv_system_t server;
 ***  dof_t *d;
 ***  Returns a pointer to the system's dof structure.
 ***  This structure can be for a logical solver or a nonlinear
 ***  solver.
 ***  Data in the structure should be consistent with
 ***  some interpretation of the solvers_var/rel lists.
 ***  The DEFAULT interpretation has not yet been established.
 ***  The pointer this returns cannot be freed.
 ***  If server is not NULL, d will not be NULL.
 **/

extern const mtx_block_t *slv_get_solvers_blocks(slv_system_t);
extern const mtx_block_t *slv_get_solvers_log_blocks(slv_system_t);
/**< 
 ***  bl = slv_get_solvers_blocks(sys); 
 ***  bl = slv_get_solvers_log_blocks(sys); 
 ***  slv_system_t sys;
 ***  mtx_block_t *bl;
 ***
 ***  Decomposition information for the nonlinear and logical solvers.
 ***  For the nonlinear case:
 ***  The blocks of bl contain decomposition information about the
 ***  Jacobian of the equations(included) and variables(free and
 ***  incident) if it is constructed in the ordering of relations/
 ***  variables in the solvers_rel/var_lists.
 ***  That is, we have done the subproblem partitioning already.
 ***  Each region may be solved as a separate subproblem in the
 ***  order given in the list.
 ***  We may have also done what we think is a good ordering
 ***  for row-wise lower-triangular linear factorization codes
 ***  within each of the blocks. We may have even arranged the
 ***  columns so that we believe we have made a 'good' set of
 ***  variables non-basic in the event that the last block is
 ***  rectangular.
 ***  
 ***  You are free to reorder any matrix you construct from 
 ***  our equation gradients to suit any factorization method
 ***  you choose. We strongly recommend that you not do this.
 ***
 ***  bl is a pointer to the struct with the number of blocks
 ***  and the data for the blocks. Each block but the last one
 ***  will be square and will contain a set of rows/columns that
 ***  should be solved simultaneously. The last block may be
 ***  rectangular. Rectangular last blocks will be wider.
 ***
 ***  In the event that we have a structurally overspecified
 ***  problem, we will have excluded the redundant equations from 
 ***  the region covered by the block list and partitioned those
 ***  equations remaining. If you are a solver client which solves
 ***  least squares problems, you should probably just ignore our
 ***  block structure completely.
 ***
 ***  This will never return NULL unless sys is NULL, but if the
 ***  length of the block array is 0, then the region pointer will
 ***  be NULL.
 **/

extern void slv_set_solvers_blocks(slv_system_t,int32,mtx_region_t *);
extern void slv_set_solvers_log_blocks(slv_system_t,int32,mtx_region_t *);
/**< 
 ***  slv_get_solvers_blocks(sys,len,data);
 ***  slv_get_solvers_log_blocks(sys,len,data);
 ***  slv_system_t sys;
 ***  int32 len;
 ***  mtx_region_t *data;
 ***  Set the block data for the solvers to the array given which
 ***  should be of length len. Solvers may be nonlinear or logical.
 ***  If the system in question already has a list of blocks,
 ***  it will be freed. This may have antisocial consequences
 ***  for registered clients if they have stored a copy of the pointer to the
 ***  old data. The should be told to reinitialize themselves.
 **/

extern void slv_check_var_initialization(slv_system_t);
extern void slv_check_dvar_initialization(slv_system_t);
/**< 
 ***  slv_check_var_initialization(sys);
 ***  slv_check_dvar_initialization(sys);
 ***  slv_system_t sys;
 ***  
 ***  Checks that all the variables on the solvers_var_list have
 ***  been assigned at least once. If any has not, it is assigned
 ***  its scaling value (var_nominal) since this is generally a
 ***  much better starting value than 0.0.
 ***  Checks that all the boolean variables on the solvers_dvar_list have
 ***  been assigned at least once. If any has not, it is assigned
 ***  a value of TRUE.
 **/

extern void slv_bnd_initialization(slv_system_t);
/**< 
 ***  slv_bnd_initialization(sys);
 ***  slv_system_t sys;
 ***  
 ***  Initialize the status of a boundary (satisfied ?)
 ***  At the initial point, it will be given the same value to
 ***  the current status and the previous status. Therefore, the bit
 ***  crossed (which can be modified during the iterative scheme)
 ***  is initialized to FALSE.
 ***  The evaluation of the status is performed with a call to the
 ***  function provided in bndman.
 **/

extern void slv_set_solvers_var_list(slv_system_t, struct var_variable **,int);
extern void slv_set_solvers_par_list(slv_system_t, struct var_variable **,int);
extern void slv_set_solvers_unattached_list(slv_system_t,
                                            struct var_variable **,int);
extern void slv_set_solvers_dvar_list(slv_system_t,struct dis_discrete **,int);
extern void slv_set_solvers_disunatt_list(slv_system_t,
					  struct dis_discrete **,int);
extern void slv_set_solvers_rel_list(slv_system_t, struct rel_relation **,int);
extern void slv_set_solvers_condrel_list(slv_system_t,
					 struct rel_relation **,int);
extern void slv_set_solvers_obj_list(slv_system_t, struct rel_relation **,int);
extern void slv_set_solvers_logrel_list(slv_system_t,
					struct logrel_relation **,int);
extern void slv_set_solvers_condlogrel_list(slv_system_t,
					    struct logrel_relation **,int);
extern void slv_set_solvers_when_list(slv_system_t, struct w_when **,int);
extern void slv_set_solvers_bnd_list(slv_system_t, struct bnd_boundary **,int);
/**< 
 ! !  slv_set_solvers_var_list(sys,vlist,size);
 ! !  slv_set_solvers_par_list(sys,vlist);
 ! !  slv_set_solvers_unattached_list(sys,vlist,size);
 ! !  slv_set_solvers_dvar_list(sys,dvlist,size);
 ! !  slv_set_solvers_disunatt_list(sys,dvlist,size);
 ! !  slv_set_solvers_rel_list(sys,rlist,size);
 ! !  slv_set_solvers_condrel_list(sys,clist,size);
 ! !  slv_set_solvers_obj_list(sys,rlist,size);
 ! !  slv_set_solvers_logrel_list(sys,lrlist,size);
 ! !  slv_set_solvers_condlogrel_list(sys,lrlist,size);
 ! !  slv_set_solvers_when_list(sys,wlist,size);
 ! !  slv_set_solvers_bnd_list(sys,blist,size);
 ***  slv_system_t sys;
 ***  struct var_variable **vlist;
 ***  struct rel_relation **rlist;
 ***  struct logrel_relation **lrlist;
 ***  struct w_when **wlist;
 ***  struct bnd_boundary **blist;
 ***  int size;
 ***
 ***  The list should be NULL terminated and the size should be the length
 ***  of the list  EXCLUDING  the terminal NULL.
 ***  Sets the system's pointer list to the list given. If the system already
 ***  has such a list, the old list will be freed unless the two lists are
 ***  in fact the same (in which case why are you calling this?).
 ***  Size is the length of the vlist (excluding the terminal NULL entry).
 ***  The sindex field of each var in the list should match it's list position.
 ! !
 ! !  There are now 2 var lists: the master var list pulled of the instance
 ! !  tree, and the solvers var list is to be fetched by the solvers.
 ! !  Eventually the solvers_varlist will only include those vars the specific
 ! !  solver needs to know about.
 ! !  For the moment, the content of the two lists is the same, but the ordering
 ! !  is not. The master list is in the order collected. The solvers list
 ! !  is reordered in some useful fashion defined elsewhere.
 **/

extern struct var_variable **slv_get_solvers_var_list(slv_system_t);
extern struct var_variable **slv_get_solvers_par_list(slv_system_t);
extern struct var_variable **slv_get_solvers_unattached_list(slv_system_t);
extern struct dis_discrete **slv_get_solvers_dvar_list(slv_system_t);
extern struct dis_discrete **slv_get_solvers_disunatt_list(slv_system_t);
extern struct var_variable **slv_get_master_var_list(slv_system_t);
extern struct var_variable **slv_get_master_par_list(slv_system_t);
extern struct var_variable **slv_get_master_unattached_list(slv_system_t);
extern struct dis_discrete **slv_get_master_dvar_list(slv_system_t);
extern struct dis_discrete **slv_get_master_disunatt_list(slv_system_t);
/**< 
 ***  vlist = slv_get_*_*_list(sys)
 ***  slv_system_t sys;
 ***  struct var_variable **vlist;
 ***  int size;
 ***
 ***  slv_get_*_*_list will return the most recently set variable list
 ***  (never NULL) for the convenience of those who need it.
 ! !
 ! !  There are now 2 var lists: the master var list pulled of the instance
 ! !  tree, and the solvers var list isto be handed to the solvers.
 ! !  Eventually the solvers_varlist will only include those vars the specific
 ! !  solver needs to know about.
 ! !  For the moment, the content of the two lists is the same, but the ordering
 ! !  is not. The master list is in the order collected. The solvers list
 ! !  is reordered in some useful fashion defined by a client.
 ! !  Solver clients don't need to know about the master list. UI clients may.
 ***
 ***  Parameters are problem invariant constants that the GUI
 ***  user might change before solving another problem using the
 ***  same MODEL.
 **/

extern struct rel_relation **slv_get_solvers_rel_list(slv_system_t);
extern struct rel_relation **slv_get_solvers_condrel_list(slv_system_t);
extern struct rel_relation **slv_get_solvers_obj_list(slv_system_t);
extern struct logrel_relation **slv_get_solvers_logrel_list(slv_system_t);
extern struct logrel_relation **slv_get_solvers_condlogrel_list(slv_system_t);
extern struct w_when **slv_get_solvers_when_list(slv_system_t);
extern struct bnd_boundary **slv_get_solvers_bnd_list(slv_system_t);
extern struct rel_relation **slv_get_master_rel_list(slv_system_t);
extern struct rel_relation **slv_get_master_condrel_list(slv_system_t);
extern struct rel_relation **slv_get_master_obj_list(slv_system_t);
extern struct logrel_relation **slv_get_master_logrel_list(slv_system_t);
extern struct logrel_relation **slv_get_master_condlogrel_list(slv_system_t);
extern struct w_when **slv_get_master_when_list(slv_system_t);
extern struct bnd_boundary **slv_get_master_bnd_list(slv_system_t);
/***
 ***  Returns a list of relations, logical relations, boundaries,
 ***  objective relations and whens. 
 ***  The list is NULL terminated.
 ***  Example: For relations and objective relations:
 ***  rlist = slv_get_solvers_rel_list(sys)
 ***  olist = slv_get_solvers_obj_list(sys);
 ***  rlist = slv_get_master_rel_list(sys)
 ***  rlist = slv_get_master_obj_list(sys)
 ***  slv_system_t sys;
 ***  struct rel_relation **rlist;
 ***  struct rel_relation **olist;
 ***
 **/

extern struct gl_list_t *slv_get_symbol_list(slv_system_t);
/**< 
 * return the list of SymbolValues struct os a slv_sys
 */

extern int32 slv_need_consistency(slv_system_t);
/**<  
 * slv_need_consistency(sys);
 * Gets the int need_consitency associated with the system.
 */

extern int32 slv_get_num_solvers_vars(slv_system_t);
extern int32 slv_get_num_solvers_pars(slv_system_t);
extern int32 slv_get_num_solvers_unattached(slv_system_t);
extern int32 slv_get_num_solvers_dvars(slv_system_t);
extern int32 slv_get_num_solvers_disunatt(slv_system_t);
extern int32 slv_get_num_solvers_rels(slv_system_t);
extern int32 slv_get_num_solvers_condrels(slv_system_t);
extern int32 slv_get_num_solvers_objs(slv_system_t);
extern int32 slv_get_num_solvers_logrels(slv_system_t);
extern int32 slv_get_num_solvers_condlogrels(slv_system_t);
extern int32 slv_get_num_solvers_whens(slv_system_t);
extern int32 slv_get_num_solvers_bnds(slv_system_t);
extern int32 slv_get_num_master_vars(slv_system_t);
extern int32 slv_get_num_master_pars(slv_system_t);
extern int32 slv_get_num_master_unattached(slv_system_t);
extern int32 slv_get_num_master_dvars(slv_system_t);
extern int32 slv_get_num_master_disunatt(slv_system_t);
extern int32 slv_get_num_master_rels(slv_system_t);
extern int32 slv_get_num_master_condrels(slv_system_t);
extern int32 slv_get_num_master_objs(slv_system_t);
extern int32 slv_get_num_master_logrels(slv_system_t);
extern int32 slv_get_num_master_condlogrels(slv_system_t);
extern int32 slv_get_num_master_whens(slv_system_t);
extern int32 slv_get_num_master_bnds(slv_system_t);
/**< 
 ***  len = slv_get_num_*_*(sys);
 ***  slv_system_t sys;
 ***  int len;
 ***
 ***  Returns the length of the corresponding pointer list.
 ***  The length does NOT include the terminating NULL.
 ***  
 **/

extern int32 slv_get_num_models(slv_system_t);
/**< 
 ***  len = slv_get_num_models(sys);
 ***  slv_system_t sys;
 ***  int len;
 ***
 ***  Returns the number of models found in the tree the
 ***  problem was constructed from. There is no corresponding list.
 ***  Rel_relations will know which of these models they came from.
 **/

extern int32 slv_count_solvers_vars(slv_system_t,var_filter_t *);
extern int32 slv_count_solvers_pars(slv_system_t,var_filter_t *);
extern int32 slv_count_solvers_unattached(slv_system_t,var_filter_t *);
extern int32 slv_count_solvers_dvars(slv_system_t,dis_filter_t *);
extern int32 slv_count_solvers_disunatt(slv_system_t,dis_filter_t *);
extern int32 slv_count_solvers_rels(slv_system_t,rel_filter_t *);
extern int32 slv_count_solvers_condrels(slv_system_t,rel_filter_t *);
extern int32 slv_count_solvers_objs(slv_system_t,rel_filter_t *);
extern int32 slv_count_solvers_logrels(slv_system_t,logrel_filter_t *);
extern int32 slv_count_solvers_condlogrels(slv_system_t,logrel_filter_t *);
extern int32 slv_count_solvers_whens(slv_system_t,when_filter_t *);
extern int32 slv_count_solvers_bnds(slv_system_t,bnd_filter_t *);
extern int32 slv_count_master_vars(slv_system_t,var_filter_t *);
extern int32 slv_count_master_pars(slv_system_t,var_filter_t *);
extern int32 slv_count_master_unattached(slv_system_t,var_filter_t *);
extern int32 slv_count_master_dvars(slv_system_t,dis_filter_t *);
extern int32 slv_count_master_disunatt(slv_system_t,dis_filter_t *);
extern int32 slv_count_master_rels(slv_system_t,rel_filter_t *);
extern int32 slv_count_master_condrels(slv_system_t,rel_filter_t *);
extern int32 slv_count_master_objs(slv_system_t,rel_filter_t *);
extern int32 slv_count_master_logrels(slv_system_t,logrel_filter_t *);
extern int32 slv_count_master_condlogrels(slv_system_t,logrel_filter_t *);
extern int32 slv_count_master_whens(slv_system_t,when_filter_t *);
extern int32 slv_count_master_bnds(slv_system_t,bnd_filter_t *);
/**< 
 ***  count = slv_count_*_*(sys,vfilter)
 ***  count = slv_count_*_*rels(sys,rfilter)
 ***  slv_system_t sys;
 ***  var_filter_t *vfilter;
 ***  rel_filter_t *rfilter;
 ***  int count;
 ***
 ***  Counts the variables, boundaries, relations or whens currently
 ***  in the system's solvers or master list which match the
 ***  specified filter. 
 ***
 ***  Efficiency note: if you are using this with a match anything
 ***  filter, you would be better off just calling the slv_get_num_*
 ***  function for the list in question.
 **/

/**************************************************************************\
  Registered client queries.
\**************************************************************************/
extern void slv_set_obj_relation(slv_system_t,struct rel_relation *);
extern struct rel_relation *slv_get_obj_relation(slv_system_t);
/**< 
 ***  slv_set_obj_relation(sys,obj)
 ***  obj = slv_get_obj_relation(sys)
 ***  slv_system_t sys;
 ***  struct rel_relation *obj;
 ***
 ***  slv_set_obj_relation sets the objective relation of the solver to the
 ***  given one which should come from the objective list.  A special value
 ***  of NULL for the objective function indicates no objective function.
 ***
 ***  slv_get_obj_relation returns the internal copy of the objective
 ***  function, or NULL if none was specified.
 ***
 ***  Client solvers should minimize the residual of this equation.
 **/

extern void slv_set_obj_variable(slv_system_t, struct var_variable *,unsigned);
extern struct var_variable *slv_get_obj_variable(slv_system_t);
extern real64 slv_get_obj_variable_gradient(slv_system_t);
/**< 
 ***  slv_set_obj_variable(sys,objvar,maximize);
 ***  objvar = slv_get_obj_variable(sys);
 ***  objvar_grad = slv_get_obj_variable_gradient(sys);
 ***  slv_system_t sys;
 ***  struct var_variable *var;
 ***  unsigned int maximize;
 ***  real64 objvar_grad;
 ***
 ***  By default, the objective var is NULL, even if there is
 ***  and objective relation (maximize,minimize) in the ASCEND MODEL.
 ***  (ascend MODEL objectives are handled with obj_relation functions)
 ***  Optimizers should use objective var in preference to the obj
 ***  relation if the var is defined.
 ***
 ***  slv_set_obj_variable
 ***  specifies the var to use for an objective and whether it should
 ***  be maximized or minimized. Var must be from the slv_system or
 ***  complete insanity may result.
 *** 
 ***  slv_get_obj_variable
 ***  Returns the var used for an objective or NULL if none set.
 ***
 ***  slv_get_obj_variable_gradient
 ***  Returns the unscaled gradient of the objective variable, or 0
 ***  if no var is set.
 ***  There is no value function here. just use var_value
 ***
 ***  Client solvers should minimize this variable.
 **/

extern int slv_eligible_solver(slv_system_t);
/**< 
 ***  eligible = slv_eligible_solver(sys)
 ***  int eligible;
 ***  slv_system_t sys;
 ***
 ***  Determines whether or not the current solver. 
 ***  is capable of solving the given system as it is currently set up
 ***  (e.g. some solvers cannot do optimization, or inequalities, etc.).
 ***  The system must be set up first before calling this function, or the
 ***  return value may be misleading.
 ***  The solver in question will be asked to pass judgement on the
 ***  data in the slv_system_t wrt the solver being useful.
 ***  If no solver is registered, this returns FALSE.
 **/

extern int slv_select_solver(slv_system_t,int);
extern int slv_get_selected_solver(slv_system_t);
/**< 
 ***  solver=slv_select_solver(sys,solver)
 ***  solver = slv_get_selected_solver(sys)
 ***  slv_system_t sys;
 ***  int solver;
 ***
 ***  slv_select_solver sets the given solver to be the current solver
 ***  for the system.  The intelligence or stupidity of this move is not
 ***  investigated. If the system has already has a solver selected and
 ***  it is not the same solver, the data structures of the old selection
 ***  will be blown away.
 ***  Return value is number of solver actually selected.
 ***  If failure, return is -1;
 ***
 ***  slv_get_selected_solver returns the current solver number.
 **/

extern int slv_switch_solver(slv_system_t,int);
/**< 
 ***  solver=slv_switch_solver(sys,solver)
 ***  slv_system_t sys;
 ***  int solver;
 ***
 ***  slv_switch_solver sets the given solver to be the current solver
 ***  for the system.
 ***  Return value is number of solver actually selected.
 ***  If failure, return is -1;
 ***
 **/

extern int32 slv_get_default_parameters(int32,slv_parameters_t *);
/**< KHACK: NEED TO COMMENT */

extern void slv_get_parameters(slv_system_t,slv_parameters_t *);
extern void slv_set_parameters(slv_system_t,slv_parameters_t *);
/**< 
 ***  slv_get_parameters(sys,parameters)
 ***  slv_set_parameters(sys,parameters)
 ***  slv_system_t sys;
 ***  slv_parameters_t *parameters;
 ***
 ***  (Do not confuse these parameters [algorithm control variables]
 ***   with the parameter list which is a list of pointers to var_variable.)
 ***
 ***  slv_get_parameters will copy the current system parameters to the
 ***  given structure.
 ***
 ***  slv_set_parameters will set the current system parameters to the
 ***  values contained in the given structure.  It is recommended that one
 ***  gets the parameters first, before one modifies them and sets them,
 ***  especially if not all of the parameters are to be modified (and you
 ***  never know when that might suddenly become true because a new
 ***  parameter was added to the structure).  Parameters will only be
 ***  accepted by an engine if they came from that engine, so fetching
 ***  before setting is not only a good idea, it's the law (gas engines
 ***  don't run on diesel very well...).
 **/

extern SlvClientToken slv_get_client_token(slv_system_t);
extern void slv_set_client_token(slv_system_t,SlvClientToken);
/**< 
 ***  ct=slv_get_client_token(sys)
 ***  slv_system_t sys;
 ***  Return/sets  the client token of the system_t
 **/

extern void slv_set_solver_index(slv_system_t, int);
/**< 
 ***  slv_set_solver_index(sys)
 ***  slv_system_t sys;
 ***  Sets the solver index of the slv_system_t
 **/

extern void slv_get_status(slv_system_t,slv_status_t *);
/**< 
 ***  slv_get_status(sys,status)
 ***  slv_system_t sys;
 ***  slv_status_t *status;
 ***
 ***  Copies the current system status into the given structure.
 **/

extern linsolqr_system_t slv_get_linsolqr_sys(slv_system_t);
/**< THIS CALL SHOULD GO AWAY */
/**< 
 ***  lqrsys = slv_get_linsolqr_sys(sys)
 ***  linsolqr_system_t lqrsys;
 ***  slv_system_t sys;
 ***
 ***  Returns the linsolqr system used, or NULL if none.
 **/

extern linsol_system_t slv_get_linsol_sys(slv_system_t);
/**< THIS CALL SHOULD GO AWAY */
/**< 
 ***  lsys = slv_get_linsol_sys(sys)
 ***  linsol_system_t lsys;
 ***  slv_system_t sys;
 ***
 ***  Returns the linsol system used, or NULL if none.
 **/

extern mtx_matrix_t slv_get_sys_mtx(slv_system_t);
/**< THIS CALL SHOULD GO AWAY */
/**< 
 ***  mtx = slv_get_sys_mtx(sys)
 ***  mtx_matrix_t mtx;
 ***  slv_system_t sys;
 ***
 ***  Returns the mtx  used, or NULL if none. The user should check.
 **/

extern void slv_dump_internals(slv_system_t,int);
/**< 
 ***  slv_dump_internals(sys,level)
 ***  slv_system_t sys;
 ***  int level;
 ***
 ***  Will spew whatever the solver interface developer feels like to
 ***  stderr.  Larger values of level will give more detailed information,
 ***  we hope.  No specification is made with regard to what the
 ***  information will be.  returns -1 if solver gutless.  This is provided
 ***  principally to facilitate debugging a little.
 **/

extern void slv_presolve(slv_system_t);
/**< 
 ***  slv_presolve(sys)
 ***  slv_system_t sys;
 ***
 ***  Prepares the system for solving.  This must be called before the
 ***  system is solved, but after everything about the system is set up
 ***  (i.e. variables and relations cannot be changed IN ANY WAY, objective
 ***  function cannot be modified, boundaries cannot be modified, or even
 ***  repermuted, and a new solver cannot be selected: some parameters may
 ***  be modified, they will be marked as such).  The system essentially
 ***  becomes "read-only".  If anything is modified after slv_presolve was
 ***  called, slv_presolve must be called again before solving (EXCEPTIONS:
 ***  slv_resolve may do for a certain restricted class of changes).
 ***
 ***  It is at this point that the variable list is created if it does not
 ***  already exist and the newly created variables are indexed in the
 ***  order they end up.  The relation list is indexed as well in the order
 ***  it is received.
 ***
 ***  Among other things, this function will perform structural analysis
 ***  so that structural analysis flags in the status will be accurate.
 **/

extern void slv_resolve(slv_system_t);
/**< 
 ***  slv_resolve(sys)
 ***  slv_system_t sys;
 ***
 ***  This function re-prepares the system for solving.  This function may
 ***  be used instead of slv_presolve, provided the system was partially
 ***  or completely solved before, and then the only changes to the system
 ***  since are as follows:
 ***     - any parameter except "partition".
 ***     - variable values.
 ***     - variable nominal values.
 ***     - variable bounds.
 ***
 ***  In particular, the following changes are NOT allowed:
 ***     - variable fixed flag.
 ***     - relation included flag.
 ***     - variable/relation list contents, including order.  Also, the
 ***       variable/relation indices must continue to be consistent with
 ***       the list.
 ***     - definition of relations, objective function, and boundaries:
 ***       including structural rearrangement on relations, although any
 ***       expression may be simplified.
 ***
 ***  This function is considerably more efficient when it is usable.
 **/

extern void slv_iterate(slv_system_t);
extern void slv_solve(slv_system_t);
/**< 
 ***  slv_iterate(sys)
 ***  slv_solve(sys)
 ***  slv_system_t sys;
 ***
 ***  slv_iterate performs one iteration toward the ultimate solution (or
 ***  failure thereof) of the system.  The user can obtain information
 ***  from the status and from the variables and relations themselves
 ***  (some care should be taken in examining the residuals of relations;
 ***  they may not be up to date).  The user may not modify the system in
 ***  any way between iterations (i.e. you may look, but don't touch: see
 ***  slv_presolve).
 ***
 ***  slv_solve attempts to solve the entire system in one shot (i.e.
 ***  performs as many iterations as needed or allowed).  For some solvers,
 ***  slv_iterate and slv_solve may mean the same thing.
 **/

extern void slv_destroy_client(slv_system_t);
/**< 
 ***  slv_destroy_client(sys)
 ***  slv_system_t sys;
 ***  Destroy the client token of slv_system_t. It does not deallocate
 ***  the allocated data space of sys
 **/

extern boolean slv_change_basis(slv_system_t,int32,mtx_range_t *);
/**< THIS CALL SHOULD GO AWAY */
/**< 
 *** slv_change_basis (sys,index,rng);
 *** slv_system_t sys;
 *** int32 index;
 *** mtx_range_t *rng;
 ***
 *** Move var (given by index #) to the unassigned region (far right)
 *** of the solver matrix if possible. returns FALSE if impossible
 *** because structural infeasibility would occur or because solver selected
 *** won't do it.
 **/

extern void slv_print_output(FILE *, slv_system_t);

/**< 
 *** start of some report generation routines. Just prints out
 *** the variable values and residuals at the moment. Will be
 *** made more general in the future.
 **/

#endif
