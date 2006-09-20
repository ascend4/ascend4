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
*/
#ifndef ASCXX_VARIABLE_H
#define ASCXX_VARIABLE_H

#include <string>
#include <vector>

#include "simulation.h"
#include "instance.h"

struct Relation;

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/var.h>
}

/**
	This is a wrapper for the var_variable type in ASCEND. This
	type is used in reporting the variables in an instance, including
	when looking for 'eligible' variables which can be fixed.
*/
class Variable{
private:
	Simulation *sim;
	struct var_variable *var;

public:
	Variable();
	Variable(const Variable &old);
	Variable(Simulation *sim, var_variable *var);

	const std::string getName() const;
	const double getValue() const;
	const double getNominal() const;
	const double getUpperBound() const;
	const double getLowerBound() const;
	const std::vector<Relation> getIncidentRelations() const;
	const int getNumIncidentRelations() const;
	
	Instanc getInstance();
};

#endif
