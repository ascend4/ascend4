#ifndef VarCpp_h_seen
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
 *  Contents:     C++ Variable interface definition.
 *  Authors:      Ben Allan
 *  Dates:        08/2000 - original version
 *  Description:  This is the abstract interface that ASCEND solver tools
 *                use to access variables from any model database source.
 *
 *  Derived from C var.h:
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                01/94 - added var_make_name() to create a copy of the
 *                        instance name
 *                04/94 - added var_apply_filter() which uses the new
 *                        var_filter_t data structure for perfoming all
 *                        variable filtering needs (eliminating the
 *                        filter module)
 *		    08/94 - added var_BackendTokens_to_vars. BAA.
 *
 *  Description:  This is the ascend version of the var module.  This
 *                version should be used by any user who receives his/her
 *                equations indirectly from an instance tree created by the
 *                ASCEND compiler.
 *
 *                The idea of a var makes no sense outside the context of
 *                a slv_system_t, so some of the functions here may require
 *                the passing of a slv_system_t with the var.
 */
#define VarCpp_h_seen

/** Interface we expect all variables from any model server
 *  to provide.
 *  We assume several things about the relationship of server to variable:
 *  <ul>
 *  <li>All variables originate from exactly one model server,
 *      but may participate in several systems of equations.
 *      Typically multiple participation only occurs when a compound
 *      algorithm solver iterates among subproblems, as in
 *      an MINLP or DAE.</li>
 *  <li>Solvers may make changes in the variable value or attributes 
 *      and reevaluate relations in which the variable is incident 
 *      without telling the model server about it until the solver
 *      relinquishes control.</li>
 * <li>Most practical math programming algorithms need lower and
 *     upper bounds, scaling values,
 *  </ul>
 *  We assume more things about the implementations of this interface:
 *  <ul>
 *  <li>No exceptions are thrown by any of the functions.</li>
 *  </ul>
 *  Requires:
 *  #include "utilities/ascConfig.h"
 *  #include "solver-cpp/Attributes.h"
 */
class VarCpp {

public:

  /** The largest value a model server should ever send us. */
  static const double NO_UPPER_BOUND = MAXDOUBLE;

  /** The largest negative value a model server should ever send us. */
  static const double NO_LOWER_BOUND = (-MAXDOUBLE/2);


  virtual ~VarCpp() {}

  /**@ Core var functions.  */
  //@{

  /** Return a globally unique identifier. In the serial case,
      contextId is always 0. In the distributed case,
      contextId is significant, probably a processor/process
      id. ID is probably a pointer to the underlying variable
      in a modeling system.
  */
  virtual void getGlobalIds(int64 & contextId, int64 & id) = 0;

  /** Define another system that the variable participates in.
  */
  virtual void addToSystem(SystemCpp *systemId) = 0;

  /** Remove the specified system from the Var's records.
   */
  virtual void removedFromSystem(SystemCpp *systemId) = 0;

  /** Returns the current value of the variable. */
  virtual real64 getValue() = 0;

  /** Refresh var value from its original model server. */
  virtual void getServerValue() = 0;

  /** Changes the current value of the variable. */
  virtual void setValue(real64 val) = 0;

  /** Update value known to model server from var current value. */
  virtual void setServerValue() = 0;

  /**
   *  Gets the index of the variable as it appears
   *  in the variable list for systemId.
   *  Typically the index indicates a column of some matrix.
   *  Returns -1 if unknown systemId.
   *  Returns -2 if known systemId, but unassigned index as yet.
   */
  virtual int32 getIndex(SystemCpp *systemId) = 0;

  /**
   *  Sets the index of the variable as it appears
   *  in the variable list for systemId.
   *  Typically the index indicates a column of some matrix.
   */
  virtual void setIndex(SystemCpp *systemId, int32 ind) = 0;

  /*
   *  ra = getIncidenceList(sys)
   *  RelCpp **ra;
   *
   *  Returns a pointer to an array nIncidences(sysId) long of relations.
   *  Each element of the array is a RelCpp *.
   *  Check the var index(sysId) to see where each might go in a jacobian.
   *  If there is no incidence, NULL is returned.
   *  Pointers in this array will be unique. 
   *  The list belongs to the variable. Do not destroy it. Do not change it.
   *
   *  ra is NOT a null-terminated list.
   */
  virtual const RelCpp **getIncidenceList(SystemCpp *systemId) = 0;

  /*
   *  nIncidences(systemId)
   *
   *  Not everything in the incidence list is necessarily a
   *  relation for your particular solver -- check the rel flags.
   *  @return the length of the incidence list, the relations
   *  in which the variable appears.
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

  /** The current attributes of the VarCpp as perhaps modified by solvers.
   *
   * Not all attributes are defined for all vars. Not all attributes
   * are defined in every ascend model. Not all attributes appearing on
   * a VarCpp will have a corresponding model attribute.
   *
   * The first few are floating point, next few integer, and rest boolean.
   * <ul>
   * <li> nominal: a user-defined scaling value, usually near
   *               the expected solution, that can be used in 
   *               scaling algorithms.
   * <li> lower_bound: if the model server has no user-defined bound,
   *                   use some notion of -Infinity >= NO_LOWER_BOUND.
   * <li> upper_bound: if the model server has no user-defined bound,
   *                   use some notion of Infinity <= NO_UPPER_BOUND.
   * <li> ode_rtol: relative tolerance for integration algorithms which
   *                consider this a state variable.
   * <li> ode_atol: absolute tolerance for integration algorithms which
   *                consider this a state variable.
   * </ul>
   * <ul>
   * <li> ode_type: type of DAE variable -1 indep, 0 alg, 1 state, 2 deriv.
   * <li> ode_id: integer identifier of the ode state variable.
   * <li> obs_id: integer identifier of the observation variable.
   * </ul>
   * By default, the following boolean fields are set by model servers and
   * might be manipulated by exceptionally clever clients:
   * <ul>
   * <li> REAL: someday may allow complex, too, if we template this interface.
   * <li> INCIDENT: is this variable incident on some equation in 
   *                the system?
   * <li> PVAR: is this var a 'solver_par'?
   * <li> SVAR: is this var a 'solver_var'?
   * <li> INTEGER: is this var a 'solver_integer'?
   * <li> BINARY: is this var a 'solver_binary'?
   * <li> SEMICONT: is this var a 'solver_semi', a semi_continuous variable?
   * <li> is_zero: is this var a semi_continuous variable at 0?
   * </ul>
   *
   * The remaining boolean flag definitions are those flags to be
   * manipulated by registered clients. Unregistered clients
   * should not change these bits unless the manner in which
   * they affect the bits is explained in the header for the
   * unregistered client.
   * <ul>
   * <li>PARAM: is this variable considered parametric currently?
   * <li>fixed: is this variable considered fixed currently?
   * <li>INBLOCK: is this var in the columns of the block we are
   *              currently solving?
   * <li>interface: is this var an interface var currently?
   * <li>relaxed: is this var currently a relaxed int or binary or semi var?
   * <li>ACTIVE: is this var currently a part of my problem?
   * <li>NONBASIC: is this var currently an independent variable in 
   *               optimization?
   * <li>ACTIVE_AT_BND: is this var active in some of the subregions
   *                    neighboring the boundary on which the problem
   *                    currently lies ?
   * <li>ELIGIBLE: is this variable eligible to be fixed (globally)?
   * <li>ELIGIBLE_IN_SUBREGION: is this variable eligible to be fixed
   *                            (in current subregion) ?
   * <li>POTENTIALLY_FIXED: this bit is auxiliar while iteratively 
   *                        and recurively
   *                        we are performing a combinatorial search, looking
   *                        for a consitent partition for all the alternatives
   *                        in an conditional model.
   * <li>INCIDENT_IN_CASE: is this variable incident in some relation of a
   *                       CASE in a WHEN statement ?. This is for purposes of
   *                       simplfying the analysis of conditional models.
   * </ul>
   * And finally here's a known symbol attribute:
   *    message.
   */
  virtual Attributes *getAttributes(SystemCpp *sysId) = 0;

  /** The model backend database attributes corresponding to the VarCpp.
      Changes made in these attributes will be communicated ASAP back
      to the corresponding database. */
  virtual Attributes *getServerAttributes() = 0;

  //@}

  /**@ Output- eg: char *message = "foo"; p()->out(message);. */
  virtual Output *p() = 0;

};
#endif // VarCpp_h_seen
