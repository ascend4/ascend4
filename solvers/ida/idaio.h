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
	Input/output routines used by the ASCEND wrapping of the IDA integrator.
*/

#include "ida.h"
#include "idatypes.h"

typedef struct IntegratorIdaStatsStruct{
	long nsteps;
	long nrevals;
	long nlinsetups;
	long netfails;
	int qlast, qcur;
	realtype hinused, hlast, hcur;
	realtype tcur;
} IntegratorIdaStats;

/* error handler forward declaration */
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
);

void integrator_ida_write_stats(IntegratorIdaStats *stats);

IntegratorDebugFn integrator_ida_debug;
IntegratorWriteMatrixFn integrator_ida_write_matrix;

mtx_matrix_t integrator_ida_dgdya(const IntegratorSystem *sys);

int integrator_ida_debug(const IntegratorSystem *sys, FILE *fp);

void integrator_ida_write_incidence(IntegratorSystem *integ);

