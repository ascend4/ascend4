#include "library.h"
#include "simulation.h"
#include "solver.h"
#include "solverreporter.h"
#include "solverparameters.h"

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace std;

namespace {

string path_stem(const string &path){
	string::size_type slash = path.find_last_of("/\\");
	string::size_type start = (slash == string::npos) ? 0 : slash + 1;
	string::size_type dot = path.find_last_of('.');
	if(dot == string::npos || dot < start){
		dot = path.size();
	}
	return path.substr(start, dot - start);
}

void print_usage(const char *prog){
	cerr << "Usage: " << prog << " [options] [model_file]" << endl;
	cerr << "Options:" << endl;
	cerr << "  --model NAME        Model name (defaults to file stem)" << endl;
	cerr << "  --solver NAME       Solver name (default: HiGHS)" << endl;
	cerr << "  --no-progress       Disable progress callbacks (solver param)" << endl;
	cerr << "  --progress-log      Enable solver progress logging to console" << endl;
	cerr << "  --no-tdlm           Print each progress line (no in-place updates)" << endl;
	cerr << "  --no-solve          Instantiate only, do not solve" << endl;
	cerr << "  -h, --help          Show this help" << endl;
}

class TdlmReporter : public SolverReporter{
public:
	explicit TdlmReporter(bool tdlm_mode, Simulation *sim_ptr)
		: tdlm(tdlm_mode), last_len(0), sim(sim_ptr), have_primal(false), last_primal(0.0){}

	void reportProgress(const char *solver_name, const char *message){
		string line;
		bool milestone = false;
		if(sim != NULL){
			SolverStatus status;
			status.getSimulationStatus(*sim);
			line = format_status_line(status, solver_name);
			if(status.isMIP() && status.hasMipPrimalBound()){
				double primal = status.getMipPrimalBound();
				if(!have_primal || primal != last_primal){
					milestone = true;
					have_primal = true;
					last_primal = primal;
				}
			}
		}
		if(line.empty()){
			if(message == NULL)return;
			if(solver_name && solver_name[0] != '\0'){
				line = string(solver_name) + ": " + message;
			}else{
				line = message;
			}
		}

		if(tdlm){
			if(milestone){
				if(last_len > 0){
					cerr << "\r" << string(last_len,' ') << "\r";
					last_len = 0;
				}
				cerr << line << endl;
				return;
			}
			cerr << "\r" << line;
			if(last_len > line.size()){
				cerr << string(last_len - line.size(),' ');
			}
			cerr << flush;
			last_len = line.size();
		}else{
			cerr << line << endl;
		}
	}

	void finalise(SolverStatus *status){
		if(tdlm && last_len > 0){
			cerr << endl;
			last_len = 0;
		}
		SolverReporter::finalise(status);
	}

private:
	string format_status_line(const SolverStatus &status, const char *solver_name){
		ostringstream oss;
		if(solver_name && solver_name[0] != '\0'){
			oss << solver_name << ": ";
		}
		oss << "iter=" << status.getIterationNum();
		oss << ", cpu=" << fixed << setprecision(2) << status.getCpuElapsed() << "s";
		if(status.isMIP()){
			if(status.hasMipNodeCount()) oss << ", nodes=" << status.getMipNodeCount();
			if(status.hasMipTotalLpIterations()) oss << ", lp_iter=" << status.getMipTotalLpIterations();
			if(status.hasMipPrimalBound()) oss << ", mip_primal=" << setprecision(8) << status.getMipPrimalBound();
			if(status.hasMipDualBound()) oss << ", mip_dual=" << setprecision(8) << status.getMipDualBound();
			if(status.hasMipGap()) oss << ", mip_gap=" << setprecision(6) << status.getMipGap();
			else if(status.hasMipAbsGap()) oss << ", mip_abs_gap=" << setprecision(8) << status.getMipAbsGap();
		}else if(status.isLP()){
			if(status.hasLpObjective()) oss << ", obj=" << setprecision(8) << status.getLpObjective();
		}
		return oss.str();
	}

	bool tdlm;
	size_t last_len;
	Simulation *sim;
	bool have_primal;
	double last_primal;
};

bool set_bool_param(Simulation &S, const string &name, bool value){
	try{
		SolverParameters pp = S.getParameters();
		SolverParameter p = pp.getParameter(name);
		p.setBoolValue(value);
		S.setParameters(pp);
		return true;
	}catch(std::runtime_error &){
		return false;
	}
}

bool get_bool_param(Simulation &S, const string &name, bool &out){
	try{
		SolverParameters pp = S.getParameters();
		SolverParameter p = pp.getParameter(name);
		out = p.getBoolValue();
		return true;
	}catch(std::runtime_error &){
		return false;
	}
}

} // namespace

int main(int argc, char **argv){
	string file = "models/johnpye/orienteer.a4c";
	string model;
	string solver_name = "HiGHS";
	bool progress = true;
	bool progress_log = false;
	bool tdlm = true;
	bool solve = true;

	for(int i = 1; i < argc; ++i){
		string arg = argv[i];
		if(arg == "--model" && i + 1 < argc){
			model = argv[++i];
		}else if(arg == "--solver" && i + 1 < argc){
			solver_name = argv[++i];
		}else if(arg == "--no-progress"){
			progress = false;
		}else if(arg == "--progress-log"){
			progress_log = true;
		}else if(arg == "--no-tdlm"){
			tdlm = false;
		}else if(arg == "--no-solve"){
			solve = false;
		}else if(arg == "-h" || arg == "--help"){
			print_usage(argv[0]);
			return 0;
		}else if(arg.rfind("--",0) == 0){
			print_usage(argv[0]);
			return 2;
		}else{
			file = arg;
		}
	}

	try{
		Library L;
		L.load(file.c_str());
		if(model.empty()){
			model = path_stem(file);
		}
		Type t = L.findType(model);
		Simulation S = t.getSimulation("sim",1); // run on_load

		if(!solver_name.empty()){
			Solver solver(solver_name);
			S.setSolver(solver);
		}

		if(!set_bool_param(S,"progress_callbacks",progress)){
			if(!progress){
				cerr << "Note: progress_callbacks param not available for solver" << endl;
			}
		}

		if(solve){
			TdlmReporter R(tdlm, &S);
			S.presolve(S.getSolver());

			if(!set_bool_param(S,"progress_callbacks",progress)){
				if(!progress){
					cerr << "Note: progress_callbacks param not available for solver" << endl;
				}
			}
			set_bool_param(S,"progress_log",progress_log);
			{
				bool cur = false;
				if(get_bool_param(S,"progress_callbacks",cur)){
					cerr << "progress_callbacks = " << (cur ? "TRUE" : "FALSE") << endl;
				}else{
					cerr << "progress_callbacks param unavailable" << endl;
				}
				if(get_bool_param(S,"progress_log",cur)){
					cerr << "progress_log = " << (cur ? "TRUE" : "FALSE") << endl;
				}else{
					cerr << "progress_log param unavailable" << endl;
				}
			}

			struct ProgressReporterScope{
				ProgressReporterScope(SolverReporter *reporter){
					setSolverProgressReporter(reporter);
				}
				~ProgressReporterScope(){
					setSolverProgressReporter(NULL);
				}
			} progress_reporter_scope(&R);
			cerr << "progress reporter enabled" << endl;

			SolverStatus status;
			status.getSimulationStatus(S);
			R.report(&status);

			int res = 0;
			bool stop = false;
			while(!stop){
				if(status.isReadyToSolve()){
					res = S.iterate();
				}else{
					stop = true;
				}
				status.getSimulationStatus(S);
				if(res || R.report(&status)){
					stop = true;
				}
			}
			S.postsolve(status);
			if(res){
				throw runtime_error("Error in slv_iterate");
			}
		}
	}catch(std::runtime_error &e){
		cerr << "testhighs: " << e.what() << endl;
		return 1;
	}

	return 0;
}
