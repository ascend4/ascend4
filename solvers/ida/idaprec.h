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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Preconditioner routines for the ASCEND wrapping of the IDA integrator.

	EXPERIMENTAL! Use with caution.
*/

#include "ida.h"
#include "idatypes.h"

/** Function type for allocation of storage for preconditioner */
typedef void IntegratorIdaPrecCreateFn(IntegratorSystem *integ);

/**
	Hold all the function pointers associated with a particular preconditioner
	We don't need to store the 'pfree' function here as it is allocated to the enginedata struct
	by the pcreate function (ensures that corresponding 'free' and 'create' are always used)

	@TODO is there any way we can also store the FREE function in this struct?
	
	@note IDA uses a different convention for function pointer types, so no '*'.
*/
typedef struct IntegratorIdaPrecStruct{
	IntegratorIdaPrecCreateFn *pcreate;
	IDASpilsPrecSetupFn psetup;
	IDASpilsPrecSolveFn psolve;
} IntegratorIdaPrec;


/*------------------------------------------------------------------------------
  FULL JACOBIAN PRECONDITIONER
*/

/**
  Internal data for use by full Jacobian preconditioner.
*/
typedef struct IntegratorIdaPrecDJFStruct{
	linsolqr_system_t L;
} IntegratorIdaPrecDataJacobian;

/**	@todo FIXME seems that this pfree function is not being used anywhere?? */
void integrator_ida_pfree_jacobian(IntegratorIdaData *enginedata);

const IntegratorIdaPrec prec_jacobian;

/*------------------------------------------------------------------------------
  JACOBI PRECONDITIONER
  FIXME need to add some description here!
*/

/**
	Internal data for use by Jacobi preconditioner.
*/
typedef struct IntegratorIdaPrecDJStruct{
	N_Vector PIii; /**< diagonal elements of the inversed Jacobi preconditioner */
} IntegratorIdaPrecDataJacobi;

/**	@todo FIXME seems that this pfree function is not being used anywhere?? */
void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata);

const IntegratorIdaPrec prec_jacobi;


