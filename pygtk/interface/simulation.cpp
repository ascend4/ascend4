#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
using namespace std;

extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <general/dstring.h>
#include <general/tm_time.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instantiate.h>
#include <compiler/bintoken.h>
#include <compiler/instance_enum.h>
#include <compiler/instquery.h>
#include <compiler/check.h>
#include <compiler/name.h>
#include <compiler/pending.h>

#include <utilities/readln.h>
#include <solver/mtx.h>
#include <solver/slv_types.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/discrete.h>
#include <solver/conditional.h>
#include <solver/logrel.h>
#include <solver/bnd.h>
#include <solver/calc.h>
#include <solver/relman.h>
#include <solver/slv_common.h>
#include <solver/linsol.h>
#include <solver/linsolqr.h>
#include <solver/slv_client.h>
#include <solver/system.h>
#include <solver/slv_interface.h>
#include <solver/slvDOF.h>
#include <solver/slv3.h>
#include <solver/slv_stdcalls.h>
}

#include "simulation.h"
#include "solver.h"
#include "solverparameters.h"
#include "name.h"
#include "incidencematrix.h"
#include "variable.h"
#include "solverstatus.h"
#include "solverreporter.h"

/**
	Create an instance of a type (call compiler etc)

	@TODO fix mutex on compile command filenames
*/
Simulation::Simulation(Instance *i, const SymChar &name) : Instanc(i, name), simroot(GetSimulationRoot(i),SymChar("simroot")){
	is_built = false;
	// Create an Instance object for the 'simulation root' (we'll call
	// it the 'simulation model') and it can be fetched using 'getModel()'
	// any time later.
	//simroot = Instanc(GetSimulationRoot(i),name);
}

Simulation::Simulation(const Simulation &old) : Instanc(old), simroot(old.simroot){
	is_built = old.is_built;
	sys = old.sys;
	bin_srcname = old.bin_srcname;
	bin_objname = old.bin_objname;
	bin_libname = old.bin_libname;
	bin_cmd = old.bin_cmd;
	bin_rm = old.bin_rm;
}

Simulation::~Simulation(){
	//CONSOLE_DEBUG("Deleting simulation %s", getName().toString());
}

Instanc &
Simulation::getModel(){
	if(!simroot.getInternalType()){
		throw runtime_error("Simulation::getModel: simroot.getInternalType()is NULL");
	}
	return simroot;
}

void
Simulation::checkDoF() const{
		cerr << "CHECKING DOF..." << endl;
        int dof, status;
        if(!sys){
                throw runtime_error("System not yet built");
        }
        slvDOF_status(sys, &status, &dof);
        switch(status){
                case 1: ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Underspecified; %d degrees of freedom",dof); break;
                case 2: ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Square"); break;
                case 3: ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Structurally singular"); break;
                case 4: ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Overspecified"); break;
                case 5:
                        throw runtime_error("Unable to resolve degrees of freedom"); break;
                default:
                        throw runtime_error("Invalid return status from slvDOF_status");
        }
}

void
Simulation::checkConsistency() const{
	cerr << "CHECKING CONSISTENCY..." << endl;
	int *fixedarrayptr;

	int res = consistency_analysis(sys, &fixedarrayptr);
	struct var_variable **vp = slv_get_master_var_list(sys);

	if(res==1){
		cerr << "STRUCTURALLY CONSISTENT" << endl;
		return;
	}else{
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Structurally inconsistent. Free the variables listed on the console\nin order to make system consistent.");
		cerr << "INCONSISTENT: Free these vars:" << endl;
		for(int i=0; fixedarrayptr[i]!=-1; ++i){
			Instanc i1((struct Instance *)var_instance(vp[fixedarrayptr[i]]));
			cerr << "  " << getInstanceName(i1) << endl;
		}
	}
}

void
Simulation::checkStructuralSingularity() const{
	cerr << "CHECKING STRUCTURAL SINGULARITY..." << endl;

	int *vil;
	int *ril;
	int *fil;

	int res = slvDOF_structsing(sys, mtx_FIRST, &vil, &ril, &fil);
	struct var_variable **varlist = slv_get_solvers_var_list(sys);
	struct rel_relation **rellist = slv_get_solvers_rel_list(sys);

	if(res==0){
		cerr << "UNABLE TO DETERMINE SINGULARITY LISTS" << endl;
		return;
	}else if(res==1){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Structurally singular. Check the listing on the console.");
		cerr << "STRUCTURALLY SINGULAR: The found singularity involves these relations:" << endl;
		for(int i=0; ril[i]!=-1; ++i){
			Instanc i1((struct Instance *)rel_instance(rellist[ril[i]]));
			cerr << "  " << getInstanceName(i1) << endl;
		}

		cerr << "STRUCTURALLY SINGULAR: ... and these variables:" << endl;
		for(int i=0; vil[i]!=-1; ++i){
			Instanc i1((struct Instance *)var_instance(varlist[vil[i]]));
			cerr << "  " << getInstanceName(i1) << endl;
		}

		cerr << "STRUCTURALLY SINGULAR: ... and may be mitigated by freeing these variables:" << endl;
		for(int i=0; fil[i]!=-1; ++i){
			Instanc i1((struct Instance *)var_instance(varlist[fil[i]]));
			cerr << "  " << getInstanceName(i1) << endl;
		}
	}else{
		throw runtime_error("Invalid return from slvDOF_structsing.");
	}
	ascfree(vil);
	ascfree(ril);
	ascfree(fil);	
}

void
Simulation::run(const Method &method){
	cerr << "RUNNING PROCEDURE " << method.getName() << endl;
	Nam name = Nam(method.getSym());
	//cerr << "CREATED NAME '" << name.getName() << "'" << endl;
	Proc_enum pe;
	pe = Initialize(
		&*(getModel().getInternalType()) ,name.getInternalType(), "__not_named__"
		,ASCERR
		,0, NULL, NULL
	);

	if(pe == Proc_all_ok){
		ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Method '%s' was run (check above for errors)\n",method.getName());
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

const bool
Simulation::check(){
	cerr << "CHECKING SIMULATION" << endl;
	Instance *i1 = getModel().getInternalType();
	CheckInstance(stderr, &*i1);
	cerr << "...DONE CHECKING" << endl;
	this->checkConsistency();
	this->checkStructuralSingularity();
}

void
Simulation::build(){
	cerr << "BUILDING SIMULATION..." << endl;
	Instance *i1 = getModel().getInternalType();
	sys = system_build(&*i1);
	if(!sys){
		throw runtime_error("Unable to build system");
	}
	is_built = true;
	cerr << "...DONE BUILDING" << endl;
}

vector<Variable>
Simulation::getFixableVariables(){
	cerr << "GETTING FIXABLE VARIABLES..." << endl;
	vector<Variable> vars;

	if(!sys){
		throw runtime_error("Simulation system not yet built");
	}

	int32 *vip; /** TODO ensure 32 bit integers are used */

	// Get IDs of elegible variables in array at vip...
	if(!slvDOF_eligible(sys,&vip)){
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"No fixable variables found.");
	}else{
		//cerr << "FIXABLE VARS FOUND" << endl;
		struct var_variable **vp = slv_get_solvers_var_list(sys);

		/*struct var_variable *first_var = vp[0];
		char *first_var_name = var_make_name(sys,first_var);
		cerr << "FIRST SYS VAR IS NAMED " << var_make_name(s,first_var) << endl;
		ascfree(first_var_name);*/

		if(vp==NULL){
			throw runtime_error("Simulation variable list is null");
		}

		// iterate through this list until we find a -1:
		int i=0;
		int var_index = vip[i];
		while(var_index >= 0){
			//cerr << "FOUND VARIABLE var_index = " << var_index << endl;
			struct var_variable *var = vp[var_index];
			//cerr << "VARIABLE " << var_index << " IS ELIGIBLE" << endl;
			char *var_name = var_make_name(sys,var);
			//cerr << "ELIGIBLE VAR: " << var_name << endl;
			ascfree(var_name);
			vars.push_back( Variable(this, var) );
			++i;
			var_index = vip[i];
		}
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"Found %d fixable variables.",i);
		//cerr << "END ELEGIBLE VARS LIST" << endl;
		ascfree(vip);
		//cerr << "FREED VIP LIST" << endl;
	}

	//cerr << "FINISHED WITH FINDING ELEGIBLE VARIABLES" << endl;
	return vars;
}

	
void
Simulation::solve(Solver solver, SolverReporter &reporter){
	if(!is_built){
		throw runtime_error("Simulation::solver: simulation is not yet built, can't start solving.");
	}

	cerr << "SIMULATION::SOLVE STARTING..." << endl;
	enum inst_t k = getModel().getKind();
	if(k!=MODEL_INST)throw runtime_error("Can't solve: not an instance of type MODEL_INST");

	Instance *i1 = getInternalType();
	int npend = NumberPendingInstances(&*i1);
	if(npend)throw runtime_error("Can't solve: There are still %d pending instances");

	if(!sys)throw runtime_error("Can't solve: Simulation system has not been built yet.");

	cerr << "SIMULATION::SOLVE: SET SOLVER..." << endl;
	setSolver(solver);


	cerr << "PRESOLVING SYSTEM...";
	slv_presolve(sys);
	cerr << "DONE" << endl;

	cerr << "SOLVING SYSTEM..." << endl;
	// Add some stuff here for cleverer iteration....
	unsigned niter = 1000;
	double updateinterval = 0.02;

	double starttime = tm_cpu_time();
	double lastupdate = starttime;
	SolverStatus status;
	int solved_vars=0;
	bool stop=false;

	status.getSimulationStatus(*this);
	reporter.report(&status);

	for(int iter = 1; iter <= niter && !stop; ++iter){

		if(status.isReadyToSolve()){
			slv_iterate(sys);
		}

		status.getSimulationStatus(*this);
		
		if(reporter.report(&status)){
			stop = true;
		}
	}

	double elapsed = tm_cpu_time() - starttime;


	activeblock = status.getCurrentBlockNum();

	reporter.finalise(&status);

	if(status.isOK()){
		cerr << "... SOLVED, STATUS OK" << endl;
	}else{
		cerr << "... SOLVER FAILED" << endl;
	}

	cerr << "SOLVER PERFORMED " << status.getIterationNum() << " ITERATIONS IN " << elapsed << "s" << endl;

	/*
	if(status.hasExceededTimeLimit()){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Exceeded interation limit");
	}

	if(status.isConverged()){
		ERROR_REPORTER_NOLINE(ASC_USER_SUCCESS,"Solver converged: %d iterations (%.2f s)"
			,status.getIterationNum(),elapsed);
	}else{
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Solver not converged in block %d after overall %d iterations (see console)"
			" (%.2f s).",status.getCurrentBlockNum(),status.getIterationNum(),elapsed);
		IncidenceMatrix inc = getIncidenceMatrix();

		cerr << "VARIABLES IN NON-CONVERGED BLOCK:" << endl;
		vector<Variable> v = inc.getBlockVars(status.block.current_block);
		for(vector<Variable>::iterator vi = v.begin(); vi < v.end(); ++vi){
			cerr << vi->getName() << " = " << vi->getValue() << endl;
		}

		cerr << "RELATIONS IN NON-CONVERGED BLOCK:" << endl;
		vector<Relation> r = inc.getBlockRels(status.block.current_block);
		for(vector<Relation>::iterator ri = r.begin(); ri < r.end(); ++ri){
			cerr << ri->getName() << endl;
		}
		
	}
	*/

}

void
Simulation::write(){
	simroot.write();
}

//------------------------------------------
// ASSIGNING SOLVER TO SIMULATION

void
Simulation::setSolver(Solver &solver){
	cerr << "SETTING SOLVER ON SIMULATION TO " << solver.getName() << endl;

	if(!sys)throw runtime_error("Can't solve: Simulation system has not been built yet.");
	// Update the solver object because sometimes an alternative solver can be returned, apparently.

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


/**
	Get solver parameters struct wrapped up as a SolverParameters class.
*/
SolverParameters
Simulation::getSolverParameters() const{
	if(!sys)throw runtime_error("Can't getSolverParameters: Simulation system has not been built yet.");

	slv_parameters_t p;
	slv_get_parameters(sys,&p);
	return SolverParameters(p);
}

/**
	Update the solver parameters by passing a new set back
*/
void
Simulation::setSolverParameters(SolverParameters &P){
	if(!sys)throw runtime_error("Can't set solver parameters: simulation has not been built yet.");
	slv_set_parameters(sys, &(P.getInternalType()));
}

slv_system_structure *
Simulation::getSystem(){
	if(!sys)throw runtime_error("Can't getSystem: simulation not yet built");
	return sys;
}

IncidenceMatrix
Simulation::getIncidenceMatrix(){
	return IncidenceMatrix(*this);
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

void
Simulation::processVarStatus(){

	// this is a cheap function call:
	const mtx_block_t *bb = slv_get_solvers_blocks(getSystem());

	var_variable **vlist = slv_get_solvers_var_list(getSystem());
	int nvars = slv_get_num_solvers_vars(getSystem());

	slv_status_t status;
	slv_get_status(getSystem(), &status);

	if(status.block.number_of == 0){
		cerr << "Variable statuses can't be set: block structure not yet determined." << endl;
		return;
	}

	int activeblock = status.block.current_block;
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
}

const int
Simulation::getActiveBlock() const{
	return activeblock;
}
