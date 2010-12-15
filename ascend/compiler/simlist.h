/*	ASCEND modelling environment
	Copyright (C) 1997 Benjamin Andrew Allan
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
	Simulation Management for Ascend

	This module initializes and manages a global list of simulations.
	These may be interrelated in very twisty ways due to UNIVERSAL and
	parameter passing.

	@TODO Simulations need much better management than they currently get,
	once we start building simulations out of other simulations.
	For now this file is largely empty.

	FIXME the PyGTK doesn't use the g_simulation_list because it only has a
	single simulation at a time. So beware of using Asc_DestroySimulations if
	you haven't recorded your simulation.
*//*
	by Ben Allan
	Version: $Revision: 1.2 $
	Version control file: $RCSfile: simlist.h,v $
	Date last modified: $Date: 1997/07/18 12:34:54 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_SIMLIST_H
#define ASC_SIMLIST_H

#include "compiler.h"
#include "instance_enum.h"
#include <ascend/general/list.h>

/**	@addtogroup compiler_simlist Compiler Simulation List
	@{
*/

ASC_DLLSPEC int g_compiler_timing;
/**<  Global flag for whether to perform timing of compiler operations. */

ASC_DLLSPEC struct gl_list_t *g_simulation_list;
/**<  Global simulation list.*/

ASC_DLLSPEC void sim_destroy(struct Instance *inst);
/**<
 *  Destroys the instance given.
 *  inst should be a simulation instance, and may be NULL.
 */

/** Compilation types for SimsCreateInstance(). */
enum CreateInst_format {
  e_normal = 0,     /**< Normal compilation (default). */
  e_no_relations,   /**< Compile with no relations. */
  e_patch           /**< Compile a patch. */
};

ASC_DLLSPEC struct Instance *SimsCreateInstance(
	symchar *type,
	symchar *name,
	enum CreateInst_format format,
	symchar *defmethod
);
/**<
	Creates a new simulation instance.
	This function sets up the call to instantiate with different
	compiler settings.  In all cases Instantiate() will make a copy
	of the name that is given.  format should perhaps be an array
	of enums or a bit structure to deal with multiple compilation
	flags. At the moment it is just a simple enum.  NULL is
	returned if either type or name is NULL.

	The returned instance should be destroyed by the caller using
	sim_destroy() or Asc_DestroySimulations().

	@param type      Name of the model type to create.
	@param name      Name to give the new simulation.
	@param format    Type of compilation to perform.
	@param defmethod The method to call after instantiation, if present.
	@return A pointer to the newly-created simulation instance.

	@TODO is there a good reason why we don't pass a TypeDescription* instead
	of a symchar for the name?
*/

ASC_DLLSPEC void Asc_DestroySimulations(void);
/**<  Destroys all known instances on the simulation list. */


/*
 *  Stuff from SimsProc.h
 */

ASC_DLLSPEC struct Instance *Asc_FindSimulationTop(symchar *str);
/**<  Return the sim pointer if there is a sim named *str. */

ASC_DLLSPEC struct Instance *Asc_FindSimulationRoot(symchar *str);
/**<  Return the top inst pointer if there is a sim named *str. */

ASC_DLLSPEC symchar *Asc_SimsFindSimulationName(CONST struct Instance *sim);
/**<
 *  Return the name string of the instance given if it is in the
 *  global sim list. Return null if not.
 */

ASC_DLLSPEC void Asc_SetCurrentSim(struct Instance *sim);
/**<  Sets the current working simulation to the specified simulation. */

ASC_DLLSPEC struct Instance *Asc_GetCurrentSim(void);
/**<
 *  Returns a pointer to the current working simulation.
 *  Makes no checks on the state of the simulation.
 */

ASC_DLLSPEC int Asc_SimsUniqueName(symchar *str);
/**<
 *  Checks whether a simulation exists having specified name.
 *  Returns 0 if the name was found, 1 otherwise.
 */

ASC_DLLSPEC int Asc_SimsCmpSim(struct Instance *sim1, struct Instance *sim2);
/**<
 *  Compares two simulations, based on their names.
 *  Returns 0 if they are the same, non-zero if different.
 */

ASC_DLLSPEC void sim_destroy(struct Instance *sim);
/**<
 *  Deallocates a simulation instance.
 *  The instance and its associated root instance are destroyed using
 *  DestroyInstance() (in instance*.h).  The name string i(which it owns)
 *  is also destroyed, any external vars associated with the simulation
 *  are cleaned up.<br><br>
 *
 *  NOTE:
 *  sim is the *Top* of the simulation and *not* the root. As such this
 *  function works on instances of kind SIM_INST.
 */

/* @} */

#endif  /* ASC_SIMLIST_H */
