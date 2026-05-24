/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//* @file
	Conditional Module
*//*
	by Vicente Rico-Ramirez, 09/96
	Last in CVS: $Revision: 1.10 $ $Date: 1998/03/30 22:06:52 $ $Author: rv2a $
*/

#include "conditional.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/dstring.h>
#include <ascend/compiler/instance_enum.h>

#include <ascend/compiler/child.h>


#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/exprs.h>

#include "slv_server.h"
#include "analyze.h"


#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif
#define WHEN_DEBUG FALSE

/*
 *                        When utility functions
 */

/* forward declaration */
void when_case_destroy(struct when_case *);

static const struct w_when g_when_defaults = {
   NULL,		/* instance */
   NULL,		/* variables */
   NULL,		/* cases */
   -1,		        /* number of cases */
   -1,			/* mindex */
   -1,			/* sindex */
   -1,			/* model index */
   (WHEN_INCLUDED)	/* flags */
};


static struct w_when *when_copy(const struct w_when *when){
   struct w_when *newwhen;
   newwhen = (struct w_when *)ascmalloc( sizeof(struct w_when) );
   *newwhen = *when;
   return(newwhen);
}


struct w_when *when_create(SlvBackendToken instance, struct w_when *newwhen){
  if (newwhen==NULL) {
    newwhen = when_copy(&g_when_defaults); /* malloc the when */
  }else{
    *newwhen = g_when_defaults;        /* init the space we've been sent */
  }
  assert(newwhen!=NULL);
  newwhen->instance = instance;
  return(newwhen);
}


void when_destroy_cases(struct w_when *when){
  int32 c,len;
  struct when_case *cur_case;

  len = gl_length(when->cases);
  for(c=1;c<=len;c++){
    cur_case = (struct when_case *)(gl_fetch(when->cases,c));
    when_case_destroy(cur_case);
  }
  gl_destroy(when->cases);
}


void when_destroy(struct w_when *when){
   struct Instance *inst;
   if (when==NULL) return;
   if (when->dvars != NULL) {
     gl_destroy(when->dvars);
     when->dvars = NULL;
   }
   if (when->cases != NULL) {
     when_destroy_cases(when);
     when->cases = NULL;
   }
   inst = IPTR(when->instance);
   if (inst) {
     if (GetInterfacePtr(inst)==when) {
       SetInterfacePtr(inst,NULL);
     }
   }
}

SlvBackendToken when_instance(struct w_when *when){
	if (when==NULL) return NULL;
	return (SlvBackendToken) when->instance;
}


char *when_make_name(slv_system_t sys,struct w_when *when){
  return WriteInstanceNameString(IPTR(when->instance),IPTR(slv_instance(sys)));
}


void when_write_name(slv_system_t sys,struct w_when *when, FILE *fp){
  if (when == NULL || fp==NULL) return;
  if (sys!=NULL) {
    WriteInstanceName(fp,when_instance(when),slv_instance(sys));
  }else{
    WriteInstanceName(fp,when_instance(when),NULL);
  }
}


struct gl_list_t *when_dvars_list( struct w_when *when){
	assert(when);
	return( when->dvars );
}


void when_set_dvars_list( struct w_when *when, struct gl_list_t *dvlist){
	assert(when);
	when->dvars = dvlist;
}


struct gl_list_t *when_cases_list( struct w_when *when){
	assert(when);
	return( when->cases );
}


void when_set_cases_list( struct w_when *when, struct gl_list_t *clist){
	assert(when);
	when->cases = clist;
}

int32 when_num_cases(struct w_when *when){
	return( when->num_cases );
}

void when_set_num_cases( struct w_when *when, int32 num_cases){
	when->num_cases = num_cases;
}

int32 when_mindex( struct w_when *when){
	return( when->mindex );
}

void when_set_mindex( struct w_when *when, int32 mindex){
	when->mindex = mindex;
}


int32 when_sindex( struct w_when *when){
	return( when->sindex );
}

void when_set_sindex( struct w_when *when, int32 sindex){
	when->sindex = sindex;
}

int32 when_model(const struct w_when *when){
	return((const int32) when->model );
}

void when_set_model( struct w_when *when, int32 mindex){
	when->model = mindex;
}

int32 when_apply_filter(struct w_when *w, const when_filter_t *filter){
  if (w==NULL || filter==NULL) {
    FPRINTF(stderr,"when_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & w->flags) ==
           (filter->matchbits & filter->matchvalue) );
}


uint32 when_flags( struct w_when *when){
  return when->flags;
}

void when_set_flags(struct w_when *when, uint32 flags){
  when->flags = flags;
}

uint32 when_flagbit(struct w_when *when, uint32 one){
  if (when==NULL || when->instance == NULL) {
    FPRINTF(stderr,"ERROR: when_flagbit called with bad when.\n");
    return 0;
  }
  return (when->flags & one);
}

void when_set_flagbit(struct w_when *when, uint32 field, uint32 one){
  if (one) {
    when->flags |= field;
  }else{
    when->flags &= ~field;
  }
}


/*
 *                  When Case  utility functions
 */


/* we are depending on ansi initialization to 0 for the
 * values field OF this struct.
 */
static
const struct when_case g_case_defaults = {
   {0},			/* values */
   NULL,		/* condition */
   NULL,		/* applies */
   NULL,		/* lowered region */
   WHEN_REGION_NONE,	/* lowered region source */
   NULL,		/* source module */
   0,			/* source line */
   NULL,		/* relations */
   NULL,		/* logrelations */
   NULL,		/* whens */
   NULL,                /* reinits */
   -1,                  /* case number */
   -1,                  /* number relations */
   -1,                  /* number vars */
   NULL,                /* master indeces of incidente vars */
   (0x0)
};

static const struct when_reinit g_when_reinit_defaults = {
  NULL,
  NULL,
  NULL
};


static struct when_case *when_case_copy(const struct when_case *wc){
   struct when_case *newcase;
   newcase = (struct when_case *)ascmalloc( sizeof(struct when_case) );
   *newcase = *wc;
   return(newcase);
}


struct when_case *when_case_create(struct when_case *newcase){
  if (newcase==NULL) {
    newcase = when_case_copy(&g_case_defaults); /* malloc the case */
  }else{
    *newcase = g_case_defaults;    /* init the space we've been sent */
  }
  return(newcase);
}

struct when_reinit *when_reinit_create(struct when_reinit *newreinit){
  if(newreinit == NULL){
    newreinit = (struct when_reinit *)ascmalloc(sizeof(struct when_reinit));
  }
  *newreinit = g_when_reinit_defaults;
  return newreinit;
}

void when_reinit_destroy(struct when_reinit *wr){
  if(wr == NULL)return;
  ascfree((POINTER)wr);
}

SlvBackendToken when_reinit_target(const struct when_reinit *wr){
  assert(wr);
  return wr->target;
}

void when_reinit_set_target(struct when_reinit *wr, SlvBackendToken target){
  assert(wr);
  wr->target = target;
}

const struct Expr *when_reinit_rhs(const struct when_reinit *wr){
  assert(wr);
  return wr->rhs;
}

void when_reinit_set_rhs(struct when_reinit *wr, const struct Expr *rhs){
  assert(wr);
  wr->rhs = rhs;
}

const struct Expr *when_reinit_guard(const struct when_reinit *wr){
  assert(wr);
  return wr->guard;
}

void when_reinit_set_guard(struct when_reinit *wr, const struct Expr *guard){
  assert(wr);
  wr->guard = guard;
}


void when_case_destroy(struct when_case *wc){
   when_case_clear_region_predicate(wc);
   if (wc->rels != NULL) {
     gl_destroy(wc->rels);
     wc->rels = NULL;
   }
   if (wc->logrels != NULL) {
     gl_destroy(wc->logrels);
     wc->logrels = NULL;
   }
   if (wc->whens != NULL ) {
     gl_destroy(wc->whens);
     wc->whens = NULL;
   }
   if (wc->reinits != NULL ) {
     unsigned long i, len = gl_length(wc->reinits);
     for(i = 1; i <= len; ++i){
       struct when_reinit *wr = (struct when_reinit *)gl_fetch(wc->reinits, i);
       when_reinit_destroy(wr);
     }
     gl_destroy(wc->reinits);
     wc->reinits = NULL;
   }
   if (wc->ind_inc != NULL ) {
     ascfree(wc->ind_inc);
   }

   ascfree((POINTER)wc);
}


int32 *when_case_values_list( struct when_case *wc){
   assert(wc);
   return( &(wc->values[0]) );
}


void when_case_set_values_list( struct when_case *wc, int32 *vallist){
   int32 *value,vindex;
   assert(wc);
   value = &(wc->values[0]);
   for(vindex=0;vindex<MAX_VAR_IN_LIST;vindex++) {
      *value = *vallist;
      value++;
      vallist++;
   }
}

const struct Expr *when_case_condition(const struct when_case *wc){
   assert(wc);
   return wc->condition;
}

void when_case_set_condition(struct when_case *wc, const struct Expr *condition){
   assert(wc);
   wc->condition = condition;
}

const struct Expr *when_case_applies(const struct when_case *wc){
   assert(wc);
   return wc->applies;
}

void when_case_set_applies(struct when_case *wc, const struct Expr *applies){
   assert(wc);
   wc->applies = applies;
}

int when_case_has_classifier_predicate(const struct when_case *wc){
   assert(wc);
   return wc->condition != NULL || wc->applies != NULL;
}

int when_has_classifier_predicates(const struct w_when *when){
   struct gl_list_t *cases;
   unsigned long c, clen;

   if(when == NULL){
      return 0;
   }

   cases = when->cases;
   if(cases == NULL){
      return 0;
   }

   clen = gl_length(cases);
   for(c = 1; c <= clen; ++c){
      struct when_case *wc = (struct when_case *)gl_fetch(cases, c);
      struct gl_list_t *nested;
      unsigned long w, wlen;

      if(wc == NULL){
         continue;
      }
      if(when_case_has_classifier_predicate(wc)){
         return 1;
      }

      nested = wc->whens;
      wlen = nested != NULL ? gl_length(nested) : 0;
      for(w = 1; w <= wlen; ++w){
         struct w_when *nested_when = (struct w_when *)gl_fetch(nested, w);
         if(when_has_classifier_predicates(nested_when)){
            return 1;
         }
      }
   }

   return 0;
}

const struct Expr *when_case_region_predicate(const struct when_case *wc){
   assert(wc);
   return wc->region;
}

int32 when_case_region_source(const struct when_case *wc){
   assert(wc);
   return wc->region_source;
}

void when_case_clear_region_predicate(struct when_case *wc){
   assert(wc);
   if(wc->region != NULL){
      DestroyExprList(wc->region);
      wc->region = NULL;
   }
   wc->region_source = WHEN_REGION_NONE;
}

void when_case_set_region_predicate(struct when_case *wc, struct Expr *region,
                                    int32 source){
   assert(wc);
   when_case_clear_region_predicate(wc);
   wc->region = region;
   wc->region_source = region != NULL ? source : WHEN_REGION_NONE;
}

struct module_t *when_case_source_module(const struct when_case *wc){
   assert(wc);
   return wc->source_module;
}

unsigned long when_case_source_line(const struct when_case *wc){
   assert(wc);
   return wc->source_line;
}

void when_case_set_source(struct when_case *wc, struct module_t *module,
      unsigned long line){
   assert(wc);
   wc->source_module = module;
   wc->source_line = line;
}

static int when_case_is_otherwise(const struct when_case *wc){
   assert(wc);
   return wc->values[0] == -1;
}

static struct Expr *when_expr_not_copy(const struct Expr *expr){
   return JoinExprLists(CopyExprList(expr),CreateOpExpr(e_not));
}

static struct Expr *when_expr_and_owned(struct Expr *left, struct Expr *right){
   if(left == NULL){
      return right;
   }
   if(right == NULL){
      return left;
   }
   return JoinExprLists(JoinExprLists(left,right),CreateOpExpr(e_and));
}

static int when_lower_nested_classifier_regions(struct when_case *wc,
      enum when_region_request request){
   struct gl_list_t *nested;
   unsigned long w, wlen;

   nested = when_case_whens_list(wc);
   if(nested == NULL){
      return 0;
   }

   wlen = gl_length(nested);
   for(w = 1; w <= wlen; ++w){
      struct w_when *nested_when = (struct w_when *)gl_fetch(nested,w);
      if(when_lower_classifier_regions(nested_when,request)){
         return 1;
      }
   }
   return 0;
}

static int when_lower_applies_regions(struct w_when *when,
      enum when_region_request request){
   struct gl_list_t *cases;
   unsigned long c, clen;

   cases = when_cases_list(when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
      const struct Expr *applies = when_case_applies(wc);
      if(applies == NULL){
         FPRINTF(stderr,
            "APPLIES IF classifier lowering requires every case to provide APPLIES IF\n");
         return 1;
      }
      when_case_set_region_predicate(
         wc,CopyExprList(applies),WHEN_REGION_APPLIES
      );
      if(when_lower_nested_classifier_regions(wc,request)){
         return 1;
      }
   }

   return 0;
}

static int when_lower_case_if_regions(struct w_when *when,
      enum when_region_request request){
   struct gl_list_t *cases;
   struct Expr *excluded = NULL;
   unsigned long c, clen;

   cases = when_cases_list(when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
      const struct Expr *condition = when_case_condition(wc);
      struct Expr *region = NULL;

      if(when_case_is_otherwise(wc)){
         region = excluded != NULL ? CopyExprList(excluded) : CreateTrueExpr();
         when_case_set_region_predicate(
            wc,region,WHEN_REGION_CASE_IF_OTHERWISE
         );
      }else{
         if(condition == NULL){
            if(excluded != NULL){
               DestroyExprList(excluded);
            }
            FPRINTF(stderr,
               "CASE IF classifier lowering requires every non-OTHERWISE case to provide CASE IF\n");
            return 1;
         }
         region = when_expr_and_owned(
            excluded != NULL ? CopyExprList(excluded) : NULL,
            CopyExprList(condition)
         );
         when_case_set_region_predicate(wc,region,WHEN_REGION_CASE_IF);
         excluded = when_expr_and_owned(
            excluded,when_expr_not_copy(condition)
         );
      }

      if(when_lower_nested_classifier_regions(wc,request)){
         if(excluded != NULL){
            DestroyExprList(excluded);
         }
         return 1;
      }
   }

   if(excluded != NULL){
      DestroyExprList(excluded);
   }
   return 0;
}

int when_lower_classifier_regions(struct w_when *when,
      enum when_region_request request){
   struct gl_list_t *cases;
   unsigned long c, clen;
   int has_case_if = 0;
   int has_applies = 0;

   (void)request;
   if(when == NULL){
      return 0;
   }

   cases = when_cases_list(when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
      when_case_clear_region_predicate(wc);
      if(when_case_condition(wc) != NULL){
         has_case_if = 1;
      }
      if(when_case_applies(wc) != NULL){
         has_applies = 1;
      }
   }

   if(has_case_if && has_applies){
      FPRINTF(stderr,
         "CASE IF and APPLIES IF cannot be lowered in the same WHEN\n");
      return 1;
   }

   if(has_applies){
      return when_lower_applies_regions(when,request);
   }
   if(has_case_if){
      return when_lower_case_if_regions(when,request);
   }

   for(c = 1; c <= clen; ++c){
      struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
      if(when_lower_nested_classifier_regions(wc,request)){
         return 1;
      }
   }
   return 0;
}

static int when_validate_case_if_guard_plan(const struct w_when *when,
      int32 *nguards){
   struct gl_list_t *cases;
   unsigned long c, clen;
   int saw_otherwise = 0;
   int32 guard_count = 0;

   if(nguards != NULL){
      *nguards = 0;
   }
   if(when == NULL){
      return 1;
   }

   cases = when_cases_list((struct w_when *)when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      const struct when_case *wc =
         (const struct when_case *)gl_fetch(cases,c);
      if(when_case_applies(wc) != NULL){
         return 1;
      }
      if(when_case_is_otherwise(wc)){
         saw_otherwise = 1;
         continue;
      }
      if(saw_otherwise || when_case_condition(wc) == NULL){
         return 1;
      }
      ++guard_count;
   }

   if(guard_count == 0){
      return 1;
   }
   if(guard_count > MAX_VAR_IN_LIST){
      return 1;
   }
   if(nguards != NULL){
      *nguards = guard_count;
   }
   return 0;
}

int when_case_if_guard_count(const struct w_when *when, int32 *nguards){
   return when_validate_case_if_guard_plan(when,nguards);
}

const struct Expr *when_case_if_guard(const struct w_when *when,
      int32 guard_index){
   struct gl_list_t *cases;
   unsigned long c, clen;
   int32 g = 0;

   if(guard_index < 0
      || when_validate_case_if_guard_plan(when,NULL)){
      return NULL;
   }

   cases = when_cases_list((struct w_when *)when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      const struct when_case *wc =
         (const struct when_case *)gl_fetch(cases,c);
      if(when_case_is_otherwise(wc)){
         break;
      }
      if(g == guard_index){
         return when_case_condition(wc);
      }
      ++g;
   }
   return NULL;
}

int when_case_if_pattern(const struct w_when *when,
      const struct when_case *wc, int32 *values, int32 *nvalues){
   struct gl_list_t *cases;
   unsigned long c, clen;
   int32 nguards;
   int32 g = 0;
   int found = 0;

   if(values == NULL || nvalues == NULL || wc == NULL
      || when_validate_case_if_guard_plan(when,&nguards)){
      return 1;
   }

   for(g = 0; g < nguards; ++g){
      values[g] = -2; /* ANY / don't-care */
   }
   *nvalues = nguards;

   cases = when_cases_list((struct w_when *)when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1, g = 0; c <= clen; ++c){
      const struct when_case *cur =
         (const struct when_case *)gl_fetch(cases,c);
      if(cur == wc){
         found = 1;
         break;
      }
      if(when_case_is_otherwise(cur)){
         break;
      }
      ++g;
   }

   if(!found){
      return 1;
   }

   if(when_case_is_otherwise(wc)){
      for(g = 0; g < nguards; ++g){
         values[g] = 0;
      }
      return 0;
   }

   for(c = 0; c < (unsigned long)g; ++c){
      values[c] = 0;
   }
   values[g] = 1;
   return 0;
}

static void when_guard_materialization_init(
      struct when_guard_materialization *plan){
   if(plan != NULL){
      plan->guard_booleans = 0;
      plan->reusable_named_guards = 0;
      plan->real_boundaries = 0;
      plan->logical_boundaries = 0;
      plan->boolean_ops = 0;
      plan->unsupported_dynamic_terms = 0;
      plan->hidden_boolean_instances = 0;
      plan->hidden_relation_instances = 0;
      plan->hidden_logrel_instances = 0;
      plan->requires_named_instances = 0;
   }
}

static void when_guard_scan_expr(const struct Expr *expr,
      struct when_guard_materialization *plan){
   while(expr != NULL){
      switch(ExprType(expr)){
      case e_equal:
      case e_notequal:
      case e_less:
      case e_greater:
      case e_lesseq:
      case e_greatereq:
         ++(plan->real_boundaries);
         break;
      case e_boolean_eq:
      case e_boolean_neq:
      case e_satisfied:
         ++(plan->logical_boundaries);
         break;
      case e_and:
      case e_or:
      case e_not:
         ++(plan->boolean_ops);
         break;
      case e_pre:
      case e_der:
         ++(plan->unsupported_dynamic_terms);
         break;
      default:
         break;
      }
      expr = NextExpr(expr);
   }
}

static int when_guard_is_reusable_named_boolean(const struct Expr *expr){
   if(expr == NULL || NextExpr(expr) != NULL){
      return 0;
   }
   switch(ExprType(expr)){
   case e_var:
   case e_boolean:
      return 1;
   default:
      return 0;
   }
}

enum when_guard_expr_kind {
   WHEN_GUARD_EXPR_OTHER = 0,
   WHEN_GUARD_EXPR_REAL,
   WHEN_GUARD_EXPR_BOOL,
   WHEN_GUARD_EXPR_NAMED
};

static int when_guard_scan_pop(enum when_guard_expr_kind *stack,
      int32 *sp, enum when_guard_expr_kind *kind){
   if(stack == NULL || sp == NULL || kind == NULL || *sp <= 0){
      return 1;
   }
   *kind = stack[--(*sp)];
   return 0;
}

static int when_guard_scan_push(enum when_guard_expr_kind *stack,
      int32 stack_size, int32 *sp, enum when_guard_expr_kind kind){
   if(stack == NULL || sp == NULL || *sp >= stack_size){
      return 1;
   }
   stack[(*sp)++] = kind;
   return 0;
}

static int when_guard_artifact_count_named_operand(
      enum when_guard_expr_kind kind){
   return kind == WHEN_GUARD_EXPR_NAMED ? 1 : 0;
}

static void when_guard_artifact_scan_expr(const struct Expr *expr,
      struct when_guard_artifact *artifact){
   enum when_guard_expr_kind *stack;
   int32 stack_size;
   int32 sp = 0;

   if(expr == NULL || artifact == NULL){
      return;
   }

   stack_size = (int32)ExprListLength(expr);
   stack = ASC_NEW_ARRAY_CLEAR(enum when_guard_expr_kind,stack_size + 1);
   if(stack == NULL){
      ++(artifact->unsupported_dynamic_terms);
      return;
   }

   while(expr != NULL){
      enum when_guard_expr_kind left, right;
      switch(ExprType(expr)){
      case e_var:
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_NAMED)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_boolean:
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_zero:
      case e_int:
      case e_real:
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_REAL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_equal:
      case e_notequal:
      case e_less:
      case e_greater:
      case e_lesseq:
      case e_greatereq:
         if(when_guard_scan_pop(stack,&sp,&right)
            || when_guard_scan_pop(stack,&sp,&left)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         ++(artifact->real_boundaries);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_boolean_eq:
      case e_boolean_neq:
         if(when_guard_scan_pop(stack,&sp,&right)
            || when_guard_scan_pop(stack,&sp,&left)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         artifact->reusable_named_terms +=
            when_guard_artifact_count_named_operand(left)
            + when_guard_artifact_count_named_operand(right);
         ++(artifact->logical_boundaries);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_satisfied:
         ++(artifact->logical_boundaries);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_and:
      case e_or:
         if(when_guard_scan_pop(stack,&sp,&right)
            || when_guard_scan_pop(stack,&sp,&left)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         artifact->reusable_named_terms +=
            when_guard_artifact_count_named_operand(left)
            + when_guard_artifact_count_named_operand(right);
         ++(artifact->boolean_ops);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_not:
         if(when_guard_scan_pop(stack,&sp,&right)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         artifact->reusable_named_terms +=
            when_guard_artifact_count_named_operand(right);
         ++(artifact->boolean_ops);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_BOOL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_uminus:
      case e_func:
         if(when_guard_scan_pop(stack,&sp,&right)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_REAL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_plus:
      case e_minus:
      case e_times:
      case e_divide:
      case e_power:
      case e_ipower:
         if(when_guard_scan_pop(stack,&sp,&right)
            || when_guard_scan_pop(stack,&sp,&left)){
            ++(artifact->unsupported_dynamic_terms);
            break;
         }
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_REAL)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      case e_pre:
      case e_der:
         ++(artifact->unsupported_dynamic_terms);
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_OTHER)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      default:
         if(when_guard_scan_push(stack,stack_size + 1,&sp,
               WHEN_GUARD_EXPR_OTHER)){
            ++(artifact->unsupported_dynamic_terms);
         }
         break;
      }
      expr = NextExpr(expr);
   }

   ascfree(stack);
}

struct when_guard_artifact *when_guard_artifact_create(
      const struct w_when *when, int32 guard_index){
   struct gl_list_t *cases;
   unsigned long c, clen;
   int32 g = 0;
   struct when_guard_artifact *artifact;

   if(guard_index < 0
      || when_validate_case_if_guard_plan(when,NULL)){
      return NULL;
   }

   cases = when_cases_list((struct w_when *)when);
   clen = cases != NULL ? gl_length(cases) : 0;
   for(c = 1; c <= clen; ++c){
      const struct when_case *wc =
         (const struct when_case *)gl_fetch(cases,c);
      if(when_case_is_otherwise(wc)){
         break;
      }
      if(g == guard_index){
         artifact = ASC_NEW_CLEAR(struct when_guard_artifact);
         if(artifact == NULL){
            return NULL;
         }
         artifact->guard_index = guard_index;
         artifact->source_case = wc;
         artifact->guard = when_case_condition(wc);
         artifact->reuse_existing_boolean =
            when_guard_is_reusable_named_boolean(artifact->guard);
         artifact->generated_boolean =
            artifact->reuse_existing_boolean ? 0 : 1;
         artifact->source_module = when_case_source_module(wc);
         artifact->source_line = when_case_source_line(wc);
         if(artifact->reuse_existing_boolean){
            artifact->reusable_named_terms = 1;
         }else{
            when_guard_artifact_scan_expr(artifact->guard,artifact);
         }
         return artifact;
      }
      ++g;
   }

   return NULL;
}

void when_guard_artifact_destroy(struct when_guard_artifact *artifact){
   if(artifact != NULL){
      ascfree(artifact);
   }
}

struct gl_list_t *when_case_if_artifacts_create(const struct w_when *when){
   struct gl_list_t *artifacts;
   int32 nguards;
   int32 g;

   if(when_validate_case_if_guard_plan(when,&nguards)){
      return NULL;
   }
   artifacts = gl_create(nguards);
   if(artifacts == NULL){
      return NULL;
   }

   for(g = 0; g < nguards; ++g){
      struct when_guard_artifact *artifact =
         when_guard_artifact_create(when,g);
      if(artifact == NULL){
         when_case_if_artifacts_destroy(artifacts);
         return NULL;
      }
      gl_append_ptr(artifacts,artifact);
   }

   return artifacts;
}

void when_case_if_artifacts_destroy(struct gl_list_t *artifacts){
   unsigned long i, len;

   if(artifacts == NULL){
      return;
   }
   len = gl_length(artifacts);
   for(i = 1; i <= len; ++i){
      when_guard_artifact_destroy(
         (struct when_guard_artifact *)gl_fetch(artifacts,i)
      );
   }
   gl_destroy(artifacts);
}

int when_case_if_materialization_plan(const struct w_when *when,
      struct when_guard_materialization *plan){
   int32 nguards;
   int32 g;

   if(plan == NULL){
      return 1;
   }
   when_guard_materialization_init(plan);
   if(when_validate_case_if_guard_plan(when,&nguards)){
      return 1;
   }

   plan->guard_booleans = nguards;
   for(g = 0; g < nguards; ++g){
      const struct Expr *guard = when_case_if_guard(when,g);
      if(guard == NULL){
         when_guard_materialization_init(plan);
         return 1;
      }
      if(when_guard_is_reusable_named_boolean(guard)){
         ++(plan->reusable_named_guards);
      }
      when_guard_scan_expr(guard,plan);
   }

   /*
    * Instance-backed CMSlv/CMSlv2 lowering needs one generated Boolean for
    * each CASE IF guard, one conditional relation for each real comparison,
    * and one logical relation defining each generated Boolean. The generated
    * relations must be name-addressable because SATISFIED(...) terms resolve
    * relations by name in the compiler expression path.
    */
   plan->hidden_boolean_instances =
      plan->guard_booleans - plan->reusable_named_guards;
   plan->hidden_relation_instances = plan->real_boundaries;
   plan->hidden_logrel_instances =
      plan->guard_booleans - plan->reusable_named_guards;
   plan->requires_named_instances =
      (plan->hidden_boolean_instances
       || plan->hidden_relation_instances
       || plan->hidden_logrel_instances) ? 1 : 0;

   return 0;
}

struct gl_list_t *when_case_rels_list( struct when_case *wc){
   assert(wc);
   return( wc->rels );
}


void when_case_set_rels_list( struct when_case *wc, struct gl_list_t *rlist){
   assert(wc);
   wc->rels = rlist;
}


struct gl_list_t *when_case_logrels_list( struct when_case *wc){
   assert(wc);
   return( wc->logrels );
}


void when_case_set_logrels_list(struct when_case *wc,
                                struct gl_list_t *lrlist){
   assert(wc);
   wc->logrels = lrlist;
}


struct gl_list_t *when_case_whens_list( struct when_case *wc){
   assert(wc);
   return( wc->whens );
}


void when_case_set_whens_list( struct when_case *wc, struct gl_list_t *wlist){
   assert(wc);
   wc->whens = wlist;
}

struct gl_list_t *when_case_reinits_list(struct when_case *wc){
   assert(wc);
   return wc->reinits;
}

void when_case_set_reinits_list(struct when_case *wc, struct gl_list_t *rlist){
   assert(wc);
   wc->reinits = rlist;
}


int32 when_case_case_number( struct when_case *wc){
  assert(wc);
  return wc->case_number;
}

void when_case_set_case_number(struct when_case *wc, int32 case_number){
  assert(wc);
  wc->case_number = case_number;
}

int32 when_case_num_rels( struct when_case *wc){
  assert(wc);
  return wc->num_rels;
}

void when_case_set_num_rels(struct when_case *wc, int32 num_rels){
  assert(wc);
  wc->num_rels = num_rels;
}

int32 when_case_num_inc_var( struct when_case *wc){
  assert(wc);
  return wc->num_inc_var;
}

void when_case_set_num_inc_var(struct when_case *wc, int32 num_inc_var){
  assert(wc);
  wc->num_inc_var = num_inc_var;
}

int32 *when_case_ind_inc( struct when_case *wc){
  assert(wc);
  return wc->ind_inc;
}

void when_case_set_ind_inc(struct when_case *wc, int32* ind_inc){
  assert(wc);
  wc->ind_inc = ind_inc;
}

int32 when_case_apply_filter(struct when_case *wc,
		const when_case_filter_t *filter
){
  if (wc==NULL || filter==NULL) {
    FPRINTF(stderr,"when_case_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & wc->flags) ==
           (filter->matchbits & filter->matchvalue) );
}

uint32 when_case_flags( struct when_case *wc){
  assert(wc);
  return wc->flags;
}

void when_case_set_flags(struct when_case *wc, uint32 flags)
{
  assert(wc);
  wc->flags = flags;
}

uint32 when_case_flagbit(struct when_case *wc, uint32 one)
{
  if (wc==NULL) {
    FPRINTF(stderr,"ERROR: when_case_flagbit called with bad case.\n");
    return 0;
  }
  return (wc->flags & one);
}

void when_case_set_flagbit(struct when_case *wc, uint32 field,
		           uint32 one)
{
  if (one) {
    wc->flags |= field;
  }else{
    wc->flags &= ~field;
  }
}
