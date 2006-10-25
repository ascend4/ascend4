/*	ASCEND modelling environment
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
	Implementation of non-GUI-specific parts of the Integration Interface.

	Here we are integrating a model with independent variable t for a specified
	set of timesteps, and and observing the values of certain of the
	independent variables.

	You can specify the values of the time steps that you want via the
	'samplelist' interface.

	You can have your results reported however you like using the
	'IntegratorReporter' interface.

	There's nothing here yet to explicitly support DAEs -- that's the next task.

	(old annotation:)
	The following functions fetch and set parts with names specific to
	the type definitions in ivp.lib, the ASCEND initial value problem
	model file. They are for use by any ivp solver interface.
	All names are given at the scope of ivp.

*//*
	by John Pye, May 2006.

	Derived from tcltk/.../Integrators.h (with heavy editing). 
	Removed all global variables and created a 'IntegratorReporter' interface 
	for reporting results of integration to whatever interface you're using.
*/

#ifndef ASC_INTEGRATOR_H
#define ASC_INTEGRATOR_H

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>

#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/units.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/expr_types.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/atomvalue.h>
#include <compiler/instance_name.h>
#include <compiler/instquery.h>
#include <compiler/parentchild.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>

#include "slv_types.h"
#include "mtx.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_common.h"
#include "samplelist.h"

/*---------------------------*/
/** 
	Struct containin the list of supported integrators
*/
typedef enum{
  INTEG_UNKNOWN,
  INTEG_LSODE,
  INTEG_IDA
} IntegratorEngine;

/*------------------------
  abstraction of the integrator output interface
*/
struct IntegratorSystemStruct;

/** 
	Initialisation. This hook allows initialisation of the GUI or reporting 
	mechanism to be performed when integration begins
*/
typedef int IntegratorOutputInitFn(struct IntegratorSystemStruct *);

/** Status report. This hook allows raw data to be output as integration
	proceeds, or for a GUI to perform status updates. An integrator should check
	the return status on this one, as this is the suggested way to perform
	GUI interruption of the integrator.

	@return 1 on success, 0 on user interrupt
*/
typedef int IntegratorOutputWriteFn(struct IntegratorSystemStruct *);

/**
	Observation reporting. This hook should be implemented to record 
	observations in a way that can be presented to the use, recorded in a 
	file, etc.
*/
typedef int IntegratorOutputWriteObsFn(struct IntegratorSystemStruct *);

/**
	Finalisation. This hook can be used to terminate recording of observations, 	
	close files, terminate GUI status reporting, etc.
*/
typedef int IntegratorOutputCloseFn(struct IntegratorSystemStruct *);

/**
	This struct allows arbitrary functions to be used for the reporting
	of integrator progress.
*/
typedef struct{
	IntegratorOutputInitFn *init;
	IntegratorOutputWriteFn *write;
	IntegratorOutputWriteObsFn *write_obs;
	IntegratorOutputCloseFn *close;
} IntegratorReporter;

/*------------------------------------*/
/**
	Initial value problem description struct. Anyone making a copy of
	the y, ydot, or obs pointers who plans to free that pointer later
	should increment the reference count for that pointer so if blsys
	is destroyed later we don't end up double freeing it. Note this
	scheme will only work for the first copy and could be eliminated
	if we decoupled blsode from lsode as we will once things restabilize.

	JP: adding to this structure the instance being solved, the engine
	that we're solving it with, and a struct containing the output function ptrs
*/
struct IntegratorSystemStruct{
  struct Instance *instance;  /**< not sure if this one is really necessary... -- JP */
  slv_system_t system;        /**< the system that we're integrating in ASCEND */
  IntegratorEngine engine;    /**< enum containing the ID of the integrator engine we're using */
  IntegratorReporter *reporter;/**< functions for reporting integration results */ 
  SampleList *samples;        /**< pointer to the list of samples. we *don't own* this **/
  void *enginedata;           /**< space where the integrator engine can store stuff */
  void *clientdata;           /**< any stuff that the GUI/CLI needs to associate with this */

  int nstates;                /**< was a local global in integrator.c, moved it here. */
  int nderivs;                /**< ditto, as for nstates */

  /* temporaries for build. these elements will be NULL to clients */
  struct gl_list_t *indepvars;  /**< all apparent independent vars */
  struct gl_list_t *dynvars;  /**< all state and deriv instances plus indices */
  struct gl_list_t *obslist;  /**< observation instance plus indices */
  struct gl_list_t *states;   /**< ordered list of state variables and indices*/
  struct gl_list_t *derivs;   /**< ordered list of derivative (ydot) insts */

  /* persistent, these elements are valid to C clients. */
  struct var_variable *x;     /**< independent variable */
  struct var_variable **y;    /**< array form of states */
  struct var_variable **ydot; /**< array form of derivatives */
  struct var_variable **obs;  /**< array form of observed variables */
  long *y_id;                 /**< array form of y/ydot user indices */
  long *obs_id;               /**< array form of obs user indices */
  long n_y;
  long n_obs;
  long currentstep;           /**< current step number (also @see integrator_getnsamples) */
  int ycount;                 /**< number of external references to  y */
  int ydotcount;              /**< number of external references to  ydot */
  int obscount;               /**< number of external references to  obs */
  int maxsubsteps;               /**< most steps between mesh poins */
  double stepzero;            /**< initial step length, SI units. */
  double minstep;             /**< shortest step length, SI units. */
  double maxstep;             /**< longest step length, SI units. */
};

typedef struct IntegratorSystemStruct IntegratorSystem;

/*------------------------------------------------------------------------------
  PUBLIC INTERFACE (for use by the GUI/CLI)]
*/

ASC_DLLSPEC(IntegratorSystem *) integrator_new(slv_system_t sys, struct Instance *inst);

ASC_DLLSPEC(int) integrator_analyse(IntegratorSystem *blsys);

ASC_DLLSPEC(int) integrator_solve(IntegratorSystem *blsys, long i0, long i1);
/**<
	Takes the type of integrator and sets up the global variables into the
	current integration instance.

	@return 1 on success, 0 on failure.
*/

ASC_DLLSPEC(void) integrator_free(IntegratorSystem *blsys);
/**<
	Deallocates any memory used and sets all integration global points to NULL.
*/

ASC_DLLSPEC(int) integrator_set_engine(IntegratorSystem *blsys, IntegratorEngine engine);
/**
	Sets the engine for this integrator. Checks that the integrator can be used
	on the given system.

	@return 1 on success, if engine is compatible with the system being integrated.
*/

ASC_DLLSPEC(IntegratorEngine) integrator_get_engine(const IntegratorSystem *blsys);
/**
	Returns the engine (ID) selected for use in this IntegratorSystem (may return
	INTEG_UNKNOWN if none or invalid setting).
*/

ASC_DLLSPEC(int) integrator_set_reporter(IntegratorSystem *blsys
	, IntegratorReporter *r);
/**<
	Use this function from your interface to tell the integrator how it needs
	to make its output. You need to pass in a struct containing the required
	function pointers. (We will wrap IntegratorReporter and access it from
	Python, eventually)
*/

ASC_DLLSPEC(int) integrator_find_indep_var(IntegratorSystem *blsys);
/**<
	Attempt to locate the independent variable. It will be stored in the blsys
	structure if found.
	@return 1 if found, 0 if not found.
*/

extern int integrator_checkstatus(slv_status_t status);
/**<
 *  Takes a status and checks for convergence.  If converged then return 1
 *  else return 0 and print error message if appropriate.
 */

ASC_DLLSPEC(void) integrator_set_samples(IntegratorSystem *blsys, SampleList *samples);
/**<
	Sets values of time samples to the values given (ns of them) and
	keeps both the dim pointer and vector given. The vector and dimp
	given may be freed by calling this again, but xsamples owns
	them until then. If called with ns<1 or values==NULL, will free any
	previously captured values/dimp. If called with dimp==NULL, will
	assume WildDimension. Don't call this with a dimp we can't free later.
	Return is 1 if for some reason we can't set as expected, 0 otherwise.
*/

ASC_DLLSPEC(void) integrator_set_stepzero(IntegratorSystem *blsys, double);
ASC_DLLSPEC(double) integrator_get_stepzero(IntegratorSystem *blsys);
/**<
	Returns the length of the initial step user specified,
	or 0.0 if none was set.
*/

ASC_DLLSPEC(void) integrator_set_minstep(IntegratorSystem *blsys, double);
ASC_DLLSPEC(double) integrator_get_minstep(IntegratorSystem *blsys);
/**<
	Returns the length of the longest allowable step.
	or 0.0 if none was set by user.
*/

ASC_DLLSPEC(void) integrator_set_maxstep(IntegratorSystem *blsys, double);
ASC_DLLSPEC(double) integrator_get_maxstep(IntegratorSystem *blsys);
/**<
	Returns the length of the shortest allowable step.
	or 0.0 if none was set by user.
*/

ASC_DLLSPEC(void) integrator_set_maxsubsteps(IntegratorSystem *blsys, int);
ASC_DLLSPEC(int) integrator_get_maxsubsteps(IntegratorSystem *blsys);
/**<
	Returns the most internal steps allowed between
	two time samples,  or 0 if none was set by user.
*/

ASC_DLLSPEC(long) integrator_getnsamples(IntegratorSystem *blsys);
/**<
	Returns the number of values currently stored in xsamples.
*/

ASC_DLLSPEC(long) integrator_getcurrentstep(IntegratorSystem *blsys);
/**<
	Returns the current step number (blsys->currentstep). Should be reset to
	zero inside your solver's integrator_*_solve method.
*/

extern double integrator_getsample(IntegratorSystem *blsys, long i);
/**<	The following functions fetch and set parts with names specific to
	the type definitions in ivp.lib, the ASCEND initial value problem
	model file. They are for use by any ivp solver interface.
	All names are given at the scope of ivp.

	Returns the value stored in xsamples[i].  Will whine if
	if xsample[i] does not exist.   No, there is no way to get
	back the pointer to the xsamples vector.
*/

extern void integrator_setsample(IntegratorSystem *blsys, long i, double val);
/**<
	Sets the value stored in xsamples[i] to val.  Will whine if
	if xsample[i] does not exist.   No, there is no way to get
	back the pointer to the xsamples vector.
*/

extern const dim_type *integrator_getsampledim(IntegratorSystem *blsys);
/**<
	Returns a pointer to the dimensionality of the samples currently
	stored, or NULL if none are stored. Do not free this pointer: we
	own it.
*/

/*------------------------------------------------------------------------------
  BACKEND INTERFACE (for use by the integrator engine)
*/

/* Problem size parameters. */

/* Parts of type definition derivatives refinements. */

ASC_DLLSPEC(double) integrator_get_t(IntegratorSystem *blsys);
/**<
	Gets value of the independent variable value from the ASCEND model 
	(retrieves the value from the ASCEND compiler's instance hierarchy)
*/

extern void integrator_set_t(IntegratorSystem *blsys, double value);
/**<
	Sets value of the independent variable in the model (inserts the value in 
	the correct place in the compiler's instance hierarchy via the var_variable 
	API).
*/

extern double *integrator_get_y(IntegratorSystem *blsys, double *vector);
/**<
	Gets the value of vector 'y' from the model (what 'y' is depends on your
	particular integrator).

	If vector given is NULL, allocates vector, which the caller then owns.
	Vector, if given, should be IntegGet_d_neq()+1 long.
*/

extern void integrator_set_y(IntegratorSystem *blsys, double *vector);
/**<
	Sets d.y[] to values in vector.
*/

extern double *integrator_get_ydot(IntegratorSystem *blsys, double *vector);
/**<
	Returns the vector ydot (derivatives of the 'states')

	If vector given is NULL, allocates vector, which the caller then owns.
	Vector, if given, should be IntegGet_d_neq()+1 long.
*/

extern void integrator_set_ydot(IntegratorSystem *blsys, double *vector);
/**<
	Sets d.dydx[] to values in vector.
*/

ASC_DLLSPEC(double *) integrator_get_observations(IntegratorSystem *blsys, double *vector);
/**<
	Returns the vector d.obs.
	Vector should be of sufficient length (g_intginst_d_n_obs+1).
	If NULL is given a vector is allocated and is the caller's
	responsibility to deallocate.
*/

ASC_DLLSPEC(struct var_variable *) integrator_get_observed_var(IntegratorSystem *blsys, const long i);
/**<
	Returns the var_variable contained in the ith position in the observed variable list.
*/

ASC_DLLSPEC(struct var_variable *) integrator_get_independent_var(IntegratorSystem *blsys);
/**<
	Return a pointer to the variable identified as the independent variable.
*/

/*-------------------------------
  Stuff to facilitate output to the interface
*/

/**
	This call should be used to get the file streams ready, output column 
	headings etc.
*/
extern int integrator_output_init(IntegratorSystem *blsys);

/**
	This call should be used to output the immediate integration results from
	a single timestep. For an ODE solver, this means just the 'y' values but
	perhaps not the values of the algebraic varaibles, which must be calculated 
	separately. They'll be stored in the Integ_system_t somehow (not yet
	known how)
*/
extern int integrator_output_write(IntegratorSystem *blsys);

/**
	Write out the 'observed values' for the integration. In the case of ODE
	integration, we assume that the values of all the algabraic variables are
	also now calculated.
*/
extern int integrator_output_write_obs(IntegratorSystem *blsys);

/**
	This call will close file stream and perhaps perform some kind of
	user notification or screen update, etc.
*/
extern int integrator_output_close(IntegratorSystem *blsys);

#endif
