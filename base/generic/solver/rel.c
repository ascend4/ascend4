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
#include <compiler/instance_io.h>
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

#ifdef DEBUG
# define REL_DEBUG CONSOLE_DEBUG
#else
# define REL_DEBUG(MSG,...) ((void)0)
#endif

#define IPTR(i) ((struct Instance *)(i))
#define REIMPLEMENT 0 /* if set to 1, compiles code tagged with it. */

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


/* forward decls */

static struct rel_relation *rel_create_extnode(struct rel_relation * rel
		, struct ExtCallNode *ext
);

/*------------------------------------------------------------------------------
  GENERAL 'RELATION' ROUTINES
*/

static struct rel_relation *rel_copy(const struct rel_relation *rel){
	struct rel_relation *newrel;
	newrel = ASC_NEW(struct rel_relation);
	REL_DEBUG("Copying REL_RELATION from %p to %p",rel,newrel);
	*newrel = *rel;
	return(newrel);
}

struct rel_relation *rel_create(SlvBackendToken instance
		, struct rel_relation *newrel
){
	CONST struct relation *instance_relation;
	struct ExtCallNode *ext;
	enum Expr_enum ctype;

	REL_DEBUG("instance = %p",IPTR(instance));
	REL_DEBUG("REL_RELATION newrel = %p",newrel);

	if(newrel==NULL){
		/* if newrel was not provided, create new copy of a 'default relation' */
		newrel = rel_copy(&g_rel_defaults);
		REL_DEBUG("CREATED NEW REL_RELATION at %p",newrel);
	}else{
		/* else copy the default relation into the memory space we were given */
		*newrel = g_rel_defaults;
		REL_DEBUG("CLEARED REL_RELATION at %p, SETTING DEFAULTS", newrel);
	}
	assert(newrel!=NULL);

	/* the rel_relation points to the instance */
	newrel->instance = instance;

	/* get the 'struct relation' object for this relation */
	instance_relation = GetInstanceRelation(IPTR(instance),&ctype);

	REL_DEBUG("Instance %p --> RELATION = %p",IPTR(instance),instance_relation);
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
			REL_DEBUG("Blackbox...");
			newrel->type = e_rel_blackbox;
			ext = BlackBoxExtCall(instance_relation);

			REL_DEBUG("Subject instance at %p",ExternalCallVarInstance(ext));
			REL_DEBUG("Subject instance type '%s'",instance_typename(ExternalCallVarInstance(ext)));

		    if(ext){
				REL_DEBUG("REL_EXTNODE FOUND, ATTACHING REL_RELATION TO EXT at %p",ext);
			    newrel = rel_create_extnode(newrel,ext);
		    }else{
			    REL_DEBUG("SET NODEINFO TO NULL IN NEWREL AT %p",newrel);
			    newrel->nodeinfo = NULL;
		    }

			REL_DEBUG("Subject instance is at %p",ExternalCallVarInstance(ext));
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

/**
	This is an evil routine to determine a variable pointer inside a
	relation, given the variable's Instance (backend) pointer.

	Good design will eliminate the need for this function.

	Aborts with Asc_Panic in case of var not found.

	@param rel relation in which we're looking
	@param inst token that we need to match
	@return var_variable corresponding to the inst parameter
*/
static struct var_variable *rel_instance_to_var(struct rel_relation *rel,
	SlvBackendToken inst
){
	int j, nincid;
	struct var_variable *var;
	struct var_variable **incid;

	incid = rel_incidence_list_to_modify(rel);
	nincid = rel_n_incidences(rel);

	CONSOLE_DEBUG("Looking for var in list of %d incident on rel %p",nincid,rel);

	var = NULL;
	for(j=0;j<nincid;++j){
		if(( var_instance(incid[j]) )==inst){
			var = incid[j];
			break;
		}
	}
	if(var==NULL){
		Asc_Panic(2,__FUNCTION__,"Var not found");
	}

	return var;
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

/**
	Implementation function for rel_n_incidences().  Do not call
	this function directly - use rel_n_incidences() instead.
*/
int32 rel_n_incidencesF(struct rel_relation *rel){
  if (rel!=NULL) return rel->n_incidences;
  FPRINTF(stderr,"rel_n_incidences miscalled with NULL\n");
  return 0;
}

void rel_set_incidencesF(struct rel_relation *rel,int32 n,
		struct var_variable **i
){
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
  /* struct Instance *inst; */

  /* REL_DEBUG("Creating rel_extnode"); */
  /* REL_DEBUG("REL = %p",rel); */
  nodeinfo = ASC_NEW(struct rel_extnode);
  nodeinfo->whichvar = (int)ExternalCallVarIndex(ext);
  asc_assert(nodeinfo->whichvar >= 1);
  nodeinfo->cache = NULL;
  rel->nodeinfo = nodeinfo;

  /* inst = ExternalCallVarInstance(ext); */
  /* REL_DEBUG("rel_extnode whichvar IS INSTANCE AT %p",inst); */
  /* REL_DEBUG("INSTANCE type is %s",instance_typename(inst)); */

  /* REL_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
  return rel;
}

struct rel_extnode *rel_extnodeinfo( struct rel_relation *rel){
  /* REL_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
  return(rel->nodeinfo);
}

unsigned long rel_extwhichvar( struct rel_relation *rel){
  /* REL_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
  if (rel->nodeinfo) {
    return(rel->nodeinfo->whichvar);
  } else {
    return 0; /* never a valid index */
  }
}

struct ExtRelCache *rel_extcache( struct rel_relation *rel){
  /* REL_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
  if(rel->nodeinfo!=NULL){
    return(rel->nodeinfo->cache);
  }else{
    return NULL;
  }
}

/**
	This function is naughty!
*/
struct Instance *rel_extsubject(struct rel_relation *rel){
	unsigned long subject = rel_extwhichvar(rel);
	struct ExtRelCache *cache = rel_extcache(rel);
	return GetSubjectInstance(cache->arglist,subject);
}

void rel_set_extnodeinfo( struct rel_relation *rel
		, struct rel_extnode *nodeinfo
){
  rel->nodeinfo = nodeinfo;
  /* REL_DEBUG("REL NODEINFO = %p",rel->nodeinfo); */
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
  REL_DEBUG("ASSIGNED efunc %p to ExtRelCache %p",cache->efunc,cache);

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

  /*
	Create the 'invars' and 'outvars' lists so that we can insert stuff
	into the solverside matrix correctly
  */

  cache->invars = NULL;
  cache->outvars = ASC_NEW_ARRAY_CLEAR(struct var_variable *,noutputs);

  REL_DEBUG("ALLOCATED FOR %d OUTPUTS",noutputs);
  cache->inputs = ASC_NEW_ARRAY_CLEAR(double,ninputs);
  cache->outputs = ASC_NEW_ARRAY_CLEAR(double,noutputs);
  cache->jacobian = ASC_NEW_ARRAY_CLEAR(double,ninputs*noutputs);

  /* Setup default flags for controlling calculations. */
  cache->evaluation_required = 1;
  cache->first_func_eval = 1;
  cache->first_deriv_eval = 1;

  REL_DEBUG("NEW CACHE = %p",cache);
  return cache;
}

struct ExtRelCache *CreateCacheFromInstance(SlvBackendToken relinst){
  struct ExtCallNode *ext;
  struct ExtRelCache *cache;
  CONST struct relation *reln;
  enum Expr_enum type;

  REL_DEBUG("CREATE CACHE FROM INSTANCE");

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

void extrel_store_output_var(struct rel_relation *rel){
	struct ExtRelCache *cache;
	int whichvar;
	struct var_variable *var;
	struct Instance *inst;

	cache = rel_extcache(rel);
	inst = rel_extsubject(rel);
	var = rel_instance_to_var(rel,inst);
	whichvar = rel_extwhichvar(rel);

	REL_DEBUG("outvar[%d] at %p",whichvar-cache->ninputs-1,var);
	asc_assert(cache->outvars!=NULL);
	cache->outvars[whichvar - cache->ninputs - 1] = var;
}

static struct var_variable *extrel_outvar(
		struct ExtRelCache *cache
		,int whichvar
){
	asc_assert(cache->outvars!=NULL);
	return cache->outvars[whichvar-cache->ninputs-1];
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

/**
	This function creates the lists of input and output var_variables so that
	we can insert variables where required in the overall matrix (mtx).

	This might form the core of a new implementation, if we decided to keep
	the external relation caching stuff on the solver side.

	At the moment the implementation is pretty naive, because it's the only
	part in the ExtRel stuff where we have anything to do with var_variable
	pointers. Ideally, all references to Instances in the ExtRelCache will
	be switched over to var_variable, so we don't have the border trafficking
	in struct Instance pointers, and so it all goes via var_variable...?
*/
void extrel_store_input_vars(struct rel_relation *rel){
	struct ExtRelCache *cache;
	int i, n;
	struct var_variable *var;

	cache = rel_extcache(rel);
	n = rel_n_incidences(rel);

	/* new stuff: create the 'invars' and 'outvars' lists */
	cache->invars = ASC_NEW_ARRAY_CLEAR(struct var_variable *,cache->ninputs);

	for(i=0;i<cache->ninputs;++i){
		var = rel_instance_to_var(rel, gl_fetch(cache->inputlist,i+1));
		cache->invars[i] = var;
		REL_DEBUG("invar[%d] at %p",i,var);
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
  REL_DEBUG("CACHE = %p",cache);
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
  if(init_func!=NULL){
    nok = (*init_func)(&slv_interp,cache->data,cache->arglist);

	if (nok) {
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error running init function (%d)",nok);
	  return 1;
	}
    REL_DEBUG("Ran init function");
  }

  /* Save the user's data and update our status. */
  cache->user_data = slv_interp.user_data;
  cache->evaluation_required = (unsigned)1;	/* force at least one calculation */
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
	REL_DEBUG("EVALUATING RELATION %p",rel);
	value = ExtRel_Evaluate_RHS(rel) - ExtRel_Evaluate_LHS(rel);
	REL_DEBUG("RESIDUAL = %f",value);
	return value;
}

real64 ExtRel_Evaluate_RHS(struct rel_relation *rel){
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  struct Instance *arg;
  struct gl_list_t *inputlist;
  double value;
  long unsigned c;
  int32 ninputs;
  int32 nok;
  unsigned long whichvar;
  int32 newcalc_reqd=0;

  /* REL_DEBUG("REL_RELATION = %p",rel); */

  ExtBBoxFunc *eval_func;

  assert(rel_extnodeinfo(rel));

  /*
  struct rel_extnode *nodeinfo = rel_extnodeinfo(rel);
  REL_DEBUG("REL NODEINFO = %p",nodeinfo);
  */

  cache = rel_extcache(rel);
  efunc = cache->efunc;
  /*
  REL_DEBUG("CACHE = %p",cache);
  REL_DEBUG("efunc = %p",efunc);
  REL_DEBUG("efunc->etype = %u",efunc->etype);
  REL_DEBUG("efunc->value = %p",efunc->u.black.value);
  */

  eval_func = GetValueFunc(efunc);
  inputlist = cache->inputlist;
  ninputs = cache->ninputs;
  whichvar = rel_extwhichvar(rel);

  /*
	The determination of whether a new calculation is required needs
	some more thought. One thing we should insist upon is that all
	the relations for an external relation are forced into the same
	block.
  */
  for (c=0;c<ninputs;c++) {
    arg = (struct Instance *)gl_fetch(inputlist,c+1);
    value = RealAtomValue(arg);
	REL_DEBUG("FOR INPUT %lu, value=%f (arg at %p), CACHED=%f"
		,c+1, value, arg,cache->inputs[c]
	);
    if(ArgsDifferent(value, cache->inputs[c])){
	  REL_DEBUG("ARGS ARE DIFFERENT, new calc will be required");
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
	REL_DEBUG("NEW CALCULATION REQUIRED");
    Init_Slv_Interp(&slv_interp);
    slv_interp.nodestamp = cache->nodestamp;
    slv_interp.user_data = cache->user_data;

    slv_interp.task = bb_func_eval;

  	for (c=0;c<ninputs;c++){
	  REL_DEBUG("input %lu: value = %f",c+1, cache->inputs[c]);
	}

    nok = (*eval_func)(&slv_interp, ninputs, cache->noutputs,
		       cache->inputs, cache->outputs, cache->jacobian);
	if(nok){
		REL_DEBUG("EXTERNAL CALCULATION ERROR (%d)",nok);
		/* return, don't change the output values, don't update flags */
		return 0;
	}else{
		REL_DEBUG("EVAL FUNC OK");
	}

  	for (c=0;c<cache->noutputs;c++){
	  REL_DEBUG("output %lu: value = %f",c+1, cache->outputs[c]);
	}

    value = cache->outputs[whichvar - ninputs - 1];
	/* REL_DEBUG("CALCULATED VALUE IS %f",value); */
    cache->evaluation_required = (unsigned)1;			/* newcalc done */
    cache->user_data = slv_interp.user_data;		/* update user_data */
  }
  else{
    value = cache->outputs[whichvar - ninputs - 1];
    cache->evaluation_required = 0; /* a result was simply returned */
  }

  REL_DEBUG("RHS VALUE = %f",value);
  /*
	FIXME need to get the value of the output and return the difference from
	the computed value and the current value as the residual
  */
  return value;
}

real64 ExtRel_Evaluate_LHS(struct rel_relation *rel){
	struct Instance *inst;
	double value;

	REL_DEBUG("...");

	assert(rel_extnodeinfo(rel));

	inst = rel_extsubject(rel);

	/* REL_DEBUG("VAR IS INSTANCE AT %p",inst); */

	/* REL_DEBUG("INSTANCE TYPE = %s",instance_typename(inst)); */

    value = RealAtomValue(inst);
	REL_DEBUG("LHS VALUE = %f",value);
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

	@param inputlist Instance objects corresponding to the blackbox inputs, in order
	@param whichvar Output Instance index (for use with GetSubjectInstance(cache->arglist,whichvar))
	@param ninputs The length of the inputlist
	@param jacobian Dense-matrix Jacobian data returned from the blackbox func.
	@param deriv_data Data cache (See rel.c)

	index = (whichvar - ninputs - 1) * ninputs

	Example: a problem with 4 inputs, 3 outputs and whichvar = 6.
	with the counting for vars 1..nvars, but the jacobian indexing
	starting from 0 (c-wise).

	        v-------- first output variable
	I I I I O O O
	1 2 3 4 5 6 7
	          ^--------- whichvar

	                               ------------------ grads for whichvar = 6
	                               |    |    |    |
    row        1    1    1    1    2    2    2    2   3    3   3    3
    col        1    2    3    4    1    2    3    4   1    2   3    4

	index   =  0    1    2    3    4    5    6    7   8    9  10   11
	jacobian =2.0  9.0  4.0  6.0  0.5  1.3  0.0  9.7  80  7.0 1.0 2.5

	Hence jacobian index = (6 - 4 - 1) * 4 = 4

	@NOTE This only corresponds to jacobian elements from the RHS of
	blackbox equations. There remain minus-ones to put down the diagonal for
	each output variable, since

		RESID[j] = Y(x[i],..x[n]) - y[j]

	(noting that it's RHS - LHS, see ExtRel_Evaluate_Residual),	so

		dRESID[j]/dy[i] = -1
		dRESID[j]/dx[i] = dy[j]/dx[i]

	The latter is what's being filled in here.
*/
static void ExtRel_MapDataToMtx(struct ExtRelCache *cache,
		unsigned long whichvar,
		struct deriv_data *d
){
  struct var_variable *var = NULL;
  double value, *ptr;
  //boolean used;
  unsigned long c;
  int32 index;
  unsigned long ninputs;

  ninputs = cache->ninputs;

  REL_DEBUG("whichvar = %lu, ninputs = %lu",whichvar, ninputs);
  index = ((int)whichvar - ninputs - 1) * ninputs;
  REL_DEBUG("JACOBIAN INDEX = %d",index);
  ptr = &(cache->jacobian[index]);

  asc_assert(ninputs >= 0);

  /*
  REL_DEBUG("Filter matchbits %x, matchvalue %x"
	,d->filter->matchbits
	,d->filter->matchvalue
  );
  */

  /* for input variables, the residual is taken from the matrix */
  for (c=0;c<(unsigned long)ninputs;c++) {
    var = cache->invars[c];
	REL_DEBUG("invar[%lu] at %p",c+1,var);
    /*
	// this is perhaps conditional modelling stuff, broken for the moment
	// for this first crack, all input variables are active and in the block
	used = var_apply_filter(var,d->filter);
	if (used) {
	*/
      d->nz.col = mtx_org_to_col(d->mtx,var_sindex(var));
	  REL_DEBUG("column = %d",d->nz.col);
      value = ptr[c] + mtx_value(d->mtx,&(d->nz));
	  REL_DEBUG("input %lu is used, value = %f",c,value);
      mtx_set_value(d->mtx,&(d->nz), value);
	/*
	// disused, continued
    }else{
	  var_filter_t f2 = {VAR_ACTIVE,VAR_ACTIVE};
	  if(!var_apply_filter(var,&f2)){
		REL_DEBUG("var is not active");
	  }else{
		REL_DEBUG("var not used...???");
	  }
	}
	*/
  }
}


static double CalculateInterval(double varvalue){
  UNUSED_PARAMETER(varvalue);

  return (1.0e-05);
}

/**
	Evaluate jacobian elements for an external relation
	using finite difference (peturbation of each input variable).

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
static int32 ExtRel_FDiff(struct Slv_Interp *slv_interp,
		ExtBBoxFunc *eval_func,
		int32 ninputs, int32 noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
  int32 c1,c2, nok = 0;
  double *tmp_vector;
  double *ptr;
  double old_x,interval,value;

  REL_DEBUG("NUMERICAL DERIVATIVE...");

  tmp_vector = ASC_NEW_ARRAY_CLEAR(double,noutputs);
  for (c1=0;c1<ninputs;c1++){
    /* perturb x */
    old_x = inputs[c1];
    interval = CalculateInterval(old_x);
    inputs[c1] = old_x + interval;
	REL_DEBUG("PETURBATION WITH input[%d]=%f",c1+1,inputs[c1]);

	/* call routine */
    nok = (*eval_func)(slv_interp, ninputs, noutputs, inputs, tmp_vector, jacobian);
    if(nok){
	    REL_DEBUG("External evaluation error (%d)",nok);
		break;
	}

	/* load jacobian */
    ptr = &jacobian[c1];
    for (c2=0;c2<noutputs;c2++) {
      value = (tmp_vector[c2] - outputs[c2])/interval;
	  REL_DEBUG("output[%d]: value = %f, gradient = %f",c2+1,tmp_vector[c2],value);
      *ptr = value;
      ptr += ninputs;
    }
    inputs[c1] = old_x;
  }
  ASC_FREE(tmp_vector);
  if(nok){
    REL_DEBUG("External evaluation error");
  }
  return nok;
}

static int32 ExtRel_CalcDeriv(struct rel_relation *rel, struct deriv_data *d){
  int32 nok = 0;
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  unsigned long whichvar;
  ExtBBoxFunc *eval_func;
  ExtBBoxFunc *deriv_func;

  REL_DEBUG("...");

  assert(rel_extnodeinfo(rel));
  cache = rel_extcache(rel);
  whichvar = rel_extwhichvar(rel);
  efunc = cache->efunc;

  /*
   * Check and deal with the special case of the first
   * computation.
   */
  if(cache->first_deriv_eval) {
	REL_DEBUG("FIRST DERIV EVAL");
    cache->evaluation_required = (unsigned)1;
    cache->first_deriv_eval = (unsigned)0;
  }

  /*
   * If a function evaluation was not recently done, then we
   * can return the results from the cached jacobian.
   */
  if(!cache->evaluation_required){
	REL_DEBUG("NO NEW CALC DONE, RETURN CACHED JACOBIAN");
    ExtRel_MapDataToMtx(cache, whichvar, d);
    return 0;
  }

  /*
   * If we are here, we have to do the derivative calculation.
   * The only thing to determine is whether we do analytical
   * derivatives (deriv_func != NULL) or finite differencing.
   * In any case init the interpreter.
   */
  Init_Slv_Interp(&slv_interp);
  slv_interp.task = bb_deriv_eval;
  slv_interp.user_data = cache->user_data;

  deriv_func = GetDerivFunc(efunc);
  if(deriv_func){
	REL_DEBUG("USING EXTERNAL DERIVATIVE FUNCTION");
    nok = (*deriv_func)(&slv_interp, cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if(nok){
      cache->evaluation_required = 1;
      return nok;
	}
  }else{
	REL_DEBUG("USING NUMERICAL DERIVATIVE");
    eval_func = GetValueFunc(efunc);
    nok = ExtRel_FDiff(&slv_interp, eval_func,
			cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if(nok){
      cache->evaluation_required = 1;
      return nok;
	}
  }

  /*
   * Cleanup. Ensure that we update the users data, and load
   * the main matrix with the derivative information.
   */
  cache->user_data = slv_interp.user_data;	/* save user info */
  ExtRel_MapDataToMtx(cache, whichvar, d);
  return 0;
}

/**
	ExtRel Deriv routines.

	This is the entry point for most cases. ExtRel_CalcDeriv depends
	on ExtRel_Evaluate being called  immediately before it.
*/

double ExtRel_Diffs_RHS(struct rel_relation *rel,
		var_filter_t *filter,
		int32 row,
		mtx_matrix_t mtx
){
  int32 nok;
  double rhs;
  struct deriv_data data;

  data.filter = filter;
  data.mtx = mtx;
  data.nz.row = row;

  rhs = ExtRel_Evaluate_RHS(rel);
  nok = ExtRel_CalcDeriv(rel,&data);
  if (nok)
    return 0.0;
  else
    return rhs;
}


double ExtRel_Diffs_LHS(struct rel_relation *rel, var_filter_t *filter,
		int32 row, mtx_matrix_t mtx
){
	double lhs;
	struct ExtRelCache *cache;
	int whichvar;
	struct var_variable *var;
	mtx_coord_t nz;

	lhs = ExtRel_Evaluate_LHS(rel);

	cache = rel_extcache(rel);
	whichvar = rel_extwhichvar(rel);
	var = extrel_outvar(cache,whichvar);

	/* enter '-1' into the matrix where the output var belongs */
	nz.row = row;
	nz.col = mtx_org_to_col(mtx,var_sindex(var));
	mtx_set_value(mtx,&(nz), -1.0 );

	/*
	REL_DEBUG("OUTPUTING MATRIX");
	mtx_region_t r;
	mtx_region(&r, 0, 2, 0, 4);
	mtx_write_region_human(ASCERR,mtx,&r);
	REL_DEBUG("Mapping LHS jacobian entry -1.0 to var at %p",var);
	*/

	return lhs;
}

double extrel_resid_and_jacobian(struct rel_relation *rel
	, var_filter_t *filter, int32 row, mtx_matrix_t mtx
){
	return ExtRel_Diffs_RHS(rel,filter,row,mtx)
		 - ExtRel_Diffs_LHS(rel,filter,row,mtx);
}
