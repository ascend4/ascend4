/**< 
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: slv2.h,v $
 *  Date last modified: $Date: 1997/07/18 12:16:05 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */
#ifndef slv2__already_included
#define slv2__already_included
/**< requires #include "slv_client.h" */

typedef struct slv2_system_structure *slv2_system_t;

int slv2_register(SlvFunctionsT *);
/**********************************************************************\
  This is the function that tells the system about the Opt solver.
  Our index is not necessarily going to be 2. That everything here is
  named slv2* is just a historical event.
\**********************************************************************/

#endif

/**< REMOVE EVERYTHING BELOW THIS POINT */     
# if 0 
/*********************************************************************
This file created by make_slv_header, so don't modify it yourself.
All functions defined in this header have identical protocols to the
corresponding functions in slv.h except that slv_system_t ==>
slv2_system_t and slv2_eligible_solver() only takes one parameter:
the system.  Note also that the select solver functions don't exist.
*********************************************************************/

typedef struct slv2_system_structure *slv2_system_t;

#ifdef STATIC_OPTSQP
#define slv2_solver_name "OPT"   /**< Solver's name */
#define slv2_solver_number 2   /**< Solver's number */

extern slv2_system_t slv2_create();
extern int slv2_destroy(); /**< note error return needs to be implemented */
extern void slv2_set_rel_list();
extern struct rel_relation **slv2_get_rel_list();
extern void slv2_set_extrel_list();
extern struct ExtRelCache **slv2_get_extrel_list();
extern void slv2_set_var_list();
extern struct var_variable **slv2_get_var_list();
extern void slv2_set_bnd_list();
extern bnd_boundary_t *slv2_get_bnd_list();
extern int slv2_count_rels();
extern int slv2_count_vars();
extern int slv2_count_bnds();
extern void slv2_set_obj_relation();
extern expr_t slv2_get_obj_relation();
extern boolean slv2_eligible_solver();
extern void slv2_get_parameters();
extern void slv2_set_parameters();
extern void slv2_get_status();
extern linsol_system_t slv2_get_linsol_sys();
extern slv2_dump_internals();
extern int slv2_presolve();
extern boolean slv2_change_basis();
extern void slv2_resolve();
extern void slv2_iterate();
extern void slv2_solve();

/**< the following will be stomped on by make_slvheaders. beware. */
#define slv2_IA_SIZE 9
#define slv2_RA_SIZE 2
#define slv2_CA_SIZE 0
#define slv2_VA_SIZE 0

/*** subscripts for ia ***/
#define SP2_OPTION 0
#define SP2_ISCALE 1
#define SP2_ICHOOSE 2
#define SP2_IMULT 3
#define SP2_ISAFE 4
#define SP2_ICORR 5
#define SP2_KPRINT 6
#define SP2_IIEXACT 7
#define SP2_IDEBUG 8

/*** subscripts for ra ***/
#define SP2_EPS 0
#define SP2_VV 1

/*** subscripts for ca ***/

/*** subscripts for va ***/

#else
#define slv2_solver_name "no_opt"   /**< Solver's name */
#define slv2_solver_number 2   /**< Solver's number */
#endif

#endif
