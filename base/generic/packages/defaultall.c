/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
	The following functions give an automatic, default form for the
	'default_all' and 'default_self' methods usually written as explicit
	METHODs in a model. EXPERIMENTAL.
*//*
	by John Pye, 15 Feb 2007.
*/

#include <packages/defaultall.h>

#include <compiler/proc.h>
#include <compiler/name.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPrint.h>
#include <utilities/ascPanic.h>

#include <compiler/instquery.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/symtab.h>
#include <compiler/atomvalue.h>
#include <compiler/visitinst.h>
#include <compiler/parentchild.h>
#include <compiler/library.h>
#include <compiler/initialize.h>

/* #define DEFAULT_DEBUG */

/*------------------------------------------------------------------------------
  visit child atoms of the current model (don't visit sub models) and set
  to ATOM default values
*/

static int defaultself_visit_childatoms1(struct Instance *inst);

int defaultself_visit_childatoms(struct Instance *root, struct gl_list_t *arglist, void *userdata){
	/* arglist is a list of gllist of instances */
	if (arglist == NULL ||
	    gl_length(arglist) == 0L ||
	    gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
	    gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL) {
	return defaultself_visit_childatoms1(root);
	}else{
		return defaultself_visit_childatoms1(
			(struct Instance *)gl_fetch( (struct gl_list_t *)gl_fetch(arglist,1),1 )
		);
	}
}


/**
	Find atom children in the present model and set them to their ATOM DEFAULT
	values.
*/
static
int defaultself_visit_childatoms1(struct Instance *inst){
	int i,n;
	struct Instance *c;
	struct TypeDescription *type;	

	/* default any child atoms' values */
	n = NumberChildren(inst);
	for(i = 1; i <= n; ++i){
#ifdef DEFAULT_DEBUG
		CONSOLE_DEBUG("Child %d...", i);
#endif
		c = InstanceChild(inst,i);
		if(c==NULL)continue;

		type = InstanceTypeDesc(c);
		if(BaseTypeIsAtomic(type)){
			if(!AtomDefaulted(type))continue;
			switch(GetBaseType(type)){
				case real_type:
#ifdef DEFAULT_DEBUG
					CONSOLE_DEBUG("Setting to atom default = %f",GetRealDefault(type));
#endif
					SetRealAtomValue(c, GetRealDefault(type), 0); 
					break;
				case integer_type: SetIntegerAtomValue(c, GetIntDefault(type), 0); break;
				case boolean_type: SetBooleanAtomValue(c, GetBoolDefault(type), 0); break;
				case symbol_type: SetSymbolAtomValue(c, GetSymDefault(type)); break;
				case set_type: /* what is the mechanism for defaulting of sets? */
				default: ASC_PANIC("invalid type");
			}
#ifdef DEFAULT_DEBUG
			CONSOLE_DEBUG("Reset atom to default value");
#endif
		}else if(GetBaseType(type)==array_type){
			/* descend into arrays */
			defaultself_visit_childatoms1(c);
		}
	}
#ifdef DEFAULT_DEBUG
	CONSOLE_DEBUG("defaultself_visit_childatoms1 returning %d",0);
#endif
	return 0;
}

/*------------------------------------------------------------------------------
  visit submodels, running 'default_self' on each
*/

struct DefaultAll_data{
	symchar *method_name;
};

static int defaultself_visit_submodels1(struct Instance *inst
		, struct DefaultAll_data *data
);

int defaultself_visit_submodels(struct Instance *root
		, struct gl_list_t *arglist, void *userdata
){
	struct DefaultAll_data data;
	data.method_name = AddSymbol("default_self");
	
	/* arglist is a list of gllist of instances */
	if (arglist == NULL ||
		    gl_length(arglist) == 0L ||
		    gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
		    gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL) {
		return defaultself_visit_submodels1(root,&data);
	}else{
		return defaultself_visit_submodels1(
			(struct Instance *)gl_fetch( (struct gl_list_t *)gl_fetch(arglist,1),1 )
			, &data
		);
	}
}

/**
	Find child models in the present model and run their 'default_self' methods
*/
static
int defaultself_visit_submodels1(struct Instance *inst
		, struct DefaultAll_data *data
){
	int i, n, err = 0;
	struct Instance *c;
	struct TypeDescription *type;
	struct InitProcedure *method;
	enum Proc_enum pe;

	type = InstanceTypeDesc(inst);

	/* loop through child atoms */
	n = NumberChildren(inst);
	for(i = 1; i <= n; ++i){
		c = InstanceChild(inst,i);
		if(c==NULL)continue;

		type = InstanceTypeDesc(c);
		if(model_type == GetBaseType(type)){
			/* run 'default_all' for all child models */
			method = FindMethod(type,data->method_name);
			if(method){
#ifdef DEFAULT_DEBUG
				CONSOLE_DEBUG("Running METHOD %s on '%s'",SCP(data->method_name),SCP(GetName(type)));
#endif
				pe = Initialize(c , CreateIdName(ProcName(method))
					, SCP(data->method_name)
					,ASCERR
					,0, NULL, NULL
				);
				if(pe!=Proc_all_ok)err += 1;
			}else{
#ifdef DEFAULT_DEBUG
				CONSOLE_DEBUG("Recursing into array...");
#endif
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"No '%s' found for type '%s'",SCP(data->method_name),SCP(GetName(type)));
				return 1;
			}
		}else if(array_type == GetBaseType(type)){
			if(defaultself_visit_submodels1(c,data))err += 1;
		}
	}

#ifdef DEFAULT_DEBUG
	CONSOLE_DEBUG("defaultself_visit_submodels1 return ing %d",err);
#endif
	return err;
}

#if 0
/**
	NOTE YET IMPLEMENTED: we need to be able to pass string constants to
	methods, which I don't think is possible yet.
*/
int Asc_VisitSubmodels(struct Instance *root
		, struct gl_list_t *arglist, void *userdata
){
	struct DefaultAll_data data;
	(void)userdata;

	ERROR_REPORTER_HERE(ASC_USER_ERROR,"not implemented");		
	return 1;
	if (arglist == NULL
		    || gl_length(arglist) == 0L
		    || gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 2
		    || gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL
			|| gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),2) == NULL
	){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"EXTERNAL visit_submodels(SELF,'methodname') called with bad argument list");
		return 1;
	}

	data.method_name = AddSymbol("default_all");
	
	return Asc_DefaultAll1(
		(struct Instance *)gl_fetch( (struct gl_list_t *)gl_fetch(arglist,1),1 )
		, &data
	);
}
#endif
