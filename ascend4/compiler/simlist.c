/*
 *  Simulation Management for Ascend
 *  by Ben Allan
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: simlist.c,v $
 *  Date last modified: $Date: 1997/07/18 12:34:53 $
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

#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "general/list.h"
#include "compiler/instance_enum.h"
#include "compiler/destroyinst.h"
#include "compiler/simlist.h"
 
#ifndef lint
static CONST char simlistID[] = "$Id: simlist.c,v 1.3 1997/07/18 12:34:53 mthomas Exp $";
#endif


struct gl_list_t *g_simulation_list = NULL;

void Asc_DeAllocSim(struct Instance *sim)
{
  if (sim) {
    DestroyInstance(sim,NULL);
  }
}


void Asc_DestroySimulations(void)
{
  FPRINTF(stderr,"Destroying simulations\n");
  if (g_simulation_list) {
    gl_iterate(g_simulation_list,(void (*)(VOIDPTR))Asc_DeAllocSim);
    gl_destroy(g_simulation_list);      /* Asc_DeAllocSim takes care of the
                                         * memory -- see SimsProc.c */
    g_simulation_list = NULL;
  }
}

