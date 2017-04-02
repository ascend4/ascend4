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
#include <ascend/general/ascMalloc.h>

/* fill out the details of this struct now, only inside this code file */

struct rel_errorlist_struct{
	enum relation_errorsx errcode;
	enum logrelation_errorsx lrcode;
	enum find_errors ferr;
	unsigned long ferrpos;
/*	
	struct gl_list *errs;
	enum relation_errorsx lastcode;
	enum find_errors ferr;
	unsigned long ferrpos;
*/
};


rel_errorlist *rel_errorlist_new(){
	rel_errorlist *err = ASC_NEW_CLEAR(rel_errorlist);
	err->errcode = okay;
	err->lrcode = lokay;
	err->ferr = correct_instance;
	return err;
}


void rel_errorlist_destroy(rel_errorlist *err){
	ASC_FREE(err);
}


int rel_errorlist_set_find_error(rel_errorlist *err, enum find_errors ferr){
	assert(err!=NULL);
	err->ferr = ferr;
	return 0;
}


int rel_errorlist_get_find_error(rel_errorlist *err){
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

