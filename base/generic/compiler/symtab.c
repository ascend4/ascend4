/*
 *  Symbol Table Management
 *  by Tom Epperly
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: symtab.c,v $
 *  Date last modified: $Date: 1998/03/17 12:36:52 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
#include "symtab.h"
#include <general/hashpjw.h>

#ifndef lint
static CONST char SymbolTableID[] = "$Id: symtab.c,v 1.10 1998/03/17 12:36:52 ballan Exp $";
#endif

/*
 *  For various reasons which are still unclear to me, it is best to chose
 *  a symbol table size which is a prime number.
 */
#define SYMTABSIZE 1021

struct StringSpaceRec {
  struct StringSpaceRec *next;
  unsigned long used;
  char data[MAXIMUM_STRING_LENGTH];
};

struct symbol_entry {
  symchar *entry; /* pointer to char*. entry - (sizeof(int))bytes is the
                   * address of the length of the string.
                   */
  struct symbol_entry *next;
};

/*********************************************************************\
Global Variables
\*********************************************************************/
static struct StringSpaceRec *g_string_space=NULL;
struct symbol_entry *g_symbol_table[SYMTABSIZE];
static unsigned long g_symbol_size = 0;
static unsigned long g_symbol_collisions = 0;
static struct symbol_entry *g_big_strings = NULL; /* linked list for oversize*/
static int g_big_strings_cnt = 0; /* linked list for oversize*/


/*********************************************************************\
  String Space Management. Support routines for the symbol table.
\*********************************************************************/

/*********************************************************************\
  Makes space for oversized strings.
  The extra item overhead is cheap enough.
\*********************************************************************/
static symchar *AddBigString(CONST char *str)
{
  struct symbol_entry *item;
  char *sptr;
  int *lenptr;
  int slen;
  item =  ASC_NEW(struct symbol_entry);
  item->next = g_big_strings;
  g_big_strings = item;
  slen = strlen(str);
  lenptr = (int *)ascmalloc(sizeof(int)+slen+1);
  lenptr[0] = slen;
  sptr = (char *)(&(lenptr[1]));
  strcpy(sptr,str);
  item->entry = (symchar *)sptr;
  g_big_strings_cnt++;
  return item->entry;
}
/* clears big strings, leaving the global NULL */
void DestroyBigStrings(void)
{
  struct symbol_entry *item;
  item = g_big_strings;
  while (item != NULL) {
    g_big_strings = item->next;
    ascfree((void *)(((char *)item->entry)-(char *)sizeof(int)));
    ascfree(item);
    item = g_big_strings;
  }
  g_big_strings_cnt = 0;
}

/*********************************************************************\
const char *CopyString(str)
const char *str;
Make a copy of "str" in the string space.
If string is too large, makes free copy rather than one stuffed
in string space.
\*********************************************************************/
static symchar *CopyString(CONST char *str,int userlen)
{
  register unsigned long length;
  register struct StringSpaceRec *ptr;
  register char *result;
  int *lenptr;
  assert(userlen == (int)strlen(str));
  length = (unsigned long)userlen+1;
  if (length > MAXIMUM_STRING_LENGTH) {
    return AddBigString(str);
  }
  ptr = g_string_space;
  if (ptr != NULL){
    while ((ptr != NULL) &&
           ((length+ptr->used+sizeof(int)) >= MAXIMUM_STRING_LENGTH)) {
      ptr = ptr->next;
    }
    if (ptr != NULL){
      /* ptr must sit next symchar on a word boundary. */
      assert((ptr->used & (sizeof(int)-1)) == 0); 
      lenptr = (int *)&(ptr->data[ptr->used]);
      lenptr[0] = userlen;
      result = (char *)(&lenptr[1]);
      strcpy(result,str);
      ptr->used += (length+sizeof(int));
      while((ptr->used & (unsigned long)(sizeof(int)-1)) != 0 && 
            ptr->used < MAXIMUM_STRING_LENGTH) {
        /* pad string to next word boundary after nul with 0xFF */
        ptr->data[ptr->used] = (char)UCHAR_MAX;
        (ptr->used)++;
      } 
      return (symchar *)result;
    }
    ptr = g_string_space;
  }
  if ((g_string_space = ASC_NEW(struct StringSpaceRec))==NULL){
    ASC_PANIC("Unable to allocate string space.\n");
    
  }
  g_string_space->next = ptr;
  g_string_space->used = length + sizeof(int);
  strcpy((g_string_space->data + sizeof(int)),str);
  ((int *)g_string_space->data)[0] = userlen;
  while(((g_string_space->used) & (sizeof(int)-1)) != 0 && 
        g_string_space->used < MAXIMUM_STRING_LENGTH) {
    /* pad string to next word boundary after nul with 0xFF */
    g_string_space->data[g_string_space->used] = (char)UCHAR_MAX;
    (g_string_space->used)++;
  } 
  return (symchar *)(g_string_space->data+sizeof(int));
}

void DestroyStringSpace(void)
{
  register struct StringSpaceRec *ptr,*next;
  ptr = g_string_space;
  while (ptr != NULL){
    next = ptr->next;
#ifndef NDEBUG
    ascbzero((char *)ptr,(int)sizeof(struct StringSpaceRec));
#endif
    ascfree((char *)ptr);
    ptr = next;
  }
  g_string_space = NULL;
  DestroyBigStrings();
}

static void StringSpaceReport(FILE *fp)
{
  register unsigned long num_blocks=0,used_memory=0;
  register struct StringSpaceRec *ptr;
  ptr = g_string_space;
  while(ptr != NULL){
    num_blocks++;
    used_memory += ptr->used;
    ptr = ptr->next;
  }
  FPRINTF(fp,"Oversized string : %d\n",g_big_strings_cnt);
  FPRINTF(fp,"Number of blocks : %lu\n",num_blocks);
  FPRINTF(fp,"Total Memory use : %lu\n",
          num_blocks*sizeof(struct StringSpaceRec));
  FPRINTF(fp,"String memory use: %lu\n",num_blocks*MAXIMUM_STRING_LENGTH);
  FPRINTF(fp,"Actual usage     : %lu\n",used_memory);
  FPRINTF(fp,"Fraction filled  : %f\n",(double)used_memory/
	 ((double)num_blocks*(double)sizeof(struct StringSpaceRec)));
}


/*********************************************************************\
  Real Symbol Table Routines
\*********************************************************************/

void InitSymbolTable(void)
{
  register int c;
  for(c=0;c<SYMTABSIZE;g_symbol_table[c++]=NULL);
  g_symbol_size = 0;
  g_symbol_collisions = 0;
  g_big_strings = NULL;
  g_big_strings_cnt = 0;
}


#define SymHashFunction(s) hashpjw(SCP(s),SYMTABSIZE)

symchar *AddSymbolL(CONST char *c, int l)
{
  register unsigned long hv;
  register int cmp;
  register struct symbol_entry *item,*next;
  hv = SymHashFunction(c);
  if (l<=0) l = strlen(c);
  if (g_symbol_table[hv]==NULL) {
    item = ASC_NEW(struct symbol_entry);
    item->next = NULL;
    item->entry = CopyString(c,l);
    g_symbol_table[hv] = item;
    g_symbol_size++;
  }
  else {
    item = g_symbol_table[hv];
    if ((cmp = strcmp(SCP(item->entry),c))>0) {
      item = ASC_NEW(struct symbol_entry);
      item->next = g_symbol_table[hv];
      item->entry = CopyString(c,l);
      g_symbol_table[hv] = item;
      g_symbol_size++;
      g_symbol_collisions++;
    }
    else if (cmp<0) {
      for(next = item->next; next != NULL; item = next, next = next->next) {
	if ((cmp=strcmp(SCP(next->entry),c))>=0) {
	  if (cmp==0) return next->entry;
	  break;
	}
      }
      item->next = ASC_NEW(struct symbol_entry);
      item = item->next;
      item->next = next;
      item->entry = CopyString(c,l);
      g_symbol_size++;
      g_symbol_collisions++;
    }
  }
  return item->entry;
}

symchar *AddSymbol(CONST char *c)
{
  return AddSymbolL(c,strlen(c));
}

/*
 * this function tests whether an alleged symchar is in the table.
 */
symchar *AscFindSymbol(symchar *c)
{
  register unsigned long hv;
  register struct symbol_entry *next;

  hv = SymHashFunction(c);
  for (next = g_symbol_table[hv]; next != NULL; next = next->next) {
    if (next->entry == c) {
      return c;
    }
  }
  return NULL;
}

static
void PrintTabF(int noisy,FILE *fp)
{
  unsigned long c,counts[SYMTABSIZE];
  struct symbol_entry *item;
  for(c=0; c < SYMTABSIZE; counts[c++]=0);
  FPRINTF(fp,"Symbol table contains %lu entries with %lu collisions.\n",
	 g_symbol_size,g_symbol_collisions);
  if (noisy) {
    for(c = 0; c < SYMTABSIZE; c++) {
      for(item=g_symbol_table[c]; item!=NULL; item=item->next) {
        FPRINTF(fp,"%lu %s (%d)\n",c,SCP(item->entry),SCLEN(item->entry));
        counts[c]++;
      }
    }
    if ((item=g_big_strings) != NULL) {
      FPRINTF(fp,"Oversized string information\n");
      while(item !=NULL) {
        FPRINTF(fp,"%d %s (%d)\n",1,SCP(item->entry),SCLEN(item->entry));
        item = item->next;
      }
    }
    FPRINTF(fp,"Distribution information\n");
    for(c=0;c<SYMTABSIZE;c++) FPRINTF(fp,"%lu\t%lu\n",c,counts[c]);
  }
  StringSpaceReport(fp);
}

void PrintTab(int noisy)
{
  PrintTabF(noisy,stdout); 
}

#define STDUMP 0
void DestroySymbolTable(void)
{
  unsigned int c;
  struct symbol_entry *item,*next;
#if STDUMP
  FILE *fp;
  fp = fopen("/tmp/st.dat","w+");
  PrintTabF(1,fp);
  fclose(fp);
#endif
  for(c=0;c<SYMTABSIZE;g_symbol_table[c++]=NULL) {
    for(item=g_symbol_table[c];item!=NULL;item=next) {
      next = item->next;
      ascfree((char *)item);
    }
  }
}
