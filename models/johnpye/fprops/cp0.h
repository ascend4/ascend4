/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

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
*/
#ifndef FPROPS_CP0_H
#define FPROPS_CP0_H

#include "rundata.h"

/* FIXME rename this to phi0_prepare, along with the filename */
/**
	Prepare the runtime data required for calculation of the ideal component of 
	the reduced Helmholtz function, \f$\phi = \frac{a}{R T}\f$ and its 
	derivatives \f$\phi_\tau\f$ and \f$\phi_{\tau\tau}\f$. Note that even
	though this is called 'cp0_prepare', it is unable on its own to calculate
	\f$c_p^o\f$ because the values of Tstar (needed for \f$\tau = \frac{T^{*}}{T}\f$) and R are stored at higher-level in 
	the FluidData object.
*/
Phi0RunData *cp0_prepare(const IdealData *I, double R, double Tstar);


void cp0_destroy(Phi0RunData *cp0);

/**
	Ideal-gas component of the reduced Helmholtz function
	\f$\phi = \frac{a}{R T}\f$
*/
double ideal_phi(double tau, double delta, const Phi0RunData *data);

/**
	Ideal-gas component of the reduced Helmholtz function: first partial derivative wrt reduced temperature 
	\f$\phi_\tau = \left(\frac{\partial \phi}{\partial \tau}\right)_\delta = \frac{1}{R T} \frac{\partial T}{\partial \tau} \frac{\partial a}{\partial T} \f$
*/
double ideal_phi_tau(double tau, double delta, const Phi0RunData *data);

/**
	Ideal-gas component of the reduced Helmholtz function: second partial derivative wrt reduced temperature
	\f$\phi_{\tau\tau}
		 = \left(\frac{\partial^2 \phi}{\partial \tau^2}\right)_\delta
		 = \frac{1}{R T} \left(\frac{d T}{d \tau}\right)^2 \frac{\partial^2 a}{\partial T^2} 
	\f$
*/
double ideal_phi_tautau(double tau, const Phi0RunData *data);

#endif

