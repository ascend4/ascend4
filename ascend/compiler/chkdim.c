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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Dimensionality checks for relations.
*//*
	by John Pye
	Created: Dec 2023
*/

#include "chkdim.h"
#include <ascend/utilities/error.h>
#include <ascend/compiler/relation_type.h>
#include <ascend/compiler/instance_types.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/general/list.h>

#define ASC_CHKDIM_DEBUG
#ifdef ASC_CHKDIM_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...) 
#endif

ASC_DLLSPEC int chkdim_check_relation(CONST struct Instance *i){
	if(i->t != REL_INST){
		MSG("not a relation");
		return 1;
	}
	struct RelationInstance *ri = (struct RelationInstance *)i;
	if(ri->type == e_token){
		struct TokenRelation tr = RTOKEN(ri->ptr);
		struct gl_list_t *vl = ri->ptr->vars;
		fprintf(stderr,"Relation: ");
		WriteRelation(stderr,i,(ri->parent)[0]);
		fprintf(stderr,"\n");
		dim_type D;
		int res = RelationCheckDimensions((struct Instance*)i,&D);
		if(!res)return 1; /* RelationCheckDimensions returns 'TRUE' if consistent and not 'wild' */
	}else{
		MSG("non-token relation");
	}
	return 0;
}

