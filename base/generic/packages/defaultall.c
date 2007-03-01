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
#include <compiler/watchpt.h>
#include <compiler/initialize.h>

/* #define DEFAULT_DEBUG */

/**
	Find atom children in the present model and set them to their ATOM DEFAULT
	values.
*/
static
int Asc_DefaultSelf1(struct Instance *inst){
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
				case real_type: SetRealAtomValue(c, GetRealDefault(type), 0); break;
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
			Asc_DefaultSelf1(c);
		}
	}
	return 0;
}

struct DefaultAll_data{
	symchar *default_all;
};

/**
	Find child models in the present model and run their 'default_all' methods
*/
static
int Asc_DefaultAll1(struct Instance *inst, struct DefaultAll_data *data){
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
			method = FindMethod(type,data->default_all);
			if(method){
#ifdef DEFAULT_DEBUG
				CONSOLE_DEBUG("Running default_all on '%s'",SCP(GetName(type)));
#endif
				pe = Initialize(c , CreateIdName(ProcName(method)), "__not_named__"
					,ASCERR
					,0, NULL, NULL
				);
				if(pe!=Proc_all_ok)err += 1;
			}else{
#ifdef DEFAULT_DEBUG
				CONSOLE_DEBUG("Recursing into array...");
#endif
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"No 'default_all' found for type '%s'",SCP(GetName(type)));
				return 1;
			}
		}else if(array_type == GetBaseType(type)){
			if(Asc_DefaultAll1(c,data))err += 1;
		}
	}

	return err;
}

int Asc_DefaultSelf(struct Instance *root, struct gl_list_t *arglist, void *userdata){
  /* arglist is a list of gllist of instances */
  if (arglist == NULL ||
      gl_length(arglist) == 0L ||
      gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
      gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL) {
    return Asc_DefaultSelf1(root);
  }else{
    return Asc_DefaultSelf1((struct Instance *)gl_fetch( (struct gl_list_t *)gl_fetch(arglist,1),1 ));
  }
}

int Asc_DefaultAll(struct Instance *root, struct gl_list_t *arglist, void *userdata){
	struct DefaultAll_data data;
	data.default_all = AddSymbol("default_all");
	
	/* arglist is a list of gllist of instances */
	if (arglist == NULL ||
		    gl_length(arglist) == 0L ||
		    gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
		    gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL) {
		return Asc_DefaultAll1(root,&data);
	}else{
		return Asc_DefaultAll1((struct Instance *)gl_fetch( (struct gl_list_t *)gl_fetch(arglist,1),1 )
			, &data
		);
	}
}
