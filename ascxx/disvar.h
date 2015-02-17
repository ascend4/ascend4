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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ASCXX_DISVAR_H
#define ASCXX_DISVAR_H

#include <string>
#include <vector>

#include "simulation.h"
#include "instance.h"

#include "config.h"
extern "C"{
#include <ascend/general/platform.h>
#include <ascend/system/slv_types.h>
#include <ascend/system/discrete.h>
}

/**
	This is a wrapper for the dis_discrete type in ASCEND.
*/
class Disvar{
private:
	Simulation *sim;
	struct dis_discrete *disvar;

public:
	Disvar();
	Disvar(const Disvar &old);
	Disvar(Simulation *sim, dis_discrete *disvar);

	const std::string getName() const;
	const double getValue() const;

	Instanc getInstance();
};

#endif
