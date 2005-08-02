/*
 *  DebugProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.14 $
 *  Version control file: $RCSfile: DebugProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
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
 *  Debugging Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/DebugProc.h"
 *  </pre>
 */

#ifndef DebugProc_module_loaded
#define DebugProc_module_loaded

extern int Asc_DebuGetBlkOfVar(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Get the block number for a variable.
 *  Returns "none" if var is not in a partition (not assigned included).
 *  Returns tclerror if var isn't in system.
 *  Returns block # that var is in from C structure.
 *  Assumes g_solvsys_cur exists.
 *  var_block wrapper <br><br>
 *  
 *  Registered as:  dbg_get_blk_of_var varindex
 */

extern int Asc_DebuGetBlkOfEqn(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Get the block number for an equation.
 *  Returns "none" if eqn is not in a partition (not assigned included).
 *  Returns tclerror if eqn isn't in system.
 *  Returns block # that eqn is in from C structure.
 *  Assumes g_solvsys_cur exists.
 *  rel_block wrapper <br><br>
 *  
 *  Registered as:  dbg_get_blk_of_eqn eqnindex
 */

extern int Asc_DebuGetBlkCoords(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  Get the coordinates of a block.
 *  returns "none" if block is not a partition.
 *  Returns collo rowlo colhi rowhi from C structure.
 *  Assumes g_solvsys_cur exists.
 *  mtx_block wrapper <br><br>
 *  
 *  Registered as:  dbg_get_blk_coords blocknumber
 */

extern int Asc_DebuGetEqnOfVar(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Get the equation number for a variable.
 *  Returns "none" if var is not assigned (fixed or not incident).
 *  Returns tclerror if var isn't in system.
 *  Returns eqn # that var is assigned to from C structure.
 *  Assumes g_solvsys_cur. not yet working. needs some linsol. <br><br>
 *  
 *  Registered as:  dbg_get_eqn_of_var  varindex
 */

extern int Asc_DebuGetVarPartition(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Return a / and space delimited list of the variables in the system.
 *  / separates partitions, space separates var #s. There's a trailing
 *  / at end.
 *  No args. <br><br>
 *  
 *  Registered as:  dbg_get_varpartition
 */

extern int Asc_DebuGetEqnPartition(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Return a / and space delimited list of the equations in the system.
 *  / separates partitions, space separates eqn #s. There's a trailing
 *  / at end.
 *  No args. <br><br>
 *  
 *  Registered as:  dbg_get_eqnpartition
 */

extern int Asc_DebuListRels(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Return the C index list of rels matching the simple filter specified.
 *  No filter --> usage message,error.
 *  Not all of filter.h is yet supported.
 *  Unrecognized filters return an error.
 *  Any argument in the anything position cause negation of the filter. <br><br>
 *  
 *  Registered as:  dbg_list_rels <filtercode> [anything]
 */

extern int Asc_DebuListVars(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Return the C index list of vars matching the simple filter specified.
 *  No filter --> usage message,error.
 *  Unrecognized filters return an error.
 *  An argument in the anything position cause negation of the filter.
 *  Note: solvers which do not partition do not assign, e.g. MINOS
 *  leaves all variables 'unassigned'.<br><br>
 *  
 *  Registered as: dbg_list_vars <filtercode> [anything]
 */

extern int Asc_DebuWriteVar(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Writes a real variable.  Conditions:
 *   -# output device; stderr stdout or interp (0,1,2)
 *   -# it's C index number
 *   -# with detail level: 0 name 1 val 2 dims 3 var# 4 fixedflag
 *      5 lower_bound 6 nominal 7 upper_bound
 *   -# the selection of the master list or
 *      solvers list of variables. 0 is solver list, 1 is master.
 *   -# with optional string prepended to the qlfdid
 *
 *  Name returned is qualified id from root instance g_solvinst_root.
 *  The string is not prepended to the interp qlfdid,
 *  an ampersand marks the root and should be edited in tcl before
 *  displaying the result. interp is returned as a list.
 *  If cdata 1,uses g_browsys_cur instead of g_solvsys_cur.<br><br>
 *  
 *  Registered as:  dbg_write_var out# Cindex detail# [string]
 */

extern int Asc_DebuWriteUnattachedVar(ClientData cdata, Tcl_Interp *interp,
                                      int argc, CONST84 char *argv[]);
/**<
 *  Writes the list of unattached variables to stderr, stdout, or interp
 *  (0,1,2).
 *  Name returned is qualified id from root instance g_solvinst_root.
 *  The string is not prepended to the interp qlfdid,
 *  an ampersand marks the root and should be edited in tcl before
 *  displaying the result. interp is returned as a list.
 *  If cdata 1,uses g_browsys_cur instead of g_solvsys_cur.<br><br>
 *
 *  It is not assumed anymore that the unattached variables are in the
 *  solver list of variables. There is a specific list now ofr this
 *  nonincident variables.<br><br>
 *
 *  Registered as:  dbg_write_unattvar out#  [simname]
 */

extern int Asc_DebuWriteRel(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Writes a relation to stderr stdout or interp (0,1,2).
 *  Conditions:
 *    -# it's a C index number
 *    -# with detail level 0 name 1 resid 2 rel# 3 include flag
 *    -# cumulative
 *
 *  If detail level is 4, only the relation itself is written/returned.
 *  Name returned is qualified id from root instance g_solvinst_root.
 *  The string is not prepended to the interp qlfdid,
 *  an ampersand marks the root and should be edited in tcl before
 *  displaying the result. interp is returned as a list
 *  If cdata 1,uses g_browsys_cur instead of g_solvsys_cur.<br><br>
 *
 *  SIDE EFFECTS: updates the residual of the relation examined.
 *  bugs: detail level is not exactly analogous to write_Var
 *  does not trap fp exceptions. <br><br>
 *
 *  Registered as:  dbg_write_rel <out> <Cindex> <detail> [string]
 */

extern int Asc_DebuWriteObj(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Same as Asc_DebuWriteRel() except applies to objective relations. <br><br>
 *  
 *  Registered as:  dbg_write_obj
 */

extern int Asc_DebuWriteVarAttr(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  Writes a solver_var instance including some ip properties.
 *  Conditions:  it's a C index number in a list:
 *  atom type, dims, value, shortest qlfdid, ip data items
 *  atom children and values.
 *  Current ip data items are var.index, var.incident, and var.in_block.
 *
 *  When called s dbg_write_qlfattr <qlfdid> (cdata==1), same as
 *  dbg_write_varattr except takes qlfdid and
 *  if ip is null (var has never been to solver) ip data will be -1<br><br>
 *
 *  If something is amiss with the inst, returns nothing and TCL_ERROR. <br><br>
 *
 *  Dual registered as:
 *    - dbg_write_varattr <Cindex> (cdata==0)
 *    - dbg_write_qlfattr <qlfdid> (cdata==1)
 */

extern int Asc_DebuRelIncluded(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Returns 1 if rel included flag set true, 0 if not.
 *  This call, along with Asc_DebuVarFixed(), is used primarily 
 *  for speed in the incidence matrix construction.  The normal 
 *  rel and var queries give more useful information in most cases. <br><br>
 *
 *  Registered as:  dbg_rel_included <index>
 */
extern int Asc_DebuVarFixed(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Returns 1 if var fixed flag set true, 0 if not.
 *  This call, along with Asc_DebuRelIncluded(), is used primarily
 *  for speed in the incidence matrix construction.  The normal 
 *  rel and var queries give more useful information in most cases. <br><br>
 *
 *  Registered as:  dbg_var_fixed <index>
 */

extern int Asc_DebuGetIncidence(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  Returns the list of indexes of variables occuring in the relation
 *  index given.  Uses NumberVaribles and RelationVariable routines to
 *  determine incidence, so it does not rely on a specific solver to set
 *  incidence flags.  Uses the rel and var lists from g_solvsys_cur. <br><br>
 *
 *  Registered as:  dbg_get_incidence
 */

extern int Asc_DebuGetOrder(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Returns the list of original indexes as found running
 *  along the mtx of the current system's linsol_sys.  Will be
 *  relation indexes if arg is row. Will be var indexes if col <br><br>
 *
 *  Registered as:  dbg_get_order  <row,col>
 */

extern int Asc_DebuWriteIncidence(ClientData cdata, Tcl_Interp *interp,
                                  int argc, CONST84 char *argv[]);
/**<
 *  Dumps the incidence matrix from slv. <br><br>
 *
 *  Registered as:  dbg_write_incidence <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuFindEligible(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  Output will be a list of indexes of the eligible variables
 *  that if fixed reduce the excess column count of the system.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices.
 *  The DOF solver will do its own presolve, so our DOF analysis may
 *  not line up with an arbitrary clients.
 *  The list may come back empty if appropriate. 
 *  Operates on g_solvsys_cur. <br><br>
 *
 *  Registered as:  dbg_find_eligible <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuConsistencyAnalysis(ClientData cdata, Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/**<
 *  Operates on g_solvsys_cur.
 *  Output will a set of indices of the variables , which, if fixed, will
 *  result in all the alternatives of a conditional model being square and
 *  structurally consistent.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices.
 *  The list may come back empty if appropriate.  <br><br>
 *
 *  Registered as:  dbg_consistency_analysis <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuFindGlobalEligible(ClientData cdata, Tcl_Interp *interp,
                                      int argc, CONST84 char *argv[]);
/**<
 *  Operates on g_solvsys_cur.
 *  Output will be a list of indexes of the eligible variables
 *  that if fixed reduce the excess column count of the system. The
 *  resulting set is of eligible variables consistent will all the
 *  alternatives in a conditional model.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices.
 *  The list may come back empty if appropriate. <br><br>
 *
 *  Registered as:  dbg_global_eligible <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuInstEligible(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Operates on newly made g_browsys_cur made from curinst
 *  and destroys g_browsys_cur when done, leaving it NULL.
 *  Output will be a list of indexes of the eligible variables
 *  that if fixed reduce the excess column count of the system.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices.
 *  The list may come back empty if appropriate.
 *  if dest is 0 or 1, output wil be names instead of indices.  <br><br>
 *
 *  Registered as:  brow_find_eligible <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuFindActive(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/**<
 *  Operates on g_solvsys_cur.
 *  Output will be a list of indexes of the active relations in the
 *  system.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices. <br><br>
 *
 *  Registered as:  dbg_find_activerels <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuInstActive(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/**<
 *  Operates on newly made g_browsys_cur made from curinst
 *  and destroys g_browsys_cur when done, leaving it NULL.
 *  Output will be a list of indexes of the active relations
 *  of the system.
 *  This list will be enclosed in {} so the output of the function is
 *  always a 1 item list whose first item is the list of indices.
 *  if dest is 0 or 1, output wil be names instead of indices. <br><br>
 *
 *  Registered as:  brow_find_activerels <dest> 0= stdout 1 = stderr 2= list
 */

extern int Asc_DebuNumBlockSing(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  output will be a list(or lists):
 *  {index {eqn coef} {eqn coef}}
 *  where index is the index of an unpivoted relation and the set of
 *  eqn coef pairs is the linear combination that contribute.
 *  Interpretation of the coefs (how small is zero?) is left to tcl.
 *  Dependency will be done with rows or colums as specified.
 *  The lists may come back empty if appropriate.
 *  Works only for Slv, since no other solver is currently blocking. <br><br>
 *
 *  Registered as:  dbg_num_block_singular <dest> 0= stdout 1 = stderr 2= interp
 *  <block #> <row,col>
 */

extern int Asc_DebuStructSing(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/**<
 *  Output will be a list of indexes of the responsible relations
 *  and a list of indexes of the vars in the singularity(s)
 *  and a list of vars that reduce singularity if freed.
 *  If dest is 0 or 1, list will be names instead of indices.
 *  relindex should be the index of an unassigned but included row, or
 *  -1 if you want the total cause of all singular rows.
 *  The lists may come back empty if appropriate. <br><br>
 *
 *  Registered as:  dbg_struct_singular <dest> 0= stdout 1 = stderr 2= list <relindex>
 */

extern int Asc_DebuVarFree2Nom(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Moves all free vars in current solve system to their nominal values.
 *  This is basically a crash recovery call. Vars must also be incident. <br><br>
 *
 *  Registered as:  var_free2nom <no args>
 */

extern int Asc_DebuVarNom2Free(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Moves all free vars nominals in current solve system to var values.
 *  Vars must also be incident. <br><br>
 *
 *  Registered as:  var_nom2free <no args>
 */

extern int Asc_DebuCheckRelFp(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/**<
 *  Returns a list which is
 *  {{index lstat rstat dlstat drstat} ...}.
 *  There is a quintuplet for each relation in the current solver list
 *  which produces a float or calc_ok error.
 *  lstat rstat reflect any problems evaluating the function l/rhs
 *  dl/rstat reflect any problems evaluating the derivatives of l/rhs
 *  A stat flag of 0 -> ok, 1 -> exception. <br><br>
 *
 *  Registered as:  dbg_check_rels
 *
 *  @todo In Asc_DebuCheckRelFp(), dlstat, drstat are not 
 *        currently implemented: always 0.
 */

extern int Asc_DebuCalcRelNominals(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Returns a list which is
 *  {{index lstat rstat nom} ...}.
 *  There is a quintuplet for each relation in the current solver list
 *  which produces a float or calc_ok error.
 *  lstat rstat reflect any problems evaluating the function l/rhs
 *  A stat flag of 1 -> !calc_ok.  2 -> float error. 0->ok
 *  nom is the nominal the relation was left with after the error. <br><br>
 *
 *  Registered as:  dbg_calc_relnoms
 */

extern int Asc_DebuWriteSystem(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Write a system description suitable for a standalone slv0 to the
 *  file given. destroys anything in the file if it already exists.
 *  All values will be in internal units (si) <br><br>
 *
 *  Registered as:  dbg_write_slv0_sys
 *
 *  @todo Asc_DebuWriteSystem() doesn't deal with bnds.
 */

extern int Asc_DebuWriteKirkSystem(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  KAA_DEBUG.
 *  Registered as:  dbg_write_kirk_xsys
 */
extern int Asc_DebuWriteGAMSSystem(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  KAA_DEBUG.
 *  Registered as:  dbg_write_gams_xsys
 */

extern int Asc_DebuMtxCalcJacobianCmd(ClientData cdata, Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[]);
/**<
 *  The jacobian is calculated at the current point.
 *  At the moment which solver must be an int which is the index of
 *  the solver. Only slv5 is now supported. <br><br>
 *
 *  Registered as:  dbg_calc_jacobian <whichsolver>
 */

extern int Asc_DebuMtxWritePlotCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Write a mtx to the named file.
 *  Arguments:
 *   - file is an openable file name or path/filename
 *   - type is 'coeff' (coefficient) or anything else (inverse)
 *   - fmt is the file format to use ('mtx','smms','plot','csr')
 *
 *  Trashes the file if the name already exists.
 *  smms, plot, and csr uses the symbolic rank to determine the size
 *  of the region to write.
 *  mtx writes the entire matrix.
 *  A lot smarter.
 *  Deals only with solvers which can produce an mtx.<br><br>
 *
 *  WARNINGS:
 *  This is not a command for use in a production interface -- in
 *  particular solvers are not and should not be in the habit of exposing
 *  their factorized matrices to casual users. <br><br>
 *
 *  Registered as:  debug_mtxwriteplot <file> <type> <fmt>
 *
 *  @todo Asc_DebuMtxWritePlotCmd() does not accept a block no.
 *  @todo Asc_DebuMtxWritePlotCmd() does not accept a set of 
 *        region coordinates.
 */

extern int Asc_DebuHelpList(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Debug help command for tcl.
 *  No arg -> return tcl list.
 *  "s" -> list names only, "l" -> short explanations also, to stderr. <br><br>
 *
 *  Registered as:  dbghelp [s,l]
 */

#endif  /* DebugProc_module_loaded */

