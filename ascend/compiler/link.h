/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	Link Routines.

*//*
	by Dante Stroe
	Created: 07/07/2009
*/



/*------------------------------------------------------------------------------
	DS: LINK TABLE METHODS (I think maybe they should be part of instaniate.h? or a new file)
*/

#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include "slist.h"
#include "statement.h"
#include "stattypes.h"
#include "instance_enum.h" 
#include <stdio.h>



/** Entry in the Link table (which is in fact a list) associated with each model TypeDescription and/or model Instance
 */
struct link_entry_t {

  union {
	 struct Statement * statptr; /* pointer to the declarative statement */
	 struct VariableList * vl; 
  } u;
  struct gl_list_t *instances_cache;    /**< (cache) pointer to the list of instances that are linked*/
	enum LinkKind link_type;
  symchar* key_cache;					/**< key that defines the linked instances */
  unsigned int length;   	/**< Number of instances in the linked_instances list. */
  unsigned long	capacity; /**< Capacity of the link entry, in terms of the number of the instances that can be linked. */
  unsigned int flags;     /**< Status flags.used to differentiate between LINK statements in the declarative part(FIXED) and the ones in the methods (not fied). */

};



extern struct gl_list_t *getLinkTypes (struct Instance *model, int recursive);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param recursive - the child instances of the scope Instance are also scanned for LINK types

	@return a gl_list with symchars that represent all the distinct LINK keys in the link_table(s)
*/


extern struct gl_list_t *getLinks (struct Instance *model, symchar *key, int recursive);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param key - pointer to the symchar of the key we are looking for in all the LINKs
	@param recursive - the child instances of the scope Instance are also scanned for the given LINK key

	@return a gl_list with link entries (link_entry_t *) that hold all the LINKs that have the the given LINK key
*/


extern struct gl_list_t *getLinksReferencing (struct Instance *model, symchar* key, struct Instance *targetInstance, int recursive);
/**<
	@param model - the scope Instance of the LINKs (should be a model)
	@param key - pointer to the symchar of the key we are looking for in all the LINKs
	@param targetInstance - pointer to the instance we looking for inside the variable lists of the LINK tables
	@param recursive - the child instances of the scope Instance are also scanned for the given LINK key and target instance

	@return a gl_list with link entries (link_entry_t *) that hold all the LINKs that have the the given LINK key and the target instance
*/


extern void addLinkEntry(struct Instance *model, symchar *key, struct gl_list_t *instances,struct Statement *stat, unsigned int declarative);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param key - pointer to the symchar of the key we are adding to the LINK table
	@param instances - pointer to the list of instances that are "LINK-ed" by "key"
	@param stat - pointer to the Statement object that was executed for this LINK
	@param declarative - 1 if the statement was in the declarative part of the model, 0 if in the Methods (decides whether the LINK entry is added to the 											 TypeDescription LINK table or to the ModelInstance one	

	@return void (probably should return 1 in case of success and 0 otherwise)

	Called by the LINK statements, both in the declarative and non-declarative part;
*/


extern void removeLinkEntry(struct Instance *model, symchar *key, struct VariableList *vlist);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param key - pointer to the symchar of the key we are removing from the LINK table
	@param vlist - variable list that hold the names of the instances that should be unlinked 

	@return void (probably should return 1 in case of success and 0 otherwise)

	Called by the UNLINK statement in the non-declarative part (METHODS). Current implementation requires that the UNLINK paramters in the statements should 		match the parameters of a previous LINK statement in the non-declarative part.
*/

extern void ignoreDeclLinkEntry(struct Instance *model, symchar *key, struct VariableList *vlist);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param key - pointer to the symchar of the key we are removing from the LINK table
	@param vlist - variable list that hold the names of the instances that should be unlinked 

	@return void (probably should return 1 in case of success and 0 otherwise)

	Called by the executeLNK function, when the key in a declarative LINK statement is 'ignore', and is used to ignore the link entry
*/


extern void removeNondeclarativeLinkEntry(struct Instance *model, symchar *key, int recursive);
/**<
	@param model - the scope Instance of the LINKs (should be a model)
	@param key - pointer to the symchar of the key we are removing from the LINK table
	@param recursive - the child instances of the scope Instance are also scanned for the given LINK key (if key is null all are removed)
	@return void (probably should return 1 in case of success and 0 otherwise)
*/


extern CONST struct gl_list_t *getLinkInstances(struct Instance *model, struct link_entry_t *link_entry, int status);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param link_entry - pointer to a link_entry in some link_table
	@param status - Find status of the instances in the LINK (TODO not yet impl)

	@return gl_list with all the instances that are LINK-ed by the given LINK entry
*/


extern CONST struct gl_list_t *getLinkInstancesFlat(struct Instance *model, struct link_entry_t *link_entry, int status);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param link_entry - pointer to a link_entry in some link_table
	@param status - Find status of the instances in the LINK (TODO not yet impl)

	@return gl_list with all the instances that are LINK-ed by the given LINK entry
*/


extern struct gl_list_t *getLinkTableDeclarative(struct Instance *model);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	
	@return gl_list with all the LINK entries that are declarative for the given model
*/


extern struct gl_list_t *getLinkTableProcedural(struct Instance *model);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	
	@return gl_list with all the LINK entries that are non-declarative for the given model
*/

extern int isDeclarative(struct Instance* model, struct link_entry_t *link_entry);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param link_entry - pointer to a link_entry in some link_table
	
	@return 1 if the LINK entry is declarative (part of the TypeDescription LINK table), 0 if the LINK entry is non-declarative (part of the ModelInstance LINK table) 
*/

extern void clearLinkCache(struct Instance* model);
/**<
	@param model - the scope Instance of the LINK (should be a model)

	empty instance search optimization. 
*/

extern void populateLinkCache(struct Instance* model);
/**<
	@param model - the scope Instance of the LINK (should be a model)

	fill instance search optimization.	
*/

extern int getOdeId(struct Instance *model,struct Instance *inst);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param inst - the instance of the variable we are interested to find its differntial state

	@return The Id of the derivative chain the variable belongs to, if any

	Note: If the variable is algebraic or an independent variable the return value is 0
				The Ids are integers that are intilized with their position in the list of 'ode' LINKs for each derivative chain
*/

extern int getOdeType(struct Instance *model,struct Instance *inst);
/**<
	@param model - the scope Instance of the LINK (should be a model)
	@param inst - the instance of the variable we are interested to find its differntial state

	@return- The order of the variable in the derivative chain to which it belongs, if any:
							-1 - the variable instance is an independent variable
							 0 - the variable instance is not part of any derivative chain
							 1 - the variable instance is a state variable
							 2 - the variable instance is a first order derivative
							 3 - the variable instance is a second order derivative
							 etc.
			
	Note: The values are taken from the position of the variables in the LINK instance list:
				LINK('ode',dx_dt,x,t),where the first instance are the differential and derivative variables in decreasing order and the last instance is always the independent variable.
*/

/*DS: Function used to test the above implmented functions. Note: It will be removed eventually */
void TestingRoutine(struct Instance* model);



