/*
 *  Discrete Variable Module
 *  by Vicente Rico-Ramirez
 *  Created: 06/96
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: discrete.h,v $
 *  Date last modified: $Date: 1998/03/30 22:06:54 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */                                   

/** @file
 *  Discrete Variable Module.
 *  <pre>
 *  Contents:     Discrete Variable module (ascend)
 *
 *  Dates:        06/96 - original version
 *
 *  Description:  This is the ascend version of the bvar module.  This
 *                version should be used by any user who receives his/her
 *                equations indirectly from an instance tree created by the
 *                ASCEND compiler.
 *
 *  Requires:     #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *                #include "slv_types.h"
 *                #include "list.h"
 *  </pre>
 */

#ifndef ASC_DISCRETE_H
#define ASC_DISCRETE_H

/** Kinds of discrete variables. */
enum discrete_kind {
  e_dis_boolean_t,
  e_dis_integer_t,
  e_dis_symbol_t,
  e_dis_error_t
};

/** Discrete variable data structure. */
struct dis_discrete {
  enum discrete_kind t;         /**< kind of discrete variable */
  SlvBackendToken datom;        /**< the associated ascend ATOM */
  struct dis_discrete **sos;    /**< not implemented yet. Used to represent */
                                /**< integer and symbols in terms of booleans */
  struct dis_discrete *source;  /**< Not implemented yet. This would be used for
                                     booleans, members of sos, to point back to
                                     the variable which is using it */
  struct gl_list_t *whens;      /**< whens in which this variable is used */
  int32 range;                  /**< Ranges for integer or symbols.Sme as above */
  int32 cur_value;              /**< current value */
  int32 pre_value;              /**< previous value */
  int32 sindex;                 /**< index in the solver clients list */
  int32 mindex;                 /**< index in the slv_system_t master list */
  uint32 flags;                 /**< batch of binary flags */
};

/**
 * Discrete variable filter info.
 * Do not dereference this structure except
 * via macros/functions, because we make no commitments about being
 * backward compatible with such code.
 */
typedef struct dis_filter_structure {
  uint32 matchbits;
  uint32 matchvalue;
} dis_filter_t;


extern struct dis_discrete *dis_create(SlvBackendToken instance,
                                       struct dis_discrete *newdis);
/**<
 *  <!--  dis_create(instance,newdis)                                  -->
 *  <!--  struct dis_discrete *newdis;                                 -->
 *  <!--  SlvBackendToken instance;                                    -->
 *
 *  Creates a discrete variable given the variable instance.
 *  If the discrete var  supplied is NULL, we allocate the memory for the
 *  discrete var  we return, else we just init the memory you hand us and
 *  return it to you.
 *  We set the fields instance. Setting the rest of the information
 *  is the job of the bridge building function between the ascend instance
 *  tree (or other discrete var  back end) and the slv_system_t.
 */

#ifdef NDEBUG
#define dis_instance(dis) ((dis)->datom)
#else
#define dis_instance(dis) dis_instanceF(dis)
#endif
/**<
 * Returns the ATOM instance associated with the variable.
 * @param dis   const struct dis_discrete*, the discrete var to query.
 * @return The instance as a SlvBackendToken.
 * @see dis_instanceF()
 */

#ifdef NDEBUG
#define dis_set_instance(dis,inst) ((dis)->datom = (inst))
#else
#define dis_set_instance(dis,inst) dis_set_instanceF((dis),(inst))
#endif
/**<
 * Sets the ATOM instance associated with the variable.
 * @param dis  const struct dis_discrete*, the discrete var to modify.
 * @param inst SlvBackendToken, the new instance to associate with dis.
 * @return No return value.
 * @see dis_set_instanceF()
 */

extern SlvBackendToken dis_instanceF(const struct dis_discrete *dis);
/**<
 * Implementation function for dis_instance() (debug mode).
 * Do not call this function directly - use dis_instance() instead.
*/
extern void dis_set_instanceF(struct dis_discrete *dis, SlvBackendToken i);
/**<
 * <!--  i = dis_instance(dis);                                        -->
 * <!--  dis_set_instance(dis,i);                                      -->
 * <!--  struct dis_discrete *dis;                                     -->
 * <!--  SlvBackendToken i;                                            -->
 * <!--  Sets/returns the ATOM instance associated with the variable.  -->
 * Implementation function for dis_set_instance() (debug mode).
 * Do not call this function directly - use dis_set_instance() instead.
 */

extern char *dis_make_name(const slv_system_t sys,
                           const struct dis_discrete *dis);
/**<
 *  <!--  name = dis_make_name(sys,dis)                                -->
 *  <!--  name = dis_make_xname(dis)                                   -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Creates and returns a sufficiently large string storing the
 *  qualified name of var as known by the solver instance tree.
 *  If the instance is not found, "?" is returned.  The string
 *  should be destroyed when no longer in use.<br><br>
 *  <!--  dis_make_xname returns the index name, eg x23 rather than full name. -->
 *
 *  The name of a dis is context dependent, so you have to provide the
 *  slv_system_t from which you got the dis.
 */
extern char *dis_make_xname(const struct dis_discrete *dis);
/**< 
 * Returns the index name, eg x23 rather than full name.
 * @see dis_make_name()
 */

extern void dis_write_name(const slv_system_t sys,
                           const struct dis_discrete *dis,
                           FILE *file);
/**<
 *  <!--  dis_write_name(sys,dis,file);                                -->
 *  Writes a name to the file given.
 *  Does not print any whitespace, including carriage returns.
 *  If sys is NULL, writes full ascend name. If file or var is NULL
 *  does not write.
 */

extern void dis_destroy(struct dis_discrete *dis);
/**<
 *  <!--  dis_destroy(dis);                                            -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Since someone else allocates dis en masse, this just sets
 *  our integrity check to ERROR.
 */

extern struct gl_list_t *dis_whens_list(struct dis_discrete *dis);
/**<  Retrieves the list of whens of the given dis. */
extern void dis_set_whens_list(struct dis_discrete *dis,
                               struct gl_list_t *wlist);
/**<
 *  <!--  wlist = dis_whens_list(dis)                                  -->
 *  <!--  dis_set_whens_list(dis,wlist)                                -->
 *  <!--  struct gl_list_t *wlist;                                     -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Sets the list of whens of the given dis.
 */

#ifdef NDEBUG
#define dis_kind(dis) (dis)->t
#else
#define dis_kind(dis) dis_kindF(dis)
#endif 
/**<
 * Return the enumerated type that indicates the type of dis.
 * @param dis   const struct dis_discrete*, the discrete var to query.
 * @return The type as an enum discrete_kind.
 * @see dis_kindF()
 */

#ifdef NDEBUG
#define dis_set_kind(dis,kind) (dis)->t = (kind)
#else
#define dis_set_kind(dis,kind) dis_set_kindF((dis),(kind))
#endif 
/**<
 * Sets the enumerated type that indicates the type of dis.
 * @param dis   const struct dis_discrete*, the discrete var to modify.
 * @param kind  enum discrete_kind, the new type for dis.
 * @return No return value.
 * @see dis_set_kindF()
 */

extern enum discrete_kind dis_kindF(const struct dis_discrete *dis);
/**<
 * Implementation function for dis_kind() (debug mode).
 * Do not call this function directly - use dis_kind() instead.
 */
extern void dis_set_kindF(struct dis_discrete *dis,
                          enum discrete_kind kind);
/**<
 * <!--  enum discrete_kind dis_kind(dis);                             -->
 * <!--  void dis_set_kind(dis,kind);                                  -->
 * <!--  const struct dis_discrete *dis;                               -->
 * <!--  enum discrete_kind kind;                                      -->
 * <!--  Gets/sets the enumerated type that indicates the type of dis. -->
 * Implementation function for dis_set_kind() (debug mode).
 * Do not call this function directly - use dis_set_kind() instead.
 */

#ifdef NDEBUG
#define dis_mindex(dis) (dis)->mindex
#else
#define dis_mindex(dis) dis_mindexF(dis)
#endif 
/**<
 *  Gets the index of the variable as it appears in a variable list.
 *  @param dis    const struct dis_discrete*, the discrete var to query.
 *  @return The index as an int32.
 *  @see dis_mindexF()
 */

#ifdef NDEBUG
#define dis_set_mindex(dis,index) (dis)->mindex = (index)
#else
#define dis_set_mindex(dis,index) dis_set_mindexF((dis),(index))
#endif 
/**<
 *  Sets the index of the variable as it appears in a variable list.
 *  @param dis    const struct dis_discrete*, the discrete var to modify.
 *  @param index  int32, the index value.
 *  @return No return value.
 *  @see dis_set_mindexF()
 */

extern int32 dis_mindexF(const struct dis_discrete *dis);
/**<
 *  Implementation function for dis_mindex() (debug mode).
 *  Do not call this function directly - use dis_mindex() instead.
 */
extern void dis_set_mindexF(struct dis_discrete *dis, int32 mindex);
/**<
 *  <!--  index = dis_mindex(dis)                                      -->
 *  <!--  dis_set_mindex(dis,index)                                    -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  <!--  Gets/sets the index of the variable as it appears in a variable list. -->
 *  Implementation function for dis_set_mindex() (debug mode).
 *  Do not call this function directly - use dis_set_mindex() instead.
 */

#ifdef NDEBUG
#define dis_sindex(dis) (dis)->sindex
#else
#define dis_sindex(dis) dis_sindexF(dis)
#endif 
/**<
 *  Gets the index of the variable as it appears in a solvers variable list.
 *  @param dis    const struct dis_discrete*, the discrete var to query.
 *  @return The index as an int32.
 *  @see dis_sindexF()
 */

#ifdef NDEBUG
#define dis_set_sindex(dis,index) (dis)->sindex = (index)
#else
#define dis_set_sindex(dis,index) dis_set_sindexF((dis),(index))
#endif 
/**<
 *  Sets the index of the variable as it appears in a solvers variable list.
 *  @param dis    const struct dis_discrete*, the discrete var to modify.
 *  @param index  int32, the index value.
 *  @return No return value.
 *  @see dis_set_sindexF()
 */

extern int32 dis_sindexF(const struct dis_discrete *dis);
/**<
 *  Implementation function for dis_sindex() (debug mode).
 *  Do not call this function directly - use dis_sindex() instead.
 */
extern void dis_set_sindexF(struct dis_discrete *dis, int32 sindex);
/**<
 *  <!--  index = dis_sindex(dis)                                      -->
 *  <!--  dis_set_sindex(dis,index)                                    -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  <!--  Gets/sets the index of the variable as it appears            -->
 *  <!--  in a solvers variable list.                                  -->
 *  Implementation function for dis_set_sindex() (debug mode).
 *  Do not call this function directly - use dis_set_sindex() instead.
 */

extern int32 dis_value(const struct dis_discrete *dis);
/**<
 *  Gets the currrent value field of the discrete variable.
 */
extern void dis_set_value(struct dis_discrete *dis, int32 value);
/**<
 *  <!--  value = dis_value(dis)                                       -->
 *  <!--  dis_set_value(dis,value)                                     -->
 *  <!--  int32 value;                                                 -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Sets the currrent value field of the discrete variable.
 *  dis_set_value() also assigns the value before modification to
 *  the previous value field of the dis_discrete
 */

extern void dis_set_inst_and_field_value(struct dis_discrete *dis,
                                         int32 value);
/**<
 *  Set the current value of the dis_discrete and the value of
 *  the corresponding instance simultaneously. Such a value is
 *  passed as argument
 *  In the case of symbols, dis_set_inst_and_field_value() has no effect,
 *  since the value of a symbol instance is not a integer, but a symchar,
 *  and a solver client will never redefine it.
 *  If the token is constant, the assignment wont be done and the
 *  value is not affected.
 *
 *  It also assigns the value before modification to
 *  the previous value field of the dis_discrete
 */
extern void dis_set_value_from_inst(struct dis_discrete *dis,
                                    struct gl_list_t *symbol_list);
/**<
 *  Set the current value of a dis_discrete based on the value of the
 *  corresponding instance.
 *  If the token is constant, the assignment wont be done and the
 *  value is not affected.
 *
 *  It also assigns the value before modification to
 *  the previous value field of the dis_discrete
 */
#define dis_set_boolean_value(dis,val) dis_set_inst_and_field_value(dis,val)
/**<
 *  <!--  void dis_set_inst_and_field_value                            -->
 *  <!--  Set the current value of the dis_discrete and the value of   -->
 *  <!--  the corresponding instance simultaneously. Such a value is   -->
 *  <!--  passed as argument                                           -->
 *  <!--  void dis_set_value_from_inst:                                -->
 *  <!--  Set the current value of a dis_discrete based on the value of the     -->
 *  <!--  corresponding instance.                                      -->
 *  <!--  In the case of symbols, dis_set_inst_and_field_value has no effect,   -->
 *  <!--  since the value of a symbol instance is not a integer, but a symchar, -->
 *  <!--  and a solver client will never redefine it.                  -->
 *  <!--  If the token is constant, the asssignment wont be done and the        --> 
 *  <!--  value is not affected.                                       -->
 *  <!--  the las function is particularly useful for symbol instances.-->
 *  <!--  It is used to assign integer values to the field             -->
 *  <!--  value of a dis, using values coming from dis->datom where    -->
 *  <!--  datom is a inst. In teh case of symbol, we get an "equivalent" -->
 *  <!--  integer value.                                               -->
 *
 *  <!--  They also assign the value before modification to            -->
 *  <!--  the previous value field of the dis_discrete                 -->
 *  Used to assign integer values to the field
 *  value of a dis, using values coming from dis->datom where
 *  datom is a inst. In the case of symbol, we get an "equivalent"
 *  integer value.
 *  This function is particularly useful for symbol instances.
 *
 *  It also assigns the value before modification to
 *  the previous value field of the dis_discrete
 */

extern int32 dis_previous_value(const struct dis_discrete *dis);
/**<  Gets the previous value field of the discrete variable. */
extern void dis_set_previous_value(struct dis_discrete *dis,
                                   int32 value);
/**<
 *  <!--  value = dis_previous_value(dis)                              -->
 *  <!--  dis_set_previous_value(dis,value)                            -->
 *  <!--  int32 value;                                                 -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Sets the previous value field of the discrete variable.
 */


/*
 * What constitutes a boolean_var
 * is controlled by the ascend server via the following functions.
 * Clients shouldn't use these.
 */

#define BOOLEAN_VAR_STR  "boolean_var"

extern boolean boolean_var(SlvBackendToken inst);
/**<
 *  <!--  e.g. if (boolean_var(inst)) {}                               -->
 *  <!--  SlvBackendToken inst;                                        -->
 *  Returns true if the instance in question matches the currently
 *  known definition of boolean_var.
 */

extern boolean set_boolean_types(void);
/**<
 *  Sets (changes) the current definition of boolean_var to match
 *  the current library. Returns 1 if unsuccessful, 0 if ok.
 */

extern int32 dis_nominal(struct dis_discrete *dis);
/**<
 *  Gets the nominal value of the boolean variable.
 *  If no nominal field in dis, returns 1.
 */
extern void dis_set_nominal(struct dis_discrete *dis, int32 nominal);
/**<
 *  <!--  nominal = dis_nominal(dis)                                   -->
 *  <!--  dis_set_nominal(dis,nominal)                                 -->
 *  <!--  int32 nominal;                                               -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Sets the nominal value of the boolean variable.
 *  <!--  If no nominal field in dis, returns 1.                       -->
 */

extern uint32 dis_fixed(struct dis_discrete *dis);
/**<
 *  Gets the fixed flag of the boolean variable. This
 *  has side effects in the ascend instance, with which
 *  we are keeping the bits in sync.
 */
extern void dis_set_fixed(struct dis_discrete *dis, uint32 fixed);
/**<
 *  <!--  fixed = dis_fixed(dis)                                       -->
 *  <!--  dis_set_fixed(dis,fixed)                                     -->
 *  <!--  uint32 fixed;                                                -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *
 *  Sets the fixed flag of the boolean variable. This
 *  has side effects in the ascend instance, with which
 *  we are keeping the bits in sync.
 */

/*
 * The section of flagbit definitions
 */
#define DIS_INCIDENT          0x1    /**< is this variable incident on some equation in
                                          the slv_system? */
#define DIS_INWHEN            0x2    /**< is this variable in some WHEN in the slv_system? */
#define DIS_BVAR              0x4    /**< do we think this var a 'boolean_var'? */
#define DIS_CONST             0x8    /**< is this discrete variable a constant? */
#define DIS_FIXED             0x10   /**< is this variable considered fixed currently? */
#define DIS_INBLOCK           0x20   /**<  */
#define DIS_ACTIVE            0x40   /**< is this variable currently active ? */
#define DIS_BOOLEAN           0x80   /**< is this variable of type e_dis_boolean_t ? */
#define DIS_VAL_MODIFIED      0x100  /**< Did the value of this variable change after the
                                          last logical solution ? */
#define DIS_CHANGES_STRUCTURE 0x200  /**< Is this discrete variable associated with a WHEN
                                          changes the structural analysis of a conditional
                                          model */

#ifdef NDEBUG
#define dis_flags(dis) ((dis)->flags)
#else
#define dis_flags(dis) dis_flagsF(dis)
#endif 
/**<
 *  Returns the flags field of the var.
 *  @param dis    const struct dis_discrete*, the discrete var to query.
 *  @return The flags as a uint32.
 *  @see dis_flagsF()
 */

#ifdef NDEBUG
#define dis_set_flags(dis,flags) ((dis)->flags = (flags))
#else
#define dis_set_flags(dis,flags) dis_set_flagsF((dis),(flags))
#endif 
/**<
 *  Sets the entire flag field to the value of flags given. 
 *  This flags value should be composed of the DIS_xxx macro values.
 *  @param dis    const struct dis_discrete*, the discrete var to modify.
 *  @param flags  uint32, the flags value.
 *  @return No return value.
 *  @see dis_set_flagsF()
 */

extern uint32 dis_flagsF(const struct dis_discrete *dis);
/**<
 * Implementation function for dis_flags() (debug mode).
 * Do not call this function directly - use dis_flags() instead.
 */
extern void dis_set_flagsF(struct dis_discrete *dis, uint32 flags);
/**<
 *  <!--  dis_flags(dis);                                              -->
 *  <!--  dis_set_flags(dis,flags);                                    -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *  <!--  uint32 flags;                                                -->
 *
 * <!--  dis_flags(dis) returns the flags field of the var.            -->
 * <!--  dis_set_flags(dis,flags) sets the entire flag field to the    -->
 * <!--  value of flags given. This flags value should be composed     -->
 * <!--  of the dis_xxx values defined above.                          -->
 * Implementation function for dis_set_flags() (debug mode).
 * Do not call this function directly - use dis_set_flags() instead.
 */

extern uint32 dis_flagbit(const struct dis_discrete *dis, 
                          const uint32 name);
/**<
 *  <!--  dis_flagbit(dis,name);                                       -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *  <!--  uint32 name;                                                 -->
 *  Returns the value of the bit specified from the variable flags.
 *  name should be a DIS_xx flag defined above)
 */

extern void dis_set_flagbit(struct dis_discrete *dis, 
                            uint32 NAME, uint32 oneorzero);
/**<
 *  <!--  struct dis_discrete *dis;                                    -->
 *  <!--  unsigned int NAME,oneorzero;                                 -->
 *  <!--  dis_set_flagbit(dis,NAME,oneorzero)                          -->
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 */

extern int32 dis_apply_filter(const struct dis_discrete *dis,
                              const dis_filter_t *filter);
/**<
 *  <!--  value = dis_apply_filter(dis,filter)                         -->
 *  <!--  int32 value;                                                 -->
 *  <!--  struct dis_discrete *dis;                                    -->
 *  <!--  dis_filter_t *filter;                                        -->
 *
 *  Returns 1 if filter and var flags are compatible, 0 elsewise.
 *  See the filter description in rel.h. This is exactly the same.
 */

#ifdef NDEBUG
#define dis_inwhen(dis)            ((dis)->flags & DIS_INWHEN)
#define dis_const(dis)             ((dis)->flags & DIS_CONST)
#define dis_in_block(dis)          ((dis)->flags & DIS_INBLOCK)
#define dis_incident(dis)          ((dis)->flags & DIS_INCIDENT)
#define dis_active(dis)            ((dis)->flags & DIS_ACTIVE)
#define dis_boolean(dis)           ((dis)->flags & DIS_BOOLEAN)
#define dis_val_modified(dis)      ((dis)->flags & DIS_VAL_MODIFIED)
#define dis_changes_structure(dis) ((dis)->flags & DIS_CHANGES_STRUCTURE)
#else
#define dis_inwhen(dis)            dis_flagbit((dis),DIS_INWHEN)
#define dis_const(dis)             dis_flagbit((dis),DIS_CONST)
#define dis_in_block(dis)          dis_flagbit((dis),DIS_INBLOCK)
#define dis_incident(dis)          dis_flagbit((dis),DIS_INCIDENT)
#define dis_active(dis)            dis_flagbit((dis),DIS_ACTIVE)
#define dis_boolean(dis)           dis_flagbit((dis),DIS_BOOLEAN)
#define dis_val_modified(dis)      dis_flagbit((dis),DIS_VAL_MODIFIED)
#define dis_changes_structure(dis) dis_flagbit((dis),DIS_CHANGES_STRUCTURE)
#endif /* NDEBUG */

#define dis_set_inwhen(dis,b)      dis_set_flagbit((dis),DIS_INWHEN,(b))
#define dis_set_const(dis,b)       dis_set_flagbit((dis),DIS_CONST,(b))
#define dis_set_in_block(dis,b)    dis_set_flagbit((dis),DIS_INBLOCK,(b))
#define dis_set_incident(dis,b)    dis_set_flagbit((dis),DIS_INCIDENT,(b))
#define dis_set_active(dis,b)      dis_set_flagbit((dis),DIS_ACTIVE,(b))
#define dis_set_boolean(dis,b)     dis_set_flagbit((dis),DIS_BOOLEAN,(b))
#define dis_set_val_modified(dis,b)   \
               dis_set_flagbit((dis),DIS_VAL_MODIFIED,(b))
#define dis_set_changes_structure(dis,b)   \
               dis_set_flagbit((dis),DIS_CHANGES_STRUCTURE,(b))

/*
 *  incident = dis_incident(dis)
 *  dis_set_incident(dis,incident)
 *  uint32 incident;
 *  struct dis_discrete *dis;
 *
 *  Gets/sets the incident flag of the discrete variable.
 */

extern
struct dis_discrete **dis_BackendTokens_to_dis(slv_system_t sys,
                                               SlvBackendToken *tokenlist,
                                               int32 len);
/**<
 *  <!--  dislist = dis_BackendTokens_to_dis(sys,tokenlist,len);       -->
 *  <!--  slv_system_t sys;			System to get indexing from.             -->
 *  <!--  SlvBackendToken tokenlist[];	Array of backend tokens.        -->
 *  <!--  int32 len;			        Tokenlist size.                         -->
 *  <!--  struct dis_discrete *dislist[];	aka **dislist;               -->
 *
 *  dislist is NULL iff something is a miss, OTHERWISE it
 *  contains len struct dis_discrete *.
 *  The user should free the array dislist when done with it.
 *  Some entries in the array dislist may be NULL if the tokenlist
 *  contains a token which is not from the sys given.
 *  tokenlist[i] <--> dislist[i];<br><br>
 *
 *  The whole point of a slv_system_t is to isolate the client from
 *  the compiler backend. Clients who find themselves in need of
 *  this function are very much in need of rethinking as well.
 *  For that reason, among others, this function is not heavily
 *  optimized, it is however reasonable for 1-off jobs.
 */

#endif /* ASC_DISCRETE_H */

