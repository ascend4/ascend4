/*
 *  listio.c
 *  List I/O Routines
 *  by Ben Allan
 *  Created: 12/97
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: listio.c,v $
 *  Date last modified: $Date: 1998/06/16 15:47:41 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "platform.h"
#include "panic.h"
#include "list.h"
#include "listio.h"

void gl_write_list(FILE *fp, struct gl_list_t *l)
{
  unsigned long c,len;
  FILE *myfp;

  asc_assert(NULL != l);

  if (fp==NULL) {
    myfp = stderr;
  } else {
    myfp = fp;
  }
  len = gl_length(l);
  for (c= 1; c <= len ; c++) {
	/* FIXME the pointer to integer thing here is not working */
    fprintf(myfp,"%lu: 0x%p (" ASC_PTRFMT ")\n",c,gl_fetch(l,c),
             (asc_intptr_t)gl_fetch(l,c));
  }
}


int gl_write_list_item_str(FILE *fp, void *item){
	if(item == NULL){
		fprintf(fp,"(NULL)");
		return -1;
	}
	return fprintf(fp,"%s",(const char *)item);
}


ASC_DLLSPEC void gl_write_list_str(FILE *fp, struct gl_list_t *l, WriteItemFn *write_item){
	unsigned long c, len;
	len = gl_length(l);
	fputs("(",fp);
	for(c=1; c<= len; ++c){
		if(c>1)fputs(",",fp);
		(*write_item)(fp,(void *)gl_fetch(l,c));
	}
	fputs(")",fp);
}
/* @} */



