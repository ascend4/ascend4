#ifndef RelCpp_h_seen
/*
 *  This file is part of the SLV-C++ solver.
 *
 *  Copyright (C) 2000 Benjamin Andrew Allan
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
 *
 *  Contents:     C++ Relation interface definition.
 *  Authors:      Ben Allan
 *  Dates:        08/2000 - original version
 *  Description:  This is the abstract interface that ASCEND solver tools
 *                use to access relations from any model database source.
 *
 *  Derived from C rel.h:
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                04/94 - added rel_apply_filter() which uses the new
 *                        rel_filter_t data structure for perfoming all
 *                        relation filtering needs (eliminating the
 *                        filter module)
 *
 *  Description:  This is the ascend version of the rel module.  This
 *                version should be used by any user who receives his/her
 *                equations directly from an instance tree created by the
 *                ASCEND compiler.
 *
 *                The idea of a relation makes no sense outside the context of
 *                a slv_system_t, so some of the functions here may require
 *                the passing of a slv_system_t with the var.
 */
#define RelCpp_h_seen

/** Interface we expect all floating point relations from any model server
 *  to provide.
 *  We assume several things about the relationship of server ro relation:
 *  <ul>
 *  <li>All relations originate from exactly one model server,
 *      but may participate in several systems of equations.
 *      Typically multiple participation only occurs when a compound
 *      algorithm solver iterates among subproblems, as in
 *      an MINLP or DAE, or when 2 independent systems coexist.</li>
 *  <li>Solvers may make changes in the relation residual or attributes 
 *      and reevaluate relations 
 *      without telling the model server about it until the solver
 *      relinquishes control.</li>
 * <li>Most practical math programming algorithms need scaling values,
 *  </ul>
 *  We assume more things about the implementations of this interface:
 *  <ul>
 *  <li>No exceptions are thrown by any of the functions. Particularly,
 *      relations don't leave us to catch their ieee errors; if
 *      necessary this is achieved by returning NaN/Inf/etc. while
 *      setting a flag.</li>
 *  </ul>
 *  Requires:
 *  #include "utilities/ascConfig.h"
 *  #include "solver-cpp/Attributes.h"
 */
class RelCpp {

public:

  /**@ Enumeration, with embedded bit pattern (bits 2 3 and 4), of
      the relation types. */
  //@{
  /** Objective minimization expression. */
  static const int32 opMinimize  =  1;

  /** Objective maximization expression. */
  static const int32 opMaximize  = -1;

  /** Equality relation. */
  static const int32 opEqual     =  2;

  /** LHS > RHS inequality. */
  static const int32 opGreater   =  4;

  /** LHS < RHS inequality. */
  static const int32 opLess      =  8;

  /** LHS >= RHS inequality. */
  static const int32 opGreaterEq =  6;

  /** LHS <= RHS inequality. */
  static const int32 opLessEq    = 10;

  /** LHS <> RHS inequality. */
  static const int32 opNotEq     = 12;

  //@}


  virtual ~RelCpp() {}

  /**@ Core rel functions.  */
  //@{

  /** Return a globally unique identifier. In the serial case,
      contextId is always 0. In the distributed case,
      contextId is significant, probably a processor/process
      id. ID is probably a pointer to the underlying variable
      in a modeling system.
  */
  virtual void getGlobalIds(int64 & contextId, int64 & id) = 0;

  /** Define another system that the relation participates in.
  */
  virtual void addToSystem(SystemCpp *systemId) = 0;

  /** Remove the specified system from the RelCpp's records.
   */
  virtual void removedFromSystem(SystemCpp *systemId) = 0;

  /** Get the model that the relation belongs to, if any.
   */
  virtual ModCpp *getModel();

  /** @return the RelCpp::op* defined above that describes
      the floating point relation in the model backend.
   */
  virtual int32 getOperator();

  /** Returns the current value of the signed residual.
   *  The residual is LHS-RHS
   *  regardless of inequality direction or objective type.
   *  Objectives are always LHS. The product of the
   *  objective value (gradient) and opMax or opMin above is 
   *  appropriate to feed to a minimizing optimization algorithm.
   */
  virtual real64 getValue() = 0;

  /** Refresh relation residual from its original model server.
      Don't know when this is desirable, but there it is. */
  virtual void getServerValue() = 0;

  /** Changes the current value of the residual. */
  virtual void setValue(real64 resid) = 0;

  /** Update value known to model server from current residual value. */
  virtual void setServerValue() = 0;

  /**
   *  Gets the index of the relation as it appears
   *  in the corresponding list for systemId.
   *  Typically the index indicates a row of some matrix,
   *  or the position in a list.
   *  Returns -1 if unknown systemId.
   *  Returns -2 if known systemId, but unassigned index as yet.
   */
  virtual int32 getIndex(SystemCpp *systemId) = 0;

  /**
   *  Sets the index of the relation as it appears
   *  in the corresponding list for systemId.
   *  Typically the index indicates a column of some matrix.
   */
  virtual void setIndex(SystemCpp *systemId, int32 ind) = 0;

  /*
   *  va = getIncidenceList(sys)
   *  VarCpp **va;
   *
   *  Returns a pointer to an array nIncidences(sysId) long of variables.
   *  Each element of the array is a VarCpp *.
   *  Check the var index(sysId) to see where each might go in a matrix.
   *  VarCpp may have an index < 0 for a given system, in which case that
   *  VarCpp is not "an unknown" for the SystemCpp in question.
   *  If there is no incidence, NULL is returned (but that's one WEIRD Rel!).
   *  Pointers in this array will be unique. 
   *  The list belongs to the relation. Do not destroy it. Do not change it.
   *  The relation is not obliged to store its VarCpp references internally
   *  as a list.
   *
   *  @param nV is the length of va; va is NOT a null-terminated list.
   */
  virtual const VarCpp **getIncidenceList(SystemCpp *systemId, int & nV) = 0;

  /*
   *  nIncidences(systemId)
   *
   *  Not everything in the incidence list is necessarily a
   *  relation for your particular solver -- check the VarCpp flags and index.
   *  It is useful for allocation purposes to know the upper limit on
   *  unknowns in a RelCpp.
   *  @return the length of the incidence list, the total count of VarCpp
   *  in the relation, nV. 
   */
  virtual int32 nIncidences(SystemCpp *systemId) = 0;

  //@}

  /**@ User interface functions. Naming, units, dimensions, output.  */
  //@{
  /**
   *  Returns a the qualified name of var as known by its model server.
   *  The char * remains the property of the variable.
   */
  virtual const char *getNameInServer() = 0;

  /** Specify the name of the variable in the specified system.
   *  Returns something printable even if systemId is not known.
   */
  virtual const char *getNameInSystem(SystemCpp *systemId) = 0;

  /**
   *  getIndexName returns the index name, eg x23 rather than full name.
   *  as the var is known to the specified system.
   */
  virtual const char *getIndexName(SystemCpp *systemId) = 0;

  //@}


  /**@  extensible attributes.   A particular model server
        can implement Attributes any way it wants for its own efficiency,
        perhaps assuming some values always and running a list for the
        rest.
    */
  //@{

  /** The current attributes of the RelCpp as perhaps modified in the
   *  system given.
   *
   * Not all attributes are defined for all relations. Not all attributes
   * are defined in every ascend model. Not all attributes appearing on
   * a RelCpp will have a corresponding model attribute.
   *
   * <p>The following attributes are ASCEND implementation legacy artifacts:
   * <ul>
   * <li> int asc.Kind: 0 foreign, 1 token, 2 glassbox, 3 blackbox.</li>
   * <li> void * asc.ExtNodeInfo: Legacy pointer to ASCEND 
   *                              struct rel_extnode *.</li>
   * <li> void * asc.ExtRelCache: Legacy pointer to ASCEND 
   *                              struct ExtRelCache *.</li>
   * <li> void * asc.ExtWhichVar: Legacy pointer to VarCpp.</li>
   * </ul>
   * </p>
   * <p>The following are transient properties set by solvers or tools.:
   * <ul>
   * <li> double nominal: Row scaling value for equations</li>
   * <li> double multiplier: Lagrange multiplier in system wrt most recent 
   *                        objective, if any.</li>
   * <li> bool partition: For reordering clients; is rel in the 
   *                      interesting region.</li>
   * <li> bool torn: From reordering clients output; relation is a 
   *                 tear equation.</li>
   * <li> bool satisfied: Has rel been pronounced satisfied by someone?</li>
   * <li> bool inBlock: Is the relation in the current partition for a
   *                    partitioning client</li>
   * <li> bool inCurSubregion: Is the relation in the subregion currently 
   *                      analyzed by a when-aware client?</li>
   * <li> bool generated: Is rel fake and cooked up for this system only?</li>
   * </ul>
   * </p>
   * <p>The following attributes are generally set by the model backend or the
   *    SystemCpp factory and not manipulated by clients.
   * <ul>
   * <li> bool interface: For solvers from ui clients; user suggests it's a 
   *                      tear equation</li>
   * <li> bool included: For solvers from ui clients; user wants eqn 
   *                     in problem.</li>
   * <li> bool inWhen: Is relation in a when in this system?</li>
   * <li> bool conditional: is relation conditional in the system?</li>
   * <li> bool active: Is this relation currently a part of my problem, 
   *                   for when-aware clients?</li>
   * <li> bool invariant: Is this relation an invariant in the conditional 
   *                      modeling analysis.</li>
   * <li> bool objNegate: Is objective a maximize?</li>
   * </ul>
   * </p>
   * And finally here's a known symbol attribute:
   *    message.
   */
  virtual Attributes *getAttributes(SystemCpp *sysId) = 0;

  /** The model backend database attributes corresponding to the RelCpp.
      Changes made in these attributes will be communicated ASAP back
      to the corresponding database. */
  virtual Attributes *getServerAttributes() = 0;

  //@}

  /**@ Output- eg: char *message = "foo"; p()->out(message);. */
  virtual Output *p() = 0;

};
#endif // RelCpp_h_seen
