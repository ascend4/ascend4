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
#include "name.h"

/**
	Create an instance of a type (call compiler etc)

	@TODO fix mutex on compile command filenames
*/
Simulation::Simulation(Instance *i, const SymChar &name) : Instanc(i, name), simroot(GetSimulationRoot(i),SymChar("simroot")){
	// Create an Instance object for the 'simulation root' (we'll call
	// it the 'simulation model') and it can be fetched using 'getModel()' 
	// any time later.
	//simroot = Instanc(GetSimulationRoot(i),name);
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
        if(sys==NULL){
                throw runtime_error("System not yet built");
        }
        slvDOF_status(sys, &status, &dof);
        switch(status){
                case 1: error_reporter(ASC_USER_ERROR,NULL,0,"Underspecified; %d degrees of freedom",dof); break;
                case 2: error_reporter(ASC_USER_NOTE,NULL,0,"Square"); break;
                case 3: error_reporter(ASC_USER_ERROR,NULL,0,"Structurally singular"); break;
                case 4: error_reporter(ASC_USER_ERROR,NULL,0,"Overspecified"); break;
                case 5:
                        throw runtime_error("Unable to resolve degrees of freedom"); break;
                default:
                        throw runtime_error("Invalid return status from slvDOF_status");
        }
}

void
Simulation::run(const Method &method){
	cerr << "RUNNING PROCEDURE " << method.getName() << endl;
	Nam name = Nam(method.getSym());
	cerr << "CREATED NAME '" << name.getName() << "'" << endl;
	Proc_enum pe;
	pe = Initialize(
		getModel().getInternalType() ,name.getInternalType(), "__not_named__"
		,ASCERR
		,0, NULL, NULL
	);

	if(pe == Proc_all_ok){
		error_reporter(ASC_PROG_NOTE,NULL,0,"Method '%s' was run (check above for errors)",method.getName());
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
	CheckInstance(stderr, i1);
	cerr << "...DONE CHECKING" << endl;
}		

void
Simulation::build(){
	cerr << "BUILDING SIMULATION..." << endl;
	sys = system_build(getModel().getInternalType());
	if(sys == NULL){
		throw runtime_error("Unable to build system");
	}
	cerr << "...DONE BUILDING" << endl;
}

vector<Variable>
Simulation::getFixableVariables(){
	cerr << "GETTING FIXABLE VARIABLES..." << endl;
	vector<Variable> vars;
	vars.reserve(100);

	if(sys==NULL){
		throw runtime_error("Simulation system not yet built");
	}

	int32 **vip; /** TODO ensure 32 bit integers are used */

	// Get IDs of elegible variables in array at vip...
	if(!slvDOF_eligible(sys,vip)){
		error_reporter(ASC_USER_NOTE,NULL,0,"No fixable variables found.");
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
		int var_index = (*vip)[i];
		while(var_index >= 0){
			//cerr << "FOUND VARIABLE var_index = " << var_index << endl;
			struct var_variable *var = vp[var_index];
			//cerr << "VARIABLE " << var_index << " IS ELIGIBLE" << endl;
			char *var_name = var_make_name(sys,var);
			//cerr << "ELIGIBLE VAR: " << var_name << endl;
			ascfree(var_name);
			vars.push_back(Variable( sys,var ));
			++i;
			var_index = (*vip)[i];
		}
		error_reporter(ASC_USER_NOTE,NULL,0,"Found %d fixable variables.",i);
		//cerr << "END ELEGIBLE VARS LIST" << endl;
		ascfree(*vip);
		//cerr << "FREED VIP LIST" << endl;
	}

	//cerr << "FINISHED WITH FINDING ELEGIBLE VARIABLES" << endl;
	return vars;
}

void
Simulation::solve(Solver solver){
	cerr << "SIMULATION::SOLVE STARTING..." << endl;
	enum inst_t k = getModel().getKind();
	if(k!=MODEL_INST)throw runtime_error("Can't solve: not an instance of type MODEL_INST");
	
	int npend = NumberPendingInstances(getInternalType());
	if(npend)throw runtime_error("Can't solve: There are still %d pending instances");	

	if(!sys)throw runtime_error("Can't solve: Simulation system has not been built yet.");
	
	cerr << "SIMULATION::SOLVE: SET SOLVER..." << endl;
	setSolver(solver);
	
	
	cerr << "PRESOLVING SYSTEM..." << endl;
	slv_presolve(sys);
	cerr << "... DONE PRESOLVING" << endl;
	
	cerr << "SOLVING SYSTEM..." << endl;
	slv_solve(sys);

	slv_status_t slvstat;
	slv_get_status(sys,&slvstat);
	if(slvstat.ok){
		cerr << "... DONE SOLVING SYSTEM" << endl;
	}else{
		error_reporter(ASC_USER_ERROR,NULL,0,"Solver failed");
	}

	cerr << "SOLVER PERFORMED " << slvstat.iteration << " ITERATIONS IN " << slvstat.cpu_elapsed << "s" << endl;

	if(slvstat.iteration_limit_exceeded){
		error_reporter(ASC_USER_ERROR,NULL,0,"Exceeded interation limit");
	}

	if(slvstat.converged){
		error_reporter(ASC_USER_SUCCESS,NULL,0,"Solver converged: %d iterations, %3.2e s"
			,slvstat.iteration,slvstat.cpu_elapsed);
	}else{
		error_reporter(ASC_USER_ERROR,NULL,0,"Solver not converged after %d iteratoins.",slvstat.iteration);
	}
	

	//slv_print_output(stderr,solver);

	/*
	if(s==NULL){
		throw runtime_error("System not yet built");
	}
	cerr << "PRESOLVING SYSTEM..." << endl;
	slv_presolve(s);
	cerr << "... DONE PRESOLVING" << endl;
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
	if(!sys)throw runtime_error("Can't solve: Simulation system has not been built yet.");
	// Update the solver object because sometimes an alternative solver can be returned, apparently.

	int selected = slv_select_solver(sys, solver.getIndex());
	cerr << "Simulation::setSolver: slv_select_solver returned " << selected << endl;

	if(selected<0){
		error_reporter(ASC_PROG_ERROR,NULL,0,"Failed to select solver");
		throw runtime_error("Failed to select solver");
	}

	if(selected!=solver.getIndex()){
		solver = Solver(selected);
		error_reporter(ASC_PROG_NOTE,NULL,0,"Substitute solver '%s' (index %d) selected.\n", solver.getName().c_str(), selected);
	}
	
	if( slv_eligible_solver(sys) <= 0){
		error_reporter(ASC_PROG_ERROR,NULL,0,"Inelegible solver '%s'", solver.getName().c_str() );
		throw runtime_error("Inelegible solver");
	}
}

const Solver
Simulation::getSolver() const{
	int index = slv_get_selected_solver(sys);
	cerr << "Simulation::getSolver: index = " << index << endl;
	if(index<0)throw runtime_error("No solver selected");

	return Solver(index);
}



