#ifdef __CHILD_ILLEGAL_ACCESS__
/*
 *
 *  Child List Internal Implementation details
 *  by Benjamin Allan
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: childpriv.h,v $
 *  Date last modified: $Date: 1998/03/27 10:43:58 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
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
 *  Internal headers of child data structures. 
 *  Not for use except by child.c, childio.c.
 */
#ifndef _CHILDPRIV_H_SEEN_
#define _CHILDPRIV_H_SEEN_

#ifndef lint
static CONST char ChildPrivID[] = "$Id: childpriv.h,v 1.1 1998/03/27 10:43:58 ballan Exp $";
#endif

/* origin of sanity (or sanity of origin) check. dependent on header */
#define LegalOrigin(n) ( ((n)>0 && (n) <=8) ? (n) : 0 )

struct ChildHash {
  struct ChildHash *next;
  symchar *sym;	/* key */
  unsigned long clindex; /* position of key in child list */
};

/*
 * Hash function on heap pointers for 32 bit hardware. 
 * May need to revisit on high address architectures with
 * 64 bit pointers. Creates a number 0 - 255.
 */
#define CHILDHASHINDEX(p) (((((long) (p))*1103515245) >> 22) & 255)
/*
 * Must be 2^N with N even and must match CHILDHASHINDEX.
 * buckets in every child hash table regardless of # of children.
 */
#define CHILDHASHTABLESIZE 256 
/*
 * table is sized so that there is almost never a collision
 * in well structured models.
 */
struct ChildListStructure {
  struct gl_list_t *list;
  struct gl_list_t *symbols; /* possibly redundant */
  struct ChildHash *table[CHILDHASHTABLESIZE];
  struct ChildHash *data; /* probably supercedes symbols */
};

/* return nth element of a child list ptr l */
#define CGET(l,n) ((struct ChildListEntry *)gl_fetch((l)->list,(n)))
/* return nth name of a child list ptr l */
#define CNAME(l,n) ((l)->data[(n)-1].sym)
/* return nth element of a gl list ptr l of entries to read. */
#define GGET(l,n) ((struct ChildListEntry *)gl_fetch((l),(n)))

/* the following will always amount to an append of child list ptr with p */
/* remember to reconstruct the list of names and hash table when done. */
#define CPUT(l,p) gl_insert_sorted((l)->list,(p),(CmpFunc)CmpChildListEntries)

/* macro to fetch the gl list of entries from the ChildListPtr */
#define GL(a) (a)->list
/* macro to fetch the gl list of names from the ChildListPtr */
#define GN(a) (a)->symbols


#endif /* _CHILDPRIV_H_SEEN_ */
#endif/* __CHILD_ILLEGAL_ACCESS__ */
