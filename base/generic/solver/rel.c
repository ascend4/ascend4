/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
	by Karl Michael Westerberg
	Created: 2/6/90
	Last in CVS $Revision: 1.32 $ $Date: 1998/01/29 00:42:28 $ $Author: ballan $
*/

#include <math.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/fractions.h>
#include <compiler/instance_enum.h>
#include <compiler/symtab.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <compiler/find.h>
#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/mathinst.h>
#include <compiler/parentchild.h>
#include <compiler/instance_io.h>
#include <compiler/relation_type.h>
#include <compiler/relation.h>
#include <compiler/relation_util.h>
#include <compiler/packages.h>
#define _SLV_SERVER_C_SEEN_ /* for the extrel stuff in header */
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_server.h"

/*------------------------------------------------------------------------------
	forward declarations, constants, typedefs
*/

static struct rel_relation *rel_create_extnode(struct rel_relation * rel
		, struct ExtCallNode *ext
);

#define IPTR(i) ((struct Instance *)(i))
#define REIMPLEMENT 0 /* if set to 1, compiles code tagged with it. */
#define REL_DEBUG FALSE

/* define symchar names needed */
static symchar *g_strings[1];
#define INCLUDED_R g_strings[0]

static const struct rel_relation g_rel_defaults = {
   NULL,		    /* instance */
   NULL,		    /* extnode */
   NULL,		    /* incidence */
   e_undefined,	    /* e_token */
   0,			    /* n_incidences */
   -1,		    	/* mindex */
   -1,			    /* sindex */
   -1,		    	/* model index */
   (REL_INCLUDED)	/* flags */
};
/*
	Don't forget to update the initialization when the structure is modified.
*/

/*------------------------------------------------------------------------------
  GENERAL 'RELATION' ROUTINES
*/

static struct rel_relation *rel_copy(const struct rel_relation *rel){
	struct rel_relation *newrel;
	newrel = ASC_NEW(struct rel_relation);
	CONSOLE_DEBUG("Copying REL_RELATION from %p to %p",rel,newrel);
	*newrel = *rel;
	return(newrel);
}

struct rel_relation *rel_create(SlvBackendToken instance
		, struct rel_relation *newrel
){
	CONST struct relation *instance_relation;
	struct ExtCallNode *ext;
	enum Expr_enum ctype;

	CONSOLE_DEBUG("instance = %p",IPTR(instance));
	CONSOLE_DEBUG("REL_RELATION newrel = %p",newrel);

	if(newrel==NULL){
		/* if newrel was not provided, create new copy of a 'default relation' */
		newrel = rel_copy(&g_rel_defaults);
		CONSOLE_DEBUG("CREATED NEW REL_RELATION at %p",newrel);
	}else{
		/* else copy the default relation into the memory space we were given */
		*newrel = g_rel_defaults;
		CONSOLE_DEBUG("CLEARED REL_RELATION at %p, SETTING DEFAULTS", newrel);
	}
	assert(newrel!=NULL);

	/* the rel_relation points to the instance */
	newrel->instance = instance;
	
	/* get the 'struct relation' object for this relation */
	instance_relation = GetInstanceRelation(IPTR(instance),&ctype);

	CONSOLE_DEBUG("Instance's RELATION = %p",IPTR(instance),instance_relation);
	switch (ctype) {
		case e_token:
		    newrel->type = e_rel_token;
		    break;
		case e_opcode:
		    Asc_Panic(2,__FUNCTION__, "switching on e_opcode");
		    break;
		case e_glassbox:
		    newrel->type = e_rel_glassbox;
		    break;
		case e_blackbox:
			CONSOLE_DEBUG("Blackbox...");
			newrel->type = e_rel_blackbox;
			ext = BlackBoxExtCall(instance_relation);
		    if(ext){
				CONSOLE_DEBUG("REL_EXTNODE FOUND, ATTACHING REL_RELATION TO EXT at %p",ext);
			    newrel = rel_create_extnode(newrel,ext);
		    }else{
			    CONSOLE_DEBUG("SET NODEINFO TO NULL IN NEWREL AT %p",newrel);
			    newrel->nodeinfo = NULL;
		    }
		    break;
		default:
		    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown relation type in rel_create");
		    break;
	}
	return(newrel);
}

SlvBackendToken rel_instance(struct rel_relation *rel){
  if (rel==NULL) return NULL;
  return (SlvBackendToken) rel->instance;
}

void rel_write_name(slv_system_t sys,struct rel_relation *rel,FILE *fp){
  if(rel == NULL || fp==NULL) return;
  if(sys!=NULL) {
    WriteInstanceName(fp,rel_instance(rel),slv_instance(sys));
  }else{
    WriteInstanceName(fp,rel_instance(rel),NULL);
  }
}

void rel_destroy(struct rel_relation *rel){
   struct Instance *inst;
   switch (rel->type) {
   case e_rel_token:
     break;
   default:
     break;
   }
   if (rel->nodeinfo) {
     rel->nodeinfo->cache = NULL;
     ascfree(rel->nodeinfo);
     rel->nodeinfo = NULL;
   }
   ascfree((POINTER)rel->incidence);
   inst = IPTR(rel->instance);
   if (inst) {
     if (GetInterfacePtr(inst)==rel) { /* we own the pointer -- reset it. */
       SetInterfacePtr(inst,NULL);
     }
   }
   ascfree((POINTER)rel);
}

uint32 rel_flags( struct rel_relation *rel){
  return rel->flags;
}

void rel_set_flags(struct rel_relation *rel, uint32 flags){
  rel->flags = flags;
}

uint32 rel_flagbit(struct rel_relation *rel, uint32 one){
  if (rel==NULL || rel->instance == NULL) {
    FPRINTF(stderr,"ERROR: rel_flagbit called with bad var.\n");
    return 0;
  }
  return (rel->flags & one);
}

void rel_set_flagbit(struct rel_relation *rel, uint32 field,uint32 one){
  if (one) {
    rel->flags |= field;
  } else {
    rel->flags &= ~field;
  }
}

/* this needs to be reimplemented properly. */
boolean rel_less(struct rel_relation *rel){
  switch( RelationRelop(GetInstanceRelationOnly(IPTR(rel->instance))) ) {
  case e_notequal:
  case e_less:
  case e_lesseq:
    return TRUE;
  default:
    return FALSE;
  }
}

/**
	this needs to be reimplemented properly.
*/
boolean rel_equal( struct rel_relation *rel){
  switch( RelationRelop(GetInstanceRelationOnly(IPTR(rel->instance))) ) {
  case e_equal:
  case e_lesseq:
  case e_greatereq:
    return TRUE;
  default:
    return FALSE;
  }
}

boolean rel_greater(struct rel_relation *rel){
  switch( RelationRelop(GetInstanceRelationOnly(IPTR(rel->instance))) ) {
  case e_notequal:
  case e_greater:
  case e_greatereq:
    return TRUE;
  default:
    return FALSE;
  }
}

static enum rel_enum compenum2relenum(enum Expr_enum t){
  switch (t) {
  case e_equal:    return e_rel_equal;
  case e_less:     return e_rel_less;
  case e_greater:  return e_rel_greater;
  case e_lesseq:   return e_rel_lesseq;
  case e_greatereq:return e_rel_greatereq;
  default:
    FPRINTF(ASCERR,"ERROR (rel.c): compenum2relenum miscalled.\n");
    return e_rel_not_equal;
  }
}
enum rel_enum rel_relop( struct rel_relation *rel){
  return
    compenum2relenum(RelationRelop(
        GetInstanceRelationOnly(IPTR(rel->instance))));
}

char *rel_make_name(slv_system_t sys,struct rel_relation *rel){
  return WriteInstanceNameString(IPTR(rel->instance),IPTR(slv_instance(sys)));
}

int32 rel_mindex( struct rel_relation *rel){
   return( rel->mindex );
}

void rel_set_mindex( struct rel_relation *rel, int32 index){
   rel->mindex = index;
}

int32 rel_sindex( const struct rel_relation *rel){
   return( rel->sindex );
}

void rel_set_sindex( struct rel_relation *rel, int32 index){
   rel->sindex = index;
}

int32 rel_model(const struct rel_relation *rel){
   return((const int32) rel->model );
}

void rel_set_model( struct rel_relation *rel, int32 index){
   rel->model = index;
}

real64 rel_residual(struct rel_relation *rel){
   return( RelationResidual(GetInstanceRelationOnly(IPTR(rel->instance))));
}

void rel_set_residual( struct rel_relation *rel, real64 residual){
  struct relation *reln;
  reln = (struct relation *)GetInstanceRelationOnly(IPTR(rel->instance));
  SetRelationResidual(reln,residual);
}

real64 rel_multiplier(struct rel_relation *rel){
  return( RelationMultiplier(GetInstanceRelationOnly(IPTR(rel->instance))));
}

void rel_set_multiplier(struct rel_relation *rel, real64 multiplier){
  struct relation *reln;
  reln = (struct relation *)GetInstanceRelationOnly(IPTR(rel->instance));
  SetRelationMultiplier(reln,multiplier);
}

real64 rel_nominal( struct rel_relation *rel){
  return( RelationNominal(GetInstanceRelationOnly(IPTR(rel->instance))));
}

void rel_set_nominal( struct rel_relation *rel, real64 nominal){
  struct relation *reln;
  reln = (struct relation *)GetInstanceRelationOnly(IPTR(rel->instance));
  SetRelationNominal(reln,nominal);
}

/**
	too bad there's no entry point that rel must call before being used 
	generally, like the FindType checking stuff in var.c
*/
static void check_included_flag(void){
  if (INCLUDED_R == NULL || AscFindSymbol(INCLUDED_R) == NULL) {
    INCLUDED_R = AddSymbolL("included",8);
  }
}
uint32 rel_included( struct rel_relation *rel){
	struct Instance *c;
	check_included_flag();
	c = ChildByChar(IPTR(rel->instance),INCLUDED_R);
	if( c == NULL ) {
		FPRINTF(stderr,"ERROR:  (rel) rel_included\n");
		FPRINTF(stderr,"        No 'included' field in relation.\n");
		WriteInstance(stderr,IPTR(rel->instance));
		return FALSE;
	}
	rel_set_flagbit(rel,REL_INCLUDED,GetBooleanAtomValue(c));
	return( GetBooleanAtomValue(c) );
}

void rel_set_included( struct rel_relation *rel, uint32 included){
	struct Instance *c;
	check_included_flag();
	c = ChildByChar(IPTR(rel->instance),INCLUDED_R);
	if( c == NULL ) {
		FPRINTF(stderr,"ERROR:  (rel) rel_set_included\n");
		FPRINTF(stderr,"        No 'included' field in relation.\n");
		WriteInstance(stderr,IPTR(rel->instance));
		return;
	}
	SetBooleanAtomValue(c,included,(unsigned)0);
	rel_set_flagbit(rel,REL_INCLUDED,included);
}

int32 rel_apply_filter( const struct rel_relation *rel, rel_filter_t *filter){
  if (rel==NULL || filter==NULL) {
    FPRINTF(stderr,"rel_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  /* AND to mask off irrelevant bits in flags and match value, then compare */
  return ( (filter->matchbits & rel->flags) ==
           (filter->matchbits & filter->matchvalue)
         );
}

int32 rel_n_incidencesF(struct rel_relation *rel){
  if (rel!=NULL) return rel->n_incidences;
  FPRINTF(stderr,"rel_n_incidences miscalled with NULL\n");
  return 0;
}

void rel_set_incidencesF(struct rel_relation *rel,int32 n,
                         struct var_variable **i)
{
  if(rel!=NULL && n >=0) {
    if (n && i==NULL) {
      FPRINTF(stderr,"rel_set_incidence miscalled with NULL ilist\n");
    }
    rel->n_incidences = n;
    rel->incidence = i;
    return;
  }
  FPRINTF(stderr,"rel_set_incidence miscalled with NULL or n < 0\n");
}

const struct var_variable **rel_incidence_list( struct rel_relation *rel){
  if (rel==NULL) return NULL;
  return( (const struct var_variable **)rel->incidence );
}

struct var_variable **rel_incidence_list_to_modify( struct rel_relation *rel){
  if (rel==NULL) return NULL;
  return( (struct var_variable **)rel->incidence );
}

#if KILL
expr_t rel_lhs( struct rel_relation *rel){
  if (rel==NULL) return NULL;
  return( rel->lhs );
}

expr_t rel_rhs( struct rel_relation *rel){
  if (rel==NULL) return NULL;
  return( rel->rhs );
}
#endif /* KILL */

/*==============================================================================
  EXTERNAL RELATIONS CACHE

	External Relations Cache for solvers.
	by Kirk Andre Abbott
	Created: 08/10/94
*/

double g_external_tolerance = 1.0e-12;

/* - - - - - - - -
  REL->EXTREL ACCESS FUNCTIONS
*/

static struct rel_relation *rel_create_extnode(struct rel_relation * rel
		, struct ExtCallNode *ext
){
  struct rel_extnode *nodeinfo;

  CONSOLE_DEBUG("Creating rel_extnode");
  CONSOLE_DEBUG("REL = %p",rel);
  nodeinfo = ASC_NEW(struct rel_extnode);
  nodeinfo->whichvar = (int)ExternalCallVarIndex(ext);
  nodeinfo->cache = NULL;
  rel->nodeinfo = nodeinfo;
  CONSOLE_DEBUG("REL NODEINFO = %p",rel->nodeinfo);
  return rel;
}

struct rel_extnode *rel_extnodeinfo( struct rel_relation *rel){
  /* CONSOLE_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
  return(rel->nodeinfo);
}

int32 rel_extwhichvar( struct rel_relation *rel){
  CONSOLE_DEBUG("REL NODEINFO = %p",rel->nodeinfo);
  if (rel->nodeinfo) {
    return(rel->nodeinfo->whichvar);
  } else {
    return 0; /* never a valid index */
  }
}

struct ExtRelCache *rel_extcache( struct rel_relation *rel){
  CONSOLE_DEBUG("REL NODEINFO = %p",rel->nodeinfo);
  if(rel->nodeinfo!=NULL){
    return(rel->nodeinfo->cache);
  }else{
    return NULL;
  }
}

void rel_set_extnodeinfo( struct rel_relation *rel
		, struct rel_extnode *nodeinfo
){
  rel->nodeinfo = nodeinfo;
  CONSOLE_DEBUG("REL NODEINFO = %p",rel->nodeinfo);
}

void rel_set_extwhichvar(struct rel_relation *rel, int32 whichvar){
  rel->nodeinfo->whichvar = whichvar;
}

void rel_set_extcache( struct rel_relation *rel,struct ExtRelCache * cache){
  rel->nodeinfo->cache = cache;
}

/*------------------------------------------------------------------------------
  EXTERNAL RELATION CACHE (EXTRELCACHE)
*/

struct ExtRelCache *CreateExtRelCache(struct ExtCallNode *ext){
  struct ExtRelCache *cache;
  unsigned long n_input_args, n_output_args;
  int32 ninputs, noutputs;

  assert(ext!=NULL);

  cache = ASC_NEW(struct ExtRelCache);
  cache->user_data = NULL; /* it's vital that this is initialized to NULL ! */

  /* Copy various pointers from the ExtCallNode to our cache object */
  cache->nodestamp = ExternalCallNodeStamp(ext);
  cache->efunc = ExternalCallExtFunc(ext);
  cache->data = ExternalCallDataInstance(ext);
  cache->arglist = ExternalCallArgList(ext);
  CONSOLE_DEBUG("ASSIGNED efunc %p to ExtRelCache %p",cache->efunc,cache);

  /* Fetch the size of the input/output argument lists */
  n_input_args = NumberInputArgs(cache->efunc);
  n_output_args = NumberOutputArgs(cache->efunc);

  /* Linearise the argument lists (for fast access, presumably) */
  cache->inputlist = LinearizeArgList(cache->arglist,1,n_input_args);
		  /* Note: we own the cache of the LinearizeArgList call. */

  ninputs = (int32)gl_length(cache->inputlist);
  noutputs = (int32)CountNumberOfArgs(cache->arglist,n_input_args+1,
				    n_input_args+n_output_args);
  cache->ninputs = ninputs;
  cache->noutputs = noutputs;

  cache->inputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
  cache->outputs = ASC_NEW_ARRAY_CLEAR(double,noutputs);
  cache->jacobian = ASC_NEW_ARRAY_CLEAR(double,ninputs*noutputs);

  /* Setup default flags for controlling calculations. */
  cache->newcalc_done = (unsigned)1;
  CONSOLE_DEBUG("NEW CACHE = %p",cache);
  return cache;

}


struct ExtRelCache *CreateCacheFromInstance(SlvBackendToken relinst)
{
  struct ExtCallNode *ext;
  struct ExtRelCache *cache;
  CONST struct relation *reln;
  enum Expr_enum type;

  CONSOLE_DEBUG("CREATE CACHE FROM INSTANCE");

  assert(relinst != NULL && InstanceKind(IPTR(relinst))==REL_INST);
  reln = GetInstanceRelation(IPTR(relinst),&type);
  if (type!=e_blackbox) {
    FPRINTF(stderr,"Invalid relation type in (CreateCacheFromInstance)\n");
    return NULL;
  }
  ext = BlackBoxExtCall(reln);
  cache = CreateExtRelCache(ext);
  return cache;
}

void ExtRel_DestroyCache(struct ExtRelCache *cache)
{
  if (cache) {
    cache->nodestamp=-1;
    cache->efunc = NULL;
    cache->data = NULL;
    gl_destroy(cache->inputlist);	/* we own the list */
    ascfree(cache->inputs);
    ascfree(cache->outputs);
    ascfree(cache->jacobian);
    cache->inputlist = NULL;
    cache->inputs = NULL;
    cache->outputs = NULL;
    cache->jacobian = NULL;
    ascfree(cache);
  }
}

/*- - - - - - -
  'INIT' FUNCTION CALLS
*/

int32 ExtRel_PreSolve(struct ExtRelCache *cache, int32 setup){
  struct ExternalFunc *efunc;
  struct Slv_Interp slv_interp;

  ExtBBoxInitFunc *init_func;
  int32 nok = 0;

  if(cache==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"cache is NULL");
    return 1;
  }

  /* prepare parameters to pass to the init function */
  efunc = cache->efunc;
  CONSOLE_DEBUG("CACHE = %p",cache);
  init_func = GetInitFunc(efunc);
  Init_Slv_Interp(&slv_interp);
  slv_interp.nodestamp = cache->nodestamp;
  slv_interp.user_data = cache->user_data;

  if(setup){
    slv_interp.task = bb_first_call;
  }else{
    slv_interp.task = bb_last_call;
  }

  /* call the init function */
  nok = (*init_func)(&slv_interp,cache->data,cache->arglist);

  if (nok) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error running init function (%d)",nok);
    return 1;
  }

  CONSOLE_DEBUG("Ran init function");

  /* Save the user's data and update our status. */
  cache->user_data = slv_interp.user_data;
  cache->newcalc_done = (unsigned)1;	/* force at least one calculation */
  cache->first_func_eval = (unsigned)0;
  return 0;
}

/* - - - - - - -
  EVALUATION FUNCTIONS
*/

/**
	Comparison for two values being sent as inputs to an external relation.
	If they are sufficently similar, we will avoid re-evaluating the external
	relation.
*/
static int ArgsDifferent(double new, double old){
  if(fabs(new - old) >= g_external_tolerance){
    return 1;
  }else{
    return 0;
  }
}

real64 ExtRel_Evaluate_Residual(struct rel_relation *rel){
	double value;
	CONSOLE_DEBUG("EVALUATING RELATION %p",rel);
	value = ExtRel_Evaluate_RHS(rel) - ExtRel_Evaluate_LHS(rel);
	CONSOLE_DEBUG("RESIDUAL = %f",value);
	return value;
}

real64 ExtRel_Evaluate_RHS(struct rel_relation *rel){
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  struct Instance *arg;
  struct gl_list_t *inputlist;
  double value;
  int32 c,ninputs;
  int32 nok;
  unsigned long whichvar;
  int32 newcalc_reqd=0;

  CONSOLE_DEBUG("REL_RELATION = %p",rel);

  ExtBBoxFunc *eval_func;

  assert(rel_extnodeinfo(rel));

  struct rel_extnode *nodeinfo = rel_extnodeinfo(rel);
  CONSOLE_DEBUG("REL NODEINFO = %p",nodeinfo);

  cache = rel_extcache(rel);
  efunc = cache->efunc;
  CONSOLE_DEBUG("CACHE = %p",cache);
  CONSOLE_DEBUG("efunc = %p",efunc);
	
  eval_func = GetValueFunc(efunc);
  inputlist = cache->inputlist;
  ninputs = cache->ninputs;
  whichvar = (unsigned long)rel_extwhichvar(rel);

  /*
	The determination of whether a new calculation is required needs
	some more thought. One thing we should insist upon is that all
	the relations for an external relation are forced into the same
	block.
  */
  for (c=0;c<ninputs;c++) {
    arg = (struct Instance *)gl_fetch(inputlist,c+1);
    value = RealAtomValue(arg);
    if(ArgsDifferent(value, cache->inputs[c])){
      newcalc_reqd = 1;
      cache->inputs[c] = value;
    }
  }
  value = 0;
  /*
	Do the calculations, if necessary. Note that we have to *ensure*
	that we send the user the information that he provided to us.
	We have to update our user_data after each call of the user's function
	as he might move information around (not smart but possible), on us.
	If a function call is made, mark a new calculation as having been,
	done, otherwise dont.
  */
  if (newcalc_reqd) {
    Init_Slv_Interp(&slv_interp);
    slv_interp.nodestamp = cache->nodestamp;
    slv_interp.user_data = cache->user_data;

    slv_interp.task = bb_func_eval;

    nok = (*eval_func)(&slv_interp, ninputs, cache->noutputs,
		       cache->inputs, cache->outputs, cache->jacobian);
    value = cache->outputs[whichvar - ninputs - 1];
    cache->newcalc_done = (unsigned)1;			/* newcalc done */
    cache->user_data = slv_interp.user_data;		/* update user_data */
  }
  else{
    value = cache->outputs[whichvar - ninputs - 1];
    cache->newcalc_done = (unsigned)0; /* a result was simply returned */
  }

  CONSOLE_DEBUG("RHS VALUE = %f",value);
  /*
	FIXME need to get the value of the output and return the difference from
	the computed value and the current value as the residual
  */
  return value;
}

real64 ExtRel_Evaluate_LHS(struct rel_relation *rel){
	struct ExtRelCache *cache;
	struct Instance *arg;
	double value;
	unsigned long whichvar;

	CONSOLE_DEBUG("...");

	assert(rel_extnodeinfo(rel));
	cache = rel_extcache(rel);
	whichvar = (unsigned long)rel_extwhichvar(rel);
    arg = (struct Instance *)gl_fetch(cache->arglist,whichvar);
    value = RealAtomValue(arg);
	CONSOLE_DEBUG("LHS VALUE = %f",value);
	return value;
}

/*- - - - - 
  GRADIENT EVALUATION

	The following code implements gradient evaluation routines for
	external relations. The routines here will prepare the arguements
	and call a user supplied derivative routine, if same is non-NULL.

	If it's NULL, the user supplied function evaluation routine will be
	used to compute the gradients via finite differencing.

	The current solver will not necessarily call for the derivative
	all at once. This makes it necessary to do the gradient
	computations (user supplied or finite difference), and to cache
	away the results, as for the residuals. Based on calculation flags, the
	appropriate *row* of this cached jacobian will be extracted and mapped to
	the	main solve matrix.
	
	The cached jacobian is a contiguous vector ninputs*noutputs long
	and is loaded row wise. Indexing starts from 0. Each row corresponds
	to the partial derivatives of the output variable (associated with
	that row, wrt to all its incident input variables.
	
	Careful attention needs to be paid to the way this jacobian is
	loaded/unloaded, because of the multiple indexing schemes in use.
	i.e, arglist's and inputlists index 1..nvars, whereas all numeric
	vectors number from 0.
*/

struct deriv_data {
  var_filter_t *filter;
  mtx_matrix_t mtx;
  mtx_coord_t nz;
};


/**
	This function attempts to map the information from the contiguous
	jacobian back into the main matrix, based on the current row <=>
	whichvar. The jacobian assumed to have been calculated.
	Since we are operating a relation at a time, we have to find
	out where to index into our jacobian. This index is computed as
	follows:
	
	index = (whichvar - ninputs - 1) * ninputs
	
	Example: a problem with 4 inputs, 3 outputs and whichvar = 6.
	with the counting for vars 1..nvars, but the jacobian indexing
	starting from 0 (c-wise).
	
 *          V-------- first output variable
 *  1 2 3 4 5 6 7
 *            ^---------------- whichvar
 *							    ------------------- grads for whichvar = 6
 *							    |    |    |    |
 *							    v    v    v    v
 *  index    =	0    1    2    3    4    5    6    7   8    9   10    11
 *  jacobian = 2.0  9.0  4.0  6.0  0.5  1.3  0.0  9.7  80  7.0  1.0  2.5
 *
 * Hence jacobian index = (6 - 4 - 1) * 4 = 4
 *
 *
 * THIS FUNCTION IS TOTALLY AND COMPLETELY BROKEN.
 */
static void ExtRel_MapDataToMtx(struct gl_list_t *inputlist,
				unsigned long whichvar,
				int32 ninputs,
				double *jacobian,
				struct deriv_data *d)
{
  struct Instance *inst;
  struct var_variable *var = NULL;
  double value, *ptr;
  boolean used;
  unsigned long c;
  int32 index;

  index = ((int)whichvar - ninputs - 1) * ninputs;
  ptr = &jacobian[index];

/* this is totally broken, thanks to kirk making the var=instance assumption */
  asc_assert(ninputs >= 0);
  Asc_Panic(2, "ExtRel_MapDataToMtx",
            "ExtRel_MapDataToMtx is totally broken");
  for (c=0;c<(unsigned long)ninputs;c++) {
    inst = (struct Instance *)gl_fetch(inputlist,c+1);
/*
    var = var_instance(inst);
*/
    used = var_apply_filter(var,d->filter);
    if (used) {
      d->nz.col = mtx_org_to_col(d->mtx,var_sindex(var));
      value = ptr[c] + mtx_value(d->mtx,&(d->nz));
      mtx_set_value(d->mtx,&(d->nz), value);
    }
  }
}


/**
	ExtRel Finite Differencing.
	
	This routine actually does the finite differencing.
	The jacobian is a single contiguous vector. We load information
	in it *row* wise. If we have noutputs x ninputs = 3 x 4, variables,
	then jacobian entry 4,
	would correspond to jacobian[1][0], i.e., = 0.5 for this eg.
	
	  2.0  9.0   4.0  6.0
	  0.5  1.3   0.0  9.7
	  80   7.0   1.0  2.5
	
	 2.0  9.0  4.0  6.0  0.5  1.3  0.0  9.7  80  7.0  1.0  2.5
	[0][0]              [1][0]		      [2][1]
	
	When we are finite differencing variable c, we will be loading
	jacobian positions c, c+ninputs, c+2*ninputs ....
*/

static double CalculateInterval(double varvalue){
  (void)varvalue;  /* stop gcc whine about unused parameter */

  return (1.0e-05);
}

static int32 ExtRel__FDiff(struct Slv_Interp *slv_interp,
		int32 (*eval_func) (/* ARGS */),
		int32 ninputs, int32 noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
  int32 c1,c2, nok = 0;
  double *tmp_vector;
  double *ptr;
  double old_x,interval,value;

  tmp_vector = ASC_NEW_ARRAY_CLEAR(double,noutputs);
  for (c1=0;c1<ninputs;c1++) {
    old_x = inputs[c1];					/* perturb x */
    interval = CalculateInterval(old_x);
    inputs[c1] = old_x + interval;
    nok = (*eval_func)(slv_interp, ninputs, noutputs,	/* call routine */
		       inputs, tmp_vector, jacobian);
    if (nok) break;
    ptr = &jacobian[c1];
    for (c2=0;c2<noutputs;c2++) {			/* load jacobian */
      value = (tmp_vector[c2] - outputs[c2])/interval;
      *ptr = value;
      ptr += ninputs;
    }
    inputs[c1] = old_x;
  }
  ascfree((char *)tmp_vector);				/* cleanup */
  return nok;
}

static int32 ExtRel_CalcDeriv(struct rel_relation *rel, struct deriv_data *d){
  int32 nok = 0;
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  unsigned long whichvar;
  int32 (*eval_func)();
  int32 (*deriv_func)();

  assert(rel_extnodeinfo(rel));
  cache = rel_extcache(rel);
  whichvar = rel_extwhichvar(rel);
  efunc = cache->efunc;

  /*
   * Check and deal with the special case of the first
   * computation.
   */
  if (cache->first_deriv_eval) {
    cache->newcalc_done = (unsigned)1;
    cache->first_deriv_eval = (unsigned)0;
  }

  /*
   * If a function evaluation was not recently done, then we
   * can return the results from the cached jacobian.
   */
  if (!cache->newcalc_done) {
    ExtRel_MapDataToMtx(cache->inputlist, whichvar,
			cache->ninputs, cache->jacobian, d);
    return 0;
  }

  /*
   * If we are here, we have to do the derivative calculation.
   * The only thing to determine is whether we do analytical
   * derivatives (deriv_func != NULL) or finite differencing.
   * In any case init the interpreter.
   */
  Init_Slv_Interp(&slv_interp);
/*
  slv_interp.deriv_eval = (unsigned)1;
*/
  slv_interp.task = bb_deriv_eval;
  slv_interp.user_data = cache->user_data;
  deriv_func = GetDerivFunc(efunc);
  if (deriv_func) {
    nok = (*deriv_func)(&slv_interp, cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if (nok) return nok;
  }
  else{
    eval_func = GetValueFunc(efunc);
    nok = ExtRel__FDiff(&slv_interp, eval_func,
			cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if (nok) return nok;
  }

  /*
   * Cleanup. Ensure that we update the users data, and load
   * the main matrix with the derivative information.
   */
  cache->user_data = slv_interp.user_data;	/* save user info */
  ExtRel_MapDataToMtx(cache->inputlist, whichvar,
		      cache->ninputs, cache->jacobian, d);
  return 0;
}

/**
	ExtRel Deriv routines.
	
	This is the entry point for most cases. ExtRel_CalcDeriv depends
	on ExtRel_Evaluate being called  immediately before it.
*/

real64 ExtRel_Diffs_RHS(struct rel_relation *rel,
		var_filter_t *filter,
		int32 row,
		mtx_matrix_t mtx
){
  int32 nok;
  real64 res;
  struct deriv_data data;

  data.filter = filter;
  data.mtx = mtx;
  data.nz.row = row;

  res = ExtRel_Evaluate_RHS(rel);
  nok = ExtRel_CalcDeriv(rel,&data);
  if (nok)
    return 0.0;
  else
    return res;
}


/** @TODO FIXME */
real64 ExtRel_Diffs_LHS(struct rel_relation *rel, var_filter_t *filter,
		int32 row, mtx_matrix_t mtx
){
  UNUSED_PARAMETER(rel);
  UNUSED_PARAMETER(filter);
  UNUSED_PARAMETER(row);
  UNUSED_PARAMETER(mtx);
  return 1.0;
}

