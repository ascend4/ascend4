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

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
	#include "compiler/compiler.h"
	#include "general/list.h"
*//*
	by Ben Allan
	Version: $Revision: 1.2 $
	Version control file: $RCSfile: simlist.h,v $
	Date last modified: $Date: 1997/07/18 12:34:54 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_SIMLIST_H
#define ASC_SIMLIST_H

#include <utilities/ascConfig.h>

extern int g_compiler_timing;
/**<  Global flag for whether to perform timing of compiler operations. */

extern struct gl_list_t *g_simulation_list;
/**<  Global simulation list.*/

extern void Asc_DeAllocSim(struct Instance *inst);
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

extern struct ASC_DLLSPEC(Instance *) SimsCreateInstance(
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
	Asc_DeAllocSim() or Asc_DestroySimulations().

	@param type      Name of the model type to create.
	@param name      Name to give the new simulation.
	@param format    Type of compilation to perform.
	@param defmethod The method to call after instantiation, if present.
	@return A pointer to the newly-created simulation instance.
*/

extern ASC_DLLSPEC(void) Asc_DestroySimulations(void);
/**<  Destroys all known instances on the simulation list. */


/*
 *  Stuff from SimsProc.h
 */

extern struct Instance *Asc_FindSimulationTop(symchar *str);
/**<  Return the sim pointer if there is a sim named *str. */

extern struct Instance *Asc_FindSimulationRoot(symchar *str);
/**<  Return the top inst pointer if there is a sim named *str. */

extern symchar *Asc_SimsFindSimulationName(CONST struct Instance *sim);
/**<
 *  Return the name string of the instance given if it is in the
 *  global sim list. Return null if not.
 */

extern void Asc_SetCurrentSim(struct Instance *sim);
/**<  Sets the current working simulation to the specified simulation. */

extern struct Instance *Asc_GetCurrentSim(void);
/**<
 *  Returns a pointer to the current working simulation.
 *  Makes no checks on the state of the simulation.
 */

extern int Asc_SimsUniqueName(symchar *str);
/**<
 *  Checks whether a simulation exists having specified name.
 *  Returns 0 if the name was found, 1 otherwise.
 */

extern int Asc_SimsCmpSim(struct Instance *sim1, struct Instance *sim2);
/**<
 *  Compares two simulations, based on their names.
 *  Returns 0 if they are the same, non-zero if different.
 */

extern void Asc_DeAllocSim(struct Instance *sim);
/**<
 *  Deallocates a simulation instance.
 *  The instance and its associated root instance are destroyed using
 *  DestroyInstance() (in instance.h).  The name string i(which it owns)
 *  is also destroyed, any external vars associated with the simulation
 *  are cleaned up.<br><br>
 *
 *  NOTE:
 *  sim is the *Top* of the simulation and *not* the root. As such this
 *  function works on instances of kind SIM_INST.
 */

#endif  /* ASC_SIMLIST_H */
