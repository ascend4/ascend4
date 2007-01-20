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
	C++ wrapper for the Integrator interface. Intention is that this will allow
	us to use the PyGTK 'observer' tab to receive the results of an integration
	job, which can then be easily exported to a spreadsheet for plotting (or
	we can implement ASCPLOT style plotting, perhaps).
*/
#ifndef ASCXX_INTEGRATOR_H
#define ASCXX_INTEGRATOR_H

#include <string>
#include <map>
#include <vector>

#include "config.h"
extern "C"{
#include <integrator/integrator.h>
#include <integrator/samplelist.h>
}

const int LSODE = INTEG_LSODE;
#ifdef ASC_WITH_IDA
const int IDA = INTEG_IDA;
#endif

#include "simulation.h"
#include "units.h"
#include "integratorreporter.h"
#include "variable.h"

class Integrator{
	friend class IntegratorReporterCxx;
	friend class IntegratorReporterConsole;

public:
	Integrator(Simulation &);
	~Integrator();

	static std::map<int,std::string> getEngines();
	void setEngine(IntegratorEngine engine);
	void setEngine(int engine);
	void setEngine(const std::string &name);
	std::string getName() const;

	SolverParameters getParameters() const;
	void setParameters(const SolverParameters &);

	void setReporter(IntegratorReporterCxx *reporter);

	void setMinSubStep(double);
	void setMaxSubStep(double);
	void setInitialSubStep(double);
	void setMaxSubSteps(int);

	void setLinearTimesteps(UnitsM units, double start, double end, unsigned long num);
	void setLogTimesteps(UnitsM units, double start, double end, unsigned long num);
	std::vector<double> getCurrentObservations();
	Variable getObservedVariable(const long &i);
	Variable getIndependentVariable();

	void findIndependentVar(); /**< find the independent variable (must not presume a certain choice of integration engine) */
	void analyse();
	void solve();
	void writeMatrix(FILE *fp) const;

	double getCurrentTime();
	long getCurrentStep();
	long getNumSteps();
	int getNumVars();
	int getNumObservedVars();

protected:
	IntegratorSystem *getInternalType();
private:
	Simulation &simulation;
	SampleList *samplelist;
	IntegratorSystem *blsys;
};

#endif
