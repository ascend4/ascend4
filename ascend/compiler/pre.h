/*	ASCEND modelling environment
	Copyright (C) 2013 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Routines for managing pointers between pre variable and their
        arguments instances.
*//*
	by Ksenija Bestuzheva (GSOC 2013)
	Created: 01/08/2013
*/

#include "instance_enum.h"
#include "name.h"

extern
void SetPreInfo(struct Instance *pre, struct Instance *prearg);
/**<
  *  Creates pointers between the instances. All the instances should be of
  *  type REAL_ATOM_INST.
  */

ASC_DLLSPEC
struct Instance *FindPreByArg(struct Instance *arg);
/**<
 *  Finds a pre variable given the argument. All the instances
 *  should be of type REAL_ATOM_INST.
 */

ASC_DLLSPEC
void WritePreInfo(FILE *f, struct Instance *inst);
/**<
 *  Output the pre variable (for usual variables) or
 *  argument (for pre variables) instance connected with inst.
 */

ASC_DLLSPEC
int IsPre(struct Instance *inst);
/**<
 *  If the instance in a pre variable, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
int IsPrearg(struct Instance *inst);
/**<
 *  If the instance in an argument of a variable, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
struct Instance *PreArg(struct Instance *inst);
/**<
 *  Return the argument for the given pre variable. If inst is not
 *  a pre variable, return NULL.
 */

ASC_DLLSPEC
struct Instance *Pre(struct Instance *inst);
/**<
 *  Return a pre variable for the given instance. If inst does not
 *  have a pre variable, return NULL.
 */

