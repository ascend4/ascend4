/*
 *  Simulation Management for Ascend
 *  by Ben Allan
 *  Version: $Revision: 1.2 $                                  
 *  Version control file: $RCSfile: simlist.h,v $
 *  Date last modified: $Date: 1997/07/18 12:34:54 $
 *  Last modified by: $Author: mthomas $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  This module initializes manages a global list simulations.
 *  These may be interrelated in very twisty ways due to UNIVERSAL and
 *  parameter passing.
 */

#ifndef ASC_SIMLIST_H
#define ASC_SIMLIST_H

#include "utilities/ascConfig.h"
#include "instance_enum.h"
#include "compiler/compiler.h"

extern int g_compiler_timing;

extern struct gl_list_t *g_simulation_list;
/**< 
 * Pointer to a simulation list. Simulations need much better
 * management than they currently get, once we start building
 * simulations out of other simulations. For now this
 * file is largely empty.
 */

extern void Asc_DeAllocSim(struct Instance *inst);
/**< 
 * Destroys the instance given. Should be a simulation instance.
 */

/*
* This function setups up to call instantiate with different
* compiler settings. In all cases Instantiate will make a copy of
* the name that is given. format should perhaps be an array of enums
* or a bit structure to deal with multiple compilation flags. At the moment
* it is just an int.
*/
extern
struct Instance *
SimsCreateInstance(symchar *type,
	symchar *name, int format,
	symchar *defmethod);

extern void Asc_DestroySimulations(void);
/**< 
 * Destroys all known instances on the simulation list.
 */

#endif  /* __SIMLIST_H_SEEN__ */

/*
 *  SimsProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: SimsProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
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
 *  Simulation procedures.
 *  <pre>
 *  To include this header, you must include the following:
 *  </pre>
 */

#ifndef SimsProc_module_loaded
#define SimsProc_module_loaded

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
/**<
 *  Sets the current working simulation to the simulation given.
 */

extern struct Instance *Asc_GetCurrentSim(void);
/**<
 *  Returns the current working simulation. Makes no checks on the state
 *  of the simulation.
 */

extern int Asc_SimsUniqueName(symchar *str);
/**<
 *  Searches the simulation list for the name of a simulation.
 *  Returns 0 if the name was found else returns 1;
 */

extern int Asc_SimsCmpSim(struct Instance *sim1, struct Instance *sim2);
/**<
 *  Compares two simulations, based on their name. Returns 0 if the same.
 *  Returns non-zero if different.
 */

extern void Asc_DeAllocSim(struct Instance *sim);
/**<
 *  <!--  void Asc_DeAllocSim(sim);                                    -->
 *  Deallocate a simulation instance, destroying the instnace and its
 *  associated root instance, by  calling DestroyInstance (in instance.h).
 *  It also frees the name string, which it owns, and cleans up any external
 *  vars associated with the simulation.<br><br>
 *
 *  NOTE:
 *  sim is the *Top* of the simulation and *not* the root. As such it
 *  works on instances of kind SIM_INST.
 */

#endif

