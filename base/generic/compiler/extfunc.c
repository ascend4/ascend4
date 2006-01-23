/*
 *  External Functions Module
 *  by Kirk Andre Abbott
 *  Created: July 4, 1994.
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: extfunc.c,v $
 *  Date last modified: $Date: 1998/02/05 22:23:26 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
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
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/hashpjw.h"
#include "general/list.h"
#include "general/table.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/extfunc.h"

#define EXTFUNCHASHSIZE 31

static struct Table *ExternalFuncLibrary = NULL;

#ifdef THIS_IS_AN_UNUSED_FUNCTION
struct ExternalFunc *CreateExternalFunc(CONST char *name)
{
  struct ExternalFunc *result;
  result = (struct ExternalFunc *)ascmalloc(sizeof(struct ExternalFunc));
  if (result == NULL) {
    return NULL;
  }
  result->name = name;
  result->n_inputs = 0;
  result->n_outputs = 0;
  result->help = NULL;
  result->value = NULL;
  result->deriv = NULL;
  result->deriv2 = NULL;
  return result;
}
#endif /*THIS_IS_AN_UNUSED_FUNCTION */


int CreateUserFunction(CONST char *name,
		       ExtEvalFunc *init,
		       ExtEvalFunc **value,
		       ExtEvalFunc **deriv,
		       ExtEvalFunc **deriv2,
		       CONST unsigned long n_inputs,
		       CONST unsigned long n_outputs,
		       CONST char *help)
{
  struct ExternalFunc *efunc;
  if (name == NULL) {
    return 1;
  }
  efunc = LookupExtFunc(name);
  if (efunc != NULL) {    /* name was pre-loaded -- just update the info */
    efunc->n_inputs = n_inputs;
    efunc->n_outputs = n_outputs;
    efunc->init = init;
    efunc->value = value;   /* should be *value */
    efunc->deriv = deriv;   /* should be *deriv */
    efunc->deriv2 = deriv2; /* should be *deriv2 */
    if (help) {
      if (efunc->help) ascfree((char *)efunc->help);
      efunc->help = (char *)ascmalloc((strlen(help)+1)*sizeof(char));
      asc_assert(efunc->help != NULL);
      strcpy(efunc->help,help);
    }
    else
      efunc->help = NULL;
  } else {
    efunc = (struct ExternalFunc *)ascmalloc(sizeof(struct ExternalFunc));
    asc_assert(efunc!=NULL);
    efunc->name = SCP(AddSymbol(name));	/* add or find name in symbol table */
                                        /* the main symtab owns the string */
    efunc->n_inputs = n_inputs;
    efunc->n_outputs = n_outputs;
    efunc->init = init;
    efunc->value = value;   /* should be *value */
    efunc->deriv = deriv;   /* should be *deriv */
    efunc->deriv2 = deriv2; /* should be *deriv2 */
    if (help) {
      efunc->help = (char *)ascmalloc((strlen(help)+1)*sizeof(char));
      asc_assert(efunc->help != NULL);
      strcpy(efunc->help,help);
    }
    else
      efunc->help = NULL;
    (void)AddExternalFunc(efunc,1);
  }
  return 0;
}

void DestroyExternalFunc(struct ExternalFunc *efunc)
{
  struct ExternalFunc *tmp;
  if (efunc) {
    tmp = efunc;
    tmp->name = NULL; 		/* the main symbol table owns the string */
    if (tmp->help) ascfree(tmp->help); /* we own the string */
    tmp->help = NULL;
    tmp->init = NULL;
    tmp->value = NULL;
    tmp->deriv = NULL;
    tmp->deriv2 = NULL;
    ascfree((char *)tmp);
    efunc = NULL;
  }
}

/*
 * This is simply a function returning a pointer to a function.
 * In this case the evaluation function. see Harbison&Steele p 258. :^)
 */

int (*GetInitFunc(struct ExternalFunc *efunc))(/* */)
{
  asc_assert(efunc!=NULL);
  return efunc->init;
}

ExtBBoxFunc *GetValueFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return (ExtBBoxFunc *)efunc->value;
}


ExtBBoxFunc *GetDerivFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return (ExtBBoxFunc *)efunc->deriv;
}

ExtBBoxFunc *GetDeriv2Func(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return (ExtBBoxFunc *)efunc->deriv2;
}

/*
 * GlassBox relations in particular register not just
 * a single function but rather a pointer to a jump table
 * of functions. There will be a jump table ptr for each
 * of value, deriv, deriv2.
 */

/*
 * The following means:
 * GetValue is a function that returning pointer to array[] of
 * pointer to functions, which take args and return an int.
 *
 * int (*(*GetValueJumpTable(struct ExternalFunc *efunc))[])(args)
 */

ExtEvalFunc **GetValueJumpTable(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->value;    /* error, efunc->value is not an array of pointers */
}

ExtEvalFunc **GetDerivJumpTable(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->deriv;    /* error, efunc->value is not an array of pointers */
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
ExtEvalFunc **GetValueDeriv2Table(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->deriv2;   /* error, efunc->value is not an array of pointers */
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


CONST char *ExternalFuncName(CONST struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->name;
}

unsigned long NumberInputArgs(CONST struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->n_inputs;
}

unsigned long NumberOutputArgs(CONST struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->n_outputs;
}

/*
 * These are the table management routines.
 */

void InitExternalFuncLibrary(void)
{
  struct Table *result;
  result = CreateTable(EXTFUNCHASHSIZE); /* this isn't destroyed at end. fix.*/
  ExternalFuncLibrary = result;
}

int AddExternalFunc(struct ExternalFunc *efunc, int force)
{

  struct ExternalFunc *found, *tmp;
  char *name;

  asc_assert(efunc!=NULL);
  name = (char *)efunc->name;
  found = (struct ExternalFunc *)LookupTableData(ExternalFuncLibrary,name);
  if (found) {		/* function name already exists */
    if (force==0)
      return 0;
    else{		/* need to update information */
      tmp = (struct ExternalFunc *)RemoveTableData(ExternalFuncLibrary,name);
      DestroyExternalFunc(tmp);
      AddTableData(ExternalFuncLibrary,(void *)efunc,name);
      return 1;
    }
  }
  else{			/* need to add function to library */
    AddTableData(ExternalFuncLibrary,(void *)efunc,name);
    return 1;
  }
}

struct ExternalFunc *LookupExtFunc(CONST char *funcname)
{
  struct ExternalFunc *found;
  if (!funcname) {
    return NULL;
  }
  found = (struct ExternalFunc *)
    LookupTableData(ExternalFuncLibrary,funcname);
  if (found) {
    return found;
  } else {
    return NULL; /* name not found */
  }
}

struct ExternalFunc *RemoveExternalFunc(char *funcname)
{
  struct ExternalFunc *found;
  if (!funcname)
    return NULL;
  found = (struct ExternalFunc *)
    RemoveTableData(ExternalFuncLibrary,funcname);
  return found;
}


static
void ExternalFuncDestroyFunc(void *efunc)
{
  struct ExternalFunc *local;
  local = (struct ExternalFunc *)efunc;
  if (local)
    DestroyExternalFunc(local);
}

void DestroyExtFuncLibrary(void)
{
  TableApplyAll(ExternalFuncLibrary,
                (TableIteratorOne)ExternalFuncDestroyFunc);
  DestroyTable(ExternalFuncLibrary,0);
  ExternalFuncLibrary = NULL;
}

static
void PrintExtFuncLibraryFunc(void *efunc, void *fp)
{
  struct ExternalFunc *local_efunc = (struct ExternalFunc *)efunc;

  if (local_efunc!=NULL) {
    FPRINTF(fp,"%s\n",ExternalFuncName(local_efunc));
    if (local_efunc->help) {
      FPRINTF(fp,"%s\n",local_efunc->help);
    } else {
      FPRINTF(fp,"No help information available for this function\n");
    }
  }
}

void PrintExtFuncLibrary(FILE *fp)
{
  if (!fp) {
    FPRINTF(ASCERR,"Invalid file handle in PrintExtFuncLibrary\n");
    return;
  }
  TableApplyAllTwo(ExternalFuncLibrary, PrintExtFuncLibraryFunc,
		   (void *)fp);
}

static
void WriteExtFuncString(struct ExternalFunc  *efunc, Asc_DString *dsPtr)
{
  if (efunc!=NULL) {
    Asc_DStringAppend(dsPtr,"{{",2);
    Asc_DStringAppend(dsPtr,ExternalFuncName(efunc),-1);
    Asc_DStringAppend(dsPtr,"} {",3);
    if (efunc->help!=NULL) {
      Asc_DStringAppend(dsPtr,efunc->help,-1);
    } else {
      Asc_DStringAppend(dsPtr,"No help available.",18);
    }
    Asc_DStringAppend(dsPtr,"}} ",3);
  }
}

char *WriteExtFuncLibraryString(void)
{
  char *result;
  Asc_DString ds, *dsPtr;
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  TableApplyAllTwo(ExternalFuncLibrary,(TableIteratorTwo)WriteExtFuncString,
		   (void *) dsPtr);
  result = Asc_DStringResult(dsPtr);
  return result;
}

void 
TraverseExtFuncLibrary(void (*func)(void *,void *), void *secondparam){
	TableApplyAllTwo(ExternalFuncLibrary, func, secondparam);
}
