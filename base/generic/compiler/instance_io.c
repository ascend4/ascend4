/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Instance output routines
*//*
	by Tom Epperly
	Created: 2/8/90
	Last in CVS: $Revision: 1.45 $ $Date: 1998/04/10 23:25:44 $ $Author: ballan $
*/

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>
#include <general/table.h>



#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statement.h"
#include "slist.h"
#include "statio.h"
#include "instance_enum.h"
#include "parentchild.h"
#include "instquery.h"
#include "atomvalue.h"
#include "arrayinst.h"
#include "mathinst.h"
#include "tmpnum.h"
#include "visitinst.h"
#include "relation_type.h"
#include "relation_util.h"
#include "relation_io.h"
#include "logical_relation.h"
#include "logrel_util.h"
#include "logrel_io.h"
#include "when_io.h"
#include "when_util.h"
#include "dimen_io.h"
#include "instance_name.h"
#include "sets.h"
#include "setio.h"
#include "setinst_io.h"
#include "bit.h"
#include "child.h"
#include "type_desc.h"
#include "copyinst.h"
#include "instance_io.h"
#include "module.h"

/*------------------------------------------------------------------------------
  globals, forward decls, typedefs
*/

static char g_string_buffer[256];
#define SB255 g_string_buffer


struct InstanceEnumLookup{
  enum inst_t t;
  const char *name;
};

static const struct InstanceEnumLookup g_instancetypenames[] = {
#define LIST_D(NAME,VALUE) {NAME,#NAME}
#define LIST_X ,
	ASC_ENUM_DECLS(LIST_D,LIST_X)
	LIST_X {DUMMY_INST,NULL}
#undef LIST_D
#undef LIST_X
	
};

/*------------------------------------------------------------------------------
  INSTANCE TYPE
*/

CONST char *instance_typename(CONST struct Instance *inst){
	int i;
	AssertMemory(inst);
	for(i=0; g_instancetypenames[i].name!=NULL; ++i){
		/* CONSOLE_DEBUG("Testing '%s' (value=%d)",g_instancetypenames[i].name,g_instancetypenames[i].t); */
		if(g_instancetypenames[i].t == inst->t){
			/* CONSOLE_DEBUG("MATCHED"); */
			return g_instancetypenames[i].name;
		}
	}
	CONSOLE_DEBUG("No match");
	Asc_Panic(2,__FUNCTION__,"Invalid instance type (inst_t '%d' not found in list)",(int)inst->t);
}

/*------------------------------------------------------------------------------
  PATH STUFF

	for working out how to output contexted instance names
*/

struct gl_list_t *ShortestPath(CONST struct Instance *i,
		CONST struct Instance *ref,
		unsigned int height, unsigned int best
){
  struct gl_list_t *path,*shortest=NULL;
  unsigned long c,len;
  unsigned mybest= UINT_MAX;
  if (height>=best) return NULL;
  if (i==ref) {
    shortest = gl_create(1L);
    gl_append_ptr(shortest,(VOIDPTR)ref);
    return shortest;
  }
  if (0 != (len=NumberParents(i))){
    for(c=len;c>=1;c--){
      path = ShortestPath(InstanceParent(i,c),ref,height+1,mybest);
      if (path!=NULL){
	if (shortest==NULL){
	  shortest=path;
	  mybest = height+gl_length(path);
	} else{
	  if (gl_length(path)<gl_length(shortest)){
	    gl_destroy(shortest);
	    shortest = path;
	    mybest = height+gl_length(path);
	  } else {
            gl_destroy(path);
          }
	}
      }
    }
    if (shortest){
      gl_append_ptr(shortest,NULL);
      for(c=gl_length(shortest);c>1;c--) {
	gl_store(shortest,c,gl_fetch(shortest,c-1));
      }
      gl_store(shortest,1,(char *)i);
      assert((ref!=NULL)||(gl_length(shortest)==InstanceShortDepth(i)));
    }
  } else {
    if (ref==NULL) {
      shortest = gl_create(1L);
      gl_append_ptr(shortest,(VOIDPTR)i);
      assert(gl_length(shortest)==InstanceShortDepth(i));
    } else {
      return NULL;
    }
  }
  return shortest;
}

int WritePath(FILE *f, CONST struct gl_list_t *path)
{
  CONST struct Instance *parent,*child;
  struct InstanceName name;
  unsigned long c;
  int count = 0;

  if (path!=NULL){
    parent = gl_fetch(path,gl_length(path));
    for(c=gl_length(path)-1;c>=1;c--){
      child = gl_fetch(path,c);
      name = ParentsName(parent,child);
      switch (InstanceNameType(name)){
      case StrName:
	if (c<(gl_length(path)-1)) PUTC('.',f);
	FPRINTF(f,SCP(InstanceNameStr(name)));
	count += SCLEN(InstanceNameStr(name));
	break;
      case IntArrayIndex:
	count += FPRINTF(f,"[%ld]",InstanceIntIndex(name));
	break;
      case StrArrayIndex:
	count += FPRINTF(f,"['%s']",SCP(InstanceStrIndex(name)));
	break;
      }
      parent = child;
    }
  }
  else{
    FPRINTF(ASCERR,"Cannot print name.\n");
    FPRINTF(f,"?????");
  }
  return count;
}

static void WritePathDS(Asc_DString *dsPtr,CONST struct gl_list_t *path){
  CONST struct Instance *parent,*child;
  struct InstanceName name;
  unsigned long c;

  if (path!=NULL){
    parent = gl_fetch(path,gl_length(path));
    for(c=gl_length(path)-1;c>=1;c--){
      child = gl_fetch(path,c);
      name = ParentsName(parent,child);
      switch (InstanceNameType(name)){
      case StrName:
	if (c<(gl_length(path)-1)) {
          Asc_DStringAppend(dsPtr,".",1);
        }
        Asc_DStringAppend(dsPtr,SCP(InstanceNameStr(name)),-1);
	break;
      case IntArrayIndex:
	sprintf(SB255,"[%ld]",InstanceIntIndex(name));
        Asc_DStringAppend(dsPtr,SB255,-1);
	break;
      case StrArrayIndex:
	sprintf(SB255,"['%s']",SCP(InstanceStrIndex(name)));
        Asc_DStringAppend(dsPtr,SB255,-1);
	break;
      }
      parent = child;
    }
  } else{
    FPRINTF(ASCERR,"Cannot print name.\n");
    Asc_DStringAppend(dsPtr, "?????",5);
  }
}

char *WritePathString(CONST struct gl_list_t *path)
{
  char *result;
  Asc_DString ds, *dsPtr;
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WritePathDS(dsPtr,path);
  result = Asc_DStringResult(dsPtr);
  return result;
}

/*------------------------------------------------------------------------------
  INSTANCE NAME OUTPUTTERS
*/

int WriteInstanceName(FILE *f
	, CONST struct Instance *i, CONST struct Instance *ref
){
  struct gl_list_t *path;
  int count;
  /*if (i==ref && i !=NULL) {
    FPRINTF(ASCERR,"WriteInstanceName called with i,ref both"
      " pointing to:\n");
    WriteInstanceName(ASCERR,i,NULL);
    FPRINTF(ASCERR,"\n");
  }
  */
  path = ShortestPath(i,ref,0,UINT_MAX);
  count = WritePath(f,path);
  gl_destroy(path);
  return count;
}

void WriteInstanceNameDS(Asc_DString *dsPtr,
		      CONST struct Instance *i,
		      CONST struct Instance *ref)
{
  struct gl_list_t *path;
  path = ShortestPath(i,ref,0,UINT_MAX);
  WritePathDS(dsPtr,path);
  gl_destroy(path);
}

char *WriteInstanceNameString(CONST struct Instance *i,
                              CONST struct Instance *ref)
{
  char *result;
  Asc_DString ds, *dsPtr;
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WriteInstanceNameDS(dsPtr,i,ref);
  result = Asc_DStringResult(dsPtr);
  return result;
}

/**
  This is a temporary fix for writing out instance names faster
  than we are now. This is for use in saving simulations. We dont
  really care if it is the shortest path or not. The third version of
  this command will be use a stack to keep track of the name segments
  so that we dont always have to search back to the ref inference.
  The fourth evolution will allow the sort of filters are now
  allowed when exporting to the probe, so as to give selective
  writing of data.
  KAA

  Ref is completely irrelevant. If it is not, one should use a 
  indexed visit tree so that the paths can be computed properly.
  Prototype code never dies. As expected, this is a production
  function now.
  BAA
*/
static
void InstanceAnyPath(struct Instance *i, struct gl_list_t *path)
{
  struct Instance *parent;
  unsigned long len;
  if (i==NULL)  {
    return;
  }
  gl_append_ptr(path,(VOIDPTR)i);
  len = NumberParents(i);
  if (len) {
    parent = InstanceParent(i,1);	/* take the first */
    InstanceAnyPath(parent,path);
  }
  return;
}

int WriteAnyInstanceName(FILE *f, struct Instance *i)
		       
{
  struct gl_list_t *path_list;
  int count;
  path_list = gl_create(23L);
  InstanceAnyPath(i,path_list);
  count = WritePath(f,path_list);
  gl_destroy(path_list);
  /* costs nothing. lists are recycled */
  return count; 
}


/**
	Copies all but the last element of path.  It allocates new memory
	for each of the NameNode structures and copies the contents from path.
*/
static struct gl_list_t *CopyPathHead(CONST struct gl_list_t *path){
  struct gl_list_t *result;
  struct NameNode *orig, *copy;
  unsigned long c,length;
  length = gl_length(path)-1;
  result = gl_create(length);
  for(c=1;c<=length;c++){
    orig = gl_fetch(path,c);
    copy = ASC_NEW(struct NameNode);
    copy->inst = orig->inst;
    copy->index = orig->index;
    gl_append_ptr(result,(VOIDPTR)copy);
  }
  return result;
}

struct gl_list_t *AllPaths(CONST struct Instance *i)
{
  struct gl_list_t *result,*tmp1,*tmp2,*path;
  CONST struct Instance *parent;
  struct NameNode *nptr;
  unsigned long length, cindex;
  unsigned c,count;
  if (NumberParents(i)==0){	/* found root. will be sim. */
    result = gl_create(1);
    path = gl_create(1);
    nptr = ASC_NEW(struct NameNode);
    nptr->inst = i;
    nptr->index = 0;
    gl_append_ptr(path,(VOIDPTR)nptr);
    gl_append_ptr(result,(VOIDPTR)path);
  } else {
    if(NumberParents(i)==1){
      result = AllPaths(InstanceParent(i,1));
    } else{
      result = gl_create(0);
      for(c=NumberParents(i);c>=1;c--){
	tmp1 = AllPaths(InstanceParent(i,c));
	tmp2 = gl_concat(result,tmp1);
	gl_destroy(result);	/* these *should* be gl_destroy */
	gl_destroy(tmp1);	/* these *should* be gl_destroy */
	result = tmp2;
      }
    }
    /* going from the end of the list to the beginning is crucial to the
     * workings of this loop because additional paths may be appended
     * onto the end of result
     */
    for(c=gl_length(result);c>=1;c--){
      path = (struct gl_list_t *)gl_fetch(result,c);
      nptr = gl_fetch(path,gl_length(path));
      parent = nptr->inst;
      length = NumberChildren(parent);
      count=0;
      for(cindex=1; cindex <= length; cindex++){
	if (InstanceChild(parent,cindex) == i){
	  if (count++) {
	    tmp1 = CopyPathHead(path);
	    gl_append_ptr(result,(VOIDPTR)tmp1);
	  } else {
            tmp1 = path;
          }
	  nptr = ASC_NEW(struct NameNode);
	  nptr->inst = i;
	  nptr->index = cindex;
	  gl_append_ptr(tmp1,(VOIDPTR)nptr);
	}
      }
      assert(count);
    }
  }
  return result;
}

/**
	@return 0 if a WILL_BE or ALIASES or ARR origin is encountered in path
*/
static
int PathOnlyISAs(CONST struct gl_list_t *path)
{
  CONST struct Instance *parent;
  CONST struct NameNode *nptr;
  ChildListPtr clist;
  unsigned int origin;
  unsigned long c,len;

  if (path!=NULL){
    len = gl_length(path);
    nptr = gl_fetch(path,1);
    parent = nptr->inst;
    for(c=2; c <= len; c++) {
      nptr = gl_fetch(path,c); /* move up nptr */
      if (IsArrayInstance(parent)==0) {
        clist = GetChildList(InstanceTypeDesc(parent));
        origin = ChildOrigin(clist,nptr->index);
        if (origin != origin_ISA && origin != origin_PISA) {
          return 0;
        }
      } /* else skip subscripts with arrays as parents. */
      parent = nptr->inst; /* move up parent 1 step behind */
    }
    return 1;
  } else {
    return 0;
  }
}

struct gl_list_t *ISAPaths(CONST struct gl_list_t *pathlist)
{
  struct gl_list_t *result, *path;
  unsigned long c,len;
  if (pathlist == NULL) {
    FPRINTF(ASCERR,"ISAPaths(p) called with NULL pathlist p!\n");
    return NULL;
  }
  result = gl_create(3);
  len = gl_length(pathlist);
  for (c = 1; c <=len; c++) {
    path = (struct gl_list_t *)gl_fetch(pathlist,c);
    if (PathOnlyISAs(path) == 1) {
      gl_append_ptr(result,(VOIDPTR)path);
    }
  }
  return result;
}

/*------------------------------------------------------------------------------
  STUFF ABOUT ALIASES
*/

static
void AliasWritePath(FILE *f, CONST struct gl_list_t *path)
{
  CONST struct Instance *parent;
  CONST struct NameNode *nptr;
  struct InstanceName name;
  unsigned long c,len;
  if (path!=NULL){
    len = gl_length(path);
    nptr = gl_fetch(path,1);
    parent = nptr->inst;
    for(c=2;c<=len;c++){
      nptr = gl_fetch(path,c);
      name = ChildName(parent,nptr->index);
      switch (InstanceNameType(name)){
      case StrName:
	if (c>2) PUTC('.',f);
	FPRINTF(f,SCP(InstanceNameStr(name)));
	break;
      case IntArrayIndex:
	FPRINTF(f,"[%ld]",InstanceIntIndex(name));
	break;
      case StrArrayIndex:
	FPRINTF(f,"['%s']",SCP(InstanceStrIndex(name)));
	break;
      }
      parent = nptr->inst;
    }
  } else{
    FPRINTF(ASCERR,"Cannot print name.\n");
    FPRINTF(f,"?????");
  }
}

static
char *AliasWritePathString(CONST struct gl_list_t *path)
{
  char buff[20], *result;
  CONST struct Instance *parent;
  CONST struct NameNode *nptr;
  struct InstanceName name;
  unsigned long c,len;
  Asc_DString ds, *dsPtr;

  dsPtr = &ds;
  Asc_DStringInit(dsPtr);

  if (path!=NULL){
    len = gl_length(path);
    nptr = gl_fetch(path,1);
    parent = nptr->inst;
    for(c=2;c<=len;c++){
      nptr = gl_fetch(path,c);
      name = ChildName(parent,nptr->index);
      switch (InstanceNameType(name)) {
      case StrName:
	if (c>2) Asc_DStringAppend(dsPtr,".",1);
        Asc_DStringAppend(dsPtr,SCP(InstanceNameStr(name)),-1);
	break;
      case IntArrayIndex:
	sprintf(buff,"[%ld]",InstanceIntIndex(name));
        Asc_DStringAppend(dsPtr,buff,-1);
	break;
      case StrArrayIndex:
	Asc_DStringAppend(dsPtr,"['",2);
	Asc_DStringAppend(dsPtr,SCP(InstanceStrIndex(name)),-1);
	Asc_DStringAppend(dsPtr,"']",2);
	break;
      }
      parent = nptr->inst;
    }
  }
  else{
	/** @TODO what is the meaning of ????? and when might it happen? */
    Asc_DStringAppend(dsPtr,"?????",5);
  }
  result = Asc_DStringResult(dsPtr);
  return result;
}

unsigned long CountAliases(CONST struct Instance *i)
{
  struct gl_list_t *paths,*path;
  unsigned long c,len;

  paths = AllPaths(i);
  len = gl_length(paths);
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);
  return len;
}

unsigned long CountISAs(CONST struct Instance *i)
{
  struct gl_list_t *paths, *path, *isapaths;
  unsigned long c,len;

  paths = AllPaths(i);
  isapaths = ISAPaths(paths);

  len = gl_length(paths);
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);
  /* do not fetch from isapaths after this point. it's data
   * pointers have been freed in destroying paths, but
   * isapaths doesn't know this.
   */
  len = gl_length(isapaths);
  gl_destroy(isapaths);
  return len;
}

void WriteAliases(FILE *f, CONST struct Instance *i)
{
  struct gl_list_t *paths,*path;
  unsigned long c,len;
  paths = AllPaths(i);
  len = gl_length(paths);
#if 1
  FPRINTF(f,"Number of names: %lu\n",len);
#endif
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    AliasWritePath(f,path);
    PUTC('\n',f);
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);		/* this *should* gl_destroy */
}

void WriteISAs(FILE *f, CONST struct Instance *i)
{
  struct gl_list_t *paths,*path, *isapaths;
  unsigned long c,len;
  paths = AllPaths(i);
  isapaths = ISAPaths(paths);
  len = gl_length(isapaths);
#if 1
  FPRINTF(f,"Number of names: %lu\n",len);
#endif
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(isapaths,c);
    AliasWritePath(f,path);
    PUTC('\n',f);
  }
  gl_destroy(isapaths);
  len = gl_length(paths);
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);
}

struct gl_list_t *WriteAliasStrings(CONST struct Instance *i)
{
  struct gl_list_t *paths,*path,*strings;
  char *tmp = NULL;
  unsigned long c,len;

  paths = AllPaths(i);
  len = gl_length(paths);
  strings = gl_create(len);
  for(c=1;c<=len;c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    tmp = AliasWritePathString(path);
    gl_append_ptr(strings,(VOIDPTR)tmp);
    tmp = NULL;
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);		/* this *should* gl_destroy */
  return strings;
}

struct gl_list_t *WriteISAStrings(CONST struct Instance *i)
{
  struct gl_list_t *paths,*path,*strings, *isapaths;
  char *tmp = NULL;
  unsigned long c,len;

  paths = AllPaths(i);
  isapaths = ISAPaths(paths);
  len = gl_length(isapaths);
  strings = gl_create(len);
  for(c = 1; c <= len; c++){
    path = (struct gl_list_t *)gl_fetch(isapaths,c);
    tmp = AliasWritePathString(path);
    gl_append_ptr(strings,(VOIDPTR)tmp);
    tmp = NULL;
  }
  gl_destroy(isapaths);
  len = gl_length(paths);
  for(c = 1; c <= len; c++){
    path = (struct gl_list_t *)gl_fetch(paths,c);
    gl_free_and_destroy(path);
  }
  gl_destroy(paths);		/* this *should* gl_destroy */
  return strings;
}

void WriteClique(FILE *f, CONST struct Instance *i)
{
  CONST struct Instance *tmp;
  tmp = i;
  do {
    WriteAliases(f,tmp);
    tmp = NextCliqueMember(tmp);
  } while(tmp != i);
}

/*------------------------------------------------------------------------------
  STUFF ABOUT PENDING STATEMENTS
*/

static
void WritePendingStatements(FILE *f, CONST struct Instance *i)
{
  CONST struct BitList *blist;
  CONST struct TypeDescription *desc;
  CONST struct StatementList *slist;
  CONST struct Statement *stat;
  CONST struct gl_list_t *list;
  unsigned long c,len;
  blist = InstanceBitList(i);
  if ((blist!=NULL)&&(!BitListEmpty(blist))){
    FPRINTF(f,"PENDING STATEMENTS\n");
    desc = InstanceTypeDesc(i);
    slist = GetStatementList(desc);
    list = GetList(slist);
    len = gl_length(list);
    for(c=1;c<=len;c++){
      if (ReadBit(blist,c-1)){
        stat = (struct Statement *)gl_fetch(list,c);
	WriteStatement(f,stat,4);
        if (StatementType(stat)== SELECT) {
          c = c + SelectStatNumberStats(stat);
        }
      }
    }
  }
}

/*------------------------------------------------------------------------------
  ATOMS AND THEIR CHILDREN
  (the nuclear family)
*/

void WriteAtomValue(FILE *f, CONST struct Instance *i)
{
  if (AtomAssigned(i)){
    switch(InstanceKind(i)){
    case REAL_INST:
    case REAL_ATOM_INST:
    case REAL_CONSTANT_INST:
      FPRINTF(f,"%.18g",RealAtomValue(i));
      break;
    case INTEGER_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
      FPRINTF(f,"%ld",GetIntegerAtomValue(i));
      break;
    case SET_INST:
    case SET_ATOM_INST:
      WriteInstSet(f,SetAtomList(i));
      break;
    case BOOLEAN_INST:
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
      FPRINTF(f,GetBooleanAtomValue(i)?"TRUE":"FALSE");
      break;
    case SYMBOL_INST:
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
      FPRINTF(f,"'%s'",SCP(GetSymbolAtomValue(i)));
      break;
    default:
      break; /* NOTREACHED normally */
    }
  }
  else{
    FPRINTF(f,"UNDEFINED");
  }
}

static
void WriteAtomChildren(FILE *f, CONST struct Instance *i)
{
  unsigned long c,len;
  struct InstanceName rec;
  CONST struct Instance *child;
  ChildListPtr clist;
  int hidecount= 0;

  len = NumberChildren(i);
  if (len){
    FPRINTF(f,"CHILDREN VALUES\n");
    clist = GetChildList(InstanceTypeDesc(i));
    for(c=1;c<=len;c++){
      if (ChildVisible(clist,c)) {
        rec = ChildName(i,c);
        assert(InstanceNameType(rec)==StrName);
        FPRINTF(f,"    %-30s ",SCP(InstanceNameStr(rec)));
        child = InstanceChild(i,c);
        WriteAtomValue(f,child);
        PUTC('\n',f);
      } else {
        hidecount++;
      }
    }
    if (hidecount !=0) {
      FPRINTF(f,"	and %d hidden children\n",hidecount);
    }
  }
}

static
void WriteNameRec(FILE *f, CONST struct InstanceName *rec)
{
  unsigned c;
  switch(InstanceNameType(*rec)){
  case IntArrayIndex:
    FPRINTF(f,"%-30ld",InstanceIntIndex(*rec));
    break;
  case StrArrayIndex:
    FPRINTF(f,"'%s'",SCP(InstanceStrIndex(*rec)));
    c = SCLEN(InstanceStrIndex(*rec))+2;
    if (c >=30) c =0;
    else c = 30-c;
    while (c--) PUTC(' ',f);
    break;
  case StrName:
    FPRINTF(f,"%-30s",SCP(InstanceNameStr(*rec)));
    break;
  }
}

static
void WriteTypeOrValue(FILE *f, CONST struct Instance *i)
{
  switch(InstanceKind(i)){
  case REL_INST:
  case LREL_INST:
  case MODEL_INST:
  case WHEN_INST:
  case DUMMY_INST:
    FPRINTF(f,SCP(InstanceType(i)));
    break;
  case REAL_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case SET_INST:
  case SET_ATOM_INST:
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
    WriteAtomValue(f,i);
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    FPRINTF(f,"ARRAY OF %s REFINEMENTS",
		SCP(GetName(GetArrayBaseType(InstanceTypeDesc(i)))));
    break;
  default:
    ASC_PANIC("Unknown instance type in WriteTypeOrValue.\n");
    break;
  }
}

static
void WriteArrayChildren(FILE *f, CONST struct Instance *i)
{
  unsigned long c,len;
  CONST struct Instance *child;
  struct InstanceName rec;
  len = NumberChildren(i);
  for(c=1;c<=len;c++){
    child = InstanceChild(i,c);
    rec = ChildName(i,c);
    WriteNameRec(f,&rec);
    WriteTypeOrValue(f,child);
    PUTC('\n',f);
  }
}

static
void ListChildren(FILE *f, CONST struct Instance *i)
{
  unsigned long c,length;
  struct InstanceName name;
  struct Instance *ch;
  ChildListPtr clist;
  int hidecount=0, PorT;

  length = NumberChildren(i);
  if (length){
    FPRINTF(f,"CHILDREN\n%-30sType\n","Name");
    clist = GetChildList(InstanceTypeDesc(i));
    for(c=1;c<=length;c++){
      if (ChildVisible(clist,c)) {
        name = ChildName(i,c);
        WriteNameRec(f,&name);
        ch = InstanceChild(i,c);
        if ( ch != NULL /* && type not hidden */) {
          FPRINTF(f,"%s\t",SCP(InstanceType(ch)));
          WriteTypeOrValue(f,ch);
#define ATDEBUG 0 /* puts out tmpnum and ptr for debugging classification */
#if ATDEBUG
          FPRINTF(f," TN = %ld ip=0x%p\n",GetTmpNum(ch),ch);
#else

          FPRINTF(f,"\n");
#endif
        } else {
          PorT = (ChildDeclaration(i,c)!=NULL &&
                  StatWrong(ChildDeclaration(i,c)));
          FPRINTF(f,
                  "NULL_INSTANCE %s\n", PorT ? "PERMANENTLY" : "TEMPORARILY");
        }
      } else {
        hidecount++;
      }
    }
    if (hidecount !=0) {
      FPRINTF(f,"	and %d hidden children\n",hidecount);
    }
  }
}

/*------------------------------------------------------------------------------
  OUTPUT FUNCTIONS FOR DEBUGGING
*/

void WriteInstance(FILE *f, CONST struct Instance *i)
{
  CONST struct logrelation *lreln;
  CONST struct relation *reln;
  enum Expr_enum reltype;


  if (i==NULL) {
    FPRINTF(ASCERR,"WriteInstance called with NULL instance.\n");
    return;
  }
  switch(InstanceKind(i)) {
  case MODEL_INST:
    FPRINTF(f,"MODEL INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
#if 0 /* old when stuff, probably dead code */
    if(model_flagbit(i,MODEL_ON)) {
      FPRINTF(f,"MODEL ON\n");
    } else {
      FPRINTF(f,"MODEL OFF\n");
    }
#endif
    WritePendingStatements(f,i);
    ListChildren(f,i);
    break;
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    FPRINTF(f,"REAL INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    if (AtomAssigned(i)){
      FPRINTF(f,"Value: %g\n",RealAtomValue(i));
    }
    else{
      FPRINTF(f,"Value: Undefined\n");
    }
    FPRINTF(f,"Dimensions: ");
    WriteDimensions(f,RealAtomDims(i));
    PUTC('\n',f);
    WriteAtomChildren(f,i);
    break;
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
    FPRINTF(f,"BOOLEAN INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    if (AtomAssigned(i)){
      FPRINTF(f,GetBooleanAtomValue(i) ? "Value: TRUE\n" : "Value: FALSE\n");
    }
    else{
      FPRINTF(f,"Value: Undefined\n");
    }
    WriteAtomChildren(f,i);
    break;
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
    FPRINTF(f,"INTEGER INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    if (AtomAssigned(i)){
      FPRINTF(f,"Value: %ld\n",GetIntegerAtomValue(i));
    }
    else{
      FPRINTF(f,"Value: Undefined\n");
    }
    WriteAtomChildren(f,i);
    break;
  case SET_INST:
  case SET_ATOM_INST:
    FPRINTF(f,"SET INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    if (AtomAssigned(i)){
      FPRINTF(f,"Value: ");
      WriteInstSet(f,SetAtomList(i));
      PUTC('\n',f);
    }
    else{
      FPRINTF(f,"Value: Undefined\n");
    }
    WriteAtomChildren(f,i);
    break;
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    FPRINTF(f,"SYMBOL INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    if (AtomAssigned(i)){
      FPRINTF(f,"Value: '%s'\n", SCP(GetSymbolAtomValue(i)));
    }
    else{
      FPRINTF(f,"Value: Undefined\n");
    }
    WriteAtomChildren(f,i);
    break;
  case REL_INST:
    /*
     * Using ref as NULL; the correct fix requires finding
     * the parent in the case of arrays of relation instances.
     */
    FPRINTF(f,"RELATION INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
#if 0 /* old WHEN stuff. probably dead code. */
    if(relinst_flagbit(((struct RelationInstance *)(i)),RELINST_ON)) {
      FPRINTF(f,"RELATION ON\n");
    } else {
      FPRINTF(f,"RELATION OFF\n");
    }
#endif
    WriteRelation(f,i,NULL);
    PUTC('\n',f);
    reln = GetInstanceRelation(i,&reltype);
    if (!reln) {
      break;
    }
#if 0 /* set to 1 if you want to dump multiple formats */
    Infix_WriteRelation(f,i,NULL);
    PUTC('\n',f);
    WriteRelationPostfix(f,i,NULL);
    PUTC('\n',f);
#endif
    FPRINTF(f,"Residual: %g\n",
	    RelationResidual(GetInstanceRelation(i,&reltype)));
    FPRINTF(f,"Multiplier: %g\n",
	    RelationMultiplier(GetInstanceRelation(i,&reltype)));
    FPRINTF(f,"Nominal: %g\n",
	    RelationNominal(GetInstanceRelation(i,&reltype)));
    FPRINTF(f,RelationIsCond(GetInstanceRelation(i,&reltype)) ?
                            "Relation is Conditional\n" : "");
    WriteAtomChildren(f,i);
    break;
  case LREL_INST:
    FPRINTF(f,"LOGRELATION INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    WriteLogRel(f,i,NULL);
    PUTC('\n',f);
    lreln = GetInstanceLogRel(i);
    if (!lreln) {
      break;
    }
#if 0 /* set to 1 if you want to dump multiple formats */
    WriteLogRelInfix(f,i,NULL);
    PUTC('\n',f);
    WriteLogRelPostfix(f,i,NULL);
    PUTC('\n',f);
#endif
      FPRINTF(f,LogRelNominal(GetInstanceLogRel(i)) ?
                       "Nominal: TRUE\n" : "Nominal: FALSE\n");
      FPRINTF(f,LogRelResidual(GetInstanceLogRel(i)) ?
                       "Residual: TRUE\n" : "Residual: FALSE\n");
      FPRINTF(f,LogRelIsCond(GetInstanceLogRel(i)) ?
                        "Logical Relation is Conditional\n" : "");
    WriteAtomChildren(f,i);
    break;
  case WHEN_INST:
    /*
     * Using ref as NULL; the correct fix requires finding
     * the parent in the case of arrays of when instances.
     */
    FPRINTF(f,"WHEN INSTANCE.\nType: %s\n",SCP(InstanceType(i)));
    WriteWhen(f,i,NULL);
    break;
  case ARRAY_INT_INST:
    FPRINTF(f,"ARRAY INSTANCE INDEXED BY integer.\n");
    FPRINTF(f,"Indirected = %ld. Deref# = %lu\n",
      InstanceIndirected(i), NumberofDereferences(i));
    WriteArrayChildren(f,i);
    break;
  case ARRAY_ENUM_INST:
    FPRINTF(f,"ARRAY INSTANCE INDEXED BY symbol.\n");
    FPRINTF(f,"Indirected = %ld. Deref# = %lu\n",
      InstanceIndirected(i), NumberofDereferences(i));
    WriteArrayChildren(f,i);
    break;
  case DUMMY_INST:
    FPRINTF(f,"GlobalDummyInstance\n");
    break;
  default:
    ASC_PANIC("Unknown instance type in WriteInstance.\n");
  }
}

/**
	This is a debugging aid and not intended for
	general use
*/
void WriteInstanceList(struct gl_list_t *list)
{
  unsigned long len,c;
  struct Instance *i;
  if (list) {
    len = gl_length(list);
    for (c=1;c<=len;c++) {
      i = (struct Instance *)gl_fetch(list,c);
      WriteInstanceName(stdout,i,NULL);
      FPRINTF(stdout,"\n");
    }
  }
}

/*------------------------------------------------------------------------------
  PERSISTENCE FUNCTIONS
*/

/**
	@TODO the following mess o' save hacks deserves its own file.,
	probably a circular file.
*/

/** @TODO what is the status of this? A lot of unused functions here. */

/** @page instancepersistence "Saving/Restoring Instance Trees"

	The below code is part of the code for saving/restoring instance
	trees. It thus allows the creation of persistent objects. At this
	time the format of the save_file is experimental, but has the
	following format:
<pre>
  $DATE
  $VERSION

  $TYPES {
      name : module ;
      [...]
      name : module ;
  }

  $COMPLEX_INST index {
      type kind name nchildren/bytesize universal ;
  }

  $ATOM_INST index {
      $VALUE : value units ';'
      type kind name value units  ';' # -- for the atom children.
      type kind name value units  ';'
      type kind name value units  ';'
  }

  $RELATION index { # -- optional
      $VALUE : value units ';'
      $VARIABLES :
      index -> index ,index [..] ,index ';'
      $CONSTANTS :
      index -> index ,index [..] ,index ';'
      $OPCODES :
      index -> index ,index [..] ,index ';'
  }


  $LRELATION index { # -- optional
      $BVARIABLES :
      index -> index ,index [..] ,index ';'
      $BCONSTANTS :
      index -> index ,index [..] ,index ';'
      $LOPCODES :
      index -> index ,index [..] ,index ';'
  }


  $GRAPH {
      index -> index ,index [..] ,index ';'
      [...]
      index -> index ,index [..] ,index ';'
  }

  $CLIQUES {
      index -> index ,index [..] ,index ';'
      [...]
      index -> index ,index [..] ,index ';'
  }
</pre>
*/

#define TYPE_HASH_SIZE 31

static int CmpDescPtrs(VOIDPTR d1, VOIDPTR d2)
{
  return (d1 < d2) ? -1 : ((d1 == d2) ? 0 : 1);
}

static
int ProcessArrayDesc(struct gl_list_t *arraytypelist,
		     CONST struct TypeDescription *desc)
{
  struct TypeDescription *tmp;

  tmp = (struct TypeDescription *)gl_search(arraytypelist,(VOIDPTR)desc,
                                            (CmpFunc)CmpDescPtrs);
  if (tmp==NULL) {
    gl_append_ptr(arraytypelist,(VOIDPTR)desc);
    return 1;		/* indicate if we added or not */
  }
  return 0;		/* indicate if we added or not */
}

/**
	Collect a unique list of the types present in the instance
	tree (which is stored in the list). We will *not* store
	typedescriptions, with NULL names; This can happen in the case
	of array types. We could probably filter here for all fundamental
	types in fact.
	
	At this time we are doing a hack in type_desc.c to *ensure*
	that the arrays have names. This means that name should not
	come up NULL *ever* in the type table. If it does, its an
	error. We now instead scan for base_types, so that we can
	write out some index stuff for arrays.
	
	(BAA: the hack has been institutionalized as MAKEARRAYNAMES
	in type_desc.h)
*/
static int DoBreakPoint(void)
{
  return 1;
}

static
void CollectTypes(struct Table *table,struct gl_list_t *list,
		  struct gl_list_t *arraytypelist)
{
  CONST struct TypeDescription *desc;
  char *name;
  struct Instance *inst;
  enum type_kind kind;
  unsigned long len,c;

  len  = gl_length(list);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(list,c);
    desc = InstanceTypeDesc(inst);
    name = (char *)SCP(GetName(desc));
    if (name==NULL) {
      FPRINTF(ASCERR,"Unknown type with no name in instance tree\n");
      DoBreakPoint();	/* later we will punt here */
    }
    kind = GetBaseType(desc);
    if (kind==array_type) {
      (void)ProcessArrayDesc(arraytypelist,desc);
    } else {
      AddTableData(table,(void *)desc,name);
    }
  }
}


static
void WriteIntegrityCheck(FILE *fp, unsigned long count)
{
  CONST char *timestring = "not_yet_fiqured_out";
  FPRINTF(fp,"$CHECKSUM {\n");
  FPRINTF(fp,"\t$DATE  \'%s\';\n",timestring);
  FPRINTF(fp,"\t$VERSION  1;\n");
  FPRINTF(fp,"\t$COUNT %lu;\n",count);
  FPRINTF(fp,"}\n\n");
}

/**
	Some special care in processing is required here.
	The name of the type may be NULL, as in the case of arrays.
	The module of the type may be NULL, as in the case of
	fundamentals. In collecting the typelist and building the
	type table we took care of the NULL type names. We will
	write out NULL for types with NULL modules.
	NOTE: The module names are written out as single-quoted strings.
	NULL modules are simply written as NULL.
*/
static
void Save__Types(void *arg1, void *arg2)
{
  CONST struct TypeDescription *desc = (CONST struct TypeDescription *)arg1;
  FILE *fp = (FILE *)arg2;
  symchar *type;
  CONST char *module;
  struct module_t *mod;

  type = GetName(desc);
  if (type==NULL) return;
  mod = GetModule(desc);
  if (mod) {
    module = Asc_ModuleName(mod);
    FPRINTF(fp,"\t%s : \'%s\';\n",SCP(type),module);
  }
  else{
    FPRINTF(fp,"\t%s : NULL;\n",SCP(type));
  }
}

static
void SaveTypes(FILE *fp, struct Table *table)
{
  FPRINTF(fp,"$TYPEDESC {\n");
  TableApplyAllTwo(table,Save__Types,(void *)fp);
  FPRINTF(fp,"}\n\n");
}



#ifdef THIS_IS_AN_UNUSED_FUNCTION
/*
 * These functions are concerned with saving arraytypes
 * in our special format.
 */
static
void SaveIndexList(FILE *fp, struct IndexType *itype)
{
  CONST struct Set *sptr, *tmp;
  tmp = sptr = GetIndexSet(itype);
  /* lots of crappy assumptions here.... */
  while (tmp) {
    WriteSetNode(fp,tmp);
    tmp = NextSet(tmp);
    if (tmp)
      FPRINTF(fp,", ");
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/**
	SYNTAX:
	arraydef : ARRAYTYPE IDENTIFIER '{' mandatory optional '}'
			 ;
	mandatory: TYPE SYMBOL ',' COUNT INTEGER ';'
		 ;
	optional: (INDEX INTEGER ':' string)* (i.e one or more)

	@NOTE It is possible for array to *not* have any indicies.
	This can happen for example, when the result of the set
	evaluation comes up NULL;
<pre>
	    foo[fooset] IS_A FOO;
	    fooset := [];
</pre>
	This code appropriately deals with these odd cases.
	@ENDNOTE
	
	@NOTE (is that you, Ben?) 
	You would not believe the stuff that is returned as a
	result of this code. Yep the above note about evaluation
	is rubbish. What is saved is the index set as found verbatim.
	Using the above example what is seens is "fooset". Likewise
	if we had foo2[alpha - [beta]], where alpha and beta are
	sets, what we see is "alpha [beta] -". I am leaving this
	code here for posterity. This craziness might just come in
	handy. For example it makes a comparison of 2 index sets
	very quick, rather than if we had saved the result of
	the evaluations.
*/
static
void Save__ArrayTypes(FILE *fp,struct TypeDescription *desc)
{
  CONST struct TypeDescription *basetype;
  CONST char *tmp;
  struct gl_list_t *indexlist;
  struct IndexType *itype;
  unsigned long len,c;

  tmp = SCP(GetName(desc));			/* dump header */
  FPRINTF(fp,"$ARRAYTYPE %s {\n",tmp);
  basetype = GetArrayBaseType(desc);
  assert(basetype!=NULL);
  FPRINTF(fp,"\t$TYPE %s, \n",SCP(GetName(basetype)));

  indexlist = GetArrayIndexList(desc);	/* save index lists strings */
  if (!indexlist) {
    FPRINTF(fp,"$COUNT 0;\n");
    goto trailer;
  }
  len = gl_length(indexlist);
  FPRINTF(fp,"$COUNT %lu;\n",len);
  for (c=1;c<=len;c++) {
    itype = (struct IndexType *)gl_fetch(indexlist,c);
    tmp = SCP(GetIndexSetStr(itype));
    assert(tmp!=NULL);
    FPRINTF(fp,"\t$INDEXES %lu : \"%s\";\n",c,tmp);
  }
 trailer:					/* dump trailer */
  FPRINTF(fp,"}\n\n");
}

static
void SaveArrayTypes(FILE *fp,struct gl_list_t *arraytypelist)
{
  struct TypeDescription *desc;
  unsigned long len,c;
  len = gl_length(arraytypelist);
  for (c=1;c<=len;c++) {
    desc = (struct TypeDescription *)gl_fetch(arraytypelist,c);
    Save__ArrayTypes(fp,desc);
  }
}

/**
	Write a comma-delimited list of children names.
	The caller must add own leaders/trailers.
*/
static
void SaveNameRec(FILE *f, CONST struct InstanceName *rec)
{
  switch(InstanceNameType(*rec)){
  case IntArrayIndex:
    FPRINTF(f,"%ld",InstanceIntIndex(*rec));
    break;
  case StrArrayIndex:
    FPRINTF(f,"'%s'",SCP(InstanceStrIndex(*rec)));
    break;
  case StrName:
    FPRINTF(f,"%s",SCP(InstanceNameStr(*rec)));
    break;
  }
}

static
void Save__ChildrenNames(FILE *fp, struct Instance *inst)
{
  unsigned long nch,c;
  struct InstanceName rec;

  nch = NumberChildren(inst);
  if (nch) {
    rec = ChildName(inst,1);
    SaveNameRec(fp,&rec);
  }
  for (c=2;c<=nch;c++) {		/* notreached for nch < 2 */
    FPRINTF(fp," ,");
    rec = ChildName(inst,c);
    SaveNameRec(fp,&rec);
  }
}


static
void Save__ComplexInsts(FILE *fp, struct Instance *inst)
{
  CONST struct TypeDescription *desc;
  symchar *type;
  enum inst_t kind;
  unsigned long count;
  unsigned int universal, intset = 0;

  kind = InstanceKind(inst);
  desc = InstanceTypeDesc(inst);
  universal = GetUniversalFlag(desc);

  switch (kind) {
  case SET_ATOM_INST:
    intset = GetSetAtomKind(inst);
    type = InstanceType(inst);
    count = GetByteSize(desc);
    FPRINTF(fp,"\t$KIND %d, $TYPE %s, $COUNT %lu;\n",
	    (int)kind, SCP(type), count);
    if (universal)
      FPRINTF(fp,"\t$UNIVCHILD %d;\n",universal);
    if (intset)
      FPRINTF(fp,"\t$INTSET %d;\n",intset);
    break;
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case LREL_INST:
  case REL_INST:
    type = InstanceType(inst);
    count = GetByteSize(desc);
    FPRINTF(fp,"\t$KIND %d, $TYPE %s, $COUNT %lu;\n",
	    (int)kind, SCP(type), count);
    if (universal)
      FPRINTF(fp,"\t$UNIVCHILD %d;\n",universal);
    break;
  case SIM_INST:
  case MODEL_INST:
    type = InstanceType(inst);
    count = NumberChildren(inst);
    FPRINTF(fp,"\t$KIND %d, $TYPE %s, $COUNT %lu;\n",
	    (int)kind, SCP(type), count);
    if (universal)
      FPRINTF(fp,"\t$UNIVCHILD %d;\n",universal);
    break;
  case ARRAY_ENUM_INST:
  case ARRAY_INT_INST:
    type = GetName(desc);
    count = NumberChildren(inst);
    FPRINTF(fp,"\t$KIND %d, $TYPE %s, $COUNT %lu;\n",
	    (int)kind, SCP(type), count);
    desc = GetArrayBaseType(desc);
    type = GetName(desc);
    FPRINTF(fp,"\t$BASETYPE %s;\n",SCP(type));		/* the base type */
    if (universal)
      FPRINTF(fp,"\t$UNIVCHILD %d;\n",universal);
    if (0 != (count=NumberofDereferences(inst)))
      FPRINTF(fp,"\t$INDIRECT %lu;\n",count);
    FPRINTF(fp,"\t$INDEXES  : ");
    Save__ChildrenNames(fp,inst);
    FPRINTF(fp,";\n");
    break;
  case REAL_INST:
  case BOOLEAN_INST:
  case INTEGER_INST:
  case SET_INST:
    break;
  case DUMMY_INST:
    FPRINTF(fp,"UNSELECTED;\n");
    break;
  default:
    ASC_PANIC("Unknown instance kind in Save__ComplexInsts.\n");
    break;
  }
}

static
void SaveComplexInsts(FILE *fp, struct gl_list_t *list)
{
  unsigned long len, c;
  struct Instance *inst;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    FPRINTF(fp,"$COMPLEX_INST %lu {\n",c);
    inst = (struct Instance *)gl_fetch(list,c);
    Save__ComplexInsts(fp,inst);
    FPRINTF(fp,"}\n\n");
  }
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void SaveRelations(FILE *fp, struct gl_list_t *list)
{
  unsigned long len, c;
  struct Instance *relinst;
  enum Expr_enum type;


  len = gl_length(list);

  for (c=1;c<=len;c++) {
    relinst = (struct Instance *)gl_fetch(list,c);
    if (InstanceKind(relinst)!=REL_INST)
      continue;
    type = GetInstanceRelationType(relinst);
    switch (type) {
    case e_token:
      SaveTokenRelation(fp,relinst);
      break;
    case e_opcode:
    case e_blackbox:
      FPRINTF(ASCERR,"Saving blackbox relations not supported\n");
      break;
    case e_glassbox:
    default:
      SaveGlassBoxRelation(fp,relinst);
      break;
    }
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


static
void SaveLogRelations(FILE *fp, struct gl_list_t *list)
{
  unsigned long len, c;
  struct Instance *lrelinst;

  len = gl_length(list);

  for (c=1;c<=len;c++) {
    lrelinst = (struct Instance *)gl_fetch(list,c);
    if (InstanceKind(lrelinst)!=LREL_INST)
      continue;
    SaveLogRel(fp,lrelinst);
  }
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void Save__Atoms(FILE *fp, struct Instance *inst)
{
  CONST struct Instance *child;
  enum inst_t kind;
  unsigned long len,c,index;

  kind = InstanceKind(inst);
  switch (kind) {
  default:
    return;
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case REL_INST:
  case LREL_INST:
  case DUMMY_INST:
    break;
  }
  index = GetTmpNum(inst);
  FPRINTF(fp,"$ATOM_INST %lu {\n",index);
  FPRINTF(fp,"\t$VALUE : ");
  if (kind!=(REL_INST || LREL_INST)) {
    WriteAtomValue(fp,inst);
  } else {
    FPRINTF(fp,"$UNDEFINED");
  }
  FPRINTF(fp,";\n");

  len = NumberChildren(inst);
  if (len) {
    for (c=1;c<=len;c++) {
      child = InstanceChild(inst,c);
      FPRINTF(fp,"\t%s ",InstanceType(child));
      WriteAtomValue(fp,child);
      FPRINTF(fp,";\n");
    }
  }
  FPRINTF(fp,"}\n\n");
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void SaveAtoms(FILE *fp, struct gl_list_t *list)
{
  struct Instance *inst;
  unsigned long len,c;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(list,c);
    Save__Atoms(fp,inst);
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


static
void Save__Link(FILE *fp,CONST struct Instance *inst)
{
  CONST struct Instance *child;
  unsigned long int nch,c,tindex;

  nch = NumberChildren(inst);
  if (!nch) return;

  tindex = GetTmpNum(inst);
  FPRINTF(fp,"$GRAPH %lu {\n",tindex);
  FPRINTF(fp,"\t%lu -> ",tindex);		/* parent tindex */
  child = InstanceChild(inst,1);		/* treat the first child */
  FPRINTF(fp,"%lu",GetTmpNum(child));		/* specially. */
  for (c=2;c<=nch;c++) {
    child = InstanceChild(inst,c);		/* not reached if nch = 1 */
    FPRINTF(fp,",%lu",GetTmpNum(child));
  }
  FPRINTF(fp,";\n");
  FPRINTF(fp,"}\n\n");
}


static
void SaveLinks(FILE *fp, struct gl_list_t *list)
{
  CONST struct Instance *inst;
  unsigned long len,c;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    inst = (CONST struct Instance *)gl_fetch(list,c);
    switch (InstanceKind(inst)) {
    case SIM_INST:
    case MODEL_INST:
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      Save__Link(fp,inst);
      break;
    default:
      break;
    }
  }
}

/*
	These functions save the connectivity graph as 1 huge
	node of all connections. It is useful for doing graph
	algorithms. For saving instances though it is perhaps
	better to use SaveLinks.
*/
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void Save__Graph(FILE *fp, struct gl_list_t *list)
{
  CONST struct Instance *inst;
  CONST struct Instance *child;
  unsigned long len,c,cc,nch,index;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    inst = (CONST struct Instance *)gl_fetch(list,c);
    switch (InstanceKind(inst)) {
    case SIM_INST:
    case MODEL_INST:
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      nch = NumberChildren(inst);
      if (!nch) break;
      index = GetTmpNum(inst);
      FPRINTF(fp,"\t%lu -> ",index);		/* parent index */
      child = InstanceChild(inst,1);		/* treat the first child */
      FPRINTF(fp,"%lu",GetTmpNum(child));	/* specially. */
      for (cc=2;cc<=nch;cc++) {
	child = InstanceChild(inst,cc);		/* not reached if nch = 1 */
	FPRINTF(fp,",%lu",GetTmpNum(child));
      }
      FPRINTF(fp,";\n");
      break;
    default:
      break;
    }
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void SaveGraph(FILE *fp, struct gl_list_t *list)
{
  FPRINTF(fp,"$GRAPH {\n");
  Save__Graph(fp,list);
  FPRINTF(fp,"}\n\n");
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


static
void SaveCliques(FILE *fp, struct gl_list_t *list)
{
  CONST struct Instance *inst;
  CONST struct Instance *ptr;
  unsigned long len,c,start,tindex;

  len = gl_length(list);

  for (c=1;c<=len;c++) {
    inst = (CONST struct Instance *)gl_fetch(list,c);
    ptr = inst;
    if (NextCliqueMember(ptr)!=ptr) {
      start = GetTmpNum(ptr);		/* head of clique */
      FPRINTF(fp,"$CLIQUE %lu {\n",start);
      FPRINTF(fp,"\t%lu -> ",start);
      do {
	ptr = NextCliqueMember(ptr);
	tindex = GetTmpNum(ptr);
	if (tindex)
	  FPRINTF(fp,"%lu ",tindex);	/* white space delimited */
      } while (ptr!=inst);
      FPRINTF(fp,";\n");
      FPRINTF(fp,"}\n\n");
    }
  }
}

void SaveInstance(FILE *fp, CONST struct Instance *i,
		  int dorelations)
{
  struct gl_list_t *list;
  struct gl_list_t *arraytypelist;
  struct Table *table = NULL;
  unsigned long len;

  (void)dorelations;  /*  stop gcc whine about unused parameter */

  if (!i) {
    return;
  }

  list = gl_create(1000L);
  arraytypelist = gl_create(50L);
  VisitInstanceTreeTwo((struct Instance *)i,(VisitTwoProc)CollectNodes,
		       1,0,(void *)list);
  len = gl_length(list);

  if (!len) {
    goto error;
  }
  table = CreateTable((unsigned long)TYPE_HASH_SIZE);
  CollectTypes(table,list,arraytypelist);
  WriteIntegrityCheck(fp,len);
  SaveTypes(fp,table);
  SaveArrayTypes(fp,arraytypelist);
  SaveComplexInsts(fp,list);
  /* SaveAtoms(fp,list); FIX FIX FIX */
  SaveLogRelations(fp,list);
  SaveLinks(fp,list);
  SaveCliques(fp,list);

 error:
  gl_destroy(list);
  gl_destroy(arraytypelist);
  DestroyTable(table,0);
}

/**
	interface pointer bulk transport to a stack functions
*/
struct pipdata {
  IPFunc makeip;
  struct gl_list_t *old;
  void *userdata;
  VOIDPTR vp;
};

#define PDC(x) ((struct pipdata *)(x))
static void CollectIpData(struct Instance *i, void *d)
{
  if (i!= NULL) {
    PDC(d)->userdata = PDC(d)->makeip(i,PDC(d)->vp);
    if (PDC(d)->userdata!=NULL) {
      gl_append_ptr(PDC(d)->old,(VOIDPTR)i);
      gl_append_ptr(PDC(d)->old,(VOIDPTR)GetInterfacePtr(i));
      SetInterfacePtr(i,PDC(d)->userdata);
    }
  }
}

struct gl_list_t *PushInterfacePtrs(struct Instance *i, IPFunc makeip,
                                    unsigned long iest,int visit,VOIDPTR vp)
{
  struct pipdata pip;
  /* use iest to get an initial list capacity so we don't go
   * into list expansion fits with the allocator.
   */
  if (iest <10) iest = 10;
  pip.old = gl_create(2*iest);
  if (pip.old == NULL) {
    FPRINTF(ASCERR,"Error PushInterfacePtrs out of memory.\n");
    return NULL;
  }
  pip.makeip = makeip;
  pip.vp = vp;
  if (pip.makeip == NULL) {
    FPRINTF(ASCERR,"Error in PushInterfacePtrs call.\n");
    return NULL;
  }
  /* do the stuff */
  SilentVisitInstanceTreeTwo(i,CollectIpData,visit,0,(VOIDPTR)&pip);
  return pip.old;
}

/* The list old contains in odd entries the instance pointers,
 * and in the succeeding even entries, the old interface pointer
 * for that preceeding odd entry.
 */

void PopInterfacePtrs(struct gl_list_t *old, IPDeleteFunc destroy, VOIDPTR vp)
{
  unsigned long c,len;
  struct Instance *i;
  if (old==NULL) return; /* should whine here */
  len = gl_length(old);
  if (destroy != NULL) {
    for (c=1; c<=len; c+=2) {
      i = (struct Instance *)gl_fetch(old,c);
      destroy(i, GetInterfacePtr(i),vp);
      SetInterfacePtr(i,gl_fetch(old,c+1));
    }
  } else {
    for (c=1; c<=len; c+=2) {
      SetInterfacePtr((struct Instance *)gl_fetch(old,c),gl_fetch(old,c+1));
    }
  }
  gl_destroy(old);
}

/**
	@TODO is this file the right place for ArrayIsRelation, ArrayIsLogRel, etc?
*/

/**
	Makes the assumption that the instance sent is not null
	and that array children for relations are all of the same
	type so that I can look at the first child only. Added to code
	to take care of empty sets -- resulting in 0 children.
*/
int ArrayIsRelation(struct Instance *i)
{
  if (i==NULL) return 0;
  /* skip past all the indirection */
  while( (InstanceKind(i)==ARRAY_INT_INST) ||
         (InstanceKind(i)==ARRAY_ENUM_INST) ) {
    if (NumberChildren(i)==0) break;
    i = InstanceChild(i,1L);
  }
  if (InstanceKind(i)==REL_INST) return 1; else return 0;
}


int ArrayIsLogRel(struct Instance *i)
{
  if (i==NULL) return 0;
  /* skip past all the indirection */
  while( (InstanceKind(i)==ARRAY_INT_INST) ||
         (InstanceKind(i)==ARRAY_ENUM_INST) ) {
    if (NumberChildren(i)==0) break;
    i = InstanceChild(i,1L);
  }
  if (InstanceKind(i)==LREL_INST) return 1; else return 0;
}

int ArrayIsWhen(struct Instance *i)
{
  if (i==NULL) return 0;
  /* skip past all the indirection */
  while( (InstanceKind(i)==ARRAY_INT_INST) ||
         (InstanceKind(i)==ARRAY_ENUM_INST) ) {
    if (NumberChildren(i)==0) break;
    i = InstanceChild(i,1L);
  }
  if (InstanceKind(i)==WHEN_INST) return 1; else return 0;
}

int ArrayIsModel(struct Instance *i)
{
  if (i==NULL) return 0;
  while( (InstanceKind(i)==ARRAY_INT_INST) ||
         (InstanceKind(i)==ARRAY_ENUM_INST) ) {
    if (NumberChildren(i)==0) break;
    i = InstanceChild(i,1L);
  }
  if (InstanceKind(i)==MODEL_INST) return 1; else return 0;
}

/* vim: set ts=8 : */
