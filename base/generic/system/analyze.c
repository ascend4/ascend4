/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan

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
	Problem Analysis Routines

	@NOTE 'ip' signifies 'interface pointer' @ENDNOTE

	The intent here is to do away with the old persistent interface pointer
	scheme by making the struct rel_relation* individually keep track of the
	map between the ascend RelationVariable list position and the
	solver's var list index (and hence the column in jacobian for jacobian
	involved clients).
	In this mapping each struct relation* has its var list and this list
	may contain RealAtomInstances that we don't consider variables.
	In the rel_relation we will have the variable index list
	which is an array of int32 the same length as the RelationVariable list.
	In the array position 0 corresponds to RelationVariable 1 since the
	compiler uses gl_lists. If in the variable index list we encounter
	a number < 0 we know that that RelationVariable doesn't map to what
	we consider a solver variable.

	In the near future we may also add to the struct rel_relation *an array,
	a, of int32 pairs like so:
	vlindex | rvindex | vlindex | rvindex
	and a length. This array could be built of the data for vars that pass
	a filter provided by the client. This way a client could help us avoid
	having to do if testing while stuffing jacobians.
	In this scheme stuffing a jacobian row (or whatever) would simply mean
	calling the compiler's derivative function (wrt RelationVariable list)

	@code
	which returns a vector d of values and then doing a loop:
	  for( i = 0 ; i < length; i++) { coord.row fixed already
	    coord.col = a[i++];
	    mtx_fill_org_value(mtx,&coord,d[a[i]])
	  }
	}
	@endcode

	One begins to wonder if there isn't a better way to do all this, but
	so far nothing has occurred.
	The best way would be to feed clients only the block of stuff they
	are interested in (no fixed, unattached vars or unincluded rels
	or vars/rels solved in previous partitions) so that the solver
	had no partitioning or var classification work to do except perhaps
	classifying things as basic/nonbasic.

	@see analyze.h
*//*
	by Benjamin Andrew Allan 5/19/96
	Last in CVS: $Revision: 1.56 $ $Date: 2003/08/23 18:43:12 $ $Author: ballan $
*/

/**	@addtogroup analyse Analyse
	@{
*/

#include "analyze.h"

#include <stdarg.h>

#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include <general/list.h>
#include <general/dstring.h>
#include <general/mathmacros.h>

#include <compiler/atomvalue.h>
#include <compiler/parentchild.h>
#include <compiler/visitinst.h>
#include <compiler/expr_types.h>
#include <compiler/exprs.h>
#include <compiler/sets.h>
#include <compiler/mathinst.h>
#include <compiler/instquery.h>
#include <compiler/instance_io.h>
#include <compiler/find.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/rel_blackbox.h>
#include <compiler/vlist.h>
#include <compiler/relation.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/relation_util.h>
#include <compiler/logical_relation.h>
#include <compiler/logrelation.h>
#include <compiler/logrel_util.h>
#include <compiler/case.h>
#include <compiler/when_util.h>

#include <linear/mtx.h>

#include "slv_server.h"
#include "cond_config.h"
#include "diffvars.h"
#include "analyse_impl.h"

/* stuff to get rid of */
#ifndef MAX_VAR_IN_LIST
#define MAX_VAR_IN_LIST 20
#endif /* MAX_VAR_IN_LIST  */
#define DEBUG_ANALYSIS FALSE

/*------------------------------------------------------------------------------
  GLOBAL VARS
*/

static symchar *g_strings[6];

/* symbol table entries we need */
#define INCLUDED_A g_strings[0]
#define FIXED_A g_strings[1]
#define BASIS_A g_strings[2]
#define DERIV_A g_strings[3]
#define ODEID_A g_strings[4]
#define OBSID_A g_strings[5]

/*
	Global variable. Set to true by classify if need be
*/
static int g_bad_rel_in_list;


/* used to give an integer value to each symbol used in a when */
struct gl_list_t *g_symbol_values_list = NULL;

/*-----------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/
static void ProcessModelsInWhens(struct Instance *, struct gl_list_t *,
                                 struct gl_list_t *, struct gl_list_t *);

/*------------------------------------------------------------------------------
  SOME STUFF WITH INTERFACE POINTERS
*/
/**
	Return a pointer to the one-size-fits-all ips we're using.
	Always returns non-NULL because if we run out we exit
*/
static
struct solver_ipdata *analyze_getip(void){
  if(g_reuse.ipbuf!=NULL && g_reuse.ipused < g_reuse.ipcap) {
    return &(g_reuse.ipbuf[g_reuse.ipused++]);
  }else{
    ASC_PANIC("Too many ips requested by analyze_getip");
  }
}

/**
	Reallocates to the requested size (newcap) the ipbuf.
	If newcap = 0, frees ipbuf.
	If newcap > 0 and reset mem != 0, initializes ipbuf to 0s.
	Resets ipused to 0.

	@return 0 on success, 1 on insufficient memory
*/
static
int resize_ipbuf(size_t newcap, int resetmem){
  struct solver_ipdata *tmp;
  if(newcap ==0) {
    if(g_reuse.ipbuf != NULL) {
      ascfree(g_reuse.ipbuf);
    }
    g_reuse.ipcap = g_reuse.ipused = 0;
    g_reuse.ipbuf = NULL;
    return 0;
  }
  if(newcap > g_reuse.ipcap) {
    if( g_reuse.ipcap >0 ) {
      tmp = (struct solver_ipdata *)
        ascrealloc(g_reuse.ipbuf,newcap*sizeof(struct solver_ipdata));
      if(tmp == NULL) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
        return 1;
      }
      g_reuse.ipbuf = tmp;
      g_reuse.ipcap = newcap;
    }else{
      tmp = ASC_NEW_ARRAY(struct solver_ipdata,newcap);
      if(tmp == NULL) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
        return 1;
      }
      g_reuse.ipbuf = tmp;
      g_reuse.ipcap = newcap;
    }
  }
  if(resetmem) {
    memset((char *)g_reuse.ipbuf,0,
           g_reuse.ipcap*sizeof(struct solver_ipdata));
  }
  g_reuse.ipused = 0;
  return 0;
}

/*------------------------------------------------------------------------------
  SOME STUFF TO DO WITH INCIDENCE SPACE
*/

/*
	checks size of request and returns a pointer to the next available
	chunk of incidence space. if too much requested or 0 requested returns
	NULL. p_data->relincidence must have been allocated for this to work.
*/
static
struct var_variable **get_incidence_space(int len, struct problem_t *p_data){
  struct var_variable **tmp;
  if(p_data->relincidence == NULL) {
    ASC_PANIC("get_incidence_space called prematurely. bye.\n");
  }
  if(len <1) return NULL;
  if(p_data->relincinuse + len > p_data->relincsize) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"get_incidence_space called excessively.");
    return NULL;
  }
  tmp = &(p_data->relincidence[p_data->relincinuse]);
  p_data->relincinuse += len;
  return tmp;
}

/*
	checks size of request and returns a pointer to the next available
	chunk of incidence space. if too much requested or 0 requested returns
	NULL. p_data->varincidence must have been allocated for this to work.
*/
static
struct rel_relation **get_var_incidence_space(int len
		,struct problem_t *p_data
){
  struct rel_relation **tmp;
  if(p_data->varincidence == NULL) {
    ASC_PANIC("get_var_incidence_space called prematurely. bye.\n");
  }
  if(len <1) return NULL;
  if(p_data->varincinuse + len > p_data->varincsize) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"get_var_incidence_space called excessively.");
    return NULL;
  }
  tmp = &(p_data->varincidence[p_data->varincinuse]);
  p_data->varincinuse += len;
  return tmp;
}


/*
	p_data->logrelinciden must have been allocated for this to work.
*/
static
struct dis_discrete **get_logincidence_space(int len
		,struct problem_t *p_data
){
  struct dis_discrete **tmp;
  if(p_data->logrelinciden == NULL) {
    ASC_PANIC("get_logincidence_space called prematurely. bye.\n");
  }
  if(len <1) return NULL;
  if(p_data->lrelincinuse + len > p_data->lrelincsize) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"get_logincidence_space called excessively.");
    return NULL;
  }
  tmp = &(p_data->logrelinciden[p_data->lrelincinuse]);
  p_data->lrelincinuse += len;
  return tmp;
}

/*------------------------------------------------------------------------------
  SOME OTHER STUFF
*/

/*
	InitTreeCounts(i); This resets the pointers and counters in
	p_data to null. p_data is supposed to be a temporary structure
	so the memory management of the pointers in p_data is the job
	of the caller.
	p_data->root is set to i.
*/
static
void InitTreeCounts(struct Instance *i,struct problem_t *p_data){
  memset((char *)p_data,0,sizeof(struct problem_t));
  p_data->root = i;
}

#define AVG_PARENTS 2
#define AVG_CHILDREN 4
#define AVG_RELATIONS 15
#define AVG_GROWTH 2
#define PART_THRESHOLD 1000

/**
	@param i atom instance in the Instance hierarchy
	@param sc name of an supposed child instance of i, a boolean atom.

	@return the value of the named boolean child instance, or NULL if i or the child doesn't exist
*/
static
int BooleanChildValue(struct Instance *i,symchar *sc){
  if(i == NULL || sc == NULL || (i=ChildByChar(i,sc)) == NULL) {
    return 0;
  }else{
    return ( GetBooleanAtomValue(i) );
  }
}

/**
	As for BooleanChildValue but for integer child atoms (ode_type in this case)
*/
static
int IntegerChildValue(struct Instance *i,symchar *sc){
  if(i == NULL || sc == NULL || (i=ChildByChar(i,sc)) == NULL) {
    return 0;
  }else{
    return ( GetIntegerAtomValue(i) );
  }
}


static 
void CollectArrayRelsAndWhens(struct Instance *i, long modindex
		,struct problem_t *p_data
){
  struct Instance *child;
  struct solver_ipdata *rip;
  unsigned long nch,c;

  if(i==NULL) return;
  nch = NumberChildren(i);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(i,c);
    if(child==NULL) continue;
    switch (InstanceKind(child)) {
    case REL_INST:
      rip = SIP(GetInterfacePtr(child));
      if(rip->u.r.model == 0) {
        rip->u.r.model = modindex;
      }else{
        /* ah hah! found an array with two distinct MODEL parents! skip it */
        break;
      }
      if(rip->u.r.cond) {
        gl_append_ptr(p_data->cnds,(VOIDPTR)rip);
      }
      else {
        if(rip->u.r.obj) {
          gl_append_ptr(p_data->objrels,(VOIDPTR)rip);
        }else{
          gl_append_ptr(p_data->rels,(VOIDPTR)rip);
        }
      }
      break;
    case LREL_INST:
      rip = SIP(GetInterfacePtr(child));
      if(rip->u.lr.model == 0) {
        rip->u.lr.model = modindex;
      }else{
        break;
      }
      if(rip->u.lr.cond) {
        gl_append_ptr(p_data->logcnds,(VOIDPTR)rip);
      }
      else {
        gl_append_ptr(p_data->logrels,(VOIDPTR)rip);
      }
      break;
    case WHEN_INST:
      rip = SIP(GetInterfacePtr(child));
      if(rip->u.w.model == 0) {
        rip->u.w.model = modindex;
      }else{
        break;
      }
      gl_append_ptr(p_data->whens,(VOIDPTR)rip);
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if(ArrayIsRelation(child) ||
          ArrayIsLogRel(child) ||
          ArrayIsWhen(child)
      ){
        CollectArrayRelsAndWhens(child,modindex,p_data);
      }
      break;
    default:
      break;
    }
  }
}


/*
	Collect all the logrels/relations at the local scope of the MODEL
	associated with ip->i. Local scope includes arrays of logrels/relations,
	mainly because these arrays don't have interface pointers so we
	can't treat them separately.
*/
static
void CollectRelsAndWhens(struct solver_ipdata *ip
		,long modindex
		,struct problem_t *p_data
){
  struct Instance *child;
  struct solver_ipdata *rip;
  unsigned long nch,c;

  if(ip->i==NULL) return;
  nch = NumberChildren(ip->i);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(ip->i,c);
    if(child==NULL) continue;
    switch (InstanceKind(child)) {
    case REL_INST:
      rip = SIP(GetInterfacePtr(child));
      rip->u.r.model = modindex;
      if(rip->u.r.cond) {
        gl_append_ptr(p_data->cnds,(VOIDPTR)rip);
      }
      else {
        if(rip->u.r.obj) {
          gl_append_ptr(p_data->objrels,(VOIDPTR)rip);
        }else{
          gl_append_ptr(p_data->rels,(VOIDPTR)rip);
        }
      }
      break;
    case LREL_INST:
      rip = SIP(GetInterfacePtr(child));
      rip->u.lr.model = modindex;
      if(rip->u.lr.cond) {
        gl_append_ptr(p_data->logcnds,(VOIDPTR)rip);
      }
      else {
        gl_append_ptr(p_data->logrels,(VOIDPTR)rip);
      }
      break;
    case WHEN_INST:
      rip = SIP(GetInterfacePtr(child));
      rip->u.w.model = modindex;
      gl_append_ptr(p_data->whens,(VOIDPTR)rip);
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if(ArrayIsRelation(child) ||
          ArrayIsLogRel(child)||
          ArrayIsWhen(child)
      ){
        CollectArrayRelsAndWhens(child,modindex,p_data);
      }
      break;
    default:
      break;
    }
  }
}

/*
	Count the instance into the required bin.

	@NOTE Call only with good relation instances.
*/
static void analyze_CountRelation(struct Instance *inst
		,struct problem_t *p_data
){
  switch( RelationRelop(GetInstanceRelationOnly(inst)) ) {
  case e_maximize:
  case e_minimize:
    p_data->no++;
    break;
  case e_less: case e_lesseq:
  case e_greater: case e_greatereq:
  case e_equal: case e_notequal:
    if( RelationIsCond(GetInstanceRelationOnly(inst)) ) {
      p_data->nc++;
    }
    else {
      p_data->nr++;
      if( GetInstanceRelationType(inst)==e_blackbox ) {
        p_data->ne++;
      }
    }
    break;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown relation type in %s",__FUNCTION__);
  }
}


/**
	Obtain an integer value from a symbol value
	Used for a WHEN statement. Each symbol value is storaged in a symbol list.
	It checks if the symval is already in the solver symbol list,
	if it is, returns the integer corresponding to the position of symval
	if it is not, appends the symval to the list and then returns the int

	@NOTE This is terrible inefficient as the number of symbols grows.
	I am keeping it by now, are they going to be so many symbols in
	whens anyway ?
*/
int GetIntFromSymbol(CONST char *symval
		,struct gl_list_t *symbol_list
){

  struct SymbolValues *entry,*dummy;
  unsigned long length;
  int len,c,value;
  int symbol_list_count;

  if(symbol_list != NULL) {
    len = gl_length(symbol_list);
    for (c=1; c<= len; c++) {
      dummy = (struct SymbolValues *)(gl_fetch(symbol_list,c));
      if(!strcmp(dummy->name,symval)) {
	return (dummy->value);
      }
    }
    symbol_list_count = len;
  }else{
    symbol_list_count = 0;
    symbol_list = gl_create(2L);
  }
  length = (unsigned long)strlen(symval);
  length++;
  symbol_list_count++;
  entry = (struct SymbolValues *)ascmalloc(sizeof(struct SymbolValues));
  entry->name = ASC_NEW_ARRAY(char,length);
  strcpy(entry->name,symval);
  entry->value = symbol_list_count;
  value = entry->value;
  gl_append_ptr(symbol_list,entry);
  return value;
}

void DestroySymbolValuesList(struct gl_list_t *symbol_list)
{
  struct SymbolValues *entry;
  int len,c;
    if(symbol_list != NULL) {
      len = gl_length(symbol_list);
      for (c=1; c<= len; c++) {
        entry = (struct SymbolValues *)(gl_fetch(symbol_list,c));
        ascfree((char *)entry->name); /* Do I need this ? */
        ascfree((char *)entry);
      }
      gl_destroy(symbol_list);
      symbol_list = NULL;
    }
}

/*------------------------------------------------------------------------------
  CLASSIFICATION OF INSTANCES, CREATING INTERFACE POINTERS
*/

/*
	classify_instance : to be called by PushInterfacePtrs.

	This function classifies the given instance and appends into
	the necessary list in the p_data structure.
	It also sets returns a pointer to the caller for insertion into
	the interface_ptr slot.

	Note, we should be passing the ipbuf info via vp so we don't
	need the nasty global variable and can be more thread safe.
	vp is a struct problem_t.
*/
static
void *classify_instance(struct Instance *inst, VOIDPTR vp){
  struct solver_ipdata *ip;
  struct problem_t *p_data;
  CONST char *symval;

  p_data = (struct problem_t *)vp;
  switch( InstanceKind(inst) ) {
  case REAL_ATOM_INST:   		/* Variable or parameter or real */
    ip = analyze_getip();
    ip->i = inst;
    ip->u.v.incident = 0;
    ip->u.v.in_block = 0;
    ip->u.v.index = 0;
    ip->u.v.active = 0;
    if(solver_var(inst)){
      ip->u.v.solvervar = 1; /* must set this regardless of what list */
      ip->u.v.fixed = BooleanChildValue(inst,FIXED_A);
      ip->u.v.basis = BooleanChildValue(inst,BASIS_A);
      ip->u.v.deriv = IntegerChildValue(inst,DERIV_A);
      ip->u.v.odeid = IntegerChildValue(inst,ODEID_A);
	  ip->u.v.obsid = IntegerChildValue(inst,OBSID_A);
	  /* CONSOLE_DEBUG("FOUND A VAR: deriv = %d, %s = %d",ip->u.v.deriv,SCP(ODEID_A),ip->u.v.odeid); */

      if(RelationsCount(inst)) {
        asc_assert((int)ip!=0x1);
        gl_append_ptr(p_data->vars,(POINTER)ip);
      }else{
        gl_append_ptr(p_data->unas,(POINTER)ip);
      }

	  if(ip->u.v.obsid){
		gl_append_ptr(p_data->obsvars,(POINTER)ip);
		/* CONSOLE_DEBUG("Added to obsvars"); */
      }
	  /* make the algebraic/differential/derivative cut */
	  if(ip->u.v.odeid){
        gl_append_ptr(p_data->diffvars,(POINTER)ip);
		/* CONSOLE_DEBUG("Added var to diffvars"); */
      }else{
		if(ip->u.v.deriv==-1){
		  gl_append_ptr(p_data->indepvars,(POINTER)ip);
		  /* CONSOLE_DEBUG("Added to indep vars"); */
		}else{
		  gl_append_ptr(p_data->algebvars,(POINTER)ip);
		  /* CONSOLE_DEBUG("Added var to algebvars"); */
		}
	  }
    }else{
      ip->u.v.fixed = 1;
      ip->u.v.solvervar=0;
      if(solver_par(inst) && RelationsCount(inst)) {
        gl_append_ptr(p_data->pars,(POINTER)ip);
      }else{
        gl_append_ptr(p_data->unas,(POINTER)ip);
      }
    }
    return ip;
  case BOOLEAN_ATOM_INST:
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 0;
    ip->u.dv.distype = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.index = 0;
    ip->u.dv.active = 0;
      ip->u.dv.value = GetBooleanAtomValue(inst);
    if(boolean_var(inst) ) {
      ip->u.dv.booleanvar = 1;
      ip->u.dv.fixed = BooleanChildValue(inst,FIXED_A);
    }else{
      ip->u.dv.fixed = 1;
      ip->u.dv.booleanvar=0;
    }
    if(LogRelationsCount(inst) || WhensCount(inst) ) {
      gl_append_ptr(p_data->dvars,(POINTER)ip);
      if(WhensCount(inst) ) {
        ip->u.dv.inwhen = 1;
      }else{
        ip->u.dv.inwhen = 0;
      }
    }else{
      gl_append_ptr(p_data->dunas,(POINTER)ip);
    }
    return ip;
  case BOOLEAN_CONSTANT_INST:
    if(!WhensCount(inst))return NULL;
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 1;
    ip->u.dv.distype = 0;
    ip->u.dv.index = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.booleanvar=0;
    ip->u.dv.fixed = 1;
    ip->u.dv.active = 0;
    ip->u.dv.value = GetBooleanAtomValue(inst);
    gl_append_ptr(p_data->dvars,(POINTER)ip);
    return ip;
  case INTEGER_ATOM_INST:
    if(!WhensCount(inst))return NULL;
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 0;
    ip->u.dv.distype = 1;
    ip->u.dv.index = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.booleanvar=0;
    ip->u.dv.fixed = 0;
    ip->u.dv.active = 0;
    ip->u.dv.value = GetIntegerAtomValue(inst);
    gl_append_ptr(p_data->dvars,(POINTER)ip);
    return ip;
  case SYMBOL_ATOM_INST:
    if(!WhensCount(inst))return NULL;
    symval = SCP(GetSymbolAtomValue(inst));
    if(symval == NULL)return NULL;
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 0;
    ip->u.dv.distype = -1;
    ip->u.dv.index = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.booleanvar=0;
    ip->u.dv.fixed = 0;
    ip->u.dv.active = 0;
    if(g_symbol_values_list == NULL) {
      g_symbol_values_list = gl_create(2L);
    }
    ip->u.dv.value = GetIntFromSymbol(symval,g_symbol_values_list);
    gl_append_ptr(p_data->dvars,(POINTER)ip);
    return ip;
  case INTEGER_CONSTANT_INST:
    if(!WhensCount(inst))return NULL;
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 1;
    ip->u.dv.distype = 1;
    ip->u.dv.index = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.booleanvar=0;
    ip->u.dv.fixed = 1;
    ip->u.dv.active = 0;
    ip->u.dv.value = GetIntegerAtomValue(inst);
    gl_append_ptr(p_data->dvars,(POINTER)ip);
    return ip;
  case SYMBOL_CONSTANT_INST:
    if(!WhensCount(inst))return NULL;
    symval = SCP(GetSymbolAtomValue(inst));
    if(symval==NULL)return NULL;
    ip = analyze_getip();
    ip->i = inst;
    ip->u.dv.isconst = 1;
    ip->u.dv.distype = -1;
    ip->u.dv.index = 0;
    ip->u.dv.incident = 0;
    ip->u.dv.booleanvar=0;
    ip->u.dv.fixed = 1;
    ip->u.dv.active = 0;
    if(g_symbol_values_list == NULL) {
      g_symbol_values_list = gl_create(2L);
    }
    ip->u.dv.value = GetIntFromSymbol(symval,g_symbol_values_list);
    gl_append_ptr(p_data->dvars,(POINTER)ip);
    return ip;
  case REL_INST:               /* Relation (or conditional or objective) */
    ip = analyze_getip();
    ip->i = inst;
    ip->u.r.active = 1;
    switch(RelationRelop(GetInstanceRelationOnly(inst))) {
    case e_minimize:
      ip->u.r.obj = 1;
      break;
    case e_maximize:
      ip->u.r.obj = -1;
      break;
    default:
      ip->u.r.obj = 0;
      break;
    }
    if(GetInstanceRelationType(inst)==e_blackbox){
      ip->u.r.ext = 1;
    }else{
      ip->u.r.ext = 0;
    }
    if(RelationIsCond(GetInstanceRelationOnly(inst))){
      ip->u.r.cond = 1;
    }else{
      ip->u.r.cond = 0;
    }
    if(WhensCount(inst)){
      ip->u.r.inwhen = 1;
    }else{
      ip->u.r.inwhen = 0;
    }
    ip->u.r.included = BooleanChildValue(inst,INCLUDED_A);
    ip->u.r.model = 0;
    ip->u.r.index = 0;
    return ip;
    /* note we do NOT append it to lists here */
  case LREL_INST:        		/* LogRelation */
    ip = analyze_getip();
    ip->i = inst;
    ip->u.lr.active = 1;
    if( LogRelIsCond(GetInstanceLogRel(inst)) ) {
      ip->u.lr.cond = 1;
    }else{
      ip->u.lr.cond = 0;
    }
    if( WhensCount(inst) ) {
      ip->u.lr.inwhen = 1;
    }else{
      ip->u.lr.inwhen = 0;
    }
    ip->u.lr.included = BooleanChildValue(inst,INCLUDED_A);
    ip->u.lr.model = 0;
    ip->u.lr.index = 0;
    return ip;
    /* note we do NOT append it to lists here */
  case MODEL_INST:
    ip = analyze_getip();
    ip->i = inst;
    ip->u.m.index = 0;
    if( WhensCount(inst) ) {
      ip->u.m.inwhen = 1;
    }else{
      ip->u.m.inwhen = 0;
    }
    gl_append_ptr(p_data->models,(POINTER)ip);
    return ip;
  case WHEN_INST:
    ip = analyze_getip();
    ip->i = inst;
    ip->u.w.index = 0;
    if( WhensCount(inst) ) {
      ip->u.w.inwhen = 1;
    }else{
      ip->u.w.inwhen = 0;
    }
    return ip;
    /* note we do NOT append it to lists here */
  default:
    return NULL;
  }
}

/*------------------------------------------------------------------------------
  MAKE PROBLEM routines
*/

/*
	This function sets g_bad_rel_in_list TRUE if it finds any unhappy
	relations. All the rest of the code depends on ALL relations being
	good, so don't disable the g_bad_rel_in_list feature.
*/
static
void CountStuffInTree(struct Instance *inst, struct problem_t *p_data){
  CONST char *symval;
  if(inst!=NULL) {
    switch (InstanceKind(inst)) {
    case REL_INST:
      if( GetInstanceRelationOnly(inst) == NULL ||
          GetInstanceRelationType(inst) == e_undefined) {
	/* guard against null relations, unfinished ones */
        ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
		FPRINTF(ASCERR,"Found bad (unfinished?) relation '");
        WriteInstanceName(ASCERR,inst,p_data->root);
        FPRINTF(ASCERR,"' (in CountStuffInTree)");
		error_reporter_end_flush();
        g_bad_rel_in_list = TRUE;
        return;
      }
      /* increment according to classification */
      analyze_CountRelation(inst,p_data);
      break;
    case REAL_ATOM_INST:
      if( solver_var(inst) && RelationsCount(inst)) {
        p_data->nv++;
      }else{
        if(solver_par(inst) && RelationsCount(inst)) {
          /* never passes right now */
          p_data->np++;
        }else{
          p_data->nu++;
        }
      }
      /* The use of  RelationsCount is heuristic since vars may be
      	used in relations higher in the tree than the problem is rooted.
       */
      break;
    case BOOLEAN_ATOM_INST:
      if( ( boolean_var(inst) && LogRelationsCount(inst) ) ||
	   WhensCount(inst) ) {
	p_data->ndv++;
      }
      else {
	p_data->nud++;
      }
      break;
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
      if(WhensCount(inst)) {
        p_data->ndv++;
      }
      break;
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
      if(WhensCount(inst)) {
        symval = SCP(GetSymbolAtomValue(inst));
        if(symval == NULL) {
          ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
		  FPRINTF(ASCERR,"CountStuffInTree found undefined symbol or symbol_constant in WHEN.\n");
          WriteInstanceName(ASCERR,inst,p_data->root);
          error_reporter_end_flush();
          g_bad_rel_in_list = TRUE;
          return;
        }
        p_data->ndv++;
      }
      break;
    case LREL_INST:
      if( GetInstanceLogRelOnly(inst) == NULL ) {
        ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
		FPRINTF(ASCERR,"CountStuffInTree found bad logrel.\n");
        WriteInstanceName(ASCERR,inst,p_data->root);
        error_reporter_end_flush();
        g_bad_rel_in_list = TRUE;
        return;
      }
      if( LogRelIsCond(GetInstanceLogRel(inst)) ) {
        p_data->ncl++;
      }
      else {
        p_data->nl++;
      }
      break;
    case WHEN_INST:
      p_data->nw++;
      break;
    case MODEL_INST:
      p_data->nm++;
      break;
    default:
      break;
    }
  }
}


/**
	Build 'master' lists of variables, relations, etc, from the Instance
	hierarchy assigned to the problem p_data.

	This takes the already derived counts,
	allocates all the memory we need to allocate for master,
	and actually builds the var/rel/MODEL/etc master lists,
	filling in p_data and interface pointers as far as possible.

	In particular, after this is done we have:

	  - vars with correct ip values for:
	     - index
	     - incident
	     - in_block
	     - fixed (as flag)
	     - basis (as flag)
	     - solvervar (as flag)
	  - relations and conditional relations with correct ip values for:
	     - index
	     - model
	     - obj (0 constraint, -1 maximize, +1 minimize)
	     - ext
	     - inwhen
	     - cond
	     - included	(as flag)
	  - discrete vars with correct ip values for:
	     - index
	     - incident
	     - isconst
	     - distype
	     - fixed (as flag)
	     - booleanvar (as flag)
	     - inwhen
	  - logrelations and conditional logrelations with correct ip values for:
	     - index
	     - model
	     - included	(as flag)
	     - inwhen
	     - cond
	  - whens with correct ip values for:
	     - index
	     - model
	     - inwhen
	  - models with correct ip values for:
	     - index

	Note that these are all indexed from 1, being stored in gl_list structs (groan).

	@param p_data partial problem definition, include Instance pointer and master list lengths assumed already computed.

	@return 0 on success, 1 on insufficient memory (user should deallocate any partial results in
	a separate cleanup function for p_data->), 2 if nothing to do.
*/
static
int analyze_make_master_lists(struct problem_t *p_data){

  long int c, len,v,vlen;
  CONST struct Instance *i;
  CONST struct relation *gut;
  CONST struct logrelation *lgut;
  struct solver_ipdata *vip;
  struct gl_list_t *scratch;
  size_t reqlen;
  int stat;
  reqlen =
    p_data->nr + p_data->no + p_data->nc + p_data->nl+ p_data->ncl+  /*rels*/
       p_data->nv + p_data->ndv +
       p_data->np + p_data->nu +  p_data->nud +  /* atoms */
       p_data->nw +                              /* when */
       p_data->nm;                               /* model */
  if(reqlen <1)  {
    return 2;
  }

  /* CREATE memory for ips */
#ifdef NDEBUG
  stat = resize_ipbuf(reqlen,0);
#else
  stat = resize_ipbuf(reqlen,1);
#endif /* NDEBUG */
  if(stat) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }

#define CL(L,N) \
  p_data->L = gl_create(p_data->N); \
  if(p_data->vars == NULL)return 1

  CL(vars,nv);	/* variables */
  CL(dvars,ndv);/* discrete variables */
  CL(pars,np);  /* parameters */
  CL(unas,nu);  /* unattached */
  CL(dunas,nud);/* discrete unattached */
  CL(cnds,nc);  /* conditional relations */
  CL(rels,nr);  /* relations */
  CL(logrels,nl);/* logical relations */
  CL(diffvars,nr); /* differential variables */
  CL(algebvars,nr); /* differential variables */
  CL(indepvars,nr); /* independent variables */ /* @TODO there's wasted memory here */
  CL(obsvars,nr); /* observed variables */
  CL(logcnds,ncl);/* conditional logrelations */
  CL(extrels,ne); /* extrelations */
  CL(objrels,no); /* objectives */
  CL(whens,nw);	/* whens */
  CL(models,nm); /* models */
#undef CL

  /* decorate the instance tree with ips, collecting vars and models. */
  p_data->oldips = PushInterfacePtrs(p_data->root,classify_instance,
                                    g_reuse.ipcap,1,p_data);
  if(p_data->oldips == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }

  /*
  	collect relations, objectives, logrels and whens recording the
  	MODEL number in each rel's ip and setting incidence.
  */
  len = gl_length(p_data->models);
  for (c=1; c <= len; c++) {
    CollectRelsAndWhens(SIP(gl_fetch(p_data->models,c)),c,p_data);
    SIP(gl_fetch(p_data->models,c))->u.m.index = c;
  }
  if((long)gl_length(p_data->rels) != p_data->nr ||
      (long)gl_length(p_data->objrels) != p_data->no ||
      (long)gl_length(p_data->logrels) != p_data->nl ||
      (long)gl_length(p_data->whens) != p_data->nw
  ){
    ERROR_REPORTER_START_HERE(ASC_PROG_WARNING);
	FPRINTF(ASCERR,"Warning: Mismatch in problem census and problem found\n");
    FPRINTF(ASCERR,"Rels: Counted %lu\t Found %ld\n",gl_length(p_data->rels), p_data->nr);
    FPRINTF(ASCERR,"Objs: Counted %lu\t Found %ld\n",gl_length(p_data->objrels), p_data->no);
    FPRINTF(ASCERR,"LogRels: Counted %lu\t Found %ld\n",gl_length(p_data->logrels),p_data->nl);
    FPRINTF(ASCERR,"Whens: Counted %lu\t Found %ld\n",gl_length(p_data->whens), p_data->nw);
    error_reporter_end_flush();
	
  }
  /*
  	relation list is now grouped by model, and the order will be
  	invariant with hardware and ascend invocation so long as
  	set FIRSTCHOICE holds in compilation.
  */
  /* mark vars in constraints incident and index rels */
  len = gl_length(p_data->rels);
  for (c=1; c <= len; c++) {
    SIP(gl_fetch(p_data->rels,c))->u.r.index = c;
    gut = GetInstanceRelationOnly(SIP(gl_fetch(p_data->rels,c))->i);
    vlen = NumberVariables(gut);
    for( v = 1; v <= vlen; v++ ) {
      i = RelationVariable(gut,v);
      SIP(GetInterfacePtr(i))->u.v.incident = 1;
    }
  }
  /* mark vars in objectives incident */
  len = gl_length(p_data->objrels);
  for (c=1; c <= len; c++) {
    gut = GetInstanceRelationOnly(SIP(gl_fetch(p_data->objrels,c))->i);
    vlen = NumberVariables(gut);
    for( v = 1; v <= vlen; v++ ) {
      i = RelationVariable(gut,v);
      SIP(GetInterfacePtr(i))->u.v.incident = 1;
    }
  }

  /* mark vars in conditional relations incident */
  len = gl_length(p_data->cnds);
  for (c=1; c <= len; c++) {
    SIP(gl_fetch(p_data->cnds,c))->u.r.index = c;
    gut = GetInstanceRelationOnly(SIP(gl_fetch(p_data->cnds,c))->i);
    vlen = NumberVariables(gut);
    for( v = 1; v <= vlen; v++ ) {
      i = RelationVariable(gut,v);
      SIP(GetInterfacePtr(i))->u.v.incident = 1;
    }
  }

  /* mark dvars in logrels incident  and index logrels */
  len = gl_length(p_data->logrels);
  for (c=1; c <= len; c++) {
    SIP(gl_fetch(p_data->logrels,c))->u.lr.index = c;
    lgut = GetInstanceLogRelOnly(SIP(gl_fetch(p_data->logrels,c))->i);
    vlen = NumberBoolVars(lgut);
    for( v = 1; v <= vlen; v++ ) {
      i = LogRelBoolVar(lgut,v);
      SIP(GetInterfacePtr(i))->u.dv.incident = 1;
    }
  }


  /* mark dvars in conditional logrels incident */
  len = gl_length(p_data->logcnds);
  for (c=1; c <= len; c++) {
    SIP(gl_fetch(p_data->logcnds,c))->u.lr.index = c;
    lgut = GetInstanceLogRelOnly(SIP(gl_fetch(p_data->logcnds,c))->i);
    vlen = NumberBoolVars(lgut);
    for( v = 1; v <= vlen; v++ ) {
      i = LogRelBoolVar(lgut,v);
      SIP(GetInterfacePtr(i))->u.dv.incident = 1;
    }
  }

  /* mark index whens */
  len = gl_length(p_data->whens);
  for (c=1; c <= len; c++) {
    SIP(gl_fetch(p_data->whens,c))->u.w.index = c;
  }
  /*
  	now we need to move all the nonincident vars off the var list
  	onto the unas list. It is easiest to make a new list and copy
  	the existing var list to either it if keep or unas if punt.
  	the same goes for parameters that aren't incident.
  	We don't do exactly the same for discrete variables because
  	many of them are not incident but they are used in when's
  */
  len = gl_length(p_data->vars);
  p_data->tmplist = gl_create(len);
  if(p_data->tmplist == NULL) return 1;
  for (c=1; c <= len; c++) {
    /* dispose of var */
    vip = SIP(gl_fetch(p_data->vars,c));
    if( vip->u.v.incident == 1) {
      gl_append_ptr(p_data->tmplist,vip);
    }else{
      gl_append_ptr(p_data->unas,vip);
    }
  }
  gl_destroy(p_data->vars);		/* punt the old list */
  p_data->vars = p_data->tmplist; 	/* make shortened list var list */
  p_data->tmplist = NULL;

  p_data->nv = gl_length(p_data->vars);

  len = gl_length(p_data->pars);
  p_data->tmplist = gl_create(len);
  if(p_data->tmplist == NULL){
    return 1;
  }
  for(c=1; c <= len; c++){
    /* dispose of par */
    vip = SIP(gl_fetch(p_data->pars,c));
    if( vip->u.v.incident == 1) {
      gl_append_ptr(p_data->tmplist,vip);
    }else{
      gl_append_ptr(p_data->unas,vip);
    }
  }
  gl_destroy(p_data->pars);		/* punt the old list */
  p_data->pars = p_data->tmplist; 	/* make shortened list par list */
  p_data->tmplist = NULL;

  p_data->np = gl_length(p_data->pars);
  p_data->nu = gl_length(p_data->unas);

  /*
  	discrete variables: take the incident dis vars in logrels first,
  	then append the dis vars which are used only in whens
  */
  p_data->lrelinc = 0;
  len = gl_length(p_data->dvars);
  if( len > 0) {
    p_data->tmplist = gl_create(len);
    scratch = gl_create(2L);
    if(p_data->tmplist == NULL) return 1;
    for (c=1; c <= len; c++) {
      /* dispose of incident discrete vars */
      vip = SIP(gl_fetch(p_data->dvars,c));
      if( vip->u.dv.incident == 1) {
        gl_append_ptr(p_data->tmplist,vip);
        p_data->lrelinc++;                  /* Number of incident dis vars */
      }else{
        gl_append_ptr(scratch,vip);
      }
    }
    gl_destroy(p_data->dvars);
    p_data->dvars = p_data->tmplist;
    p_data->tmplist = NULL;
    /* append discrete non-boolean vars at the end of the list */
    len = gl_length(scratch);
    for (c=1; c <= len; c++) {
      vip = SIP(gl_fetch(scratch,c));
      gl_append_ptr(p_data->dvars,vip);
    }
    gl_destroy(scratch);
    scratch = NULL;
  }

  /*
  	The following patch is to avoid the system to crash.
  	When multiple definitions of a solver_var have introduced into the
  	system, ASCEND may fail in identifying that a REAL_ATOM is a refinement
  	of a solver_var. This causes the system to think that, even that there
  	are relations into the system, there are no incidences in these relations
  	that fall into the category of a variable. As a consequence, the length
  	of the list of variables is zero. That of course will introduce
  	insanities while trying to build the slv system. Some
  	solutions to this problem are:

  	The easier for us is just to alert the user and to force him/her to
  	reload all the type defintions again. That is the current solution

  	The correct (but time consuming) solution is the implementation of a
  	SolverAtomInstance, which still needs parser and interpreter support
  */

  if(p_data->nr != 0 &&  p_data->nv==0) {
	ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"Problem should contain at least one variable and one relation.\n");
    FPRINTF(ASCERR,"There are relations into the system, but the number\n");
    FPRINTF(ASCERR,"of variables is zero. That means that the existent\n");
    FPRINTF(ASCERR,"vars were not recognized as solver_vars. A possible\n");
    FPRINTF(ASCERR,"reason is that you have introduced conflicting\n");
    FPRINTF(ASCERR,"definitions of solver_var into the system. Please\n");
    FPRINTF(ASCERR,"delete all your types and reload your models\n");
    FPRINTF(ASCERR,"with the appropriate definition of solver_var\n");
    FPRINTF(ASCERR,"Solver system will not be built.");
	error_reporter_end_flush();
    return 2;
  }

  return 0;
}

/**
	This function cleans up an errant problem_t or a good one that we're
	done with. We should have set to null any pointers to memory we are
	keeping elsewhere before calling this.
*/
static
void analyze_free_lists(struct problem_t *p_data){

  /* memory containing gl_lists of atomic structure pointers */
  /* if(p_data->extrels != NULL)gl_free_and_destroy(p_data->extrels); */

  if(p_data->extrels!=NULL)gl_destroy(p_data->extrels); /* -- JP HACK */

#define AFUN(P) if(p_data->P!=NULL) ascfree(p_data->P); (p_data->P) = NULL
#define ADUN(P) if(p_data->P!=NULL) gl_destroy(p_data->P); (p_data->P) = NULL

  /* gl_lists without memory items use ADUN */
  ADUN(vars);   ADUN(dvars);  ADUN(pars);
  ADUN(unas);   ADUN(dunas);  ADUN(rels);
  ADUN(objrels);  ADUN(models);  ADUN(cnds);
  ADUN(logrels);  ADUN(logcnds);  ADUN(whens);
  ADUN(diffvars);
  ADUN(algebvars);
  ADUN(indepvars);
  ADUN(obsvars); /* observed variables */

  /* blocks of memory use AFUN */
  AFUN(blocks);  AFUN(reldata);  AFUN(objdata);
  AFUN(condata);  AFUN(lrdata);  AFUN(logcondata);
  AFUN(vardata);  AFUN(pardata);  AFUN(undata);
  AFUN(disdata);  AFUN(undisdata);  AFUN(whendata);
  AFUN(bnddata);  AFUN(relincidence);  AFUN(varincidence);
  AFUN(logrelinciden);
  /* these variable names are a %*@#$ abomination */
  AFUN(mastervl);  AFUN(masterdl);  AFUN(masterrl);
  AFUN(masterol);  AFUN(mastercl);  AFUN(masterll);
  AFUN(mastercll);  AFUN(masterpl);  AFUN(masterul);
  AFUN(masterdul);  AFUN(masterwl);  AFUN(masterbl);
  AFUN(solvervl);  AFUN(solverdl);  AFUN(solverrl);
  AFUN(solverol);  AFUN(solvercl);  AFUN(solverll);
  AFUN(solvercll);  AFUN(solverpl);  AFUN(solverul);
  AFUN(solverdul);  AFUN(solverwl);  AFUN(solverbl);

#undef AFUN
#undef ADUN
}


/*------------------------------------------------------------------------------
  'WHEN' PROCESSING
*/

/**
	This function receives as argument the list of values of each of the
	CASES of a WHEN statement. The values in the list can be integer values,
	symbol values, or boolean values. So, the goal of this function is to
	obtain an equivalent list of ONLY integer values for such a list. In this
	way, for example, the boolean value TRUE is equivalent to the integer
	1. The function GentIntFromSymbol is used to generate an integer value
	which will be equivalent to a symbol value
*/
static
void ProcessValueList(struct Set *ValueList, int *value
		,struct gl_list_t *symbol_list
){
  CONST struct Expr *expr;
  struct Set *s;

  s = ValueList;
  if(ValueList!=NULL) {
    while (s != NULL) {
      expr = GetSingleExpr(s);
      switch(ExprType(expr)) {
        case e_boolean:
	  *value = ExprBValue(expr);
	  if(*value == 2) {   /* ANY */
	    *value = -2;
	  }
	  break;
        case e_int:
	  *value = ExprIValue(expr);
	  break;
        case e_symbol:
	  *value = GetIntFromSymbol(SCP(ExprSymValue(expr)),symbol_list);
	  break;
        default:
	  break;
      }
      s = NextSet(s);
      value++;
    }
  }else{
    *value = -1;  /* OTHERWISE */
  }
}


/**
	Process arrays inside WHENs (requires a recursive analysis).

	@see ProcessSolverWhens
*/
static
void ProcessArraysInWhens(struct Instance *cur_inst
		,struct gl_list_t *rels
		,struct gl_list_t *logrels
		,struct gl_list_t *whens
){
  struct rel_relation *rel;
  struct logrel_relation *lrel;
  struct w_when *w;
  struct Instance *child;
  struct solver_ipdata *ip;
  unsigned long c,nch;

  if(cur_inst==NULL) return;
  nch = NumberChildren(cur_inst);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(cur_inst,c);
    if(child==NULL) continue;
    switch (InstanceKind(child)) {
    case REL_INST:
      ip = SIP(GetInterfacePtr(child));
      ip->u.r.active = 0;
      rel = ip->u.r.data;
      gl_append_ptr(rels,rel);
      break;
    case LREL_INST:
      ip = SIP(GetInterfacePtr(child));
      ip->u.lr.active = 0;
      lrel = ip->u.lr.data;
      gl_append_ptr(logrels,lrel);
      break;
    case WHEN_INST:
      ip = SIP(GetInterfacePtr(child));
      w = ip->u.w.data;
      gl_append_ptr(whens,w);
      when_set_inwhen(w,TRUE);
      break;
    case MODEL_INST:
      ProcessModelsInWhens(child,rels,logrels,whens);
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if(ArrayIsRelation(child) || ArrayIsWhen(child)
         || ArrayIsLogRel(child) || ArrayIsModel(child)) {
        ProcessArraysInWhens(child,rels,logrels,whens);
      }
      break;
    default:
      break;
    }
  }
}

/**
	Process MODELs inside WHENs (requires a recursive analysis).

	@see ProcessSolverWhens
*/
static
void ProcessModelsInWhens(struct Instance *cur_inst, struct gl_list_t *rels
		,struct gl_list_t *logrels, struct gl_list_t *whens
){
  struct rel_relation *rel;
  struct logrel_relation *lrel;
  struct w_when *w;
  struct Instance *child;
  struct solver_ipdata *ip;
  unsigned long c,nch;

  if(cur_inst==NULL) return;
  nch = NumberChildren(cur_inst);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(cur_inst,c);
    if(child==NULL) continue;
    switch (InstanceKind(child)) {
    case REL_INST:
      ip = SIP(GetInterfacePtr(child));
      ip->u.r.active = 0;
      rel = ip->u.r.data;
      gl_append_ptr(rels,rel);
      break;
    case LREL_INST:
      ip = SIP(GetInterfacePtr(child));
      ip->u.lr.active = 0;
      lrel = ip->u.lr.data;
      gl_append_ptr(logrels,lrel);
      break;
    case WHEN_INST:
      ip = SIP(GetInterfacePtr(child));
      w = ip->u.w.data;
      gl_append_ptr(whens,w);
      when_set_inwhen(w,TRUE);
      break;
    case MODEL_INST:
      ProcessModelsInWhens(child,rels,logrels,whens);
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if(ArrayIsRelation(child) || ArrayIsWhen(child)
         || ArrayIsLogRel(child) || ArrayIsModel(child)) {
        ProcessArraysInWhens(child,rels,logrels,whens);
      }
      break;
    default:
      break;
    }
  }
}


/**
	Fill in the list of cases and variables of a w_when structure with
	the appropriate data. 

	The information required is provided by the corresponding when Instance 
	generated in the compilation time. So, what we do is:
	1) Obtain the list of variables and the list of cases from each
	   WHEN intance.
	   The list of variables is actually a list of pointers to instances
	2) From each CASE, obtain also the list of references. This list of
	references contains a list of pointers to each relation,logrelation  and
	model included inside the case.
	3) The pointers to the variables, relations, logrelations and models are
	used to obtain the solver data associated with the compiled instances.
	4) Arrays and models are managed recursively with the two previous
	functions.
*/
static
void ProcessSolverWhens(struct w_when *when,struct Instance *i){
  struct gl_list_t *scratch;
  struct gl_list_t *wvars;
  struct gl_list_t *ref;
  struct gl_list_t *rels;
  struct gl_list_t *logrels;
  struct gl_list_t *whens;
  struct gl_list_t *diswhens;
  struct Set *ValueList;
  struct Instance *cur_inst;
  struct Case *cur_case;
  struct solver_ipdata *ip;
  struct dis_discrete *dvar;
  struct rel_relation *rel;
  struct logrel_relation *lrel;
  struct w_when *w;
  struct when_case *cur_sol_case;
  int c,r,len,lref;
  int *value;

  scratch = GetInstanceWhenVars(i);
  len = gl_length(scratch);
  wvars = gl_create(len);
  when->dvars = wvars;
  for(c=1;c<=len;c++){
    cur_inst = (struct Instance *)(gl_fetch(scratch,c));
    ip = SIP(GetInterfacePtr(cur_inst));
    dvar = ip->u.dv.data;
    if(dis_whens_list(dvar)==NULL){
      diswhens = gl_create(2L);
      dis_set_whens_list(dvar,diswhens);
    }else{
      diswhens = dis_whens_list(dvar);
    }
    gl_append_ptr(diswhens,when);
    gl_append_ptr(when->dvars,dvar);
  }

  scratch = GetInstanceWhenCases(i);
  len = gl_length(scratch);
  when->cases = gl_create(len);
  for (c=1;c<=len;c++) {
    cur_sol_case = when_case_create(NULL);
    cur_case = (struct Case *)(gl_fetch(scratch,c));
    ValueList = GetCaseValues(cur_case);
    value = &(cur_sol_case->values[0]);
    if(g_symbol_values_list == NULL) {
      g_symbol_values_list = gl_create(2L);
    }
    ProcessValueList(ValueList,value,g_symbol_values_list);
    ref = GetCaseReferences(cur_case);
    lref = gl_length(ref);
    rels = gl_create(lref);  /* maybe allocating less than needed (models) */
    logrels = gl_create(lref);  /* maybe allocating less than needed */
    whens = gl_create(lref); /* maybe allocating more than needed */
    for(r=1;r<=lref;r++){
      cur_inst = (struct Instance *)(gl_fetch(ref,r));
      switch(InstanceKind(cur_inst)){
      case REL_INST:
        ip = SIP(GetInterfacePtr(cur_inst));
		ip->u.r.active = 0;
        rel = ip->u.r.data;
        gl_append_ptr(rels,rel);
	break;
      case LREL_INST:
        ip = SIP(GetInterfacePtr(cur_inst));
	ip->u.lr.active = 0;
        lrel = ip->u.lr.data;
        gl_append_ptr(logrels,lrel);
	break;
      case WHEN_INST:
        ip = SIP(GetInterfacePtr(cur_inst));
        w = ip->u.w.data;
        gl_append_ptr(whens,w);
        when_set_inwhen(w,TRUE);
	break;
      case MODEL_INST:
	ProcessModelsInWhens(cur_inst,rels,logrels,whens);
        break;
      default:
	break;
      }
    }
    when_case_set_rels_list(cur_sol_case,rels);
    when_case_set_logrels_list(cur_sol_case,logrels);
    when_case_set_whens_list(cur_sol_case,whens);
    when_case_set_active(cur_sol_case,FALSE);
    gl_append_ptr(when->cases,cur_sol_case);
  }
}

/*------------------------------------------------------------------------------
  RECONFIGURATION OF CONDITIONAL MODELS
*/

/**
	Is this (discrete) variable inside a WHEN?

	@return 
		1 if discrete var is a member of the when var list, 
		else 0

	@DEPRECATED we want to get rid of this in order to clean up the
	solver interface (divorce it from dependencies on compiler)
*/
int dis_var_in_a_when(struct Instance *var, struct w_when *when){
  struct Instance *winst;

  winst = (struct Instance *)(when_instance(when));
  return VarFoundInWhen(var,winst);
}


/**
	Determine if the conditional variable inst is part of the
	variable list of some when in the when list.

	@DEPRECATED we want to get rid of this in order to clean up the
	solver interface (divorce it from dependencies on compiler)
*/
int varinst_found_in_whenlist(slv_system_t sys, struct Instance *inst){
  struct w_when **whenlist;
  struct w_when *when;
  int c;

  whenlist = slv_get_solvers_when_list(sys);
  for (c=0; whenlist[c]!=NULL; c++) {
    when = whenlist[c];
    if(dis_var_in_a_when(inst,when)) {
      return 1;
    }
  }
  return 0;
}

/*------------------------------------------------------------------------------
  BOUNDARY PROCESSING
*/

/**
	Get the list or logrelation including a boundary (by means of a
	SATISFIED term). This function look the structures in the compiler
	side and make the same link in the solver side
*/
static
void GetListOfLogRels(struct bnd_boundary *bnd, struct Instance *inst){
  struct gl_list_t *logrels;
  unsigned long c,len;
  struct Instance *i;
  struct logrel_relation *lrel;
  struct solver_ipdata *lrip;

  len = LogRelationsCount(inst);

  if(len>0) {
    logrels = gl_create(len);
    for (c=1; c<=len; c++) {
      i = LogRelationsForInstance(inst,c);
      lrip = SIP(GetInterfacePtr(i));
      lrel = lrip->u.lr.data;
      gl_append_ptr(logrels,lrel);
    }
    bnd_set_logrels(bnd,logrels);
  }
  return;
}

/**
	Get the tolerance used to define the satisfaction of a boundary
	(Defined in the SATISFIED term)
*/
static
void GetTolerance(struct bnd_boundary *bnd){
  struct gl_list_t *logrels;
  unsigned long c,len;
  struct logrel_relation *lrel;
  struct Instance *i,*rel;
  double tolerance;

  rel = (struct Instance *)(rel_instance(bnd_rel(bnd_real_cond(bnd))));
  logrels = bnd_logrels(bnd);
  len = gl_length(logrels);
  for (c=1; c<=len; c++) {
    lrel = (struct logrel_relation *)(gl_fetch(logrels,c));
    i = (struct Instance *)(logrel_instance(lrel));
    if(FindTolInSatTermOfLogRel(i,rel,&tolerance )) {
      bnd_set_tolerance(bnd,tolerance);
      return;
    }
  }
}

/*----------------------------------------------------------------------------*/

/**
	Here we roll the master lists and bridge data into relation/var/
	logrelation/conditional/when etc. lists for the consumer.
	Includes fixing up rel caches and initing flagbits as best we can.
	includes setting master indices on rel/var/logrel/when etc.
	
	@returns 0 on success, 1 on out-of-memory, or 2 if the problem does not
	contain at least one variable in one equation
*/
static
int analyze_make_solvers_lists(struct problem_t *p_data){
  CONST struct relation *gut;
  CONST struct logrelation *lgut;
  struct Instance *i;
  struct Instance *i_r;
  struct solver_ipdata *rip = NULL, *vip;
  struct solver_ipdata *lrip, *dvip, *wip;
  struct var_variable **incidence = NULL;
  struct rel_relation **varincidence = NULL;
  struct dis_discrete **logincidence = NULL;
  struct var_variable *var;
  struct rel_relation *rel;
  struct dis_discrete *dvar;
  struct logrel_relation *lrel;
  struct bnd_boundary *bnd;
  struct w_when *when;
  int order,nnzold;
  int logorder,lognnzold;
  int c,len,v,vlen,r,found;
  uint32 flags;

  order = MAX(gl_length(p_data->vars),gl_length(p_data->rels));
  nnzold = p_data->nnz = p_data->nnztot
         = p_data->nnzobj = p_data->nnzcond = 0;
  p_data->nrow = 0; /* number of included relations */
  /* loop through the relations we found */
  for (c=1,len = gl_length(p_data->rels); c <= len; c++) {
    rip = SIP(gl_fetch(p_data->rels,c));
    gut = GetInstanceRelationOnly(rip->i);
	/* nnztot includes all vars in all relations */
    vlen = NumberVariables(gut);
    p_data->nnztot += vlen;
    if(rip->u.r.included) {
      p_data->nrow++;
      nnzold = p_data->nnz;
      for( v = 1 ; v <= vlen; v++ ) {
        i = RelationVariable(gut,v);
        vip = SIP(GetInterfacePtr(i));
        if(!(vip->u.v.fixed)) {
		  /* nnz includes only FREE vars in INCLUDE relations */
          p_data->nnz++;
        }
      }
      if(p_data->nnz==nnzold){
        ERROR_REPORTER_START_NOLINE(ASC_USER_WARNING);
		FPRINTF(ASCERR,"No free variables found in included relation '");
        WriteInstanceName(ASCERR,rip->i,p_data->root);
		FPRINTF(ASCERR,"'");
		error_reporter_end_flush();
      }
    }
  }

  /* count the objective relations */
  for (c=1,len = gl_length(p_data->objrels); c <= len; c++) {
    rip = SIP(gl_fetch(p_data->objrels,c));
    gut = GetInstanceRelationOnly(rip->i);
    vlen = NumberVariables(gut);
    p_data->nnzobj += vlen;
  }

  /* count the conditional relations */
  for (c=1,len = gl_length(p_data->cnds); c <= len; c++) {
    rip = SIP(gl_fetch(p_data->cnds,c));
    gut = GetInstanceRelationOnly(rip->i);
    vlen = NumberVariables(gut);
    p_data->nnzcond += vlen;
  }

  /* count ncol, the number of non-FIXED incident variables. note that
	non-incident variables are already taken out, in the unas list. */
  for (c=1,len = gl_length(p_data->vars); c <= len; c++) {
    vip = SIP(gl_fetch(p_data->vars,c));
    if(!(vip->u.v.fixed)) p_data->ncol++;
  }

  /*
  	now, at last we have cols jacobian in the order we want the lists to
  	be handed to the solvers.
  */

  /* count the incidences of bool vars and logrels */
  logorder = MAX((unsigned long)p_data->lrelinc,gl_length(p_data->logrels));
  lognnzold = p_data->lognnz = p_data->lrelincsize = 0;
  p_data->lognrow = 0; /* number of included logrelations */
  for (c=1,len = gl_length(p_data->logrels); c <= len; c++) {
    lrip = SIP(gl_fetch(p_data->logrels,c));
    lgut = GetInstanceLogRelOnly(lrip->i);
	/* lrelincsize includes all boolean vars in all logical relations */
    vlen = NumberBoolVars(lgut);
    p_data->lrelincsize += vlen;
    if(lrip->u.lr.included) {
      p_data->lognrow++;
      lognnzold = p_data->lognnz;
      for( v = 1 ; v <= vlen; v++ ) {
        i = LogRelBoolVar(lgut,v);
        dvip = SIP(GetInterfacePtr(i));
        if(!(dvip->u.dv.fixed)) {
		  /* lognnz includes only FREE bools from INCLUDED logrels */
          p_data->lognnz++;
        }
      }
      if(p_data->lognnz==lognnzold) {
        ERROR_REPORTER_START_NOLINE(ASC_PROG_WARNING);
		FPRINTF(ASCERR,"No free boolean variables in included logrelation:");
        WriteInstanceName(ASCWAR,rip->i,p_data->root);
		error_reporter_end_flush();
      }
    }
  }

  /* count conditional logrels */
  for (c=1,len = gl_length(p_data->logcnds); c <= len; c++) {
    lrip = SIP(gl_fetch(p_data->logcnds,c));
    lgut = GetInstanceLogRelOnly(lrip->i);
    vlen = NumberBoolVars(lgut);
    p_data->lrelincsize += vlen;
  }

  /* ensure that we have at least one var-in-a-rel, or at least one bool var
	in a logrel */
  if(!(p_data->nnztot+p_data->nnzobj) && !(p_data->lognnz)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR
		,"Problem should contain at least one variable and one relation."
	);
    return 2;
  }

  /* calculate the number of free and incident boolean variables, logncol */
  for (c=1,len = p_data->lrelinc; c <= len; c++) {
    dvip = SIP(gl_fetch(p_data->dvars,c));
    if(!(dvip->u.dv.fixed)) p_data->logncol++;
  }

  /* allocate space for the lists that we will eventually hand over to slv.
	now malloc and build things, remember to punt the matrix soon.
  	remember we must NEVER free these things individually. */

#define ALLOC_OR_NULL(T,N) ((N) > 0 ? ASC_NEW_ARRAY(T,N) : (T*)NULL)

#define AL(P,N,T) p_data->P##data = ALLOC_OR_NULL(struct T,p_data->N)
  AL(var,nv,var_variable);  AL(par,np,var_variable);  AL(un,nu,var_variable);  
  AL(dis,ndv,dis_discrete);  AL(undis,nud,dis_discrete);
  AL(rel,nr,rel_relation);  AL(obj,no,rel_relation);  AL(con,nc,rel_relation);
  AL(lr,nl,logrel_relation);  AL(logcon,ncl,logrel_relation);
  AL(when,nw,w_when);
  AL(bnd,nc + p_data->ncl,bnd_boundary);
#undef AL

#define AL(P,N,T) p_data->master##P=ALLOC_OR_NULL(struct T*,p_data->N + 1)
  AL(vl,nv,var_variable);  AL(pl,np,var_variable);  AL(ul,nu,var_variable);  
  AL(dl,ndv,dis_discrete);  AL(dul,nud,dis_discrete);
  AL(rl,nr,rel_relation);  AL(ol,no,rel_relation);  AL(cl,nc,rel_relation);
  AL(ll,nl,logrel_relation);  AL(cll,ncl,logrel_relation);
  AL(wl,nw,w_when);
  AL(bl,nc + p_data->ncl,bnd_boundary);
#undef AL

#define AL(P,N,T) p_data->solver##P=ALLOC_OR_NULL(struct T*,p_data->N + 1)
  AL(vl,nv,var_variable);  AL(pl,np,var_variable);  AL(ul,nu,var_variable);  
  AL(dl,ndv,dis_discrete);  AL(dul,nud,dis_discrete);
  AL(rl,nr,rel_relation);  AL(ol,no,rel_relation);  AL(cl,nc,rel_relation);
  AL(ll,nl,logrel_relation);  AL(cll,ncl,logrel_relation);
  AL(wl,nw,w_when);
  AL(bl,nc + p_data->ncl,bnd_boundary);
#undef AL

  p_data->relincidence = ALLOC_OR_NULL(struct var_variable*
    ,p_data->nnztot + p_data->nnzobj + p_data->nnzcond
  );

  p_data->logrelinciden = ALLOC_OR_NULL(
    struct dis_discrete *
    ,p_data->lrelincsize
  );

  /* check that lists have been allocated whereever number of expected elements > 0 */
#define C(N,P) if((p_data->N)>0 && (p_data->P##data)==NULL)return 1
  C(nv,var);   C(np,par);  C(nu,un);
  C(ndv,dis);  C(nud,undis);
  C(nr,rel);  C(no,obj);  C(nc,con);
  C(nl,lr);  C(ncl,logcon);
  C(nw,when);
  C(nc + p_data->ncl,bnd);
#undef C

  /* check that all master lists were assigned */
#define C(P) if((p_data->master##P)==NULL)return 1
  C(vl);  C(pl);  C(ul);
  C(dl);  C(dul);
  C(rl);  C(ol);  C(cl);
  C(ll);  C(cll);
  C(wl);
  C(bl);
#undef C

  /* check that all the solver's lists were assigned */
#define C(P) if((p_data->solver##P)==NULL)return 1
  C(vl);  C(pl);  C(ul);
  C(dl);  C(dul);
  C(rl);  C(ol);  C(cl);
  C(ll);  C(cll);
  C(wl);
  C(bl);
#undef C

  if(p_data->relincidence==NULL)return 1;
  if(p_data->lrelincsize > 0 && p_data->logrelinciden==NULL) return 1;

  p_data->relincsize = p_data->nnztot+p_data->nnzobj + p_data->nnzcond;
  p_data->relincinuse = 0;
  p_data->lrelincinuse = 0;

  /*-------*/
  /*
  	for c in varlist copy vardata. remember gllist # from 1 and data from 0
  */
  /*
  	for c in varlist set mastervl, solvervl pointer to point to data
  */
  vlen = gl_length(p_data->vars);
  for (v = 0; v < vlen; v++) {
	/* CONSOLE_DEBUG("VAR %d",v); */
    var = &(p_data->vardata[v]);
    vip = SIP(gl_fetch(p_data->vars,v+1));
    vip->u.v.data = var;
    var_set_instance(var,vip->i);
    var_set_mindex(var,v);
    var_set_sindex(var,v);
    flags = 0; /* all init to FALSE */
    /* turn on appropriate ones */
    if(vip->u.v.incident)  flags |= VAR_INCIDENT;
    if(vip->u.v.in_block)  flags |= VAR_INBLOCK;
    if(vip->u.v.fixed)     flags |= VAR_FIXED;
    if(!vip->u.v.basis)    flags |= VAR_NONBASIC;
    if(vip->u.v.solvervar) flags |= VAR_SVAR;
    if(vip->u.v.deriv > 1) flags |= VAR_DERIV; /* so that we can do relman_diffs with just the ydot vars */

    var_set_flags(var,flags);
    p_data->mastervl[v] = var;
    p_data->solvervl[v] = var;
  }
  p_data->mastervl[vlen] = NULL; /* terminator */
  p_data->solvervl[vlen] = NULL; /* terminator */
  /*
  	for c in parlist copy pardata. remember gllist # from 1 and data from 0
  	for c in parlist set masterpl, solverpl pointer to point to data
  */
  vlen = gl_length(p_data->pars);
  for (v = 0; v < vlen; v++) {
    var = &(p_data->pardata[v]);
    vip = SIP(gl_fetch(p_data->pars,v+1));
    vip->u.v.data = var;
    var_set_instance(var,vip->i);
    var_set_mindex(var,v);
    var_set_sindex(var,v);
    flags = 0; /* all init to FALSE */
    /* turn on appropriate ones */
    if(vip->u.v.incident)  flags |= VAR_INCIDENT;
    if(vip->u.v.in_block)  flags |= VAR_INBLOCK;
    if(vip->u.v.fixed)     flags |= VAR_FIXED;
    if(vip->u.v.solvervar) flags |= VAR_SVAR; /* shouldn't this be here? */
    var_set_flags(var,flags);
    p_data->masterpl[v] = var;
    p_data->solverpl[v] = var;
  }
  p_data->masterpl[vlen] = NULL; /* terminator */
  p_data->solverpl[vlen] = NULL; /* terminator */
  /*
  	for c in unalist copy undata. remember gllist # from 1 and data from 0
  	for c in unalist set masterul, solverul pointer to point to data
  */
  vlen = gl_length(p_data->unas);
  for (v = 0; v < vlen; v++) {
    var = &(p_data->undata[v]);
    vip = SIP(gl_fetch(p_data->unas,v+1));
    vip->u.v.data = var;
    var_set_instance(var,vip->i);
    var_set_mindex(var,v);
    var_set_sindex(var,v);
    flags = 0; /* all init to FALSE */
    /* turn on appropriate ones */
    if(vip->u.v.incident)  flags |= VAR_INCIDENT;
    if(vip->u.v.fixed)     flags |= VAR_FIXED;
    if(vip->u.v.solvervar) flags |= VAR_SVAR;
	/* CONSOLE_DEBUG("VAR AT %p IS UNASSIGNED",var); */
    /* others may be appropriate (PVAR) */
    var_set_flags(var,flags);
    p_data->masterul[v] = var;
    p_data->solverul[v] = var;
  }
  p_data->masterul[vlen] = NULL; /* terminator */
  p_data->solverul[vlen] = NULL; /* terminator */

  /*
  	process the constraining relations
  	for v in rellist copy reldata and fix extrels.
  */
  vlen = gl_length(p_data->rels);
  for (v = 0; v < vlen; v++) {
    rel = &(p_data->reldata[v]);
    rip = SIP(gl_fetch(p_data->rels,v+1));
    rel = rel_create(rip->i,rel);
    rip->u.r.data = rel;
    rel_set_mindex(rel,v);
    rel_set_sindex(rel,v);
    rel_set_model(rel,rip->u.r.model-1);
    /* here set up the var list */
    gut = GetInstanceRelationOnly(rip->i);
    assert(gut!=NULL);
    len = NumberVariables(gut);
	/* CONSOLE_DEBUG("FOR REL INSTANCE AT %p, NUM INCID VARS= %d",rip->i, len); */
    if(len > 0) {
      incidence = get_incidence_space(len,p_data);
      for( c = 0; c < len; c++ ) {
        i = RelationVariable(gut,c+1);
        /* CONSOLE_DEBUG("INCIDENCE OF REL INSTANCE %p WITH VAR %p",rip->i,i); */
        incidence[c] = SIP(GetInterfacePtr(i))->u.v.data;
      }
      /* CONSOLE_DEBUG("SETTING INCIDENCES FOR CONSTRAINING REL"); */
      rel_set_incidences(rel,len,incidence);
    }else{
      rel_set_incidences(rel,0,NULL);
    }
    flags = 0; /* all init to FALSE */
    /* TURN ON APPROPRIATE ONES */
    if(rip->u.r.included) flags |= (REL_INCLUDED | REL_INBLOCK);
    if(rip->u.r.ext) flags |= REL_BLACKBOX;
    if(rip->u.r.active) flags |= ( REL_ACTIVE | REL_INVARIANT);
    if(rip->u.r.inwhen) flags |= REL_INWHEN;
    if( RelationRelop(GetInstanceRelationOnly(rip->i)) == e_equal ) {
      flags |= REL_EQUALITY;
    }
    rel_set_flags(rel,flags);
    /* for c in rellist set masterrl, solverrl pointer to point to data */
    p_data->masterrl[v] = rel;
    p_data->solverrl[v] = rel;
  }
  p_data->masterrl[vlen] = NULL; /* terminator */
  p_data->solverrl[vlen] = NULL; /* terminator */

/*
	for c in objlist copy objdata.
	for c in objlist set masterrl, solverrl pointer to point to data.
*/
  /*
  	process the objective relations
  	for v in objlist copy objdata
  */
  vlen = gl_length(p_data->objrels);
  found = 0;
  for (v = 0; v < vlen; v++) {
    rel = &(p_data->objdata[v]);
    rip = SIP(gl_fetch(p_data->objrels,v+1));
    rel = rel_create(rip->i,rel);
    rip->u.r.data = rel;
    rel_set_mindex(rel,v);
    rel_set_sindex(rel,v);
    rel_set_model(rel,rip->u.r.model-1);
    /* here set up the var list */
    gut = GetInstanceRelationOnly(rip->i);
    assert(gut!=NULL);
    len = NumberVariables(gut);
    if(len > 0) {
      incidence = get_incidence_space(len,p_data);
      for( c = 0; c < len; c++ ) {
        i = RelationVariable(gut,c+1);
        incidence[c] = SIP(GetInterfacePtr(i))->u.v.data;
      }
      /* CONSOLE_DEBUG("SETTING INCIDENCES FOR OBJECTIVE FN REL"); */
      rel_set_incidences(rel,len,incidence);
    }else{
      rel_set_incidences(rel,0,NULL);
    }
    /* black box objectives not supported. skip it */
    flags = 0; /* all init to FALSE */
    /* TURN ON APPROPRIATE ONES  */
    if(rip->u.r.included) {
      flags |= (REL_INCLUDED | REL_INBLOCK | REL_ACTIVE);
    }
    if(rip->u.r.obj < 0) flags |= REL_OBJNEGATE;
    rel_set_flags(rel,flags);
    /* for c in objrellist set masterol, solverol pointer to point to data */
    p_data->masterol[v] = rel;
    p_data->solverol[v] = rel;
    /* set objective to first included objective on list */
    if(!found && (rip->u.r.included)) {
      p_data->obj = rel;
      found = 1;
    }
  }
  p_data->masterol[vlen] = NULL; /* terminator */
  p_data->solverol[vlen] = NULL; /* terminator */

  /*
  	process the conditional relations
  	for v in cndlist copy conddata .
  */
  vlen = gl_length(p_data->cnds);
  for (v = 0; v < vlen; v++) {
    rel = &(p_data->condata[v]);
    rip = SIP(gl_fetch(p_data->cnds,v+1));
    rel = rel_create(rip->i,rel);
    rip->u.r.data = rel;
    rel_set_mindex(rel,v);
    rel_set_sindex(rel,v);
    rel_set_model(rel,rip->u.r.model-1);
    gut = GetInstanceRelationOnly(rip->i);
    assert(gut!=NULL);
    len = NumberVariables(gut);
    if(len > 0) {
      incidence = get_incidence_space(len,p_data);
      for( c = 0; c < len; c++ ) {
        i = RelationVariable(gut,c+1);
        incidence[c] = SIP(GetInterfacePtr(i))->u.v.data;
      }
      /* CONSOLE_DEBUG("SETTING INCIDENCES FOR CONDITIONAL REL"); */
      rel_set_incidences(rel,len,incidence);
    }else{
      rel_set_incidences(rel,0,NULL);
    }
    flags = 0; /* all init to FALSE */
    /* TURN ON APPROPRIATE ONES */
    if(rip->u.r.included) {
      flags |= (REL_INCLUDED | REL_INBLOCK | REL_ACTIVE);
    }
    if(rip->u.r.cond) flags |= REL_CONDITIONAL;
    if( RelationRelop(GetInstanceRelationOnly(rip->i)) == e_equal ) {
      flags |= REL_EQUALITY;
    }
    rel_set_flags(rel,flags);
    /* for c in rellist set masterrl, solverrl pointer to point to data */
    p_data->mastercl[v] = rel;
    p_data->solvercl[v] = rel;
    /* initially in same order */
  }
  p_data->mastercl[vlen] = NULL; /* terminator */
  p_data->solvercl[vlen] = NULL; /* terminator */

  /*-------*/

  /*
  	process discrete variables
  	for c in dvarlist copy disdata. gllist # from 1 and data from 0
  	for c in dvarlist set masterdl, solverdl pointer to point to data
  */
  vlen = gl_length(p_data->dvars);
  for (v = 0; v < vlen; v++) {
    dvar = &(p_data->disdata[v]);
    dvip = SIP(gl_fetch(p_data->dvars,v+1));
    /*
      dvip->u.dv.data = dvar;
      dis_set_instance(dvar,dvip->i); */
    /* from here */
    dis_create(dvip->i,dvar);
    dvip->u.dv.data = dvar;
    /* to here */
    dis_set_mindex(dvar,v);
    dis_set_sindex(dvar,v);
    dis_set_value(dvar,dvip->u.dv.value);
    dis_set_previous_value(dvar,dvip->u.dv.value);
    switch (dvip->u.dv.distype) {
      case 0:
        dis_set_kind(dvar,e_dis_boolean_t);
        break;
      case 1:
        dis_set_kind(dvar,e_dis_integer_t);
        break;
      case -1:
	dis_set_kind(dvar,e_dis_symbol_t);
        break;
      default:
	break;
    }
    flags = 0; /* all init to FALSE */
    /* turn on appropriate ones */
    if(dvip->u.dv.isconst)  flags |= DIS_CONST;
    if(dvip->u.dv.incident)  flags |= DIS_INCIDENT;
    if(dvip->u.dv.inwhen)  flags |= DIS_INWHEN;
    if(dvip->u.dv.fixed)     flags |= DIS_FIXED;
    if(dvip->u.dv.booleanvar) flags |= DIS_BVAR;
    if(dis_kind(dvar) == e_dis_boolean_t) flags |= DIS_BOOLEAN;
    dis_set_flags(dvar,flags);
    p_data->masterdl[v] = dvar;
    p_data->solverdl[v] = dvar;
    /* initially master and solver look the same */
  }
  p_data->masterdl[vlen] = NULL; /* terminator */
  p_data->solverdl[vlen] = NULL; /* terminator */

  /*
  	for c in dunalist copy undisdata. gllist # from 1 and data from 0
  	for c in dunalist set masterdul, solverdul pointer to point to data
  */
  vlen = gl_length(p_data->dunas);
  for (v = 0; v < vlen; v++) {
    dvar = &(p_data->undisdata[v]);
    dvip = SIP(gl_fetch(p_data->dunas,v+1));
    dis_create(dvip->i,dvar);
    dvip->u.dv.data = dvar;
    dis_set_mindex(dvar,v);
    dis_set_sindex(dvar,v);
    flags = 0; /* all init to FALSE */
    /* turn on appropriate ones */
    if(dvip->u.dv.fixed)     flags |= DIS_FIXED;
    if(dvip->u.dv.booleanvar) flags |= DIS_BVAR;
    dis_set_flags(dvar,flags);
    p_data->masterdul[v] = dvar;
    p_data->solverdul[v] = dvar;
  }
  p_data->masterdul[vlen] = NULL; /* terminator */
  p_data->solverdul[vlen] = NULL; /* terminator */


/*
	for c in logrellist copy lrdata.
	for c in logrellist set masterll, solverll pointer to point to data.
*/
  /*
  	process the logical relations
  	for v in logrellist copy lrdata
  */
  vlen = gl_length(p_data->logrels);
  for (v = 0; v < vlen; v++) {
    lrel = &(p_data->lrdata[v]);
    lrip = SIP(gl_fetch(p_data->logrels,v+1));
    lrel = logrel_create(lrip->i,lrel);
    lrip->u.lr.data = lrel;
    logrel_set_mindex(lrel,v);
    logrel_set_sindex(lrel,v);
    logrel_set_model(lrel,lrip->u.lr.model-1);
    /* here set up the dis var list */
    lgut = GetInstanceLogRelOnly(lrip->i);
    assert(lgut!=NULL);
    len = NumberBoolVars(lgut);
    if(len > 0) {
      logincidence = get_logincidence_space(len,p_data);
      for( c = 0; c < len; c++ ) {
        i = LogRelBoolVar(lgut,c+1);
        logincidence[c] = SIP(GetInterfacePtr(i))->u.dv.data;
      }
      logrel_set_incidences(lrel,len,logincidence);
    }else{
      logrel_set_incidences(lrel,0,NULL);
    }
    flags = 0; /* all init to FALSE */
    /* TURN ON APPROPRIATE ONES  */
    if(lrip->u.lr.included) flags |= LOGREL_INCLUDED;
    if(lrip->u.lr.active) flags |= LOGREL_ACTIVE;
    if(lrip->u.lr.inwhen) flags |= LOGREL_INWHEN;
    if( LogRelRelop(GetInstanceLogRelOnly(lrip->i)) == e_boolean_eq ) {
      flags |= LOGREL_EQUALITY;
    }
    logrel_set_flags(lrel,flags);
    /* for c in logrellist set masterll, solverll pointer to point to data */
    p_data->masterll[v] = lrel;
    p_data->solverll[v] = lrel;
  }
  p_data->masterll[vlen] = NULL; /* terminator */
  p_data->solverll[vlen] = NULL; /* terminator */


  /*
  	process the conditional logrelations
  	for v in logcndlist copy logconddata
  */
  vlen = gl_length(p_data->logcnds);
  for (v = 0; v < vlen; v++) {
    lrel = &(p_data->logcondata[v]);
    lrip = SIP(gl_fetch(p_data->logcnds,v+1));
    lrel = logrel_create(lrip->i,lrel);
    lrip->u.lr.data = lrel;
    logrel_set_mindex(lrel,v);
    logrel_set_sindex(lrel,v);
    logrel_set_model(lrel,lrip->u.lr.model-1);
    lgut = GetInstanceLogRelOnly(lrip->i);
    assert(lgut!=NULL);
    len = NumberBoolVars(lgut);
    if(len > 0) {
      logincidence = get_logincidence_space(len,p_data);
      for( c = 0; c < len; c++ ) {
        i = LogRelBoolVar(lgut,c+1);
        logincidence[c] = SIP(GetInterfacePtr(i))->u.dv.data;
      }
      logrel_set_incidences(lrel,len,logincidence);
    }else{
      logrel_set_incidences(lrel,0,NULL);
    }
    flags = 0; /* all init to FALSE */
    /* TURN ON APPROPRIATE ONES */
    if(lrip->u.lr.included) flags |= (LOGREL_INCLUDED | LOGREL_ACTIVE);
    if(lrip->u.lr.cond) flags |= LOGREL_CONDITIONAL;
    if( LogRelRelop(GetInstanceLogRelOnly(lrip->i)) == e_boolean_eq) {
      flags |= LOGREL_EQUALITY;
    }
    logrel_set_flags(lrel,flags);
    /* for c in lrellist set masterll, solverll pointer to point to data */
    p_data->mastercll[v] = lrel;
    p_data->solvercll[v] = lrel;
    /* initially in same order */
  }
  p_data->mastercll[vlen] = NULL; /* terminator */
  p_data->solvercll[vlen] = NULL; /* terminator */


  /*
  	process the boundaries
  	for v in cndlist and logcndlist, copy bnddata.
  */
  vlen = gl_length(p_data->cnds);
  len = gl_length(p_data->logcnds);
  /* real conditions  */
  for (v = 0; v < vlen; v++) {
    bnd = &(p_data->bnddata[v]);
    bnd = bnd_create(bnd);
    bnd_set_kind(bnd,e_bnd_rel);
    rip = SIP(gl_fetch(p_data->cnds,v+1));
    bnd_real_cond(bnd) = bnd_rel(rip->u.r.data);
    bnd_set_mindex(bnd,v);
    bnd_set_sindex(bnd,v);
    bnd_set_model(bnd,rip->u.r.model-1);
    flags = 0; /* all init to FALSE */
    flags |= BND_REAL;
    bnd_set_flags(bnd,flags);
    /* for c in lrellist set masterbl, solverbl pointer to point to data */
    p_data->masterbl[v] = bnd;
    p_data->solverbl[v] = bnd;
  }
  /* logical conditions */
  for (v = vlen; v <vlen+len; v++) {
    bnd = &(p_data->bnddata[v]);
    bnd = bnd_create(bnd);
    bnd_set_kind(bnd,e_bnd_logrel);
    lrip = SIP(gl_fetch(p_data->logcnds,v-vlen+1));
    bnd_log_cond(bnd) = bnd_logrel(lrip->u.lr.data);
    bnd_set_mindex(bnd,v);
    bnd_set_sindex(bnd,v);
    bnd_set_model(bnd,lrip->u.lr.model-1);
    flags = 0; /* all init to FALSE */
    bnd_set_flags(bnd,flags);
    /* for c in lrellist set masterbl, solverbl pointer to point to data */
    p_data->masterbl[v] = bnd;
    p_data->solverbl[v] = bnd;
  }
  p_data->masterbl[vlen+len] = NULL; /* terminator */
  p_data->solverbl[vlen+len] = NULL; /* terminator */

  /*
  	Finding list of logical relations using the condition, and the
  	tolerance (only for the case of real condition ). Defining some
  	flags
  */

  for (v = 0; v < vlen; v++) {
    bnd = p_data->masterbl[v];
    rel = bnd_rel(bnd_real_cond(bnd));
    flags = bnd_flags(bnd);
    if(rel_equality(rel)) {
      flags |= BND_EQUALITY;
    }
    i =  (struct Instance *)rel_instance(rel);
    GetListOfLogRels(bnd,i);
    if(bnd_logrels(bnd)!= NULL) {
      flags |= BND_IN_LOGREL;
      GetTolerance(bnd);
    }
    bnd_set_flags(bnd,flags);
  }
  for (v = vlen; v < vlen+len; v++) {
    bnd = p_data->masterbl[v];
    lrel = bnd_logrel(bnd_log_cond(bnd));
    flags = bnd_flags(bnd);
    if(logrel_equality(lrel)) {
      flags |= BND_EQUALITY;
    }
    i =  (struct Instance *)logrel_instance(lrel);
    GetListOfLogRels(bnd,i);
    if(bnd_logrels(bnd)!= NULL) {
      flags |= BND_IN_LOGREL;
    }
    bnd_set_flags(bnd,flags);
  }

/*
	for c in whenlist copy whendata.
	for c in whenllist set masterwl, solverwl pointer to point to data.
*/
  /* process whens */

  vlen = gl_length(p_data->whens);
  for (v = 0; v < vlen; v++) {
    when = &(p_data->whendata[v]);
    wip = SIP(gl_fetch(p_data->whens,v+1));
    when = when_create(wip->i,when);
    wip->u.w.data = when;
    when_set_mindex(when,v);
    when_set_sindex(when,v);
    when_set_model(when,wip->u.w.model-1);
    p_data->masterwl[v] = when;
    p_data->solverwl[v] = when;
    flags = 0;
    if(wip->u.w.inwhen) flags |= WHEN_INWHEN;
    when_set_flags(when,flags);
  }
  p_data->masterwl[vlen] = NULL; /* terminator */
  p_data->solverwl[vlen] = NULL; /* terminator */

  /*
  	Get data from the when instance to fill the
  	list in the w_when instance
  */

  for (v = 0; v < vlen; v++) {
    when = p_data->masterwl[v];
    i = (struct Instance *)(when_instance(when));
    ProcessSolverWhens(when,i);
  }

  /* configure the problem */

  if(vlen > 0) { /* we have whens */
    configure_conditional_problem(vlen,p_data->masterwl,
                                  p_data->solverrl,p_data->solverll,
				  p_data->mastervl);
    /* Is consistency analysis required ? */
    p_data->need_consistency = 0;
    for (v = 0; v < vlen; v++) {
      when = p_data->masterwl[v];
      if(when_changes_structure(when)) {
        p_data->need_consistency = 1;
        break;
      }
    }

#if DEBUG_ANALYSIS
    if( p_data->need_consistency == 0 ) {
      CONSOLE_DEBUG("All alternativeS HAVE THE SAME STRUCTURE: Consistency"
		" analysis is not required \n"
	  );
    }else{
      CONSOLE_DEBUG("Consistency analysis may be required \n");
    }
#endif /* DEBUG_ANALYSIS  */

  }else{

    /*
    	All variables in active relations are set as active.
    	This is necessary because the existence of some variables
    	in conditional relations which should not be active.

    	Before we were doing:

    	 for (v = 0;p_data->solvervl[v]!=NULL ; v++) {
    	   var = p_data->solvervl[v];
    	   var_set_active(var,TRUE);
    	 }

    	 for (v = 0;p_data->solverdl[v]!=NULL ; v++) {
    	   dvar = p_data->solverdl[v];
    	   dis_set_active(dvar,TRUE);
    	 }

    	which do not considerate such situation
    */

     set_active_vars_in_active_rels(p_data->solverrl);
     set_active_vars_in_active_rels(p_data->solverol);
     set_active_disvars_in_active_logrels(p_data->solverll);

    /*
    	All the unattached are set active to keep an accurate
    	counting in the solver side.
    */

    for (v = 0;p_data->solverul[v]!=NULL ; v++) {
      var = p_data->solverul[v];
      var_set_active(var,TRUE);
    }

    for (v = 0;p_data->solverdul[v]!=NULL ; v++) {
      dvar = p_data->solverdul[v];
      dis_set_active(dvar,TRUE);
    }
  }

  /*
  	Code to make the variables aware of the relation they are
  	incident in. KHT
  */
  vlen = gl_length(p_data->vars);
  for (v = 0; v < vlen; v++) {
    var = &(p_data->vardata[v]);
    i = var_instance(var);
    len = RelationsCount(i);
    p_data->varincsize += len;
  }

  p_data->varincidence = ALLOC_OR_NULL(struct rel_relation*, p_data->varincsize);

  vlen = gl_length(p_data->vars);
  for (v = 0; v < vlen; v++) {
    var = &(p_data->vardata[v]);
    i = var_instance(var);
    len = RelationsCount(i);
    r = 0;
    if(len > 0) {
      varincidence = get_var_incidence_space(len,p_data);
      for( c = 1; c <= len; c++ ) {
        i_r = RelationsForAtom(i,c);
	if( i_r == rel_instance(GetInterfacePtr(i_r)) ) {
	  varincidence[r] = SIP(GetInterfacePtr(i_r))->u.r.data;
	  r++;
	}
      }
    }
    if(r > 0) {
      var_set_incidences(var,r,varincidence);
    }else{
      var_set_incidences(var,0,NULL);
    }
  }
  return 0;
}


/**
	Pass the array of stuff into the slv_system_t, and disown them. This is
	where the analyse step finishes up and the solver work starts.

	Also makes sure all solver_var have been assigned at least once,
	since 0 is a really stupid starting value.

	@TODO this code is really repetitive, can't we clean it up with some
	nice macros? The whole problem is that the naming isn't quite consistent
	enough. eg.
		slv_set_solvers_condlogrel_list(sys,p_data->solvercll,gl_length(p_data->logcnds));	
	could be better written
		slv_set_solvers_condlogrel_list(sys,p_data->solver_condlogrel,gl_length(p_data->condlogrel_list));

*/
static
int analyze_configure_system(slv_system_t sys,struct problem_t *p_data){

  /* do the slv_set_var_buf (etc) calls... */
#define SL(N,L) slv_set_##N##_buf(sys,p_data->L##data); p_data->L##data=NULL;

  SL(var,var);    SL(par,par);

  slv_set_dvar_buf(sys,p_data->disdata,gl_length(p_data->dvars));
  p_data->disdata = NULL;

  SL(rel,rel);    SL(condrel,con);  SL(obj,obj);
  SL(logrel,lr);  SL(condlogrel,logcon);

  slv_set_when_buf(sys,p_data->whendata,gl_length(p_data->whens));
  p_data->whendata = NULL;
  slv_set_bnd_buf(sys,p_data->bnddata, gl_length(p_data->cnds) + gl_length(p_data->logcnds));
  p_data->bnddata = NULL;

  SL(unattached,un);
  SL(disunatt,undis);

#undef SL

  /* set rel, var, logrel incidences... */
  slv_set_incidence(sys,p_data->relincidence,p_data->relincsize);
  p_data->relincidence = NULL;
  slv_set_var_incidence(sys,p_data->varincidence,p_data->varincsize);
  p_data->varincidence = NULL;
  slv_set_logincidence(sys,p_data->logrelinciden,p_data->lrelincsize);
  p_data->logrelinciden = NULL;
  slv_set_symbol_list(sys,g_symbol_values_list);
  g_symbol_values_list = NULL;

  /* do the slv_set_master_var_list (etc) calls... */
#define SL(N,L,L2) slv_set_master_##N##_list(sys,p_data->master##L,gl_length(p_data->L2)); p_data->master##L=NULL;
  SL(var,vl,vars);  SL(par,pl,pars);  SL(dvar,dl,dvars);
  SL(rel,rl,rels);  SL(condrel,cl,cnds);  SL(obj,ol,objrels);
  SL(logrel,ll,logrels);  SL(condlogrel,cll,logcnds);  SL(when,wl,whens);

  slv_set_master_bnd_list(sys,p_data->masterbl, gl_length(p_data->cnds) + gl_length(p_data->logcnds));
  p_data->masterbl = NULL;

  SL(unattached,ul,unas);  SL(disunatt,dul,dunas);
#undef SL

  /* do the slv_set_solvers_var_list (etc) calls... */
#define SL(N,L,L2) slv_set_solvers_##N##_list(sys,p_data->solver##L,gl_length(p_data->L2)); p_data->solver##L=NULL;
  SL(var,vl,vars);  SL(par,pl,pars);  SL(dvar,dl,dvars);
  SL(rel,rl,rels);  SL(condrel,cl,cnds);  SL(obj,ol,objrels);
  SL(logrel,ll,logrels);  SL(condlogrel,cll,logcnds);  SL(when,wl,whens);

  slv_set_solvers_bnd_list(sys,p_data->solverbl, gl_length(p_data->cnds) + gl_length(p_data->logcnds));
  p_data->solverbl = NULL;

  SL(unattached,ul,unas);  SL(disunatt,dul,dunas);
#undef SL

  /* objective relation, if found... */
  slv_set_obj_relation(sys,p_data->obj);
  p_data->obj = NULL;

  /* and finally... */
  slv_set_num_models(sys,p_data->nm);
  slv_set_need_consistency(sys,p_data->need_consistency);

  PopInterfacePtrs(p_data->oldips,NULL,NULL);
  p_data->oldips = NULL;
  return 0;
}

/*----------------------------------------------------------------------------*/
/*
	This is the entry point for problem analysis. It takes the compiler
	Instance, visits it with the 'CountStuffInTree' method, then constructs
	lists of all the different types of variables and relations etc, then
	convey them to the solver using the slv_ interface (i.e. methods that act
	on the slv_system_t we've been given).

	We're also establishing any any protocols needed to communicate with the
	instance tree, eg telling the compiler about the new values of things, etc.

	@return 0=success, 1=memory error, 2=bad instance error (nonzero=error)
		3=failure with diffvars data

	@TODO: are the following comments still relevant?
	Makes variable/relations/when lists and objective function by heuristic.
	Now also makes a list of all relations that are objectives.
	This does not affect the semantics of the previous objective
	code.
	Do NOTHING in this function which can lead to floating point
	errors -- in particular because we must leave the interface ptrs
	in the state they were found.
*/
int analyze_make_problem(slv_system_t sys, struct Instance *inst){
  int stat;

  struct problem_t thisproblem; /* note default zero intitialisation. note also: local var! */
  struct problem_t *p_data; /* need to malloc, free, or make &local */

  INCLUDED_A = AddSymbol("included");
  FIXED_A = AddSymbol("fixed");
  BASIS_A = AddSymbol("basis");
  DERIV_A = AddSymbol("ode_type");
  ODEID_A = AddSymbol("ode_id");
  OBSID_A = AddSymbol("obs_id");

  p_data = &thisproblem;
  g_bad_rel_in_list = FALSE;
  InitTreeCounts(inst,p_data);
  /* take the census */
  VisitInstanceTreeTwo(inst,(VisitTwoProc)CountStuffInTree,TRUE,FALSE,
                       (VOIDPTR)p_data);
  if(g_bad_rel_in_list) {
    p_data->root = NULL;
    return 2;
  }

  /* decorate instances with temporary ips, collect them and etc */
  stat = analyze_make_master_lists(p_data);
  if(stat == 2){
    analyze_free_lists(p_data);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Nothing to make a problem from!");
    return 2;
  }else if(stat == 1){
    analyze_free_lists(p_data);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Insufficient memory (master lists)");
    return 1;
  }

  /* rearrange all the stuff we found and index things */
  stat = analyze_make_solvers_lists(p_data);
  if(stat == 2){
    analyze_free_lists(p_data);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Nothing to make lists from (solver's lists)");
    return 2;
  }else if(stat == 1){
    analyze_free_lists(p_data);
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Insufficient memory (solver's lists)");
    return 1;
  }

#ifdef ASC_IDA_NEW_ANALYSE
  stat = system_generate_diffvars(sys,p_data);
  if(stat){
	analyze_free_lists(p_data);
	return 3;
  }
#endif

  /* tell the slv_system_t about it, and undecorate ips from instances */
  analyze_configure_system(sys,p_data);
  /* configure must set nulls in p_data for anything we want to keep */
  /* blow the temporary lists away */
  analyze_free_lists(p_data);
  return 0;
}

extern void analyze_free_reused_mem(void){
  resize_ipbuf((size_t)0,0);
}

/* @} */
/* :ex: set ts=4 : */
