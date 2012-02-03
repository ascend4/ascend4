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



#include "link.h"
#include <stdarg.h>
#include <ascend/general/list.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

#include "cmpfunc.h"
#include "find.h"
#include "forvars.h"
#include "instance_types.h"
#include "instance_enum.h"
#include "instmacro.h"
#include "instquery.h"
#include "statement.h"
#include "stattypes.h"
#include "stdio.h"
#include "symtab.h"
#include "visitinst.h"
#include "vlist.h"
#include "name.h"
#include "instance_io.h"



/**< DS: beginning of LINK functions *******/
/* implemented functions related to the LINK statements, probably they shouldn't be here*/ 

/**
	Find instances: Make sure at least one thing is found for each name item
	on list (else returned list will be NULL) and return the collected instances.
	DS: it returns a non-flattened list of the instances
*/


static
struct gl_list_t *FindInstsNonFlat(struct Instance *inst,
                            CONST struct VariableList *list,
                            enum find_errors *err)
{
  struct gl_list_t *result,*temp;

  result = gl_create(AVG_LINKS_INST);
  while(list!=NULL){
    temp = FindInstances(inst,NamePointer(list),err);
    if (temp==NULL){
      gl_destroy(result);
      return NULL;
    }
    gl_append_ptr(result,temp);
    list = NextVariableNode(list);
  }
  return result;
}

/**
	Find instances: Make sure at least one thing is found for each name item
	on list (else returned list will be NULL) and return the collected instances.
	DS: it returns a flattened list of the instances
*/

static
struct gl_list_t *FindInsts(struct Instance *inst,
                            CONST struct VariableList *list,
                            enum find_errors *err)
{
  struct gl_list_t *result,*temp;
  unsigned c,len;
  result = gl_create(AVG_LINKS_INST);
  while(list!=NULL){
    temp = FindInstances(inst,NamePointer(list),err);
    if (temp==NULL){
      gl_destroy(result);
      return NULL;
    }
    len = gl_length(temp);
    for(c=1;c<=len;c++) {
      gl_append_ptr(result,gl_fetch(temp,c));
    }
    gl_destroy(temp);
    list = NextVariableNode(list);
  }
  return result;
}


/**
 * DS: Helper function that compares a key and a list of instances with a link_entry	
			 If the tuple (key, instances) is the same as the link entry returns 0,
			 otherwise return 1.
 */
static int CmpLinkEntry(symchar *key, struct VariableList *vlist,struct link_entry_t *linkEntry)
{	
	/* DS: Note: the key_cache and the u.vl were used instead of LINKStatKey(linkEntry->u.statptr) and LINKStatVlist(linkEntry->u.statptr), because for some reason the pointer to the statement becomes NULL after the statement is executed */

	struct VariableList *linkEntry_vlist;

	if(CmpSymchar(key,linkEntry->key_cache) != 0) {	/* if the keys are different, the LINKs are different */
		return 1;
	}
	
	linkEntry_vlist = linkEntry->u.vl;
	if(VariableListLength(vlist) != VariableListLength(linkEntry_vlist)) { 	/* if the number of variables linked are different, the LINKs are different */
		return 1;
	}

	if(CompareVariableLists(vlist,linkEntry_vlist) != 0) {	/* if the variables linked are different, the LINKs are different */
		return 1;																							/* DS Note: it is ordered biased */
	}

  return 0; /* (key, instances) is the same as linkEntry */
}


void CollectLinkTypes(struct Instance *model, struct gl_list_t *result)
{
	struct TypeDescription *modelType;
	struct link_entry_t *link_entry;
  symchar *key, *key_result;
	int c1, c2, len_table, len_result, existent;
		
	modelType = InstanceTypeDesc(model);
	if(modelType->t == model_type) {
		// probe instance given (if appropriate kind) to get link info needed	from the TypeDescription link_table
		len_table = gl_length(modelType->u.modarg.link_table);

		for(c1=1;c1<=len_table;c1++){
			link_entry = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,c1);
			if(link_entry->key_cache == NULL){			/**< in case the cache is empty we need to find keys from the statements */
				key = LINKStatKey(link_entry->u.statptr);
			}else {
				key = link_entry->key_cache;
			}
			// verify that any new info obtained is kept uniquely in the result list.(not efficient at all DS TODO)
			len_result = gl_length(result);
			existent = 0;
			for(c2=1;c2<=len_result;c2++){
				key_result = (symchar *)gl_fetch(result,c2);
				if(CmpSymchar(key,key_result) == 0){
					existent = 1;	
				}
			}
			if(!existent){
					gl_append_ptr(result,(VOIDPTR)key);
			}
		}

		// probe instance given (if appropriate kind) to get link info needed	from the ModelInstances link_table
		len_table = gl_length(MOD_INST(model)->link_table);
		for(c1=1;c1<=len_table;c1++){
			link_entry = (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,c1);
			if(link_entry->key_cache == NULL){			/**< in case the cache is empty we need to find keys from the statements */
				key = LINKStatKey(link_entry->u.statptr);
			}else {
				key = link_entry->key_cache;
			}
			// verify that any new info obtained is kept uniquely in the result list.(not efficient at all DS TODO)
			len_result = gl_length(result);
			existent = 0;
			for(c2=1;c2<=len_result;c2++){
				key_result = (symchar *)gl_fetch(result,c2);
				if(CmpSymchar(key,key_result) == 0){
					existent = 1;	
				}
			}
			if(!existent){
					gl_append_ptr(result,(VOIDPTR)key);
			}
		}
	}	
	/*DS:If the instance or the child instance is not a model we do nothing since a "link_table" is only present in the typedescription of a model (modarg) */
}


/** find all the keys in link table(s), optionally recursive. */
extern struct gl_list_t *getLinkTypes (struct Instance *model, int recursive)
{	
	struct gl_list_t *result = gl_create(AVG_LINKS); 
	
	if (recursive) {
  	VisitInstanceTreeTwo(model, (VisitTwoProc)CollectLinkTypes, 0,0, result);
  }  else {
    CollectLinkTypes(model, result);
  }
	
  return result;
}


void CollectLinks(struct Instance *model, struct gl_list_t *result)
{
	struct TypeDescription *modelType;
	struct link_entry_t *link_entry, *link_entry_result;
	int c1, c2, len_table, len_result, existent;

	modelType = InstanceTypeDesc(model);
	if(modelType->t == model_type) {
		// probe instance given (if appropriate kind) to get link info needed	from the TypeDescription link_table
		len_table = gl_length(modelType->u.modarg.link_table);
		for(c1=1;c1<=len_table;c1++){
			link_entry = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,c1);
			// verify if the LINK is unique in the result list
			len_result = gl_length(result);
			existent = 0;

			for(c2=1;c2<=len_result;c2++){
				link_entry_result = (struct link_entry_t *)gl_fetch(result,c2);
				if(CmpLinkEntry(link_entry->key_cache,link_entry->u.vl,link_entry_result) == 0){
					existent = 1;	
				}
			}
			if(!existent){
					gl_append_ptr(result,(VOIDPTR)link_entry);
			}
		}
		
		// probe instance given (if appropriate kind) to get link info needed	from the ModelInstance link_table

		len_table = gl_length(MOD_INST(model)->link_table);
		for(c1=1;c1<=len_table;c1++){
			link_entry = (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,c1);
			existent = 0;
			// verify if the LINK is unique in the result list
			len_result = gl_length(result);
			
			for(c2=1;c2<=len_result;c2++){
				link_entry_result = (struct link_entry_t *)gl_fetch(result,c2);
				if(CmpLinkEntry(link_entry->key_cache,link_entry->u.vl,link_entry_result) == 0){
					existent = 1;	
				}
			}
			if(!existent){ 
					gl_append_ptr(result,(VOIDPTR)link_entry);
			}
		}
	}	
	/*DS:If the instance or the child instance is not a model we do nothing since a "link_table" is only present in the typedescription of a model (modarg) */
}


extern struct gl_list_t *getLinks(struct Instance *model, symchar *target_key, int recursive)
{
	struct gl_list_t *result = gl_create(AVG_LINKS); /**< DS: hardcoded for now but will eventually be a constant */ 
	int c1, len_result;
	struct link_entry_t *link_entry;
	if (recursive) {
  	VisitInstanceTreeTwo(model, (VisitTwoProc)CollectLinks, 0,0, result);
  }  else {
    CollectLinks(model, result);
  }
	
	len_result = gl_length(result);
	
	for(c1=1;c1<=len_result;c1++) {
		link_entry = (struct link_entry_t *)gl_fetch(result,c1);
		if(CmpSymchar(link_entry->key_cache,target_key) !=0 ){
			gl_delete(result,c1,0);	/* if the link entry does not have sought key we delete it from the result list */
			len_result--;
		}
	}

  return result;
}


/**<DS: get all links matching key, optionally recursive. */
extern struct gl_list_t *getLinksReferencing (struct Instance *model, symchar* key, struct Instance *targetInstance, int recursive)
{
	struct gl_list_t *link_instances,*result = gl_create(AVG_LINKS);
	struct link_entry_t *link_entry;
	struct Instance *inst;
  enum find_errors err;
	int c1, c2, len_result, len_inst, containsInst;


	if (recursive) {
  	VisitInstanceTreeTwo(model, (VisitTwoProc)CollectLinks, 0,0, result);
  }  else {
    CollectLinks(model, result);
  }

	/* DS: get all the links that contain the target instance */
	len_result = gl_length(result);
  for(c1=1;c1<=len_result;c1++){
  	link_entry = (struct link_entry_t *)gl_fetch(result,c1);
		if(link_entry->instances_cache == NULL ) {
			link_instances = FindInsts(model,link_entry->u.vl,&err);
		}	else {
			link_instances = link_entry->instances_cache;
		}
	
		len_inst = gl_length(link_instances);
		containsInst = 0;
		if(CmpSymchar(link_entry->key_cache,key) == 0){
			for(c2=1;c2<=len_inst;c2++){
				inst = (struct Instance *)gl_fetch(link_instances,c2);
				if(inst == targetInstance){
					containsInst = 1;
				}
			}	
		}
		if(!containsInst){
			gl_delete(result,c1,0);	/* if the link entry does not have sought key we delete it from the result list */
			len_result--;
		}	
	}


	
	return result;
}

/*DS: called by the ASCEND LINK command, both in the declarative and the non-declarative section */
extern void addLinkEntry(struct Instance *model, symchar *key, struct gl_list_t *instances, struct Statement *stat, unsigned int declarative)
{
	struct TypeDescription *modelType;
	struct link_entry_t *link_entry, *old_link_entry;
  struct for_var_t *ptr;
	int c,len,exist = 0;

	if(strcmp("TestingRoutine",SCP(key)) == 0) {
	TestingRoutine(model);
	return;
	}

	/* in case the LINK key is in fact the index of a for loop, the value of the index at the current iteration is turned into a symchar and stored as a key*/
	if(GetEvaluationForTable() && (ptr = FindForVar((struct gl_list_t *)GetEvaluationForTable(),key)) != NULL) { 
		char index_key[10];
		sprintf(index_key,"%ld",GetForInteger(ptr));
		key = AddSymbol(index_key);
	}
	
	if(declarative == 0) {
		/* we first check if the LINK we are about to add isn't already present in the declartive LINK table */
		len = gl_length(MOD_INST(model)->link_table);
		for(c=1;c<=len;c++){
	  	old_link_entry = (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,c);
			if(CmpLinkEntry(key,LINKStatVlist(stat),old_link_entry) == 0){
				exist = 1;
			}
		}
		
		if(!exist){

			link_entry = (struct link_entry_t *)ascmalloc(sizeof(struct link_entry_t));
			link_entry->key_cache = key;
		  link_entry->u.statptr = stat; 	
			link_entry->link_type = stat->v.lnk.key_type;
			link_entry->u.vl = LINKStatVlist(stat);	
			link_entry->instances_cache = instances;
			link_entry->flags = 1;
		  link_entry->length = gl_length(instances); 

			/**< DS: in case the link entry is non-declartive, it is appended to the linktable in the model instance */
			gl_append_ptr(MOD_INST(model)->link_table,(VOIDPTR)link_entry);	
			printf("\n non-declarative LINK no of instances in cache: %ld \n", gl_length(link_entry->instances_cache));
			printf("\n non-declarative LINK key %s \n", SCP(key));
		}	
		else{
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"The LINK entry to-be added is already present in the non-declarative LINK table.");
		}
	}
	else {
		if(link_entry->link_type == link_ignore) {
			ignoreDeclLinkEntry(model,key,LINKStatVlist(stat));
		}
		else {
			modelType = InstanceTypeDesc(model);
			len = gl_length(modelType->u.modarg.link_table);

			for(c=1;c<=len;c++){
		  	old_link_entry = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,c);
				if(CmpLinkEntry(key,LINKStatVlist(stat),old_link_entry) == 0){
					exist = 1;
				}
			}

			if(!exist){
				link_entry = (struct link_entry_t *)ascmalloc(sizeof(struct link_entry_t));
				link_entry->key_cache = key;
			  link_entry->u.statptr = stat; 	
				link_entry->link_type = stat->v.lnk.key_type;
				link_entry->u.vl = LINKStatVlist(stat);	
				link_entry->instances_cache = instances;
				link_entry->flags = 1;
			  link_entry->length = gl_length(instances); 
	
				/**< DS: in case the link entry is declarative, it is appeneded to the linktable in the model type description */
				gl_append_ptr(modelType->u.modarg.link_table,(VOIDPTR)link_entry); 
			
				/* DS: testing purposes: */
				printf("\n declarative LINK no of instances in cache: %ld \n", gl_length(link_entry->instances_cache));
				printf("\n declarative LINK key %s \n", SCP(key));		
			}
			else{
			  ERROR_REPORTER_HERE(ASC_USER_WARNING,"The LINK entry to-be added is already present in the declarative LINK table.");
			}
		}
	}
}

extern void ignoreDeclLinkEntry(struct Instance *model, symchar *key, struct VariableList *vlist)
{
	/*DS: used in case the 'ignore' key is used for a declarative link */
	struct TypeDescription *modelType;
	struct link_entry_t *link_entry;
	modelType = InstanceTypeDesc(model);
	int c, len, exist = 0;

	len = gl_length(modelType->u.modarg.link_table);
	printf("\n declarative link_table size %d\n",len);
 	for(c=1;c<=len;c++){
 		link_entry = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,c);
	 	if (CmpLinkEntry(key,vlist,link_entry) == 0) {
			exist = 1;
			printf("\n ignored LinkEntry from declarative link_table \n");
			gl_delete(modelType->u.modarg.link_table,c,1);
			len--;
		}
	}
	len = gl_length(modelType->u.modarg.link_table);
	printf("\n new declarative link_table size %d\n",len); 

	if(!exist){
	   ERROR_REPORTER_HERE(ASC_USER_ERROR,"The LINK entry to-be ignored does not exist.");
	}
}
/* DS: (current Implementation) check if the the non-declarative or declarative link table contains any of the instances in the list under the given key, if so remove them from the entries */
extern void removeLinkEntry(struct Instance *model, symchar *key, struct VariableList *vlist)
{
	/* used when called by the UNLINK command in the METHODS section by ASCEND */
	struct link_entry_t *link_entry;
  struct for_var_t *ptr;
	int c, len, exist = 0;


	printf("\n execute removeLinkEntry \n");

	/* in case the LINK key is in fact the index of a for loop, the value of the index at the current iteration is turned into a symchar and stored as a key*/
	if(GetEvaluationForTable() && (ptr = FindForVar((struct gl_list_t *)GetEvaluationForTable(),key)) != NULL) { 
		char index_key[10];
		sprintf(index_key,"%ld",GetForInteger(ptr));
		key = AddSymbol(index_key);
	}

	len = gl_length(MOD_INST(model)->link_table);
	printf("\n non-declarative link_table size %d\n",len);
  for(c=1;c<=len;c++){
  	link_entry = (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,c);
	 	if (CmpLinkEntry(key,vlist,link_entry) == 0) {
			exist = 1;
			printf("\n removed LinkEntry from non-declarative link_table\n");
			gl_delete(MOD_INST(model)->link_table,c,1);  /* we also deallocate the memory allocated for the link_entry */
			len--;
		}
	}

	len = gl_length(MOD_INST(model)->link_table);
	printf("\n new non-declarative link_table size %d\n",len);


	if(!exist){
	   ERROR_REPORTER_HERE(ASC_USER_ERROR,"The LINK entry to-be removed does not exist.");
	}
} 


/* DS: (current Implementation) check if the the non-declarative link table contains any of the instances in the list under the given key, if so remove them from the entries */
extern void removeNonDeclarativeLinkEntry(struct Instance *model, symchar *key, int recursive)
{
	struct gl_list_t *result=gl_create(AVG_LINKS);
	struct link_entry_t *link_entry;
	int c=1, len;


	if (recursive) {
  	VisitInstanceTreeTwo(model, (VisitTwoProc)CollectLinks, 0,0, result);
  }  else {
    CollectLinks(model, result);
  }
	
	printf("\n execute removeNonDeclarativeLinkEntry \n");
	len =  gl_length(MOD_INST(model)->link_table);
	printf("\n non-declarative link_table size: %d\n",len);
 	while(len != 0 || c<=len) {
 		link_entry = (struct link_entry_t *)gl_fetch( MOD_INST(model)->link_table,c);

	 	if ((key == NULL || CmpSymchar(key,link_entry->key_cache) == 0) && isDeclarative(model,link_entry) == 0 ) { /*DS: if the key is NULL then remove all entries from the link_table */
			printf("\n execute removed LinkEntry \n");
			gl_delete(MOD_INST(model)->link_table,c,1);
			len--;
		}
		else{
			c++;
		}
	}
	
	len = gl_length(MOD_INST(model)->link_table);
	printf("\n non-declarative link_table size: %d\n",len);
} 


CONST struct gl_list_t *getLinkInstances(struct Instance *inst, struct link_entry_t *link_entry,int status)
{
	struct gl_list_t *result = gl_create(AVG_LINKS_INST); 
  enum find_errors err;
	
	result = FindInstsNonFlat(inst,link_entry->u.vl,&err);

	if (result==NULL) {
    switch(err){
    case impossible_instance:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"LINK entry contains imposible instance name");
    default:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"incomplete instances in LINK entry");
    }
  }
  return result;
}


CONST struct gl_list_t *getLinkInstancesFlat(struct Instance *inst, struct link_entry_t *link_entry,int status)
{
	struct gl_list_t *result = gl_create(AVG_LINKS_INST); 
  enum find_errors err;
	if(link_entry->instances_cache == NULL) {
		result = FindInsts(inst,link_entry->u.vl,&err);
		if (result==NULL) {
	    switch(err){
	    case impossible_instance:
				ERROR_REPORTER_HERE(ASC_USER_ERROR,"LINK entry contains impossible instance name");
	    default:
				ERROR_REPORTER_HERE(ASC_USER_ERROR,"incomplete instances in LINK entry");
	      /* statement is not ready to be executed */
	    }
	  }
	  return result;
	}	else {
		result = link_entry->instances_cache;
	}
  return result;	
}


/**<DS: get the list (link_table) with all the declarative link entries located in the TypeDescription */
extern struct gl_list_t *getLinkTableDeclarative(struct Instance *model)
{
	struct TypeDescription *modelType;
	modelType = InstanceTypeDesc(model);

	if(modelType->t == model_type) {
		return modelType->u.modarg.link_table;
	}
	else {
		return NULL;
		ERROR_REPORTER_HERE(ASC_USER_WARNING,"the getLinkTableDeclarative procedure is called by a non-model instance");
	}
}


/**<DS: get the list (link_table) with all the non-declarative link entries located in the ModelInstance */
extern struct gl_list_t *getLinkTableProcedural(struct Instance *model)
{
	struct TypeDescription *modelType;
	modelType = InstanceTypeDesc(model);
	if(modelType->t == model_type) {
		return MOD_INST(model)->link_table;
	}
	else {
		return NULL;
		ERROR_REPORTER_HERE(ASC_USER_WARNING,"the getLinkTableProcedural procedure is called by a non-model instance");
	}
}


extern int isDeclarative(struct Instance* model, struct link_entry_t *target_link_entry)
{
	struct TypeDescription *modelType;
	struct link_entry_t *link_entry;
	int c, len;

	/* DS: get all the links that contain the target instance from the declarative part (i.e. stored in the model TypeDescription) */
	modelType = InstanceTypeDesc(model);
	len = gl_length(modelType->u.modarg.link_table);
	
  for(c=1;c<=len;c++){
  	link_entry = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,c);
  	if (CmpLinkEntry(target_link_entry->key_cache,target_link_entry->u.vl,link_entry) == 0) {
				return 1;
 	  }
	}
	return 0;
}


extern void clearLinkCache(struct Instance* model)
{
	struct gl_list_t* link_table;
	struct link_entry_t *link_entry;
	int c,len;

	link_table = getLinkTableDeclarative(model);
	len = gl_length(link_table);
	for(c=1;c<=len;c++) {
		link_entry = gl_fetch(link_table,c);
		link_entry->instances_cache = NULL;
	}

	link_table = getLinkTableProcedural(model);
	len = gl_length(link_table);
	for(c=1;c<=len;c++) {
		link_entry = gl_fetch(link_table,c);
		link_entry->instances_cache = NULL;
	}
}

extern void populateLinkCache(struct Instance* model)
{

	struct gl_list_t *link_table;
	struct link_entry_t *link_entry;
  enum find_errors err;
	int c,len;

	link_table = getLinkTableDeclarative(model);
	len = gl_length(link_table);
	for(c=1;c<=len;c++) {
		link_entry = gl_fetch(link_table,c);
		link_entry->instances_cache = FindInsts(model,link_entry->u.vl,&err);
 	  if (link_entry->instances_cache==NULL) {
	    switch(err){
	    case impossible_instance:
 				ERROR_REPORTER_HERE(ASC_USER_ERROR,"LINK statement contains an impossible instance name (populateLinkCache)");
	    default:
 				ERROR_REPORTER_HERE(ASC_USER_ERROR,"Incomplete instances in LINK (populateLinkCache)");
	    }
	  }
	}

	link_table = getLinkTableProcedural(model);
	len = gl_length(link_table);
	for(c=1;c<=len;c++) {
		link_entry = gl_fetch(link_table,c);
		link_entry->instances_cache = FindInsts(model,link_entry->u.vl,&err);
 	  if (link_entry->instances_cache==NULL) {
	    switch(err){
	    case impossible_instance:
 				ERROR_REPORTER_HERE(ASC_USER_ERROR,"LINK statement contains an impossible instance name (populateLinkCache)");
	    default:
 				ERROR_REPORTER_HERE(ASC_USER_ERROR,"Incomplete instances in LINK (populateLinkCache)");
	    }
	  }
	}
}


extern int getOdeType(struct Instance *model ,struct Instance *inst){

	struct link_entry_t *link_entry;
	struct gl_list_t *der_links,*independent_links;
	symchar *der_key,*independent_key;
	CONST struct VariableList *var;
	int i,k,maxorder;

	der_key = AddSymbol("ode");
	der_links = getLinks(model,der_key,0);

	for(i=1;i<=gl_length(der_links);i++) {
		link_entry = (struct link_entry_t*) gl_fetch(der_links,i);
		var = link_entry->u.vl;

		maxorder = link_entry->length;
		k = 0;
		while(var!=NULL){
			char *s = WriteInstanceNameString(inst,model);
			int c = strcmp(SCP(SimpleNameIdPtr(NamePointer(var))),s);
			ASC_FREE(s);
			if(c == 0 ) {
				gl_destroy(der_links);
				return maxorder - k;
			}
			k++;
			var = NextVariableNode(var);
		}
	}

	gl_destroy(der_links);
	independent_key = AddSymbol("independent");
	independent_links = getLinks(model,independent_key,0);

	for(i=1;i<=gl_length(independent_links);i++) {
		link_entry = (struct link_entry_t*) gl_fetch(independent_links,i);
		var = link_entry->u.vl;

		k = 0;
		while(var!=NULL){
			char *s = WriteInstanceNameString(inst,model);
			int c = strcmp(SCP(SimpleNameIdPtr(NamePointer(var))),s);
			ASC_FREE(s);
			if(c == 0 ) {
				gl_destroy(independent_links);
				return -1;
			}
			k++;
			var = NextVariableNode(var);
		}
	}

	gl_destroy(independent_links);
	return 0;
}

extern int getOdeId(struct Instance *model,struct Instance *inst){
	struct link_entry_t *link_entry;
	struct gl_list_t *der_links;
	symchar *der_key;
	CONST struct VariableList *var;
	int i;

	der_key = AddSymbol("ode");
	der_links = getLinks(model,der_key,0);

	for(i=1;i<=gl_length(der_links);i++) {
		CONSOLE_DEBUG("Inside for");
		link_entry = (struct link_entry_t*) gl_fetch(der_links,i);
		var = link_entry->u.vl;

		while(var!=NULL){
			char *s = WriteInstanceNameString(inst,model);
			int c = strcmp(SCP(SimpleNameIdPtr(NamePointer(var))),s);
			ASC_FREE(s);
			if(c == 0){
				gl_destroy(der_links);
				return i;
			}
			var = NextVariableNode(var);
		}
	}
	gl_destroy(der_links);
	return 0;
}


/**< DS: End of LINK functions *******/

/* function that tests the LINK functions implemented - prints the output in the console */
void TestingRoutine(struct Instance *model)
{
	struct TypeDescription *modelType;
	modelType = InstanceTypeDesc(model);
	int c1, len1, len2;
	
	/* test getLinkTypes */
	struct gl_list_t *linkTypes;
	symchar *keyc1;
	linkTypes = getLinkTypes(model,0);
	len1 = gl_length(linkTypes);
	CONSOLE_DEBUG("\n number of unique link types: %d \n",len1);
	CONSOLE_DEBUG("\n The unique link keys are: ");
  for(c1=1;c1<=len1;c1++){
  	keyc1= (symchar *)gl_fetch(linkTypes,c1);
		CONSOLE_DEBUG("%s ",SCP(keyc1));
	}				
	CONSOLE_DEBUG("\n");

	/* test getLinks */
	struct gl_list_t *links;
	struct link_entry_t *lnk;
	struct Instance *i1;
	links = getLinks(model,keyc1,0);
	len2 = gl_length(links);
	CONSOLE_DEBUG("\n number of links with key %s is: %d \n",SCP(keyc1),len2);
	


	/* just a test for comparing two instances pointer-wise */
	/*	
	struct Instance *i11,*i22;
	struct link_entry_t *lnk1,lnk2;
	
	lnk1 = (struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,1);
	lnk2 = (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,1);
	i11 = (struct Instance *)gl_fetch(lnk1->instances_cache,2);
	i22 = (struct Instance *)gl_fetch(lnk2->instances_cache,1);
	if(i11 == i22){
		printf("\n are equal \n");
	} */

	/* test getLinksReferencing */	
	lnk	= (struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,1); /* take the first link from all the non-declarative and declarative LINK Tables, just for testing */
	populateLinkCache(model);
	i1= (struct Instance *)gl_fetch(lnk->instances_cache,1);
	CONSOLE_DEBUG("\n number links referencing the first instance and key %s is %ld \n",SCP(keyc1),gl_length(getLinksReferencing(model,keyc1,i1,0)));

	/* test getLinkInstances */

	/* test getLinkInstancesFlat */

	/* test isDeclarative */
	modelType = InstanceTypeDesc(model);	
	CONSOLE_DEBUG("\n the link should be declarative %d\n",isDeclarative(model,(struct link_entry_t *)gl_fetch(modelType->u.modarg.link_table,1)));
	CONSOLE_DEBUG("\n the link should be non-declarative %d\n",isDeclarative(model,(struct link_entry_t *)gl_fetch(MOD_INST(model)->link_table,1)));

	/* test removeNonDeclarative LINKs */
	removeNonDeclarativeLinkEntry(model,NULL,0); /*since the key is NULL, all non declarative LINKS are removed */


	/* test getLinkTableDeclarative (already tested in previous functions) */
	/* test getLinkTableProcedural (already tested in previous functions) */
}

