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
	C++ wrapper for the IntegratorReporter struct in the solver C-API.
	This class is intended to be exposed via the SWIG 'director' functionality
	which allows it to be overloaded in Python, so reporting of integration
	results can be done directly with python scripts of the user's design.
*/
#ifndef ASCXX_INTEGRATORREPORTER_H
#define ASCXX_INTEGRATORREPORTER_H

extern "C"{
#include <utilities/ascConfig.h>
#include <solver/integrator.h>
}

#include <ostream>

class Integrator;

/**
	Observer API to allow ASCEND to add rows/columbs to the observer panel
	in the Python interface.
	
	Should also be generalisable so that we can output observations to files
	etc.
*/
class IntegratorReporterCxx{
	friend int ascxx_integratorreporter_init(IntegratorSystem *);
	friend int ascxx_integratorreporter_write(IntegratorSystem *);
	friend int ascxx_integratorreporter_write_obs(IntegratorSystem *);
	friend int ascxx_integratorreporter_close(IntegratorSystem *);
	friend class Integrator;

public:
	IntegratorReporterCxx(Integrator *);
	virtual ~IntegratorReporterCxx();

	//virtual void addObservedVariable(Variable v);
	virtual int initOutput();
	virtual int closeOutput();
	virtual int updateStatus();
	virtual int recordObservedValues();

	Integrator *getIntegrator();

protected:
	Integrator *integrator; /**< pointer back to integrator */
	IntegratorReporter reporter; /**< for passing to C */

	IntegratorReporter *getInternalType();
};

/**
	NULL integrator reporter. This reporter won't output ANYTHING at all.
*/
class IntegratorReporterNull : public IntegratorReporterCxx{
public:
	IntegratorReporterNull(Integrator *);
	virtual ~IntegratorReporterNull();

	virtual int initOutput();
	virtual int closeOutput();
	virtual int updateStatus();
	virtual int recordObservedValues();
};

/**
	Simple console based integrator reporter. Output the observed variables
	to the console at each sample point.
*/
class IntegratorReporterConsole : public IntegratorReporterCxx{
private:
	std::ostream &f;
public:
	IntegratorReporterConsole(Integrator *);
	virtual ~IntegratorReporterConsole();

	virtual int initOutput();
	virtual int closeOutput();
	virtual int updateStatus();
	virtual int recordObservedValues();
};


int ascxx_integratorreporter_init(IntegratorSystem *blsys);
int ascxx_integratorreporter_write(IntegratorSystem *blsys);
int ascxx_integratorreporter_write_obs(IntegratorSystem *blsys);
int ascxx_integratorreporter_close(IntegratorSystem *blsys);

#endif
