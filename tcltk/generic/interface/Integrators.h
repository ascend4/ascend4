/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
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
*//** @file
	Implementation of the Integration Interface.

	This module defines the general integration auxillaries for Ascend.

	Requires
	#include "tcl.h"
	#include "utilities/ascConfig.h
	#include "general/list.h"
	#include "compiler/instance_enum.h"
	#include "compiler/dimen.h"
	#include "solver/slv_client.h"
	#include "compiler/interface.h"
	#include <stdio.h>   (for current INTEG_DEBUG functions)
*//*
	Author:       Boyd T. Safrit
	by Kirk Abbott and Ben Allan
	Created: 1/94
	Version: $Revision: 1.16 $
	Version control file: $RCSfile: Integrators.h,v $
	Date last modified: $Date: 2003/08/23 18:43:06 $
	Last modified by: $Author: ballan $

	Changes:  8/95  Added support code for BLSODE, an
	                experimental user interface
	                which identifies states, derivatives,
	                observations, and tolerances by flags.
	                Interface redesign (ASCEND side) by
	                Jennifer Stokes and (C side) Ben Allan.
	                The only required vector left is that
	                defining the time steps to calculate
	                results at. All state and observation
	                data are (optionally) logged to files.
	          5/96  Removed old lsode interface routines.
	                The Piela version of lsode is dead.
	          8/96  Removed the var == instance assumption from
	                the implementation.
*/


#ifndef ASCTK_INTEGRATORS_H
#define ASCTK_INTEGRATORS_H

#include <solver/integrator.h>

extern int Asc_IntegSetYFileCmd(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/**<
 *  Set the next filename to be used for integration state variable
 *  logging. Does not check the filesystem sanity of the given name.<br><br>
 *
 *  Registered as: integrate_set_y_file filename
 */

extern int Asc_IntegSetObsFileCmd(ClientData cdata, Tcl_Interp *interp,
                                  int argc, CONST84 char *argv[]);
/**<
 *  Set the next filename to be used for integration observation variable
 *  logging. Does not check the filesystem sanity of the given name.<br><br>
 *
 *  Registered as: integrate_set_obs_file filename
 */

extern int Asc_IntegSetFileUnitsCmd(ClientData cdata, Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/**<
 *  Sets output to be in SI (internal) units or in the user set display
 *  units. If display is selected and the variable to be printed cannot
 *  be displayed in the display units because of floating point errors
 *  in conversion, the value will be printed in SI units and a warning
 *  issued to stderr. The user is then expected to deal with the problem
 *  of inconsistency in his output.
 *  Only the first letter of the argument is significant.<br><br>
 *
 *  Registered as: integrate_logunits <display,si>
 */

extern int Asc_IntegSetFileFormatCmd(ClientData cdata, Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[]);
/**<
 *  Sets output data to be in fixed column width (with extra white) or in
 *  space separated columns whose width is variable from line to line.
 *  Only the first letter of the argument is significant.<br><br>
 *
 *  Registered as: integrate_logformat <fixed,variable>
 */



extern int Asc_IntegGetXSamplesCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Returns the current vector of independent variable samples as fed to
 *  smarter integrators such as lsode. Returns 2 element list:
 *  {units of x} {list of doubles in the units indicated in the first
 *  element}. In the event that display is given but the sample values
 *  (which are stored in si) cannot be converted to display units, the
 *  units of x in the first element will change and the SI values will
 *  be returned in the second element. If display is not given, samples
 *  will be returned in SI units.
 *  The units used will be dimensionally consistent with those specified
 *  when integrate_set_samples was called.
 *  Only the first letter of the optional argument display is significant.<br><br>
 *
 *  Registered as: integrate_get_samples [display]
 */

extern int Asc_IntegSetXSamplesCmd(ClientData data, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Takes the values given and sets the intervals for smarter
 *  integrators accordingly. Values will be converted to SI values
 *  using the units given. ?,* is a legal unit, in which case the
 *  values will be assumed to be SI requiring no conversion.
 *  If no arguments are given, we will free memory allocated for storage
 *  of the list, otherwise at least 2 values are required.<br><br>
 *
 *  Note: It is the users responsibility to make sure that the dimens/
 *  units this is called with match the independent variable in the ivp.
 *  C clients are given the power to verify this.
 *  Also, the units given must already exist: we do not create them.<br><br>
 *
 *  Registered as:  integrate_set_samples <units> <value [value...] value>
 */

extern int Asc_IntegInstIntegrableCmd(ClientData cdata, Tcl_Interp *interp,
                                      int argc, CONST84 char *argv[]);
/**<
 *  Returns "1" if instance is integrable by the named integrator, "0"
 *  otherwise. Currently valid names are: lsode
 *  The instance examined is "current", "search", or "solver".<br><br>
 *
 *  Registered as:  integrate_able <instance> <NAME>
 */


extern int Asc_IntegSetupCmd(ClientData cdata,Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/**<
 *  Set up the integrator.
 *  itype is one of the supported integrator types (currently:
 *  BLSODE)
 *  n1 is the start index to run the integrator from.
 *  n2 is the end index to stop at.
 *
 *  Registered as:  integrateSetup itype n1 n2
 */

extern int Asc_IntegCleanupCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Do any C level housekeeping need after calling integration.
 *  Always use integrate_setup/integrate_cleanup in pairs.<br><br>
 *
 *  Registered as:  integrate_cleanup
 */

/*  functions affecting the logging of data during integration */
extern FILE *Asc_IntegOpenYFile(void);
/**<
 *  Returns the pointer to a file to which state
 *  variables should be written at each recorded time interval. Other
 *  info may be written here as well, but that is up to the designer
 *  of the specific integrator interface. The filename used will be
 *  the one most recently set via Asc_IntegSetYFileCmd().  If no
 *  name (or an empty name) has been set via Asc_IntegSetYFileCmd(),
 *  the file returned will be NULL.<br><br>
 *
 *  This function may return NULL, in which case do not write to
 *  them as there was some problem opening the file.
 *  If opened successfully, the file returned will have DATASET and
 *  the open time stamped at the bottom. This will separate multiple
 *  runs or changes in the same run.<br><br>
 *
 *  Closing the files is your job.  When finished with the file, you
 *  should call Asc_IntegReleaseYFile() before closing it.
 */
extern FILE *Asc_IntegOpenObsFile(void);
/**<
 *  Returns the pointer to a file to which observation
 *  variables should be written at each recorded time interval. Other
 *  info may be written here as well, but that is up to the designer
 *  of the specific integrator interface. The filename used will be
 *  the one most recently set via Asc_IntegSetObsFileCmd().  If no
 *  name (or an empty name) has been set via Asc_IntegSetObsFileCmd(),
 *  the file returned will be NULL.<br><br>
 *
 *  This function may return NULL, in which case do not write to
 *  them as there was some problem opening the file.
 *  If opened successfully, the file returned will have DATASET and
 *  the open time stamped at the bottom. This will separate multiple
 *  runs or changes in the same run.<br><br>
 *
 *  Closing the files is your job.  When finished with the file, you
 *  should call Asc_IntegReleaseObsFile() before closing it.
 */

/**
	these functions don't involve Tcl/Tk but they do implement stuff
	in a way that might not be useful to other interfaces.
*/

extern FILE *Asc_IntegGetYFile(void);
/**<
 *  Returns the current FILE * for writing state variables,
 *  or NULL if none.
 */
extern FILE *Asc_IntegGetObsFile(void);
/**<
 *  Returns the current FILE * for writing observation variables,
 *  or NULL if none.
 */
extern void Asc_IntegReleaseYFile(void);
extern void Asc_IntegReleaseObsFile(void);
/**<
 *  Releases the internally-stored FILE * for observation variables.
 *  This does not close the file (which you still need to do).
 *  This function should be called just before closing the files,
 *  though it may be called sooner if you want to keep the file pointer
 *  returned all to yourself.
 */


extern void Asc_IntegPrintYHeader(FILE *fp, IntegratorSystem *blsys);
/**<
 *  Prints Y header info to the file given.
 *  If FILE is NULL, returns immediately.
 *  If FILE ok, but blsys NULL prints warning to stderr and returns.
 *  At the moment, the written header info is:
 *      A vertical listing of
 *      ode_id/obs_id instance_name_as_seen_in_solver.d UNITS
 *      then a row (which will line up with printed values)
 *      of the ode_id/obs_id.
 */
extern void Asc_IntegPrintObsHeader(FILE *fp, IntegratorSystem *blsys);
/**<
 *  Prints observation header info to the file given.
 *  If FILE is NULL, returns immediately.
 *  If FILE ok, but blsys NULL prints warning to stderr and returns.
 *  At the moment header info is:
 *      A vertical listing of
 *      ode_id/obs_id instance_name_as_seen_in_solver.d UNITS
 *      then a row (which will line up with printed values)
 *      of the ode_id/obs_id.
 */

extern int Asc_IntegPrintYLine(FILE *fp, IntegratorSystem *blsys);
/**<
	Prints a Y line to the file given.
	If FILE is NULL, returns immediately.
	If FILE ok, but blsys NULL prints warning to stderr and returns.
	Each line is a set of values, identified by the header, following
	the value of the independent variable. These lines are of arbitrary
	length since it is expected that some other program will be
	reading the output, not a human.

	@return 1 on successful output
*/

extern int Asc_IntegPrintObsLine(FILE *fp, IntegratorSystem *blsys);
/**<
	Prints an observation line to the file given.
	If FILE is NULL, returns immediately.
	If FILE ok, but blsys NULL prints warning to stderr and returns.
	Each line is a set of values, identified by the header, following
	the value of the independent variable. These lines are of arbitrary
	length since it is expected that some other program will be
	reading the output, not a human.

	@return 1 on successful output
*/

/*---------------------------------------
  Output reporting interface
*/

/**
	Returns an IntegratorReporter struct. You are responsible for destroying
	this struct when you're finished with it.
*/
IntegratorReporter *Asc_GetIntegReporter();
int Asc_IntegReporterInit(IntegratorSystem *blsys);
int Asc_IntegReporterWrite(IntegratorSystem *blsys);
int Asc_IntegReporterWriteObs(IntegratorSystem *blsys);
int Asc_IntegReporterClose(IntegratorSystem *blsys);

#endif  /* ASCTK_INTEGRATORS_H */
