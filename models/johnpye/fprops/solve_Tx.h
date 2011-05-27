/*
ASCEND modelling environment
Copyright (C) 2004-2010 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef FPROPS_SOLVE_TX_H
#define FPROPS_SOLVE_TX_H

#include "helmholtz.h"

int fprops_region_Tx(double T, double x, const HelmholtzData *D);
int fprops_solve_Tx(double T, double x, double *rho, const HelmholtzData *D);

#endif
