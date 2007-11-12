/*
 *  UserData.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.19 $
 *  Version control file: $RCSfile: UserData.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:09 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#define ASC_BUILDING_INTERFACE
#include <stdarg.h>
#include <tcl.h>
#include "config.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/hashpjw.h>

#include <compiler/instance_enum.h>


#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/visitinst.h>
#include <compiler/units.h>
#include "HelpProc.h"
#include "UserData.h"
#include "Qlfdid.h"
#include "UnitsProc.h"
#include "BrowserProc.h"

#ifndef lint
static CONST char UserDataID[] = "$Id: UserData.c,v 1.19 2003/08/23 18:43:09 ballan Exp $";
#endif


#define USERDATA_HASHSIZE 31

enum UserInfo_enum {
  real_info, probe_info, inst_info, list_info,
  error_info
};

char *UserInfo_strings[] = {
  "real_info", "probe_info", "inst_info", "list_info",
  "error_info" /* must be last */
};

struct Real_Info {
  struct Instance *i;
  double value;
};

struct UserData {
  char *id;
  enum UserInfo_enum t;
  union {
    struct Instance *i;
    struct gl_list_t *list; /* store anything you want here
                             * just set the type appropriately */
  } u;
};

struct UserDataEntry {
  struct UserData *data;
  struct UserDataEntry *next;
};


/*
 *  global variables.
 */
static struct UserDataEntry *UserDataLibrary[USERDATA_HASHSIZE];
static int UserDataLibraryInited = 0;

void Asc_UserDataLibraryInitialize(void)
{
  unsigned c;
  if (!UserDataLibraryInited) {
    for ( c = 0 ; c < USERDATA_HASHSIZE ; UserDataLibrary[c++] = NULL );
    UserDataLibraryInited++;
  }
}

static
struct UserData *UserDataCreate(char *id, enum UserInfo_enum t)
{
  struct UserData *result;
  result = (struct UserData *)ascmalloc(sizeof(struct UserData));
  assert(result);
  result->t = t;
  result->id = id; /* the node owns the string */
  switch(t) {
  case real_info:
  case probe_info:
  case list_info:
    result->u.list = NULL;
    break;
  case inst_info:
    result->u.i = NULL;
    break;
  default:
    Asc_Panic(2, "UserDataCreate",
              "Unknown information type in UserDataCreate\n");
  }
  return result;
}

/*
 * Note : For the case of list_info, we can not tell what
 * information was stored in the library. It is the users
 * responsibilty to deallocate that memory, as only the list
 * structure is destroyed.
 */

static
void DestroyUserData(struct UserData *user_data)
{
  if (!user_data) {
    return;
  }
  switch(user_data->t) {
  case real_info:
    if (user_data->u.list) {
      gl_free_and_destroy(user_data->u.list);
    }
    break;
  case probe_info:
    break;
  case inst_info:
    user_data->u.i = NULL;
    break;
  case list_info:
    if (user_data->u.list) {
      gl_destroy(user_data->u.list); /* see NOTE above */
    }
    break;
  default:
    break;
  }
}

static char *GenerateId(char *prefix)
{
  static int counter = 0;
  char *id;
  /* The caller owns the string.
   * The extra bytes are for the numeric index
   */
  id = (char *)ascmalloc((strlen(prefix)+1+20) * sizeof(char));
  sprintf(id,"%s%d",prefix,counter++);
  return id;
}

static
char *UserDataId(struct UserData *user_data)
{
  assert(user_data);
  return user_data->id;
}

static
enum UserInfo_enum UserDataType(struct UserData *user_data)
{
  assert(user_data);
  return user_data->t;
}


static
void AddUserData(struct UserData *user_data)
{
  unsigned long bucket;
  struct UserDataEntry *ptr;
  CONST char *id;

  assert(user_data);
  id = UserDataId(user_data);
  bucket = hashpjw(id,USERDATA_HASHSIZE);
  ptr = UserDataLibrary[bucket];
  /* search for name collisions */
  while (ptr) {
    if (strcmp(id,UserDataId(ptr->data))==0) {
      return;
    }
    ptr = ptr->next;
  }
  /* add new function to the head of the list. */
  ptr = (struct UserDataEntry *)
           ascmalloc(sizeof(struct UserDataEntry));
  ptr->next = UserDataLibrary[bucket];
  ptr->data = user_data;
  UserDataLibrary[bucket] = ptr;
}

static
struct UserData *LookupUserData(char *id)
{
  unsigned long bucket;
  struct UserDataEntry *ptr;
  char *name;
  if (!id) {
    return NULL;
  }
  bucket = hashpjw(id,USERDATA_HASHSIZE);
  ptr = UserDataLibrary[bucket];
  while (ptr) {
    name = UserDataId(ptr->data);
    if (strcmp(name,id)==0) {
      return ptr->data;
    }
    ptr = ptr->next;
  }
  return NULL; /* name not found */
}

static
struct UserData *RemoveUserData(char *id)
{
  unsigned long bucket;
  struct UserDataEntry *ptr, **tmp;
  struct UserData *result;
  char *name;
  if (!id) {
    return NULL;
  }
  bucket = hashpjw(id,USERDATA_HASHSIZE);
  tmp = &UserDataLibrary[bucket];
  ptr = UserDataLibrary[bucket];
  while (ptr) {
    name = UserDataId(ptr->data);
    if (strcmp(name,id)==0) {
      *tmp = ptr->next;
      result = ptr->data;
      ascfree((char *)ptr); /* deallocate the node */
      return result;
    }
    tmp = &ptr->next;
    ptr = ptr->next;
  }
  return NULL; /* node info not found */
}

static
void DestroyUserDataLibrary(void)
{
  unsigned c;
  struct UserDataEntry *ptr, *next;
  for(c=0;c<USERDATA_HASHSIZE;c++) {
    if (UserDataLibrary[c]!=NULL) {
      ptr = UserDataLibrary[c];
      while(ptr!=NULL) {                   /* destroy the chain */
        DestroyUserData(ptr->data);
        next = ptr->next;
        ascfree((char *)ptr);
        ptr = next;
      }
      UserDataLibrary[c] = NULL;
    }
  }
}

static
enum UserInfo_enum MapStringToEnum(char *string)
{
  if (!string) {
    return error_info;
  }
  if (strncmp(string,"real_info",4)==0) {
    return real_info;
  }
  if (strncmp(string,"probe_info",4)==0) {
    return probe_info;
  }
  if (strncmp(string,"inst_info",4)==0) {
    return inst_info;
  }
  if (strncmp(string,"list_info",4)==0) {
    return list_info;
  }
  return error_info;
}


int Asc_UserDataCreateCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct UserData *user_data;
  enum UserInfo_enum t;
  char *id;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage __userdata_create type",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  t = MapStringToEnum(QUIET(argv[1]));
  if (t==error_info) {
    Tcl_SetResult(interp, "Unknown user_data type given", TCL_STATIC);
    return TCL_ERROR;
  }
  id = GenerateId(QUIET(argv[1]));
  user_data = UserDataCreate(id,t);
  if (user_data) {
    AddUserData(user_data);
    Tcl_AppendResult(interp,id,(char *)NULL);
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "Serious error in creating user_data", TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UserDataDestroyCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct UserData *user_data;

  UNUSED_PARAMETER(cdata);

  if (( argc > 3 )||( argc < 2 )) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __userdata_destroy ?one?all? <id>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ((strcmp(argv[1],"one")==0)&&( argc == 3 )) {
    user_data = RemoveUserData(QUIET(argv[2]));
    if (user_data) {
      DestroyUserData(user_data);
    }
    return TCL_OK;
  } else if (strcmp(argv[1],"all")==0) {
    DestroyUserDataLibrary();
    UserDataLibraryInited = 0; /* set up to allow a reinitialization */
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "incorrect args to __userdata_destroy", TCL_STATIC);
    return TCL_ERROR;
  }
}

static
void UserData_UpdateRealInfo(struct UserData *user_data)
{
  struct Real_Info *ri;
  struct Instance *i;
  unsigned long len,c;

  assert(user_data);
  if ((user_data->u.list==NULL)||(user_data->t!=real_info)) {
    FPRINTF(stderr,"Major error in UserData_RestoreRealInfo\n");
    return;
  }
  len = gl_length(user_data->u.list);
  for (c=1;c<=len;c++) {
    ri = (struct Real_Info *)gl_fetch(user_data->u.list,c);
    i = ri->i;
    ri->value = RealAtomValue(i);
  }
}

static
void UserData_RestoreRealInfo(struct UserData *user_data)
{
  struct Real_Info *ri;
  struct Instance *i;
  unsigned long len,c;

  assert(user_data);
  if ((user_data->u.list==NULL)||(user_data->t!=real_info)) {
    FPRINTF(stderr,"Major error in UserData_RestoreRealInfo\n");
    return;
  }
  len = gl_length(user_data->u.list);
  for (c=1;c<=len;c++) {
    ri = (struct Real_Info *)gl_fetch(user_data->u.list,c);
    i = ri->i;
    SetRealAtomValue(i,ri->value,(unsigned)0);
  }
}

/*
 *  global variables.
 */
struct gl_list_t *g_tmp_userlist = NULL;

/*
 * at present (and for no good reason) this function is supposed to
 * record the values of real variables and real child of atoms values.
 * It needs to be generalized.
 * It probably shouldn't record values of constants ever, but it
 * should handle all types of vars.
 */
static
void UserDataSaveValuesFunc(struct Instance *i)
{
  struct Real_Info *ri;
  if (i) {
    switch(InstanceKind(i)) {
    case REAL_INST:
    case REAL_ATOM_INST:
      ri = (struct Real_Info *)ascmalloc(sizeof(struct Real_Info));
      ri->i = i;
      ri->value = (AtomAssigned(i)) ? RealAtomValue(i) : 0.0;
      gl_append_ptr(g_tmp_userlist,(char *)ri);
      break;
    case INTEGER_INST:
    case INTEGER_ATOM_INST:
      /* do something,break; */
    case SYMBOL_INST:
    case SYMBOL_ATOM_INST:
      /* do something,break; */
    case BOOLEAN_INST:
    case BOOLEAN_ATOM_INST:
      /* do something,break; */
    case REAL_CONSTANT_INST:
    case INTEGER_CONSTANT_INST:
    case SYMBOL_CONSTANT_INST:
    case BOOLEAN_CONSTANT_INST:
    case SET_ATOM_INST:
      /* do nothing,break; */
    case REL_INST:
    case LREL_INST:
    case MODEL_INST:
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
    case WHEN_INST:
    case SET_INST:
    case DUMMY_INST:
    case SIM_INST:
      /* do nothing,break; */
      break;
    default:
      FPRINTF(stderr, "invalid type in switch in UserDataSaveValuesFunc\n");
      break;
    }
  }
}

static
struct gl_list_t *UserDataSaveValues(struct Instance *i,
                                     struct gl_list_t *list)
{
  g_tmp_userlist = list;
  VisitInstanceTree(i,UserDataSaveValuesFunc,0,1);
  g_tmp_userlist = NULL;
  return list;
}

int Asc_UserDataSaveValuesCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  struct Instance *from;
  struct UserData *user_data, *tmp;
  struct gl_list_t *list;
  char *id;

  UNUSED_PARAMETER(cdata);

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "__userdata_save from to", TCL_STATIC);
    return TCL_ERROR;
  }
  if (strcmp(argv[1],"current")==0) {
    from = g_curinst;
  } else if (strcmp(argv[1],"search")==0) {
    from = g_search_inst;
  } else {
    user_data = LookupUserData(QUIET(argv[1]));
    if (!user_data) {
      Tcl_SetResult(interp, "Error with the reference instance", TCL_STATIC);
      return TCL_ERROR;
    }
    if (UserDataType(user_data)!=inst_info) {
      Tcl_SetResult(interp, "Error with the reference instance", TCL_STATIC);
      return TCL_ERROR;
    }
    from = user_data->u.i;
  }
  if (from==NULL) {
    Tcl_SetResult(interp, "reference instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }

  tmp = LookupUserData(QUIET(argv[2]));
  if (!tmp) { /* does not exist -- so build a real_info list */
    list = gl_create(1000L);
    list = UserDataSaveValues(from,list);
    id = Asc_MakeInitString(strlen(argv[2]));
    strcpy(id,argv[2]);
    tmp = UserDataCreate(id,real_info);	 /* create the node */
    tmp->u.list = list;			 /* set node information */
    AddUserData(tmp);			 /* add node to table */
    return TCL_OK;
  } else{ /* the list exists so just update the list information */
    if (UserDataType(tmp)!=real_info) {
      Tcl_SetResult(interp, "Incompatible types with Saving Values",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    UserData_UpdateRealInfo(tmp);
    return TCL_OK;
  }
}

int Asc_UserDataRestoreValuesCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[])
{
  struct UserData *user_data;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage __userdata_restore id",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  user_data = LookupUserData(QUIET(argv[1]));
  if (!user_data) {
    Tcl_SetResult(interp, "user_data requested does not exist", TCL_STATIC);
    return TCL_ERROR;
  }
  if ((user_data->u.list==NULL)||(user_data->t!=real_info)) {
    Tcl_SetResult(interp, "cannot restore non real_info", TCL_STATIC);
    return TCL_ERROR;
  }
  UserData_RestoreRealInfo(user_data);
  return TCL_OK;
}

int Asc_UserDataInitializeCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  /* usage is __userdata_initialize */
  UNUSED_PARAMETER(cdata);
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  Asc_UserDataLibraryInitialize();
  return TCL_OK;
}

static
void UserDataTypeFunc(Tcl_Interp *interp,struct UserData *user_data)
{
  switch(UserDataType(user_data)) {
  case real_info:
    Tcl_AppendResult(interp,"real_info",(char *)NULL);
    return;
  case probe_info:
    Tcl_AppendResult(interp,"probe_info",(char *)NULL);
    return;
  case inst_info:
    Tcl_AppendResult(interp,"inst_info",(char *)NULL);
    return;
  case list_info:
    Tcl_AppendResult(interp,"list_info",(char *)NULL);
    return;
  case error_info:
  default:
    Tcl_AppendResult(interp,"error_info",(char *)NULL);
    return;
  }
}

int Asc_UserDataQueryCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  /* some general query commands
     usage is __userdata_query ?type?exists? id */
  struct UserData *user_data;

  UNUSED_PARAMETER(cdata);

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "wrong # args : __userdata_query ?type?exists? id",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"type",4)==0) { /* type */
    user_data = LookupUserData(QUIET(argv[2]));
    if (!user_data)  {
      Tcl_AppendResult(interp,"error_info",(char *)NULL);
      return TCL_OK;
    } else {
     UserDataTypeFunc(interp,user_data);
     return TCL_OK;
   }
  } else if (strncmp(argv[1],"exists",4)==0) { /* exists */
    user_data = LookupUserData(QUIET(argv[2]));
    if (user_data) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "Invalid args to __userdata_query", TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_UserDataPrintLibrary(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  unsigned c;
  struct UserDataEntry *ptr;

  UNUSED_PARAMETER(cdata);
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  Tcl_AppendResult(interp,"{",(char *)NULL); /* start building the list */
  for(c=0;c<USERDATA_HASHSIZE;c++) {
    if (UserDataLibrary[c]!=NULL) {
      ptr = UserDataLibrary[c];
      while(ptr!=NULL) {
        Tcl_AppendResult(interp,"{ ",UserDataId(ptr->data)," ",(char *)NULL);
        UserDataTypeFunc(interp,ptr->data);
        Tcl_AppendResult(interp,"} ",(char *)NULL);
        ptr = ptr->next;
      }
    }
  }
  Tcl_AppendResult(interp,"}",(char *)NULL); /* end building the list */
  return TCL_OK;
}
