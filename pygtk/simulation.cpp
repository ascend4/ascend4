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
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
using namespace std;

#include "config.h"

extern "C"{
#include <utilities/error.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

#include <general/tm_time.h>

#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instantiate.h>
#include <compiler/bintoken.h>
#include <compiler/instance_enum.h>
#include <compiler/instquery.h>
#include <compiler/check.h>
#include <compiler/name.h>
#include <compiler/pending.h>
#include <compiler/importhandler.h>
#include <linear/mtx.h>
#include <system/calc.h>
#include <system/relman.h>
#include <system/system.h>
#include <solver/slvDOF.h>
#include <system/slv_stdcalls.h>
#include <system/slv_server.h>
#include <system/graph.h>
}

#include "simulation.h"
#include "solver.h"
#include "solverparameters.h"
#include "name.h"
#include "incidencematrix.h"
#include "variable.h"
#include "solverstatus.h"
#include "solverreporter.h"
#include "matrix.h"

/**
	Create an instance of a type (call compiler etc)

	@TODO fix mutex on compile command filenames
*/
Simulation::Simulation(Instance *i, const SymChar &name) : Instanc(i, name), simroot(GetSimulationRoot(i),SymChar("simroot")){
	CONSOLE_DEBUG("Created simulation");	
	sys = NULL;
	//is_built = false;
	// Create an Instance object for the 'simulation root' (we'll call
	// it the 'simulation model') and it can be fetched using 'getModel()'
	// any time later.
	//simroot = Instanc(GetSimulationRoot(i),name);
}

Simulation::Simulation(const Simulation &old) : Instanc(old), simroot(old.simroot){
	//is_built = old.is_built;
	CONSOLE_DEBUG("Copying Simulation...");
	sys = old.sys;
	sing = NULL;
}

Simulation::~Simulation(){
	CONSOLE_DEBUG("Destroying Simulation...");
	/*
	// FIXME removing this here, because Python overzealously seems to delete simulations
	
	CONSOLE_DEBUG("Deleting simulation %s", getName().toString());
	system_free_reused_mem();
	if(sys){
		CONSOLE_DEBUG("Destroying simulation system...");
		system_destroy(sys);
	}
	*/
	sys = NULL;
}

Instanc &
Simulation::getModel(){
	if(!simroot.getInternalType()){
		throw runtime_error("Simulation::getModel: simroot.getInternalType()is NULL");
	}
	if(InstanceKind(simroot.getInternalType())!=MODEL_INST){
		throw runtime_error("Simulation::getModel: simroot is not a MODEL instance");
	}
	return simroot;
}


slv_system_t
Simulation::getSystem(){
	if(!sys)throw runtime_error("Can't getSystem: simulation not yet built");
	return sys;
}


const string
Simulation::getInstanceName(const Instanc &i) const{
	char *n;
	n = WriteInstanceNameString(i.getInternalType(),simroot.getInternalType());
	string s(n);
	ascfree(n);
	return s;
}

const int
Simulation::getNumVars(){
	return slv_get_num_solvers_vars(getSystem());
}

/**
	A general purpose routine for reporting from simulations.
*/
void
Simulation::write(FILE *fp, const char *type){
	int res;

	const var_filter_t vfilter = {
		  VAR_SVAR | VAR_ACTIVE | VAR_INCIDENT | VAR_FIXED
		, VAR_SVAR | VAR_ACTIVE | VAR_INCIDENT | 0
	};

	const rel_filter_t rfilter = {
		  REL_INCLUDED | REL_EQUALITY | REL_ACTIVE 
		, REL_INCLUDED | REL_EQUALITY | REL_ACTIVE 
	};

	if(type==NULL){
		simroot.write(fp);
	}else if(type=="dot"){
		if(!sys)throw runtime_error("Can't write DOT file: simulation not built");
		CONSOLE_DEBUG("Writing graph...");
		res = system_write_graph(sys, fp, &rfilter, &vfilter);
		if(res){
			stringstream ss;
			ss << "Error running system_write_graph (err " << res << ")";
			throw runtime_error(ss.str());
		}
	}
}

//------------------------------------------------------------------------------
// RUNNING MODEL 'METHODS'

void
Simulation::run(const Method &method){
	Instanc &model = getModel();
	this->run(method,model);
}

void
Simulation::runDefaultMethod(){
	const Type &type = getType();
	Method m;
	try{
		m = type.getMethod(SymChar("on_load"));
	}catch(runtime_error &e){
		ERROR_REPORTER_NOLINE(ASC_USER_WARNING,"There is no 'on_load' method defined for type '%s'",type.getName().toString());
		return;
	}
	run(m);		
}	

void
Simulation::run(const Method &method, Instanc &model){

	// set the 'sim' pointer to our local variable...
	CONSOLE_DEBUG("Setting shared pointer 'sim' = %p",this);
	importhandler_setsharedpointer("sim",this);

	/*if(not is_built){
		CONSOLE_DEBUG("WARNING, SIMULATION NOT YET BUILT");
	}*/

	CONSOLE_DEBUG("Running method %s...", method.getName());

	Nam name = Nam(method.getSym());
	//cerr << "CREATED NAME '" << name.getName() << "'" << endl;

	error_reporter_tree_start();

	CONSOLE_DEBUG("sys = %p",sys);
	CONSOLE_DEBUG("simroot = %p",simroot.getInternalType());

	Proc_enum pe;
	pe = Initialize(
		&*(model.getInternalType()) ,name.getInternalType(), "__not_named__"
		,ASCERR
		,0, NULL, NULL
	);

	int haserror=0;
	if(error_reporter_tree_has_error()){
		haserror=1;
	}
	error_reporter_tree_end();

	// clear out the 'sim' pointer (soon it will be invalid)
	importhandler_setsharedpointer("sim",NULL);
	CONSOLE_DEBUG("Cleared shared pointer 'sim'");

	if(pe == Proc_all_ok){
		if(haserror){
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Method '%s' had error(s).",method.getName());
			stringstream ss;
			ss << "Method '"<<method.getName()<<"' returned 'all_ok' status but output error(s)";
			throw runtime_error(ss.str());
		}else{
			ERROR_REPORTER_NOLINE(ASC_USER_SUCCESS,"Method '%s' returned 'all_ok' and output no errors.\n",method.getName());
		}
		//cerr << "METHOD " << method.getName() << " COMPLETED OK" << endl;
	}else{
		stringstream ss;
		ss << "Simulation::run: Method '" << method.getName() << "' returned error: ";
		switch(pe){
			case Proc_CallOK: ss << "Call OK"; break;
			case Proc_CallError: ss << "Error occurred in call"; break;
			case Proc_CallReturn: ss << "Request that caller return (OK)"; break;
			case Proc_CallBreak: ss << "Break out of enclosing loop"; break;
			case Proc_CallContinue: ss << "Skip to next iteration"; break;

			case Proc_break: ss << "Break"; break;
			case Proc_continue: ss << "Continue"; break;
			case Proc_fallthru: ss << "Fall-through"; break;
			case Proc_return: ss << "Return"; break;
			case Proc_stop: ss << "Stop"; break;
			case Proc_stack_exceeded: ss << "Stack exceeded"; break;
			case Proc_stack_exceeded_this_frame: ss << "Stack exceeded this frame"; break;
			case Proc_case_matched: ss << "Case matched"; break;
			case Proc_case_unmatched: ss << "Case unmatched"; break;

			case Proc_case_undefined_value: ss << "Undefined value in case"; break;
			case Proc_case_boolean_mismatch: ss << "Boolean mismatch in case"; break;
			case Proc_case_integer_mismatch: ss << "Integer mismatch in case"; break;
			case Proc_case_symbol_mismatch: ss << "Symbol mismatch in case"; break;
			case Proc_case_wrong_index: ss << "Wrong index in case"; break;
			case Proc_case_wrong_value: ss << "Wrong value in case"; break;
			case Proc_case_extra_values: ss << "Extra values in case"; break;
			case Proc_bad_statement: ss << "Bad statement"; break;
			case Proc_bad_name: ss << "Bad name"; break;
			case Proc_for_duplicate_index: ss << "Duplicate index"; break;
			case Proc_for_set_err: ss << "For set error"; break;
			case Proc_for_not_set: ss << "For not set"; break;
			case Proc_illegal_name_use: ss << "Illegal name use"; break;
			case Proc_name_not_found: ss << "Name not found"; break;
			case Proc_instance_not_found: ss << "Instance not found"; break;
			case Proc_type_not_found: ss << "Type not found"; break;
			case Proc_illegal_type_use: ss << "Illegal use"; break;
			case Proc_proc_not_found: ss << "Method not found"; break;
			case Proc_if_expr_error_typeconflict: ss << "Type conflict in 'if' expression"; break;
			case Proc_if_expr_error_nameunfound: ss << "Name not found in 'if' expression"; break;
			case Proc_if_expr_error_incorrectname: ss << "Incorrect name in 'if' expression"; break;
			case Proc_if_expr_error_undefinedvalue: ss << "Undefined value in 'if' expression"; break;
			case Proc_if_expr_error_dimensionconflict: ss << "Dimension conflict in 'if' expression"; break;
			case Proc_if_expr_error_emptychoice: ss << "Empty choice in 'if' expression"; break;
			case Proc_if_expr_error_emptyintersection: ss << "Empty intersection in 'if' expression"; break;
			case Proc_if_expr_error_confused: ss << "Confused in 'if' expression"; break;
			case Proc_if_real_expr: ss << "Real-valued result in 'if' expression"; break;
			case Proc_if_integer_expr: ss << "Integeter-valued result in 'if' expression"; break;
			case Proc_if_symbol_expr: ss << "Symbol-valued result in 'if' expression"; break;
			case Proc_if_set_expr: ss << "Set-valued result in 'if' expression"; break;
			case Proc_if_not_logical: ss << "If expression is not logical"; break;
			case Proc_user_interrupt: ss << "User interrupt"; break;
			case Proc_infinite_loop: ss << "Infinite loop"; break;
			case Proc_declarative_constant_assignment: ss << "Declarative constant assignment"; break;
			case Proc_nonsense_assignment: ss << "Nonsense assginment (bogus)"; break;
			case Proc_nonconsistent_assignment: ss << "Inconsistent assignment"; break;
			case Proc_nonatom_assignment: ss << "Non-atom assignment"; break;
			case Proc_nonboolean_assignment: ss << "Non-boolean assignment"; break;
			case Proc_noninteger_assignment: ss << "Non-integer assignment"; break;
			case Proc_nonreal_assignment: ss << "Non-real assignment"; break;
			case Proc_nonsymbol_assignment: ss << "Non-symbol assignment"; break;
			case Proc_lhs_error: ss << "Left-hand-side error"; break;
			case Proc_rhs_error: ss << "Right-hand-side error"; break;
			case Proc_unknown_error: ss << "Unknown error"; break;
			default:
				ss << "Invalid error code";
		}

		ss << " (" << int(pe) << ")";
		throw runtime_error(ss.str());
	}
}

//-----------------------------------------------------------------------------
// CHECKING METHODS

/**
	Check that all the analysis went OK: solver lists are all there, etc...?

	Can't return anything here because of limitations in the C API

	@TODO there's something wrong with this at the moment: even after 'FIX'
	methods are run, check shows them as not fixed, up until the point that 'SOLVE'
	successfully completes. Something's not being synchronised properly...
*/
void
Simulation::checkInstance(){
	Instance *i1 = getModel().getInternalType();
	CheckInstance(stderr, &*i1);
	//cerr << "DONE CHECKING INSTANCE" << endl;
}

/**
	@return 1 = underspecified, 2 = square, 3 = structurally singular, 4 = overspecified
*/
enum StructuralStatus
Simulation::checkDoF() const{
    int dof, status;

	if(!sys){
		throw runtime_error("System is not built");
	}

    /*if(!is_built){
		throw runtime_error("System not yet built");
    }*/
	CONSOLE_DEBUG("Calling slvDOF_status...");
    slvDOF_status(sys, &status, &dof);
    switch(status){
        case ASCXX_DOF_UNDERSPECIFIED:
		case ASCXX_DOF_SQUARE:
		case ASCXX_DOF_OVERSPECIFIED:
		case ASCXX_DOF_STRUCT_SINGULAR:
			return (enum StructuralStatus)status;
		case 5:
		    throw runtime_error("Unable to resolve degrees of freedom"); break;
		default:
		    throw runtime_error("Invalid return status from slvDOF_status");
    }
}

/**
	Check consistency

	@TODO what is the difference between this and checkStructuralSingularity?
	
	@return list of freeable variables. List will be empty if sys is consistent.
*/
vector<Variable>
Simulation::getFreeableVariables(){
	vector<Variable> v;

	//cerr << "CHECKING CONSISTENCY..." << endl;
	int *fixedarrayptr=NULL;

	if(!sys){
    	throw runtime_error("System not yet built");
    }

	int res = consistency_analysis(sys, &fixedarrayptr);

	if(res==1){
		cerr << "STRUCTURALLY CONSISTENT" << endl;
	}else{
		if(fixedarrayptr ==NULL){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"STRUCTURALLY INCONSISTENT");
			throw runtime_error("Invalid consistency analysis result returned!");
		}

		struct var_variable **vp = slv_get_master_var_list(sys);
		for(int i=0; fixedarrayptr[i]!=-1; ++i){
			v.push_back( Variable(this, vp[fixedarrayptr[i]]) );
		}
	}
	return v;
}

/** Returns TRUE if all is OK (not singular) */
bool
Simulation::checkStructuralSingularity(){
	int *vil;
	int *ril;
	int *fil;

	if(this->sing){
		cerr << "DELETING OLD SINGULATING INFO" << endl;
		delete this->sing;
		this->sing = NULL;
	}

	cerr << "RETRIEVING slfDOF_structsing INFO" << endl;

	int res = slvDOF_structsing(sys, mtx_FIRST, &vil, &ril, &fil);


	if(res==1){
		throw runtime_error("Unable to determine singularity lists");
	}

	if(res!=0){
		throw runtime_error("Invalid return from slvDOF_structsing.");
	}


	CONSOLE_DEBUG("processing singularity data...");
	sing = new SingularityInfo();

	struct var_variable **varlist = slv_get_solvers_var_list(sys);
	struct rel_relation **rellist = slv_get_solvers_rel_list(sys);

	// pull in the lists of vars and rels, and the freeable vars:
	for(int i=0; ril[i]!=-1; ++i){
		sing->rels.push_back( Relation(this, rellist[ril[i]]) );
	}

	for(int i=0; vil[i]!=-1; ++i){
		sing->vars.push_back( Variable(this, varlist[vil[i]]) );
	}

	for(int i=0; fil[i]!=-1; ++i){
		sing->freeablevars.push_back( Variable(this, varlist[fil[i]]) );
	}

	// we're done with those lists now
	ASC_FREE(vil);
	ASC_FREE(ril);
	ASC_FREE(fil);

	if(sing->isSingular()){
		CONSOLE_DEBUG("singularity found");
		this->sing = sing;
		return FALSE;
	}
	CONSOLE_DEBUG("no singularity");
	delete sing;
	return TRUE;
}

/**
	If the checkStructuralSingularity analysis has been done,
	this funciton will let you access the SingularityInfo data that was
	stored.
*/
const SingularityInfo &
Simulation::getSingularityInfo() const{
	if(sing==NULL){
		throw runtime_error("No singularity info present");
	}
	return *sing;
}

//------------------------------------------
// ASSIGNING SOLVER TO SIMULATION

void
Simulation::setSolver(Solver &solver){
	/* CONSOLE_DEBUG("Setting solver on sim %p, root inst %p",this,this->simroot.getInternalType()); */

	try{
		// build the system (if not built already)
		build();
	}catch(runtime_error &e){
		stringstream ss;
		ss << "Couldn't prepare system for solving:";
		ss << e.what();
		throw runtime_error(ss.str());
	}

	CONSOLE_DEBUG("Selecting solver '%s'",solver.getName().c_str());
	int selected = slv_select_solver(sys, solver.getIndex());
	//cerr << "Simulation::setSolver: slv_select_solver returned " << selected << endl;

	if(selected<0){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Failed to select solver");
		throw runtime_error("Failed to select solver");
	}

	if(selected!=solver.getIndex()){
		solver = Solver(slv_solver_name(selected));
		ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Substitute solver '%s' (index %d) selected.\n", solver.getName().c_str(), selected);
	}

	if( slv_eligible_solver(sys) <= 0){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Inelegible solver '%s'", solver.getName().c_str() );
		throw runtime_error("Inelegible solver");
	}
}

const Solver
Simulation::getSolver() const{
	int index = slv_get_selected_solver(sys);
	//cerr << "Simulation::getSolver: index = " << index << endl;
	if(index<0)throw runtime_error("No solver selected");

	return Solver(slv_solver_name(index));
}

//------------------------------------------------------------------------------
// BUILD THE SYSTEM

/**
	Build the system (send it to the solver)
*/
void
Simulation::build(){
	if(sys){
		CONSOLE_DEBUG("System is already built (%p)",sys);
		return;
	}

	if(simroot.getKind() != MODEL_INST){
		throw runtime_error("Simulation does not contain a MODEL_INST");
	}

	if(NumberPendingInstances(simroot.getInternalType())){
		throw runtime_error("System has pending instances; can't yet send to solver.");
	}
	
	CONSOLE_DEBUG("============== REALLY building system...");
	sys = system_build(simroot.getInternalType());
	if(!sys){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to build system");
		throw runtime_error("Unable to build system");
	}
	
	CONSOLE_DEBUG("System built OK");
}


//------------------------------------------------------------------------------
// SOLVER CONFIGURATION PARAMETERS

/**
	Get solver parameters struct wrapped up as a SolverParameters class.
*/
SolverParameters
Simulation::getParameters() const{
	//if(!is_built)throw runtime_error("Can't getSolverParameters: Simulation system has not been built yet.");
	if(!sys)throw runtime_error("Can't getSolverParameters: Simulation system has no 'sys' assigned.");

	slv_parameters_t p;
	slv_get_parameters(sys,&p);
	return SolverParameters(p);
}

/**
	Update the solver parameters by passing a new set back
*/
void
Simulation::setParameters(SolverParameters &P){
	if(!sys)throw runtime_error("Can't set solver parameters: simulation has not been built yet.");
	CONSOLE_DEBUG("Calling slv_set_parameters");
	slv_set_parameters(sys, &(P.getInternalType()));
}

//------------------------------------------------------------------------------
// PRE-SOLVE DIAGNOSTICS

/** 
	Get a list of variables to fix to make an underspecified system
	become square. Also seems to return stuff when you have a structurally
	singuler system.
*/
vector<Variable>
Simulation::getFixableVariables(){
	//cerr << "GETTING FIXABLE VARIABLES..." << endl;
	vector<Variable> vars;

	if(!sys){
		throw runtime_error("Simulation system not yet built");
	}

	int32 *vip; /** TODO ensure 32 bit integers are used */

	// Get IDs of elegible variables in array at vip...
	CONSOLE_DEBUG("Calling slvDOF_eligible");
	if(!slvDOF_eligible(sys,&vip)){
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"No fixable variables found.");
	}else{
		struct var_variable **vp = slv_get_solvers_var_list(sys);

		if(vp==NULL){
			throw runtime_error("Simulation variable list is null");
		}

		// iterate through this list until we find a -1:
		int i=0;
		int var_index = vip[i];
		while(var_index >= 0){
			struct var_variable *var = vp[var_index];
			vars.push_back( Variable(this, var) );
			++i;
			var_index = vip[i];
		}
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Found %d fixable variables.",i);
		ascfree(vip);
	}

	return vars;
}

/**
	Return a list of ALL the fixed variables in the solver's variable list
*/
vector<Variable>
Simulation::getFixedVariables(){
	if(!sys)throw runtime_error("Simulation system not build yet");
	vector<Variable> vars;
	var_variable **vlist = slv_get_solvers_var_list(sys);
	unsigned long nvars = slv_get_num_solvers_vars(sys);
	for(unsigned long i=0;i<nvars;++i){
		if(!var_fixed(vlist[i]))continue;
		vars.push_back(Variable(this,vlist[i]));
	}
	return vars;
}

/**
	For solvers that store a big matrix for the system, return a pointer to that
	matrix (struct mtx_header*) as a C++-wrapped object of class Matrix.
*/
Matrix
Simulation::getMatrix(){
	if(!sys)throw runtime_error("Simulation system not built yet");
	mtx_matrix_t M = slv_get_sys_mtx(sys);
	if(M==NULL)throw runtime_error("Simulation system does not possess a matrix");
	return Matrix(M);
}

/**
	Get the list of variables near their bounds. Helps to indentify why
	you might be having non-convergence problems.
*/
vector<Variable>
Simulation::getVariablesNearBounds(const double &epsilon){
	//cerr << "GETTING VARIABLES NEAR BOUNDS..." << endl;
	vector<Variable> vars;

	if(!sys){
		throw runtime_error("Simulation system not yet built");
	}

	int *vip;
	CONSOLE_DEBUG("Calling slv_near_bounds...");
	if(slv_near_bounds(sys,epsilon,&vip)){
		struct var_variable **vp = slv_get_solvers_var_list(sys);
		struct var_variable *var;
		cerr << "VARS FOUND NEAR BOUNDS" << endl;
		int nlow = vip[0];
		int nhigh = vip[1];
		int lim1 = 2 + nlow;
		for(int i=2; i<lim1; ++i){
			var = vp[vip[i]];
			char *var_name = var_make_name(sys,var);
			cerr << "AT LOWER BOUND: " << var_name << endl;
			ascfree(var_name);
			vars.push_back(Variable(this,var));
		};
		int lim2 = lim1 + nhigh;
		for(int i=lim1; i<lim2; ++i){
			var = vp[vip[i]];
			char *var_name = var_make_name(sys,var);
			cerr << "AT UPPER BOUND: " << var_name << endl;
			ascfree(var_name);
			vars.push_back(Variable(this,var));
		}
	}
	ASC_FREE(vip);
	return vars;
}

vector<Variable>
Simulation::getVariablesFarFromNominals(const double &bignum){
	vector<Variable> vars;

	if(!sys){
		throw runtime_error("Simulation system not yet built");
	}

	int *vip;
	int nv;
	CONSOLE_DEBUG("Calling slv_far_from_nominals...");
	if((nv=slv_far_from_nominals(sys, bignum, &vip))){
		struct var_variable **vp = slv_get_solvers_var_list(sys);
		struct var_variable *var;
		cerr << "VARS FAR FROM NOMINAL" << endl;
		for(int i=0; i<nv; ++i){
			var = vp[vip[i]];
			char *varname = var_make_name(sys,var);
			cerr << "FAR FROM NOMINAL: " << varname << endl;
			ASC_FREE(varname);
			vars.push_back(Variable(this,var));
		};
	}
	ASC_FREE(vip);
	return vars;
}

bool
SingularityInfo::isSingular() const{
	if(vars.size()||rels.size()){
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// SOLVING

/**
	Solve the system through to convergence. This function is hardwired with 
	a maximum of 1000 iterations, but will interrupt itself when the 'stop'
	condition comes back from the SolverReporter.
*/
void
Simulation::solve(Solver solver, SolverReporter &reporter){
	int res;

	cerr << "-----------------set solver----------------" << endl;
	CONSOLE_DEBUG("Setting solver to '%s'",solver.getName().c_str());
	setSolver(solver);

	cerr << "-----------------presolve----------------" << endl;

	//cerr << "PRESOLVING SYSTEM...";
	CONSOLE_DEBUG("Calling slv_presolve...");

	res = slv_presolve(sys);
	CONSOLE_DEBUG("slv_presolve returns %d",res);
	if(res!=0){
		throw runtime_error("Error in slv_presolve");
	}

	cerr << "-----------------solve----------------" << endl;
	//cerr << "DONE" << endl;

	//cerr << "SOLVING SYSTEM..." << endl;
	// Add some stuff here for cleverer iteration....
	unsigned niter = 1000;
	//double updateinterval = 0.02;

	double starttime = tm_cpu_time();
	//double lastupdate = starttime;
	SolverStatus status;
	//int solved_vars=0;
	bool stop=false;

	status.getSimulationStatus(*this);
	reporter.report(&status);

	for(unsigned iter = 1; iter <= niter && !stop; ++iter){

		if(status.isReadyToSolve()){
			/* CONSOLE_DEBUG("Calling slv_iterate..."); */
			res = slv_iterate(sys);
		}

		if(res)CONSOLE_DEBUG("slv_iterate returns %d",res);

		status.getSimulationStatus(*this);

		if(res || reporter.report(&status)){
			stop = true;
		}
	}

	double elapsed = tm_cpu_time() - starttime;
	CONSOLE_DEBUG("Elapsed time: %0.3f", elapsed);
	
	activeblock = status.getCurrentBlockNum();

	// reporter can do output of num of iterations etc, if it wants to.
	reporter.finalise(&status);

	// communicate solver variable status back to the instance tree
	processVarStatus();

	if(res){
		stringstream ss;
		ss << "Error in solving (res = " << res << ")";
		throw runtime_error(ss.str());
	}
	if(!status.isOK()){
		if(status.isDiverged())throw runtime_error("Solution diverged");
		if(status.isInconsistent())throw runtime_error("System is inconsistent");
		if(status.hasExceededIterationLimit())throw runtime_error("Solver exceeded iteration limit");
		if(status.hasExceededTimeLimit())throw runtime_error("Solver exceeded time limit");
		if(status.isOverDefined())throw runtime_error("Solver system is over-defined");
		if(status.isUnderDefined())throw runtime_error("Solver system is under-defined");
		throw runtime_error("Error in solver (status.isOK()==FALSE but can't see why)");
	}
}

//------------------------------------------------------------------------------
// POST-SOLVE DIAGNOSTICS

const int
Simulation::getActiveBlock() const{
	return activeblock;
}

/**
	Return an IncidenceMatrix built from the current state of the solver system.

	This will actually return something meaningful even before solve.
*/
IncidenceMatrix
Simulation::getIncidenceMatrix(){
	return IncidenceMatrix(*this);
}

/**
	This function looks at all the variables in the solve's list and updates
	the variable status for the corresponding instances.

	It does this by using the 'interface pointer' in the Instance, see
	the C-API function GetInterfacePtr.

	This is used to display visually which variables have been solved, which
	ones have not yet been attempted, and which ones were active when the solver
	failed (ASCXX_VAR_ACTIVE).
*/
void
Simulation::processVarStatus(){
	if(!sys)throw runtime_error("No system built");

	CONSOLE_DEBUG("Getting var status");

	// this is a cheap function call:
	const mtx_block_t *bb = slv_get_solvers_blocks(getSystem());

	var_variable **vlist = slv_get_solvers_var_list(getSystem());
	int nvars = slv_get_num_solvers_vars(getSystem());

	slv_status_t status;
	if(slv_get_status(sys, &status)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to update var status (get_status returns error)");
		return;
	}
	
	if(status.block.number_of == 0){
		cerr << "Variable statuses can't be set: block structure not yet determined." << endl;
		return;
	}

	if(!bb->block){
		ERROR_REPORTER_HERE(ASC_USER_WARNING,"No blocks identified in system");
		return;
	}
	
	int activeblock = status.block.current_block;
	asc_assert(activeblock <= status.block.number_of);

	int low = bb->block[activeblock].col.low;
	int high = bb->block[activeblock].col.high;
	bool allsolved = status.converged;
	for(int c=0; c < nvars; ++c){
		var_variable *v = vlist[c];
		Instanc i((Instance *)var_instance(v));
		VarStatus s = ASCXX_VAR_STATUS_UNKNOWN;
		if(i.isFixed()){
			s = ASCXX_VAR_FIXED;
		}else if(var_incident(v) && var_active(v)){
			if(allsolved || c < low){
				s = ASCXX_VAR_SOLVED;
			}else if(c <= high){
				s = ASCXX_VAR_ACTIVE;
			}else{
				s = ASCXX_VAR_UNSOLVED;
			}
		}
		i.setVarStatus(s);
	}

	CONSOLE_DEBUG(" ...done var status");
}

