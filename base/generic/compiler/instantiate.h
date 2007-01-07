/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

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
*//** @file
	Ascend Interpreter(Instantiator).
	
	Requires:
	       #include "utilities/ascConfig.h"
	       #include "compiler/instance_enum.h"
	       #include "compiler/fractions.h"
	       #include "compiler/compiler.h"
	       #include "compiler/dimen.h"
	       #include "compiler/types.h"
	       #include "compiler/stattypes.h"
	</pre>
*//*
	by Tom Epperly
	Created: 1/24/90
	Last in CVS: $Revision: 1.18 $ $Date: 1998/03/25 00:32:03 $ $Author: ballan $
*//** @mainpage

	@section intro_sec Introduction

	This the the code documentation for 'libascend', which contains the 'base'
	or 'core' components of the ASCEND modelling environment.

	The two main parts are the compiler and the solver. There are also a number
	of general and utilities files, as well as some 'internal' packages.

	For further information about ASCEND, see the Developers' Wiki at
	https://pse.cheme.cmu.edu/wiki/view/Ascend/WebHome

	@section license License

	ASCEND is licensed under the GNU General Public License. See the the file
	'LICENSE.txt' for more information, which is available at:
	https://pse.cheme.cmu.edu/svn-view/ascend/code/trunk/LICENSE.txt?view=markup
*/

#ifndef ASC_INSTANTIATE_H
#define ASC_INSTANTIATE_H

/**	@addtogroup compiler Compiler
	@{
*/

/*
 *  Relation instantiation flags.
 *  If you add/remove/change these, update the comments in
 *  SetInstantiationRelnFlags().
 */
#define NORELS 	 0x0
#define GBOXRELS 0x1
#define BBOXRELS 0x2
#define TOKRELS  0x4
#define ALLRELS (GBOXRELS | BBOXRELS | TOKRELS)
#define EXTRELS (GBOXRELS | BBOXRELS)

ASC_DLLSPEC long int g_compiler_counter;
/**<  Unique id of calls to instantiator. */

extern void SetInstantiationRelnFlags(unsigned int flag);
/**<
 *  Sets the relation instantiation state of the instantiator.
 *  The flags control when and how how relations are instantiated.
 *  of relations. This allows more control over the sequence in
 *  which instantiation is done, and for so-called 'phased-compilation'.
 *
 *  Valid flags are the following:
 *    - NORELS 	 No relations should be instantiated
 *    - GBOXRELS Glassbox relations
 *    - BBOXRELS Blackbox relations
 *    - TOKRELS  Token relations
 *    - ALLRELS  Glassbox, Blackbox, and Token relations
 *    - EXTRELS Glassbox and Blackbox relations
 */

extern unsigned int GetInstantiationRelnFlags(void);
/**<
 *  Retrieves the relation instantiation state of the instantiator.
 *  See SetInstantiationRelnFlags() for more information.
 */

/*
 *  The following define allows us to manage the development phase of
 *  the ASCEND IV instantiator so that the interface is independent
 *  of the absolute latest compiler hacks.
 *  A production interface should be written to use ONLY Instantiate
 *  and ReInstantiate macros.
 *  Hacker interfaces can define/modify Alt* functions.
 *  It is generally a dumb idea to do mix different types of
 *  Instantiate and  ReInstantiate calls on the same instance.
 */
#define Instantiate(a,b,c,d) NewInstantiate((a),(b),(c),(d))

/*
 *  The following define allows us to manage the development phase of
 *  the ASCEND IV instantiator so that the interface is independent
 *  of the absolute latest compiler hacks.
 *  A production interface should be written to use ONLY Instantiate
 *  and ReInstantiate macros.
 *  Hacker interfaces can define/modify Alt* functions.
 *  It is generally a dumb idea to do mix different types of
 *  Instantiate and  ReInstantiate calls on the same instance.
 */
#define ReInstantiate(a) NewReInstantiate(a)

extern struct Instance *NewInstantiate(symchar *type, symchar *name,
                                       int intset, symchar *defmethod);
/**<
 *  This routine will return an instance of type SIM_INST. It will make
 *  an instance of the given type this will be set as the *root* of the
 *  simulation. To access the root of the instance tree use the functions
 *  GetSimulationRoot in instquery.h
 *  intset is only used when you are making an instance of a set type.
 *  defmethod is a METHOD name we are to attempt calling
 *  on the created instance at the END of compilation. If it does not
 *  exist, we will return silently after instantiation.
 *  <pre>
 *  5 phase Algorithm (approximately. here we ignore failure modes):
 *  Phase 1
 *  level
 *  1	get type
 *  1	create sim inst
 *  1	call real_instantiate_1
 *  2	call instantiate_model_1
 *  3	steal and return universal instance of type, or
 *  3	create model instance head
 *  3	process pending instances of model
 *  4	recursive instantiate, marking rels. logrels and whens done
 *      in arrays/bitlists without actually doing them.
 *  4	if no pendings,
 *  4	else error message about parts missing,
 *     and constants undefined.
 *  ...return all the way up eventually.
 *  Go back and mark all relation statements undone, put insts pending
 *  Phase 2
 *  1	call real_instantiate_2
 *  2	call instantiate_model_2
 *  3	process pending instances of model
 *  4	recursive instantiate, doing rels only  and
 *      marking logrels and whens as done.
 *      Explain failed relations.
 *  4	All statements are now determinate,
 *           All variables must be findable or they can't be.
 *  ...return all the way up eventually.
 *  Go back and mark all logrelation statements undone, put insts pending
 *  Phase 3
 *  1	call real_instantiate_3
 *  2	call instantiate_model_3
 *  3	process pending instances of model
 *  4	recursive instantiate, doing logrels only and
 *      marking whens as done. In phase 3 there are
 *      more than 1 iteration for the instantiation of
 *      logrels, since logrels can reference logrels
 *      (logrels inside conditional statements).
 *	Explain failed logrelations.
 *  4	All logical variables and referenced rels and
 *      logrels must be findable or they can't be.
 *   ...return all the way up eventually.
 *  Go back and mark all when statements undone, put insts pending
 *  Phase 4
 *  1	call real_instantiate_4
 *  2	call instantiate_model_4
 *  3	process pending instances of model
 *  4	recursive instantiate, doing whens only.
 *	Explain failed whens.
 *  4	All conditional variables, rels or logrels
 *      referenced by the whens  must be findable or
 *      they can't be
 *  ...return all the way up eventually.
 *  Phase 5
 *      Execution of default statements. (we would like to delete phase 5).
 *  Phase 6
 * 	Execute defmethod.
 *  </pre>
 */

ASC_DLLSPEC void NewReInstantiate(struct Instance *i);
/**<
 *  This routine is used to resume execution of an instance with unexecuted
 *  statements.  It will reattempt to execute the unexecuted statement.
 *  If it is able to complete the instance, it will execute the DEFAULT
 *  assignments(assignments to reals and booleans).
 */

ASC_DLLSPEC void UpdateInstance(struct Instance *root,
                           struct Instance *target,
                           CONST struct StatementList *slist);
/**<
 *  Update instance takes a pointer to the root of a simulation (ie the
 *  instance tree), and will find instance target. It will then apply
 *  the statementlist to the given to the target instance.
 *  This is the start of some experimental encapsulation/parameterization
 *  schemes.
 */

extern struct Instance *InstantiatePatch(symchar *patch,
                                         symchar *name, int intset);
/**<
 *  Instantiate patch takes the name of a patch that is supposed to be
 *  applied to a type. It partially instantiates the instance, then
 *  applies the patch. It returns the instance created. It uses
 *  UpdateInstance to do the real work. The applicability of the patch is
 *  hence determined by what that function supports.
 */

extern void ConfigureInstFromArgs(struct Instance *inst,
                                  CONST struct Instance *arginst);
/**<
 *  inst and arginst must be of the same MODEL type.
 *  inst should have NO executed statements -- it should be fresh
 *  from CreateModelInstance.
 *  Neither inst nor arginst should have any parents yet, if ever.
 *  This function copies or references, depending on how declared
 *  in the parameter list of the type for both models, the non-NULL
 *  children of arginst into inst.
 */

extern void ReConfigureInstFromArgs(struct Instance *inst,
                                             CONST struct Instance *arginst);
/**<
 *  inst and arginst must be or are about to be of the same MODEL type.
 *  inst should have been created from a less refined version of the type
 *  and is in the process of being refined up to type of arginst.
 *  arginst should not have any parents.
 *  This function copies or references, depending on how declared
 *  in the parameter list of the type for both models, the non-NULL
 *  children of arginst into inst for the slots in inst that are not
 *  already occupied.
 */

extern void LinkToParentByPos(struct Instance *parent,
                              struct Instance *child,
                              unsigned long childnum);
/**<
 *  Add child as childnumth child of parent and add parent to child.
 */

extern int IncompleteArray(CONST struct Instance *i);
/**<
 *  Given an array instance i, returns 1 if incomplete, 0 if ok.
 *  This means all NONNULL children are done, with the possible
 *  exception of arrays of relations/logical_relations.
 */

/* @} */

#endif /* ASC_INSTANTIATE_H */

