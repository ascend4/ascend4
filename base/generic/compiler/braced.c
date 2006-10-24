/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
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
*//*
	By Benjamin Allan, March 20, 1998.
	Last in CVS: $Revision: 1.4 $ $Date: 1998/06/16 16:38:38 $ $Author: mthomas $
*/

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include "braced.h"

/*
	 We expect things in this file to get much more complicated
	to deal with notes.
*/

struct bracechar {
  symchar *lang;
  char *string;
  int slen;
  int refcount;
};

#define BCMALLOC ASC_NEW(struct bracechar)
#define BCFREE(bc) ascfree(bc)

struct bracechar *AddBraceChar(CONST char *s, symchar *l){
  struct bracechar *bc;
  assert(s != NULL);
  bc = BCMALLOC;
  assert(bc!= NULL);
  bc->lang = l;
  bc->string = ascstrdup((char *)s); /* fixme */
  bc->slen = strlen((char *)s);
  bc->refcount = 0;
  return bc;
}

struct bracechar *CopyBraceChar(struct bracechar *bc){
  if (bc!=NULL) {
    bc->refcount++;
  }
  return bc;
}

void DestroyBraceChar(struct bracechar *bc){
  if (bc==NULL) {
    return;
  }
  if (bc->refcount == 0) {
    ascfree(bc->string); /* fixme */
    bc->slen = -1;
    bc->lang = NULL;
    bc->string = NULL;
    BCFREE(bc);
  } else {
    bc->refcount--;
  }
}

CONST char *BraceCharString(struct bracechar *bc){
  assert(bc!=NULL);
  return bc->string;
}

symchar *BraceCharLang(struct bracechar *bc){
  assert(bc!=NULL);
  return bc->lang;
}

int BraceCharLen(struct bracechar *bc){
  assert(bc!=NULL);
  return bc->slen;
}
