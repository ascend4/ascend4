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

#include <ascend/integrator/integrator.h>

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
typedef int IdaFlagFn(void *, int *);
typedef char *IdaFlagNameFn(int);

/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct IntegratorIdaDataStruct{

	struct rel_relation **rellist;   /**< NULL terminated list of ACTIVE rels */
	int nrels; /* number of ACTIVE rels */

	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nbnds; /* number of boundaries */

	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	var_filter_t vfilter;
	rel_filter_t rfilter;            /**< Used to filter relations from solver's rellist (@TODO needs work) */
	void *precdata;                  /**< For use by the preconditioner */
	IntegratorIdaPrecFreeFn *pfree;	 /**< Store instructions here on how to free precdata */

	/* Error flag look-up data */
	IdaFlagFn *flagfn;
	IdaFlagNameFn *flagnamefn;
	const char *flagfntype;

} IntegratorIdaData;

/**
	Convenience function to return the IntegratorIdaData data structure
	from within the IDA IntegratorSystem object.
*/
IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ);

#endif

