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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
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
	if (pl==0) {
		pairlist_append(pl,key,value);
	}
	return gl_length(pl->keys);
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

