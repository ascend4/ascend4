/*	ASCEND modelling environment
	Copyright (C) 2010 Carnegie Mellon University

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
	Implementation of solver hooks on the C++ side, for accessing by the
	'slvreq.h' language features (SOLVER, OPTION, SOLVE) in ASCEND METHODS.
	This code allows us to make the GUI do things upon instruction from methods
	being run in the MODEL.

	See also SlvReqHooks in ascend/compiler/slvreq.h.
*/

#ifndef ASCXX_SOLVERHOOKS_H
#define ASCXX_SOLVERHOOKS_H

#include "config.h"
#include "value.h"

class SolverReporter;
class Simulation;

extern "C"{
#include <ascend/general/platform.h>
#include <ascend/compiler/slvreq.h>
};

extern "C"{
SlvReqSetSolverFn ascxx_slvreq_set_solver;
SlvReqSetOptionFn ascxx_slvreq_set_option;
SlvReqDoSolveFn ascxx_slvreq_do_solve;
};

/**
	A C++ structure to handle the calling of slvreq hooks by METHODs. This
	has to provide a mechanism that allows access to both the pure C++ API
	(see testslvreq.cpp) as well as the Python/PyGTK GUI. So we will allow
	subclassing of SolverHooks as SolverHooksPython for that case.
*/
class SolverHooks{
private:
	SolverReporter *R;
public:
	SolverHooks(SolverReporter *reporter = NULL);
	SolverHooks(SolverHooks &old);
	virtual ~SolverHooks();

	/// C++ function that will be called as a result of a 'SOLVER' command
	virtual int setSolver(const char *solvername, Simulation *S);

	/// C++ function that will be called as a result of a 'OPTION' command
	virtual int setOption(const char *optionname, Value val1, Simulation *S);

	/// C++ function that will be called as a result of a 'SOLVE' command
	virtual int doSolve(Instance *i, Simulation *S);

	SolverReporter *getSolverReporter();

	void assign(Simulation *S);
};

/**
	A 'manager' singleton for dealing with the assignment of 'slvreq' hooks
	to Simulation objects in the C++ layer. This needs to be a singleton because
	the Type::getSimulation method needs to be able to grab the solver hooks
	from an as-it-were global object of some sort. But we also need to ability
	to reassign different solver hooks in the C++ layer, because the Python
	GUI will use different hooks to the pure C++ API (see testslvreq.cpp).

	Note that if no setHooks() call has been made before the first call to
	getHooks(), the SetHooksManager will assign a default C++ SolverHooks
	object. Therefore, for users of Python or other possible interfaces based
	on this code, you must make sure you first call

	SolverHooksManager::Instance()->setHooks(mysolverhooksobject);
*/
class SolverHooksManager{
private:
	bool own_hooks;
	SolverHooks *hooks;
	SolverHooksManager(); // This class will be a singleton
	~SolverHooksManager();
	static SolverHooksManager *_instance;

public:
	static SolverHooksManager *Instance();
	void setHooks(SolverHooks *hooks);
	SolverHooks *getHooks();
};


#endif
