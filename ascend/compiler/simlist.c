/*	ASCEND modelling environment
	Copyright (C) 1997 Benjamin Andrew Allan
	Copyright 1997, Carnegie Mellon University

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
	This module initializes manages a global list simulations,
	which may be interrelated in very twisty ways due to UNIVERSAL and
	parameter passing.
*//*
	by Ben Allan
	by Kirk Abbott and Ben Allan
	Last in CVS: $Revision: 1.3 $ $Date: 1997/07/18 12:34:53 $ $Author: mthomas $
*/

#include <ascend/utilities/ascConfig.h>

#include "instance_enum.h"
#include "destroyinst.h"
#include "simlist.h"
#include "instquery.h"
#include "expr_types.h"
#include "symtab.h"
#include "extinst.h"
#include "cmpfunc.h"
#include "instantiate.h"

#include <ascend/general/list.h>
#include <ascend/general/tm_time.h>
 
struct gl_list_t *g_simulation_list = NULL;

void sim_destroy(struct Instance *sim){
  if (sim) {
	CONSOLE_DEBUG("Destroying instance %s", SCP(GetSimulationName(sim)) );
    DestroyInstance(sim,NULL);
  }
}


void Asc_DestroySimulations(void){
  if (g_simulation_list) {
    gl_iterate(g_simulation_list,(void (*)(VOIDPTR))sim_destroy);
    gl_destroy(g_simulation_list);      /* sim_destroy takes care of the
                                         * memory -- see SimsProc.c */
    g_simulation_list = NULL;
  }else{
  	CONSOLE_DEBUG("g_simulation_list is null");
  }
}

int g_compiler_timing=0;

#define MAXIMUM_INST_NAME 256

/*
 * unheadered stuff from instantiate.c
 * changed g_cursim to extern -- JP
 */
extern struct Instance *g_cursim;
struct gl_list_t *ArrayIndices(struct Name *name, struct Instance *parent);

void Asc_SetCurrentSim(struct Instance *sim)
{
  g_cursim = sim;
  return;                                         
}

struct Instance *Asc_GetCurrentSim()
{
  return g_cursim;
}

int Asc_SimsUniqueName(symchar *str)
{
  unsigned long c;
  struct Instance *ptr;
  for(c=gl_length(g_simulation_list);c>=1;c--) {
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationName(ptr) == str) {
      return 0;
    }
  }
  return 1;
}

/*
 *
 */
int Asc_SimsCmpSim(struct Instance *sim1, struct Instance *sim2)
{
  assert(sim1&&sim2);
  return CmpSymchar(GetSimulationName(sim1),GetSimulationName(sim2));
}

/*
 * Find the simulation list entry to a named simulation. Return
 * null if no simulation of that name found. Sims are inserted sorted
 * so we should do a gl_search here.
 */
struct Instance *Asc_FindSimulationTop(symchar *str)
{
  unsigned long len,c;
  struct Instance *ptr;

  len = gl_length(g_simulation_list);
  for (c=len;c>=1;c--) {
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationName(ptr) == str) {
      return ptr;
    }
  }
  return NULL;
}


/*
 * This function setups up to call instantiate with different
 * compiler settings. In all cases Instantiate will make a copy of
 * the name that is given. format should perhaps be an array of enums
 * or a bit structure to deal with multiple compilation flags. At the moment
 * it is just an int.
 */
struct Instance *SimsCreateInstance(symchar *type,
                                    symchar *name, 
                                    enum CreateInst_format format,
                                    symchar *defmethod)
{
  struct Instance *result;
  unsigned int oldflags;
  double comptime;

  if ((NULL == type) ||(NULL == name)) {
    return NULL;
  }
  g_ExtVariablesTable = NULL;		/* defined in extinst.[ch] */
  comptime = tm_cpu_time();
  switch (format) {
  case e_normal:
    result = Instantiate(type,name,0,defmethod);
    break;
  case e_no_relations:
    oldflags = GetInstantiationRelnFlags();
    SetInstantiationRelnFlags(NORELS);
    result = Instantiate(type,name,0,defmethod);
    SetInstantiationRelnFlags(oldflags);
    break;
  case e_patch:
    result = InstantiatePatch(type,name,0);
    break;
  default:
    FPRINTF(stderr,"Warning: doing standard compilation\n");
    result = Instantiate(type,name,0,defmethod);
    break;
  }
  comptime = tm_cpu_time() - comptime;
  if (g_compiler_timing) {
    FPRINTF(stderr,"Instantiation CPU time = %g seconds\n",comptime);
  }
  return result;
}


/*
 * This function searches the simulation list for
 * the *root* instance of the simulation.
 */
struct Instance *Asc_FindSimulationRoot(symchar *str)
{
  unsigned long c,len;
  struct Instance *ptr;

  len = gl_length(g_simulation_list);
  for (c = len; c >= 1; c--) {
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationName(ptr) == str) {
      return GetSimulationRoot(ptr);
    }
  }
  return NULL;
}

symchar *Asc_SimsFindSimulationName(CONST struct Instance *root)
{
  unsigned long len,c;
  struct Instance *ptr;

  if (!root) {
    return NULL;
  }
  len = gl_length(g_simulation_list);
  for(c=len;c>=1;c--) {
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationRoot(ptr)==root) {
      return GetSimulationName(ptr);
    }
  }
  return NULL;
}

