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
 *
 *  This module initializes manages a global list simulations,
 *  which may be interrelated in very twisty ways due to UNIVERSAL and
 *  parameter passing.
 */
#ifndef __SIMLIST_H_SEEN__
#define __SIMLIST_H_SEEN__

extern struct gl_list_t *g_simulation_list;
/*
 * pointer to a simulation list. simulations need much better
 * management than they currently get, once we start building
 * simulations out of other simulations. For now this
 * file is largely empty.
 */

extern void Asc_DeAllocSim(struct Instance *);
/*
 * destroys the instance given. should be a simulation instance.
 */

extern void Asc_DestroySimulations(void);
/*
 * destroys all known instances on the simulation list.
 */
#endif /* __SIMLIST_H_SEEN__ */
