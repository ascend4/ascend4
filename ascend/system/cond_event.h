#ifndef ASC_COND_EVENT_H
#define ASC_COND_EVENT_H

#include <ascend/general/platform.h>
#include <ascend/general/list.h>

#include "slv_types.h"

#ifndef MAX_VAR_IN_LIST
#define MAX_VAR_IN_LIST 20
#endif  /* MAX_VAR_IN_LIST */

/** Event data structure. */
struct e_event {
  SlvBackendToken instance;    /**< the associated ascend ATOM  */
  struct gl_list_t *dvars;     /**< index of dis vars  */
  struct gl_list_t *cases;     /**< event_case's */
  int32 num_cases;             /**< number of cases in the EVENT */
  int32 sindex;
  int32 mindex;
  int32 model;		             /**< index of a hypothetical MODEL event is from */
  uint32 flags;
};

/*
 *                        Event functions
 */

extern struct e_event *event_create(SlvBackendToken instance,
                                    struct e_event *newevent);
/**<
 *  Creates an event given the event instance.
 *  If the event supplied is NULL, we allocate the memory for the
 *  event we return, else we just init the memory you hand us and
 *  return it to you.<br><br>
 *  We set the fields instance. Setting the rest of the information
 *  is the job of the bridge building function between the ascend
 *  instance tree (or other event back end) and the slv_system_t.
 */

extern SlvBackendToken event_instance(struct e_event *event);
/**<
 *  Returns the instance pointer from an event.
 */

extern void event_destroy_cases(struct e_event *event);
/**<
 *  Destroys a the list of cases of an event.
 */

extern void event_destroy(struct e_event *event);
/**<
 *  Destroys an event.
 */

ASC_DLLSPEC char *event_make_name(slv_system_t sys, struct e_event *event);
/**<
 *  Copies of the event instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */

extern struct gl_list_t *event_dvars_list( struct e_event *event);
/**< Retrieves the list of dis variables of the given event. */

extern struct gl_list_t *event_cases_list( struct e_event *event);
/**< Retrieves the list of cases of the given event. */

extern void event_set_num_cases(struct e_event *event, int32 num_cases);
/**<
 *  Sets the number of cases of the given event as it
 *  appears in a slv_system_t master event list.
 */

extern void event_set_mindex(struct e_event *event, int32 mindex);
/**<
 *  Sets the index number of the given event as it
 *  appears in a slv_system_t master event list.
 */

extern void event_set_sindex(struct e_event *event, int32 sindex);
/**<
 *  Sets the index number of the given event as it
 *  appears in a solvers event list.
 */

extern void event_set_model(struct e_event *event, int32 mindex);
/**<
 *  Sets the model number of the given event.
 *  In a hierarchy, events come associated with
 *  models. Models are numbered from 1 to some upper limit.
 */

extern void event_set_flags(struct e_event *event, uint32 flags);
/**<
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 event_flagbit(struct e_event *event, uint32 name);
/**<
 *  Returns the value of the bit specified from the event flags.
 *  name should be an EVENT_xx flag defined above)
 */

extern void event_set_flagbit(struct e_event *event,
                              uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.<br><br>
 */

/*
 * the bit flags.
 */
#define EVENT_INWHEN             0x1
/**<  Is this event declared in a when? */
#define EVENT_INCLUDED           0x2
/**<
 *  User wants event in problem. Bit should be treated as
 *  readonly. use event_set_* to change.   solvers, ui clients.
 */
#define EVENT_VISITED            0x4
/**< Required for conditional analysis, for avoiding to reanalyze an EVENT */
#define EVENT_CHANGES_STRUCTURE  0x8
/**<
 *  Required for conditional analysis. Tells if the sutructure of
 *  the different CASES is all equal or otherwise.
 */
#define EVENT_INEVENT            0x10
/**<  Is this event nested in another event? */
#define EVENT_ACTIVE             0x20
/**<  Is this event currently a part of my problem? */
#define EVENT_METH               0x40
/**<  Is there a method which is run before triggering this event? */
#define EVENT_METH_END           0x80
/**<  Is there a method which is run after triggering this event? */

/*
 * the bit flag lookups
 */
#ifdef NDEBUG
#define event_inevent(event)    ((event)->flags & EVENT_INEVENT)
#define event_active(event)     ((event)->flags & EVENT_ACTIVE)
#define event_meth(event)       ((event)->flags & EVENT_METH)
#define event_meth_end(event)   ((event)->flags & EVENT_METH_END)
#else
#define event_inevent(event)    event_flagbit((event),EVENT_INEVENT)
#define event_active(event)     event_flagbit((event),EVENT_ACTIVE)
#define event_meth(event)       event_flagbit((event),EVENT_METH)
#define event_meth_end(event)   event_flagbit((event),EVENT_METH_END)
#endif

/*
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define event_set_inwhen(event,bv)      event_set_flagbit((event),EVENT_INWHEN,(bv))
#define event_set_inevent(event,bv)     event_set_flagbit((event),EVENT_INEVENT,(bv))
#define event_set_active(event,bv)      event_set_flagbit((event),EVENT_ACTIVE,(bv))
#define event_set_meth(event,bv)        event_set_flagbit((event),EVENT_METH,(bv))
#define event_set_meth_end(event,bv)    event_set_flagbit((event),EVENT_METH_END,(bv))

/*
 *                  Event Case  utility functions
 */

/** event case data structure */
struct event_case {
  int32 values[MAX_VAR_IN_LIST];  /**< values of conditional variables */
  struct gl_list_t *rels;         /**< pointer to relations */
  struct gl_list_t *logrels;      /**< pointer to logrelations */
  struct gl_list_t *events;       /**< pointer to events */
  int32 case_number;              /**< number of case */
  int32 num_rels;                 /**< number of relations */
  int32 num_inc_var;              /**< number of incident variables */
  int32 *ind_inc;                 /**< master indeces of incidences */
  uint32 flags;                   /**< flags ?? */
};

extern struct event_case *event_case_create(struct event_case *newcase);
/**<
 *  Creates an event case.
 *  If the case supplied is NULL, we allocate the memory for the
 *  case we return, else we just init the memory you hand us and
 *  return it to you.
 */

extern void event_case_destroy(struct event_case *ec);
/**<
 *  Destroys an event case.
 */

extern int32 *event_case_values_list( struct event_case *ec);
/**< Retrieves the list of values of the given case. */

extern struct gl_list_t *event_case_rels_list( struct event_case *ec);
/**< Retrieves the list of rels of the given case. */
extern void event_case_set_rels_list(struct event_case *ec,
                                     struct gl_list_t *rlist);
/**<
 *  Sets the list of rels of the given case.
 */

extern struct gl_list_t *event_case_logrels_list( struct event_case *ec);
/**< Retrieves the list of logrels of the given case. */
extern void event_case_set_logrels_list(struct event_case *ec,
                                        struct gl_list_t *lrlist);
/**<
 *  Sets the list of logrels of the given case.
 */

extern struct gl_list_t *event_case_events_list( struct event_case *ec);
/**<  Retrieves the list of events nested in the given case. */
extern void event_case_set_events_list( struct event_case *ec,
                                        struct gl_list_t *elist);
/**<
 *  Sets the list of events nested in the given case.
 */

extern void event_case_set_case_number(struct event_case *ec, int32 case_number);
/**<
 * Sets the case_number field of a case to the value of the case_number given.
 */

extern uint32 event_case_flagbit(struct event_case *ec, uint32 name);
/**<
 *  Returns the value of the bit specified from the case flags.
 *  name should be an EVENT_CASE_xx flag
 */

extern void event_case_set_flagbit(struct event_case *ec,
                                   uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 */

/*
 * the bit flags.
 */

#define EVENT_CASE_ACTIVE        0x1
/**<  Is this case active?  */

#ifdef NDEBUG
#define event_case_active(case) ((case)->flags & EVENT_CASE_ACTIVE)
#else
#define event_case_active(case) event_case_flagbit((case),EVENT_CASE_ACTIVE)
#endif

#define event_case_set_active(case,bv)   \
               event_case_set_flagbit((case),EVENT_CASE_ACTIVE,(bv))
/**<
 *  Sets the EVENT_CASE_ACTIVE bit flag of case to bv.
 *  Any value other than 0 for bv turns the bit flag to 1.
 *  0 sets it to 0.
 *  @param case struct event_case *, the case to modify.
 *  @param bv   uint32, the new value for the flag (1 or 0).
 *  @return No return value.
 */

#endif  /* ASC_COND_EVENT_H */

