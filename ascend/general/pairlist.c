/*
 *  Ascend pointer pairs module.
 *  by Benjamin Andrew Allan
 *  Created: 10/2006
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 2006 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include "platform.h"
#include "panic.h"
#include "ascMalloc.h"
#include "pairlist.h"
#include "list.h"

struct pairlist_t {
	struct gl_list_t *keys;
	struct gl_list_t *vals;
};


struct pairlist_t *
pairlist_create(unsigned long capacity)
{
	struct pairlist_t * pl = (struct pairlist_t *)ascmalloc(sizeof(struct pairlist_t));
	pl->keys = gl_create(capacity);
	pl->vals = gl_create(capacity);
	return pl;
}

void *
pairlist_keyAt(struct pairlist_t * pl, unsigned long eindex)
{
	assert(pl != NULL);
	return gl_fetch(pl->keys, eindex);
}

void *
pairlist_valueAt(struct pairlist_t * pl, unsigned long eindex)
{
	assert(pl != NULL);
	return gl_fetch(pl->vals, eindex);
}

unsigned long
pairlist_contains(struct pairlist_t * pl, void *key)
{
	unsigned long eindex;
	assert(pl != NULL);
	eindex = gl_ptr_search(pl->keys,key,FALSE);
	return eindex;
}

unsigned long
pairlist_append(struct pairlist_t * pl, void *key, void * value)
{
	assert(pl != NULL);
	gl_append_ptr(pl->keys, key);
	gl_append_ptr(pl->vals, value);
	return gl_length(pl->keys);
}

unsigned long pairlist_append_unique(struct pairlist_t * pl, void *key, void * value)
{
	unsigned long eindex;
	assert(pl != NULL);
	eindex = pairlist_contains(pl, key);
	if(eindex==0){
		pairlist_append(pl,key,value);
	}
	return gl_length(pl->keys);
}

void *pairlist_set(struct pairlist_t *pl, void *key, void *value){
	unsigned long eindex;
	void *oldvalue = NULL;
	assert(pl != NULL);
	eindex = pairlist_contains(pl, key);
	if(eindex){
		oldvalue = gl_fetch(pl->vals, eindex);
		gl_store(pl->vals, eindex, value);
	}else{
		pairlist_append(pl,key,value);
	}
	return oldvalue;
}

void pairlist_clear(struct pairlist_t * pl)
{
	gl_reset(pl->keys);
	gl_reset(pl->vals);
}

void pairlist_destroy(struct pairlist_t * pl)
{
	gl_destroy(pl->keys);
	gl_destroy(pl->vals);
	pl->keys = pl->vals = NULL;
	ascfree(pl);
}

struct gl_list_t *pairlist_values_and_destroy(struct pairlist_t *pl){
	struct gl_list_t *vals = pl->vals;
	gl_destroy(pl->keys);
	pl->keys = pl->vals = NULL;
	ascfree(pl);
	return vals;
}


long pairlist_length(struct pairlist_t * pl)
{
	assert(pl != NULL);
	return gl_length(pl->keys);
}
void pairlist_print(FILE *fp, struct pairlist_t * pl)
{
	(void) fp; (void) pl;
	/* FIXME. pairlist_print impl */
}

