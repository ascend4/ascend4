/*
 *  UnitsProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: UnitsProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:09 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#ifndef UnitsProc_module_loaded
#define UnitsProc_module_loaded

/* requires #include "interface1.h" */
/* requires #include "instance_enum.h" */
/* requires #include "base.h" */
/* requires #include "list.h" */
/* requires #include "dimen.h" */
/* requires #include "units.h" */

/*
 *  Display precision is handled here. (see Set/Get_Prec below.)
 *
 *  Base units are the units which correspond to and are displayed
 *  instead of single DIMENSIONs. Numbers which are dimensional but have
 *  no display unit of corresponding dimensionality will be shown in the
 *  appropriate combination of base units, aka fundamental units.
 *  This should keep the number of DIM/units on the internal lists from
 *  getting out of hand when talking about multipliers, derivatives, etc
 */

extern struct Units * g_base_units[NUM_DIMENS];
/* These are the working base units */

extern struct Units * g_SI_units[NUM_DIMENS];
/*
 *  These are the backup base units. If a number cannot be displayed
 *  due to floating point error in conversion, it will be displayed in
 *  these units (all of which have conversion factors of 1.)
 */

extern char *Asc_UnitValue(CONST struct Instance *);
/*
 *  Return a pointer to a string containing the value of i in display
 *  units/precision provided i is REAL_ATOM_INST, REAL_INST, REL_INST,
 *  INTEGER_INST, INTEGER_ATOM_INST, otherwise returns null.
 *  This string is not yours and the pointer will only be valid until
 *  the next call to Unit_Value.
 *  The contents of the string are:
 *  value units
 *  where value will be to UPREC places for reals, or all places for int*
 *  and units will be consistent with value. Wild dim'd numbers will
 *  have * for units and dimensionless numbers will have nothing. If
 *  Unit_Value is called on an invalid instance kind, return is NULL.
 *  If instance is !AtomAssigned, return is "UNDEFINED units"
 *  To destroy the current display string, call is Unit_Value(NULL).
 */

extern char *Asc_UnitlessValue(CONST struct Instance *, int);
/*
 *  Unitless_Value(i,SI);
 *  Like Unit_Value, except that contents of the string returned
 *  do not include the units and the printed value in the string
 *  will be in SI instead of display units if SI is TRUE.
 *  If the number cannot be printed in display units, the SI value
 *  is printed instead with a warning on stderr.
 */

extern char *Asc_UnitString(CONST struct Instance *, int);
/*
 *  Unit_String(i,SI);
 *  Like Unit_Value, except that contents of the string returned
 *  do not include the value of the instance. If SI is TRUE, units
 *  returned will be SI units rather than units window specified units.
 */

extern char *Asc_UnitDimString(dim_type *, int);
/*
 *  Unit_String(dimp,SI);
 *  Like Unit_String, except that the instance is not needed.
 *  If SI is TRUE, units returned will be SI units rather than units
 *  window specified units.
 *  if dimp is NULL, return is NULL.
 */

extern int Asc_UnitConvert(struct Units *, double, double *, int);
/*
 *  Unit_Convert(u,in,out,direction);
 *  Attempts to unit convert the value in double to/from the units given.
 *  If args are wrong or conversion fails, return value is 1 and *out
 *  may be undefined.
 *  If successful return is 0 and out has the resulting value.
 *  If direction is 0 convert in{units} to out{si}.
 *  If direction != 0 convert in{si} to out{units}.
 */

extern int Asc_UnitSetRealAtomValue(CONST struct Instance *,
                                char *, /* value */
                                char *, /* units */
                                unsigned /* depth */);
/*
 *  Sets value of a real or real atom instance if units are
 *  dimensionally compatible and value fits within a double in SI units.
 *  If atom is wild, dimensionality will be set by this call unless
 *  the second char argument is null/empty/"*".
 *  Return codes: 0 => ok
 *  1 => Unparseable units given- Not assigned
 *  2 => Dimensionally incompatible units- Not assigned
 *  3 => Overflow in converting to SI value- Not assigned
 *  4 => Called with non-real instance
 *  5 => Unparseable value given- Not assigned
 *  depth is passed on to the SetRealAtomValue call inside this call.
 */

extern int Asc_UnitGetCPrec();
/*
 *  return the current display precision for use in C
 */

extern int Asc_UnitDestroyDisplayList(ClientData cdata, Tcl_Interp *interp,
                                      int argc, CONST84 char *argv[]);
/*
 *  u_destroy_list
 *  trash the interface units lookup structure. Don't call this if you
 *  aren't about to exit or bad things will happen when next you display.
 */

extern int Asc_UnitDefaultBaseUnits(ClientData cdata, Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/*
 *  u_setSIdef
 *  this function establishes the short form of SI mks as the display
 *  defaults for units. (kg,s,m,K,C,mole,cd,CR)
 *  it needs to be updated if a new dimension is added.
 *  Cares not what args it gets.
 */

extern int Asc_UnitGetBaseUnits(ClientData cdata, Tcl_Interp *interp,
                                 int argc, CONST84 char *argv[]);
/*
 *  u_getbasedef
 *  This returns a list of the current default base units.
 *  If none have been set, calls Unit_default_baseunits before going on
 */

extern int Asc_UnitDump(ClientData cdata, Tcl_Interp *interp,
                        int argc, char CONST84 *argv[]);
/*
 *  Spew the units defined to stdout, stderr, or a list
 *  u_dump <0,1,2> [pretty]
 *  if pretty is present, option 2 list will have nice whitespace
 *  u_dump 2 --> {dimenId unitname convfactor unitsSI dimen} tuples
 */

extern int Asc_DimenDump(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[]);
/*
 *  Spew the dimens registered to stdout, stderr, or a list
 *  u_dims <0,1,2>
 */

extern int Asc_DimenRelCheck(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/*
 *  u_dim_setverify $dimconsistency
 *  set relation dim checking to on (1) or off (0).
 *  applies only to the checking performed in making unit strings.
 */

extern int Asc_UnitBaseDimToNum(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  return the number of a simple dimension
 *  u_dim2num <dimname>
 */

extern int Asc_UnitNumToBaseDim(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  return the Dim of a corresponding number
 *  u_num2dim <dimindex>
 */

extern int Asc_UnitMatchBaseDim(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  return all the unit names which match the dimension number given.
 *  u_frombasedim <dimindex>
 *  unit names will be in order of decreasing conversion factor
 *  and increasing length and alphabetically
 *  A bit more efficient than using UnitMatch_AtomDim.
 */

extern int Asc_UnitMatchAtomDim(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  return all the unit names which match the dimension set of
 *  given real atom type, sorted as in Match_BaseDim
 *  u_fromatomdim <atomname>
 */

extern int Asc_UnitGetAtomList(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/*
 *  return a list of dimensioned atoms and dimensionalities.
 *  wild and dimensionless are not considered dimensioned.
 *  u_getdimatoms
 */

extern int Asc_UnitChangeBaseUnit(ClientData cdata, Tcl_Interp *interp,
                                  int argc, CONST84 char *argv[]);
/*
 *  u_change_baseunit <unit>
 *  Change the default display unit of the dimension implied by unit to
 *  be unit. Will return an error if unit does not have a simple
 *  dimensionality.
 *  Note: This will not catch semantic errors with dimensionless * dimd
 */

extern int Asc_UnitSetUser(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_set_user <units>
 *  Set the user specified units for the dimensionality they imply.
 *  Note: This will not catch semantic errors with dimensionless * dimd
 */

extern int Asc_UnitGetAtomsForUnit(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/*
 * u_get_atoms <units>
 * Returns the list of atoms and constants matching the units
 * given, if any.
 */

extern int Asc_UnitGetPrec(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_getprec
 *  get display value precision, principally useful for reals.
 */

extern int Asc_UnitSetPrec(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_setprec <number 4-16>
 *  set display value precision, principally useful for reals.
 *  bugs: should use ansi prec info instead of 16 upper limit.
 */

extern int Asc_UnitGetUnits(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  u_get_units <atomname>
 *  return user set (or if never set, then default set) units for dims
 *  of atom type given.
 */

extern int Asc_UnitGetUser(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_get_user <atomname>
 *  return user set units for dims of atom given or "default" if unset.
 */

extern int Asc_UnitGetList(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_get_list
 *  return user set units for all dims DUList
 *  units of defaulted dims not returned
 */

extern int Asc_UnitClearUser(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  u_clear_user <atomname>
 *  unset user set units for dims of atom given.
 */

extern int Asc_UnitGetVal(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[]);
/*
 *  u_getval <qlfdid> returns as Asc_UnitValue if possible from the
 *  instance indicated by qlfdid. In general, this is expensive.
 */

extern int Asc_UnitBrowGetVal(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/*
 *  u_browgetval [search] returns {value} {units} if possible from the
 *  instance indicated by g_curinst (g_searchinst if "search" given)
 */

extern int Asc_UnitSlvGetRelVal(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  u_slvgetrelval <rellist index> returns as Asc_UnitValue if possible
 *  from the relation indicated by index
 */

extern int Asc_UnitSlvGetVarVal(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  u_slvgetvarval <varlist index> returns as Asc_UnitValue if possible
 *  from the variable indicated by index
 */

extern int Asc_UnitSlvGetObjVal(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  u_slvgetobjval <objlist index> returns as Asc_UnitValue if possible
 *  from the obj indicated by index. Until there is an objlist, this
 *  will simply look at obj and ignore index.
 */

extern int Asc_UnitHelpList(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  uhelp command for tcl
 *  uhelp [s,l]
 *  no arg -> return tcl list
 *  "s" -> list names only, "l" -> short explanations also, to stderr
 */

#endif
