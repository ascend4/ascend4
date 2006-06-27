/*
	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
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
	by Kirk Andre Abbott
	Created: July 4, 1994.
	Last in CVS: $Revision: 1.8 $ $Date: 1998/02/05 22:23:26 $ $Author: ballan $
*/

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/hashpjw.h>
#include <general/list.h>
#include <general/table.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "instance_enum.h"
#include "instance_io.h"
#include "extfunc.h"
#include "extcall.h"

/*------------------------------------------------------------------------------
  forward decls and typedefs etc
*/

#define EXTFUNCHASHSIZE 31

static struct Table *ExternalFuncLibrary = NULL;

/*-----------------------------------------------------------------------------
  BLACK BOX STUFF
*/

int CreateUserFunctionBlackBox(CONST char *name,
		       ExtBBoxInitFunc *init,
		       ExtBBoxFunc *value,
		       ExtBBoxFunc *deriv,
		       ExtBBoxFunc *deriv2,
		       ExtBBoxInitFunc *final,
		       CONST unsigned long n_inputs,
		       CONST unsigned long n_outputs,
		       CONST char *help)
{
  struct ExternalFunc *efunc;
  int isNew = 0;
  if (name == NULL) {
    return 1;
  }
  efunc = LookupExtFunc(name);
  if (efunc != NULL) {    /* name was pre-loaded -- just update the info */
    isNew = 0;
  } else {
    isNew = 1;
    efunc = ASC_NEW(struct ExternalFunc);
    asc_assert(efunc!=NULL);
    efunc->help = NULL;
    efunc->name = ascstrdup(SCP(AddSymbol(name)));
	/* add or find name in symbol table */
	/* the main symtab owns the string */
  }

  efunc->etype = efunc_BlackBox;
  efunc->n_inputs = n_inputs;
  efunc->n_outputs = n_outputs;
  efunc->u.black.initial = init;
  efunc->u.black.value = value;
  efunc->u.black.deriv = deriv;
  efunc->u.black.deriv2 = deriv2;
  efunc->u.black.final = final;
  if (help) {
    if (efunc->help) ascfree((char *)efunc->help);
    efunc->help = ascstrdup(help);
  } else {
    efunc->help = NULL;
  }

  if (isNew) {
    (void)AddExternalFunc(efunc,1);
  }
  return 0;
}

double blackbox_evaluate_residual(struct relation *r){
	struct ExtCallNode *ext;
	struct ExternalFunc *efunc;
	int i,n;
	char *tmp;
	struct Instance *inst;
	double *in;
	double *out;

	CONSOLE_DEBUG("...");

	n = NumberVariables(r);

	asc_assert(r!=NULL);
	asc_assert(r->share!=NULL);	
	asc_assert(r->share->bbox.ext!=NULL);
	ext = r->share->bbox.ext;
	efunc = ext->efunc;
	asc_assert(efunc!=NULL);
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"ABOUT TO EVALUATE %s",efunc->name);

	asc_assert(efunc->etype==efunc_BlackBox);

	ExtBBoxFunc *valfnptr = efunc->u.black.value;
	asc_assert(valfnptr!=NULL);

	CONSOLE_DEBUG("vars=%d, inputs=%d, outputs=%d",n
		,NumberInputArgs(efunc)
		,NumberOutputArgs(efunc)
	);

	/* list all the variables associated with this black box */
	for(i=1;i <= n; ++i){
		inst = RelationVariable(r,i);
		tmp = WriteInstanceNameString(inst, NULL);
		if(i<=NumberInputArgs(efunc)){
			CONSOLE_DEBUG("Input %d: '%s' (at %p)", i, tmp, inst);
		}else{
			CONSOLE_DEBUG("Output %d: '%s'", i-NumberInputArgs(efunc), tmp);
		}
		ASC_FREE(tmp);
	}

	/* allocate space for the evaluation inputs and outputs */
	in = ASC_NEW_ARRAY(double,NumberInputArgs(efunc));
	out = ASC_NEW_ARRAY_CLEAR(double,NumberOutputArgs(efunc));

	/* pull the current input values and place them in our 'in' array */
	for(i=0; i < NumberInputArgs(efunc); ++i){
		inst = RelationVariable(r,i+1);
		tmp = WriteInstanceNameString(inst,NULL);
		if(!AtomAssigned(inst)){
			CONSOLE_DEBUG("Var %d ('%s') has not been assigned yet!",i+1,tmp);
			in[i]=-1;
		}else{
			in[i] = RealAtomValue(inst);
		}
		CONSOLE_DEBUG("Set var %d ('%s') to '%f'",i+1,tmp,in[i]);
		ASC_FREE(tmp);
	}

	/* call the evaluation function */

	/* push the current output values back into the instance hierarchy */

	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Blackbox not implemented, returning -1");
	return -1;
}


ExtBBoxInitFunc * GetInitFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  /* return (ExtBBoxInitFunc*)efunc->u.black.init; */
  return efunc->u.black.initial;
}

ExtBBoxInitFunc * GetFinalFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  return efunc->u.black.final;
}

ExtBBoxFunc *GetValueFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_BlackBox);
  /* return (ExtBBoxFunc *)efunc->value; */
  return efunc->u.black.value;
}


ExtBBoxFunc *GetDerivFunc(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_BlackBox);
  return efunc->u.black.deriv;
}

ExtBBoxFunc *GetDeriv2Func(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_BlackBox);
  return efunc->u.black.deriv2;
}

/*------------------------------------------------------------------------------
  GLASS BOX STUFF
*/

int CreateUserFunctionGlassBox(CONST char *name,
		       ExtEvalFunc *init,
		       ExtEvalFunc **value,
		       ExtEvalFunc **deriv,
		       ExtEvalFunc **deriv2,
		       ExtEvalFunc *final,
		       CONST unsigned long n_inputs,
		       CONST unsigned long n_outputs,
		       CONST char *help)
{
  struct ExternalFunc *efunc;
  int isNew = 0;
  if (name == NULL) {
    return 1;
  }
  efunc = LookupExtFunc(name);
  if (efunc != NULL) {    /* name was pre-loaded -- just update the info */
    isNew = 0;
  } else {
    isNew = 1;
    efunc = ASC_NEW(struct ExternalFunc);
    asc_assert(efunc!=NULL);
    efunc->help = NULL;
    efunc->name = ascstrdup(SCP(AddSymbol(name)));
	/* add or find name in symbol table */
	/* the main symtab owns the string */
  }

  efunc->etype = efunc_GlassBox;
  efunc->n_inputs = n_inputs;
  efunc->n_outputs = n_outputs;
  efunc->u.glass.initial = init;
  efunc->u.glass.value = value;
  efunc->u.glass.deriv = deriv;
  efunc->u.glass.deriv2 = deriv2;
  efunc->u.glass.final = final;
  if (help) {
    if (efunc->help) ascfree((char *)efunc->help);
    efunc->help = ascstrdup(help);
  } else {
    efunc->help = NULL;
  }

  if (isNew) {
    (void)AddExternalFunc(efunc,1);
  }
  return 0;
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
  asc_assert(efunc->etype == efunc_GlassBox);
  return efunc->u.glass.value;
}

ExtEvalFunc **GetDerivJumpTable(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_GlassBox);
  return efunc->u.glass.deriv;
}

ExtEvalFunc **GetDeriv2JumpTable(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_GlassBox);
  return efunc->u.glass.deriv2;
}

/*------------------------------------------------------------------------------
  EXTERNAL METHOD STUFF
*/

int CreateUserFunctionMethod(CONST char *name,
	/* 	       ExtMethodInit *init, */
		       ExtMethodRun *run,
	/*	       ExtMethodInitEvalFunc *final, */
		       CONST long n_args,
	/*	       CONST unsigned long n_outputs, */
		       CONST char *help)
{
  struct ExternalFunc *efunc;
  int isNew = 1;
  if (name == NULL) {
    return 1;
  }
  efunc = LookupExtFunc(name);
  if (efunc != NULL) {
    isNew = 0;
   /* name was pre-loaded -- just update the info. This may cause user
      insanity if it wasn't a reload of the same thing. */
  } else {
    isNew = 1;
    efunc = ASC_NEW(struct ExternalFunc);
    asc_assert(efunc!=NULL);
    efunc->help = NULL;
    efunc->name = ascstrdup(SCP(AddSymbol(name)));
	/* add or find name in symbol table, and copy because  */
	/* the main symtab owns the string */
  }
  efunc->etype = efunc_Method;
  efunc->n_inputs = n_args;
  efunc->n_outputs = 0;
  efunc->u.method.run = run;
#if 0
  efunc->u.method.initial = init;
  efunc->u.method.final = final;
#endif
  if (help) {
    if (efunc->help) { ascfree((char *)efunc->help); }
    efunc->help = ascstrdup(help);
  } else {
    efunc->help = NULL;
  }

  if (isNew ) {
    (void)AddExternalFunc(efunc,1);
  }
  return 0;
}



ExtMethodRun *GetExtMethodRun(struct ExternalFunc *efunc)
{
  asc_assert(efunc!=NULL);
  asc_assert(efunc->etype == efunc_Method);
  return efunc->u.method.run;
}

/*------------------------------------------------------------------------------
  REGISTRATION AND LOOKUP FUNCTIONS
*/

void DestroyExternalFunc(struct ExternalFunc *efunc)
{
  struct ExternalFunc *tmp;
  if (efunc) {
    tmp = efunc;
    if (tmp->name ) ascfree((char *)(tmp->name)); /* we own the string */
    if (tmp->help) ascfree((char *)(tmp->help)); /* we own the string */
    tmp->name = NULL;
    tmp->help = NULL;
/* might want to set null pointers here depending on etype. */
    tmp->etype = efunc_ERR;
    ascfree((char *)tmp);
  }
}

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
    if (force==0) {
      return 0;
    } else {		/* need to update information */
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
