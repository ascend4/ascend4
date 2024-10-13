/*	ASCEND modelling environment
	Copyright (C) 2017 John Pye
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 1994, 1995 Kirk Andre' Abbott
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file Error-reporting functions for the compiler
*/

#include "relerr.h"
#include "statio.h"
#include "nameio.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

rel_errorlist *rel_errorlist_new(){
	rel_errorlist *err = ASC_NEW_CLEAR(rel_errorlist);
	err->errcode = okay;
	err->lrcode = lokay;
	err->ferr = correct_instance;
	return err;
}


void rel_errorlist_destroy(rel_errorlist *err){
	rel_errorlist_destroy_contents(err);
	ASC_FREE(err);
}

void rel_errorlist_destroy_contents(rel_errorlist *err){
	(void)0;
}


int rel_errorlist_set_find_error(rel_errorlist *err, enum find_errors ferr){
	assert(err!=NULL);
	err->ferr = ferr;
	err->data.name = NULL;
	return 0;
}

int rel_errorlist_set_find_error_name(rel_errorlist *err, enum find_errors ferr, const struct Name *errname){
	assert(err!=NULL);
	err->ferr = ferr;
	err->data.name = errname;
	return 0;
}

int rel_errorlist_set_name(rel_errorlist *err, const struct Name *errname){
	assert(err!=NULL);
	assert(err->ferr != correct_instance);
	err->data.name = errname;
	return 0;
}

enum find_errors rel_errorlist_get_find_error(rel_errorlist *err){
	assert(err!=NULL);
	return err->ferr;
}


int rel_errorlist_set_find_errpos(rel_errorlist *err,unsigned long ferrpos){
	assert(err!=NULL);
	err->ferrpos = ferrpos;
	return 0;
}


int rel_errorlist_set_lrcode(rel_errorlist *err, enum logrelation_errorsx lrcode){
	assert(err!=NULL);
	err->lrcode = lrcode;
	return 0;
}	


int rel_errorlist_get_lrcode(rel_errorlist *err){
	assert(err!=NULL);
	return err->lrcode;
}


int rel_errorlist_set_code(rel_errorlist *err, enum relation_errorsx errcode){
	assert(err!=NULL);
	err->errcode = errcode;
	return 0;
}


int rel_errorlist_get_code(rel_errorlist *err){
	assert(err!=NULL);
	return err->errcode;
}

int rel_errorlist_report_error(rel_errorlist *err,struct Statement *stat){
	char *namestr;
	//CONSOLE_DEBUG("Reporting error");

	switch(rel_errorlist_get_code(err)){

	case incorrect_structure:
		WSSM(ASCERR,stat, "Bad relation expression",3);
		return 1;

	case incorrect_inst_type:
		WSSM(ASCERR,stat, "Incorrect instance types in relation",3);
		return 1;

	case incorrect_boolean_inst_type:
		WSSM(ASCERR,stat, "Incorrect boolean instance in relation",3);
		return 1;

	case incorrect_integer_inst_type:
		WSSM(ASCERR,stat, "Incorrect integer instance in relation",3);
		return 1;

	case incorrect_symbol_inst_type:
		WSSM(ASCERR,stat, "Incorrect symbol instance in relation",3);
		return 1;

	case incorrect_real_inst_type:
		WSSM(ASCERR,stat,
			"Incorrect real child of atom instance in relation",3);
		return 1;

	case find_error:
		switch(rel_errorlist_get_find_error(err)){
		case unmade_instance:
		case undefined_instance:
			if(NULL != err->data.name){
				namestr = WriteNameString(err->data.name);
				WriteStatementError(ASC_USER_ERROR,stat,0,"Unmade or undefined instance '%s' in relation.",namestr);
				ASC_FREE(namestr);
			}else{
				WriteStatementError(ASC_USER_ERROR,stat,0,"%s instance in relation (name not reported).",(rel_errorlist_get_find_error(err)==unmade_instance?"Unmade":"Undefined"));
			}
			return 1;
		case impossible_instance:
			WriteStatementError(ASC_USER_ERROR,stat,1,"Relation contains an impossible instance");
			return 1;
		case correct_instance:
			ASC_PANIC("Incorrect error response.\n");/*NOTREACHED*/
		default:
			ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/
		}
	case integer_value_undefined:
	case real_value_wild:
	case real_value_undefined:
		WriteStatementError(3,stat,1,"Unassigned constants or wild dimensioned real constant in relation");
		return 1;
	case okay:
		ASC_PANIC("Incorrect 'okay' error response.\n");/*NOTREACHED*/
	}
	ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/
	return -1;
}


