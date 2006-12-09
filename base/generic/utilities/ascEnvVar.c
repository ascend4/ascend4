/*
 *  Ascend Environment Variable Imitation
 *  by Ben Allan
 *  Created: 6/3/97
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: ascEnvVar.c,v $
 *  Date last modified: $Date: 1997/07/18 12:04:07 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
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
 *  This file exists because win32, among others, can't keep their
 *  POSIX compliance up. In particular, getting and setting
 *  environment vars is exceedingly unreliable.
 *  This file implements a general way to store and fetch multiple
 *  paths.
 */

#include <ctype.h>
#include "ascConfig.h"
#include "ascPanic.h"
#include "ascMalloc.h"
#include "ascEnvVar.h"
#include <general/list.h>

#ifndef lint
static CONST char ascEnvVarid[] = "$Id: ascEnvVar.c,v 1.5 1997/07/18 12:04:07 mthomas Exp $";
#endif /* RCS ID keyword */


#ifdef __WIN32__
#define SLASH '\\'
#define PATHDIV ';'
#else /* ! __WIN32__ */
#define SLASH '/'
#define PATHDIV ':'
#endif

/*
 * structure we have for each envvar.
 * each contains a list with pointers to strings.
 */
struct asc_env_t {
  const char *name;
  struct gl_list_t *data;
};

/*
 * This list is a list of pointer to struct asc_env_t's
 * that constitute the environment.
 */

static
struct gl_list_t *g_env_list = NULL;

/*
 * ev = CreateEnvVar(envvar_name_we_own);
 * Returns NULL, or the asc_env_t requested.
 * The requested has an empty gl_list.
 * The name sent is the user's responsibility if we
 * return NULL, OTHERWISE it is henceforth ours.
 * We do not append the requested to the global list; that is the
 * caller's job.
 */
static
struct asc_env_t *CreateEnvVar(char *keepname)
{
  struct asc_env_t *ev;
  ev = ASC_NEW(struct asc_env_t);
  if (ev == NULL) {
    return NULL;
  }
  ev->data = gl_create(4);
  ev->name = keepname;
  return ev;
}

/*
 * clear out a env variable data blob.
 */
static
void DestroyEnvVar(struct asc_env_t *ev)
{
  if (ev==NULL) {
    return;
  }
  ascfree((char *)ev->name);
  gl_free_and_destroy(ev->data);
  ascfree(ev);
}

/*
 * compare two env variable data blobs by name.
 */

CmpFunc CmpEv;

static 
int CmpEV(struct asc_env_t *ev1, struct asc_env_t *ev2)
{
  if (ev1==ev2) {
    return 0;
  }
  if (ev1==NULL || ev1->name == NULL) {
    return 1;
  }
  if (ev2==NULL || ev2->name == NULL) {
    return -1;
  }
  return strcmp(ev1->name,ev2->name);
}

/*
 * returns the envvar with the name specified.
 */
static
struct asc_env_t *FindEnvVar(const char *name)
{
  unsigned long pos;
  struct asc_env_t dummy;

  dummy.name = name;
  pos = gl_search(g_env_list,&dummy,(CmpFunc)CmpEV);
  if (pos==0) {
#if 0
	FPRINTF(ASCERR,"ENV VAR '%s' NOT FOUND\n",name);
#endif
    return NULL;
  }
  return (struct asc_env_t *)gl_fetch(g_env_list,pos);
}

/*
 * add a var to the global sorted list.
 */
static
void AppendEnvVar(struct gl_list_t *env, struct asc_env_t *ev)
{
  gl_insert_sorted(env,ev,(CmpFunc)CmpEV);
}

/*
 * removes and destroys a var in the environment, if it exists.
 */
static
void DeleteEnvVar(char *name)
{
  unsigned long pos;
  struct asc_env_t *ev;
  struct asc_env_t dummy;

  dummy.name = name;

  if (name == NULL || g_env_list == NULL) {
    return;
  }
  pos = gl_search(g_env_list,&dummy,(CmpFunc)CmpEV);
  ev = (struct asc_env_t *)gl_fetch(g_env_list,pos);
  gl_delete(g_env_list,pos,0);
  DestroyEnvVar(ev);
}

int Asc_InitEnvironment(unsigned long len)
{
  if (g_env_list != NULL) {
    return 1;
  }
  g_env_list = gl_create(len);
  if (g_env_list == NULL) {
    return 1;
  }
  return 0;
}


void Asc_DestroyEnvironment(void)
{
  if (g_env_list == NULL) {
    return;
  }
  gl_iterate(g_env_list,(void (*)(VOIDPTR))DestroyEnvVar);
  gl_destroy(g_env_list);
  g_env_list = NULL;
}


int Asc_SetPathList(CONST char *envvar, CONST char *pathstring)
{
  char g_path_var[MAX_ENV_VAR_LENGTH];
  unsigned int c, length, spcseen=0;
  struct asc_env_t *ev;
  char *keepname;
  CONST char *path;
  char *putenvstring;

  if ((g_env_list == NULL) ||
      (envvar == NULL) ||
      (strlen(envvar) == 0) ||
      (pathstring == NULL) ||
      (strlen(pathstring) == 0) ||
      (strlen(envvar) >= MAX_ENV_VAR_LENGTH)) {
    return 1;
  }
  /*
   * transform envvar into a string w/out lead/trail blanks and copy.
   */
  putenvstring = g_path_var;
  snprintf(putenvstring, MAX_ENV_VAR_LENGTH, "%s", envvar);
  /* trim leading whitespace */
  while (isspace(putenvstring[0])) {
    putenvstring++;
  }
  for (c = 0; putenvstring[c] !='\0'; c++) {
    if (isspace(putenvstring[c])) {
      spcseen++;
    }
  }
  /* backup to before last trailing space */
  while (isspace(putenvstring[c-1])) {
    c--;
    spcseen--;
  }
  /* check for no spaces in keepname */
  if (spcseen) {
    return 1;
  }
  keepname = ASC_NEW_ARRAY(char,c+1);
  if (keepname == NULL) {
    return 1;
  }
  strncpy(keepname, putenvstring, c);
  keepname[c] = '\0';
  /* delete the old variable if it was already assigned */
  ev = FindEnvVar(keepname);
  if (ev!=NULL) {
    DeleteEnvVar(keepname);
  }
  ev = CreateEnvVar(keepname);
  if (ev == NULL) {
    ascfree(keepname);
    return 1;
  }
  AppendEnvVar(g_env_list, ev);

  /*
   * copy/split the pathstring
   */
  path = pathstring;

  /* strip any leading whitespace */
  while( isspace( *path ) ) {
    path++;
  }
  while( *path != '\0' ) {
    length = 0;
    /* copy the directory from path to the g_path_var */
    while(( *path != PATHDIV ) &&
          ( *path != '\0' ) &&
          ( length < (MAX_ENV_VAR_LENGTH - 1) )) {
      g_path_var[length++] = *(path++);
    }
    /* if we didn't run out of room, strip trailing whitespace */
    if (( length > 0) && ( length < (MAX_ENV_VAR_LENGTH - 1) )) {
      while (isspace(g_path_var[length-1])) {
        length--;
      }
    }
    /* otherwise advance to the next substring in path */
    else {
      while(( *path != PATHDIV ) && ( *path != '\0' )) {
        path++;
      }
    }
    /* append the value if not empty */
    if ( length > 0 ) {
      g_path_var[length++] = '\0';
      if ( Asc_AppendPath(keepname,g_path_var) != 0 ) {
        return 1;
      }
    }
    /* advance path past any whitespace & delimiters */
    while( isspace(*path) || ( *path == PATHDIV ) ) path++;
  }
  return 0;
}


int Asc_PutEnv(CONST char *envstring)
{
  char g_path_var[MAX_ENV_VAR_LENGTH];
  unsigned int c, length, spcseen=0, rhs;
  struct asc_env_t *ev;
  char *keepname, *path, *putenvstring;

  if ((g_env_list == NULL) ||
      (envstring == NULL) ||
      (strlen(envstring) == 0) ||
      (strlen(envstring) >= MAX_ENV_VAR_LENGTH)) {
    return 1;
  }

  putenvstring = g_path_var;
  snprintf(putenvstring, MAX_ENV_VAR_LENGTH, "%s", envstring);
  /* trim leading whitespace */
  while (isspace(putenvstring[0])) {
    putenvstring++;
  }
  /* locate '=' or EOS, counting whitespace along the way */
  for (c = 0; putenvstring[c] !='\0' && putenvstring[c] != '='; c++) {
    if (isspace(putenvstring[c])) {
      spcseen++;
    }
  }
  /* check for empty rhs */
  if (putenvstring[c] == '\0') {
    return 1;
  }
  rhs = c;
  /* backup space before = */
  while (isspace(putenvstring[c-1])) {
    c--;
    spcseen--;
  }
  /* check for no spaces in keepname */
  if (spcseen) {
    return 1;
  }
  keepname = ASC_NEW_ARRAY(char,c+1);
  if (keepname == NULL) {
    return 1;
  }
  strncpy(keepname,putenvstring,c);
  keepname[c] = '\0';
  /* delete the old variable if it was already assigned */
  ev = FindEnvVar(keepname);
  if (ev!=NULL) {
    DeleteEnvVar(keepname);
  }
  ev = CreateEnvVar(keepname);
  if (ev == NULL) {
    ascfree(keepname);
    return 1;
  }

  AppendEnvVar(g_env_list,ev);
  path = putenvstring + rhs + 1; /* got past the '=' */

  while( isspace( *path ) ) {
    path++;
  }
  while( *path != '\0' ) {
    length = 0;
    /* copy the directory from path to the g_path_var */
    while(( *path != PATHDIV ) && ( *path != '\0' )) {
      g_path_var[length++] = *(path++);
    }
    while (( length > 0 ) && isspace(g_path_var[length-1])) {
      length--;
    }
    if ( length > 0) {
      g_path_var[length++] = '\0';
      if (Asc_AppendPath(keepname,g_path_var)!=0) {
        return 1;
      }
    }
    while( isspace(*path) || ( *path == PATHDIV ) ) path++;
  }
  return 0;
}


int Asc_ImportPathList(CONST char *envvar)
{
  char *rhs;

  if (( g_env_list == NULL ) ||
      ( envvar == NULL ) ||
      ( strlen(envvar) == 0 )) {
    return 1;
  }
  rhs = getenv(envvar);
  if (( rhs == NULL ) || ( strlen(rhs) == 0 )) {
    return 1;
  }
  return Asc_SetPathList(envvar, rhs);
}


int Asc_AppendPath(char *envvar, char *newelement)
{
  struct asc_env_t *ev;
  char *keepname, *keepval;

  if ((g_env_list == NULL) ||
      (envvar == NULL) ||
      (newelement == NULL) ||
      (strlen(envvar) == 0) ||
      (strlen(newelement) == 0)) {
    return 1;
  }
  ev = FindEnvVar(envvar);
  if (ev == NULL) {
    keepname = ascstrdup(envvar);
    if (keepname == NULL) {
      return 1;
    }
    ev = CreateEnvVar(keepname);
    if (ev == NULL) {
      ascfree(keepname);
      return 1;
    }
    AppendEnvVar(g_env_list, ev);
  }
  keepval = ascstrdup(newelement);

  if (keepval == NULL) {
    return 1;
  }
  gl_append_ptr(ev->data, keepval);
  return 0;
}


CONST char **Asc_GetPathList(CONST char *envvar, int *argc)
{
  struct asc_env_t *ev;
  CONST char **argv;
  char *tmppos, *val;
  unsigned long len, c, slen;

  if ( argc == NULL ) {
    return NULL;
  }
  if (( g_env_list == NULL ) ||
      ( envvar == NULL )) {
  	FPRINTF(ASCERR,"G_ENV_LIST IS NULL LOOKING FOR %s\n",envvar);
    *argc = -1;
    return NULL;
  }
  if ( strlen(envvar) == 0 ) {
    *argc = 0;
    return NULL;
  }
  ev = FindEnvVar(envvar);
  if (ev==NULL ) {
  	FPRINTF(ASCERR,"UNABLE TO FINDENVVAR %s\n",envvar);

    *argc = 0;
    return NULL;
  }
  len = gl_length(ev->data);
  slen = (len+1)*sizeof(char *); /* space for argv pointers */
  for (c = 1; c <= len; c++) {
    /* space for the values */
    slen += (strlen((char *)gl_fetch(ev->data,(unsigned long)c)) +1 );
  }
  argv = ASC_NEW_ARRAY(CONST char *,slen);
  if ( argv == NULL ) {
    *argc = -1;
    return NULL;
  }
  tmppos = (char *)argv;
  tmppos += (len+1)*sizeof(char *);
  for (c = 1 ; c <= len ; c++) {
    val = (char *)gl_fetch(ev->data,(unsigned long)c);
    argv[c-1] = tmppos;
    while ( *val != '\0' ) {
      *tmppos++ = *val++;
    }
    *tmppos++ = *val;   /* include trailing '\0' */
  }
  argv[len] = NULL;
  *argc = (int)len;
  return argv;
}


char *Asc_GetEnv(const char *envvar)
{
  struct asc_env_t *ev;
  char *result, *val, *tmppos;
  unsigned long slen, c, llen;

  if ((g_env_list == NULL) ||
      (envvar == NULL) ||
      (strlen(envvar) == 0)) {
    return NULL;
  }
  ev = FindEnvVar(envvar);
  if (ev==NULL ) {
    return NULL;
  }
  slen = 0;
  llen = gl_length(ev->data);
  for (c = 1; c <= llen; c++) {
    slen += (strlen((char *)gl_fetch(ev->data,(unsigned long)c)) +1 );
  }
  result = ASC_NEW_ARRAY(char,slen+1);
  if (result == NULL) {
    return NULL;
  }
  tmppos = result;
  for  (c = 1; c <= llen; c++) {
    val = (char *)gl_fetch(ev->data,(unsigned long)c);
    while (*val != '\0') {
      *tmppos++ = *val++;
    }
    *tmppos++ = PATHDIV; /* include delimiter */
  }
  result[slen-1] = '\0'; /* overwrite final trailing delimiter */
  return result;
}


const char **Asc_EnvNames(int *argc)
{
  const char **argv;
  unsigned long c, len;

  if (g_env_list == NULL) {
    *argc = -1;
    return NULL;
  }
  len = gl_length(g_env_list);
  *argc =  (int)len;
  argv = ASC_NEW_ARRAY(CONST char *,*argc + 1);
  if (argv==NULL) {
    *argc = -1;
    return NULL;
  }
  for (c = 1; c <= len; c++) {
    argv[c-1] = ((struct asc_env_t *)gl_fetch(g_env_list,c))->name;
  }
  argv[len] = NULL;
  return argv;
}
