/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

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
*//** @file

	Implementation functions for IDA wrapper. Only put things here if they need
	to be shared between .c files in this folder, but not needed for any
	'outside' access to these files.
*/
#ifndef ASC_IDATYPES_H
#define ASC_IDATYPES_H

#include "ida.h"
#include <ascend/integrator/integrator.h>

#ifdef ASC_IDA_BACKEND_IDAS
# define ASC_INTEG_ENGINE_IS_IDA_FAMILY(integ) ((integ)->engine == INTEG_IDA || (integ)->engine == INTEG_IDAS)
#else
# define ASC_INTEG_ENGINE_IS_IDA_FAMILY(integ) ((integ)->engine == INTEG_IDA)
#endif

/* forward dec needed for IntegratorIdaPrecFreeFn */
struct IntegratorIdaDataStruct;

/**
	Function type for freeing of preconditioner data. FIXME should this be part
	of the precdata, perhaps? @see idaprec.h
*/
typedef void IntegratorIdaPrecFreeFn(struct IntegratorIdaDataStruct *enginedata);

/**
 * Function type for error flag description look-up
 */
typedef int IdaFlagFn(void *, long int *);
typedef char *IdaFlagNameFn(long int);

/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct IntegratorIdaDataStruct{

	struct rel_relation **rellist;   /**< NULL terminated list of ACTIVE rels */
	int nrels; /* number of ACTIVE rels */

	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nbnds; /* number of boundaries */
	struct when_reinit **guardroots; /**< active-case simple comparison guards used as extra IDA roots */
	struct Instance **guardcontexts; /**< evaluation context for each active direct guard root */
	int nguardroots; /* number of active direct guard roots */
	int nroots; /* total number of root functions */

	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	int warned_minstep_ignored;      /**< whether unsupported minstep has already been reported */
	var_filter_t vfilter;
	rel_filter_t rfilter;            /**< Used to filter relations from solver's rellist (@TODO needs work) */
	void *precdata;                  /**< For use by the preconditioner */
	IntegratorIdaPrecFreeFn *pfree;	 /**< Store instructions here on how to free precdata */
	realtype *event_times;           /**< ring buffer of recent boundary-event times for simple Zeno detection */
	int event_times_cap;             /**< allocated length of event_times */
	int event_times_count;           /**< number of valid entries currently stored */
	int event_times_next;            /**< next ring-buffer slot to overwrite */

#ifdef ASC_IDA_BACKEND_IDAS
	realtype *sens_p;                /**< IDAS parameter vector for forward sensitivities */
	realtype *sens_p_nominal;        /**< nominal parameter values to restore into ASCEND instances */
	realtype *sens_pbar;             /**< IDAS parameter scales for difference quotient sensitivities */
	int *sens_plist;                 /**< IDAS sensitivity-parameter index mapping */
	N_Vector *sens_y;                /**< IDAS state sensitivity vectors */
	N_Vector *sens_yp;               /**< IDAS derivative sensitivity vectors */
	int sens_np;                     /**< number of configured sensitivity parameters */
	int sens_enabled;                /**< whether IDAS sensitivities were initialized */
#endif

	/* Error flag look-up data */
	IdaFlagFn *flagfn;
	IdaFlagNameFn *flagnamefn;
	const char *flagfntype;

#if SUNDIALS_VERSION_MAJOR >= 6
	SUNContext sunctx;
#endif
#if SUNDIALS_VERSION_MAJOR >= 5
	SUNLinearSolver linear_solver;
	SUNMatrix dense_matrix;
#endif

} IntegratorIdaData;

/**
	Convenience function to return the IntegratorIdaData data structure
	from within the IDA IntegratorSystem object.
*/
IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ);

#ifdef ASC_IDA_BACKEND_IDAS
int integrator_ida_sens_setup(IntegratorSystem *integ, void *ida_mem, N_Vector y0);
void integrator_ida_sens_free(IntegratorIdaData *enginedata);
void integrator_ida_sens_restore(IntegratorSystem *integ);
int integrator_ida_sens_sync(IntegratorSystem *integ);
int integrator_ida_sens_record(IntegratorSystem *integ, void *ida_mem, realtype tret);
#endif

#endif
