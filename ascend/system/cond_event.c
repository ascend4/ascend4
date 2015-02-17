#include "cond_event.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/dstring.h>
#include <ascend/compiler/instance_enum.h>

#include <ascend/compiler/child.h>


#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instance_io.h>

#include "slv_server.h"
#include "analyze.h"

#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif

/*
 *                        Event utility functions
 */

static const struct e_event g_event_defaults = {
   NULL,		/* instance */
   NULL,		/* variables */
   NULL,		/* cases */
   -1,		        /* number of cases */
   -1,			/* mindex */
   -1,			/* sindex */
   -1,			/* model index */
   (EVENT_INCLUDED | EVENT_ACTIVE)	/* flags */
};

static struct e_event *event_copy(const struct e_event *event){
   struct e_event *newevent;
   newevent = (struct e_event *)ascmalloc( sizeof(struct e_event) );
   *newevent = *event;
   return(newevent);
}

struct e_event *event_create(SlvBackendToken instance, struct e_event *newevent){
  if (newevent==NULL) {
    newevent = event_copy(&g_event_defaults); /* malloc the event */
  }else{
    *newevent = g_event_defaults;        /* init the space we've been sent */
  }
  assert(newevent!=NULL);
  newevent->instance = instance;
  return(newevent);
}

SlvBackendToken event_instance(struct e_event *event){
	if (event==NULL) return NULL;
	return (SlvBackendToken) event->instance;
}

void event_destroy_cases(struct e_event *event){
  int32 c,len;
  struct event_case *cur_case;

  len = gl_length(event->cases);
  for(c=1;c<=len;c++){
    cur_case = (struct event_case *)(gl_fetch(event->cases,c));
    event_case_destroy(cur_case);
  }
  gl_destroy(event->cases);
}


void event_destroy(struct e_event *event){
   struct Instance *inst;
   if (event==NULL) return;
   if (event->dvars != NULL) {
     gl_destroy(event->dvars);
     event->dvars = NULL;
   }
   if (event->cases != NULL) {
     event_destroy_cases(event);
     event->cases = NULL;
   }
   inst = IPTR(event->instance);
   if (inst) {
     if (GetInterfacePtr(inst)==event) {
       SetInterfacePtr(inst,NULL);
     }
   }
}

char *event_make_name(slv_system_t sys, struct e_event *event){
  return WriteInstanceNameString(IPTR(event->instance),IPTR(slv_instance(sys)));
}

struct gl_list_t *event_dvars_list( struct e_event *event){
	assert(event);
	return( event->dvars );
}

struct gl_list_t *event_cases_list( struct e_event *event){
	assert(event);
	return( event->cases );
}

void event_set_num_cases( struct e_event *event, int32 num_cases){
	event->num_cases = num_cases;
}

void event_set_mindex( struct e_event *event, int32 mindex){
	event->mindex = mindex;
}

void event_set_sindex( struct e_event *event, int32 sindex){
	event->sindex = sindex;
}

void event_set_model( struct e_event *event, int32 mindex){
	event->model = mindex;
}

void event_set_flags(struct e_event *event, uint32 flags){
	event->flags = flags;
}

uint32 event_flagbit(struct e_event *event, uint32 one){
  if (event==NULL || event->instance == NULL) {
    FPRINTF(stderr,"ERROR: event_flagbit called with bad event.\n");
    return 0;
  }
  return (event->flags & one);
}

void event_set_flagbit(struct e_event *event, uint32 field, uint32 one){
  if (one) {
    event->flags |= field;
  }else{
    event->flags &= ~field;
  }
}

/*
 *                  Event Case  utility functions
 */


/* we are depending on ansi initialization to 0 for the
 * values field OF this struct.
 */
static
const struct event_case g_case_defaults = {
   {0},			/* values */
   NULL,		/* relations */
   NULL,		/* logrelations */
   NULL,		/* events */
   -1,                  /* case number */
   -1,                  /* number relations */
   -1,                  /* number vars */
   NULL,                /* master indeces of incidente vars */
   (0x0)
};


static struct event_case *event_case_copy(const struct event_case *ec){
   struct event_case *newcase;
   newcase = (struct event_case *)ascmalloc( sizeof(struct event_case) );
   *newcase = *ec;
   return(newcase);
}


struct event_case *event_case_create(struct event_case *newcase){
  if (newcase==NULL) {
    newcase = event_case_copy(&g_case_defaults); /* malloc the case */
  }else{
    *newcase = g_case_defaults;    /* init the space we've been sent */
  }
  return(newcase);
}

void event_case_destroy(struct event_case *ec){
   if (ec->rels != NULL) {
     gl_destroy(ec->rels);
     ec->rels = NULL;
   }
   if (ec->logrels != NULL) {
     gl_destroy(ec->logrels);
     ec->logrels = NULL;
   }
   if (ec->events != NULL ) {
     gl_destroy(ec->events);
     ec->events = NULL;
   }
   if (ec->ind_inc != NULL ) {
     ascfree(ec->ind_inc);
   }

   ascfree((POINTER)ec);
}

int32 *event_case_values_list( struct event_case *ec){
   assert(ec);
   return( &(ec->values[0]) );
}

struct gl_list_t *event_case_rels_list( struct event_case *ec){
   assert(ec);
   return( ec->rels );
}

void event_case_set_rels_list( struct event_case *ec, struct gl_list_t *rlist){
   assert(ec);
   ec->rels = rlist;
}

struct gl_list_t *event_case_logrels_list( struct event_case *ec){
   assert(ec);
   return( ec->logrels );
}

void event_case_set_logrels_list(struct event_case *ec,
                                 struct gl_list_t *lrlist){
   assert(ec);
   ec->logrels = lrlist;
}

struct gl_list_t *event_case_events_list( struct event_case *ec){
   assert(ec);
   return( ec->events );
}

void event_case_set_events_list( struct event_case *ec, struct gl_list_t *elist){
   assert(ec);
   ec->events = elist;
}

void event_case_set_case_number(struct event_case *ec, int32 case_number){
  assert(ec);
  ec->case_number = case_number;
}

uint32 event_case_flagbit(struct event_case *ec, uint32 one)
{
  if (ec==NULL) {
    FPRINTF(stderr,"ERROR: event_case_flagbit called with bad case.\n");
    return 0;
  }
  return (ec->flags & one);
}

void event_case_set_flagbit(struct event_case *ec, uint32 field,
		            uint32 one)
{
  if (one) {
    ec->flags |= field;
  }else{
    ec->flags &= ~field;
  }
}

