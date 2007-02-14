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
	The following functions give an alternative to the recursive default_self
	procedures found in standard ASCEND models.  EXPERIMENTAL.
*//*
	by John Pye, 15 Feb 2007.
*/

#include <utilities/ascConfig.h>
#include <utilities/ascPrint.h>

#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/symtab.h>
#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <compiler/visitinst.h>
#include <compiler/parentchild.h>
#include <compiler/library.h>
#include <packages/ascFreeAllVars.h>

/**
	Recursive default 'default' routine.

	If the current instance is a MODEL and it has a 'default' method, we
	run it and return.

	Otherwise, loop through child atoms in the current model. For the children,
		* for atoms, set to default values, if defaults are known.
		* for compound types, recurse.
*/
static
void Asc_RecursiveDefault(struct Instance *inst){
	int i, n;

	struct Instance *c;
	struct TypeDescription *type;

	type = InstanceTypeDesc(inst);

	if(model_type == GetBaseType(type)){
		if(0/*has a 'default' method...*/){
			/* run the 'default' method */
			return;
		}
	}

	/* default any child atoms' values */
	n = NumberChildren(inst);
	for(i = 1; i <= n; ++i){
		c = InstanceChild(inst);
		type = InstanceTypeDesc(c);
		if(BaseTypeIsAtomic(type)){
			if(!AtomDefaulted(type))continue;
			switch(GetBaseType(type)){
				case real_type: SetRealAtomValue(c, GetRealDefault(type)); break;
				case integer_type: SetIntAtomValue(c, GetIntDefault(type)); break;
				case boolean_type: SetBooleanAtomValue(c, GetBoolDefault(type)); break;
				case symbol_type: SetSymbolAtomValue(c, GetSymDefault(type)); break;
				case set_type: /* what is the mechanism for defaulting of sets? */
				default: ASC_PANIC("invalid type");
			}
		}else if(BaseTypeIsCompound(type)){
			/* descend into MODELs, arrays and 'patches' (whatever they are) */
			Asc_RecursiveDefault(c);
		}
	}
}

int Asc_DefaultAllVars(struct Instance *root, struct gl_list_t *arglist, void *userdata){
	/* arglist is a list of gllist of instances */
	if (arglist == NULL ||
	    gl_length(arglist) == 0L ||
	    gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
	    gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL
	){
		/* run on the 'root' instance */
		return Asc_RecursiveDefault(root);
	} else {
		/* take the first item from the first arglist */
		return Asc_RecursiveDefault(
			(struct Instance *)gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1)
		);
	}
}
