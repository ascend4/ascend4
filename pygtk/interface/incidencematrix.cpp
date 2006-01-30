#include "incidencematrix.h"

#include <stdexcept>
#include <iostream>
using namespace std;

#include "variable.h"

IncidencePoint::IncidencePoint(const int&row, const int &col, const IncidencePointType &type) : row(row), col(col), type(type){
	// constructor, IncidencePoint
}

IncidencePoint::IncidencePoint(const IncidencePoint &old) : row(old.row), col(old.col), type(old.type){
	// copy ctor
}

IncidencePoint::IncidencePoint() : row(-1), col(-1), type(IM_NULL){
	// default ctor... don't use. need this to keep swig happy for some strange reason.
}

IncidenceMatrix::IncidenceMatrix(Simulation &sim) : sim(sim){
	// constructor
	is_built = FALSE;
}

IncidenceMatrix::~IncidenceMatrix(){
	if(is_built){
		free_incidence_data(&i);
	}
}

void
IncidenceMatrix::buildPlotData(){
	int c=-1;

	cerr << "BUILDPLOTDATA" << endl;

	slv_system_t sys = sim.getSystem();

	cerr << "GOT SYSTEM DATA" << endl;

	if(build_incidence_data(sys,&i)) {
		cerr << "FAILED TO BUILD INCIDENCE DATA" << endl;
		free_incidence_data(&i);
		throw runtime_error("IncidenceMatrix::buildPlotData error calculating grid");
		return;
	}

	for (int r=0; r < i.nprow; r++) {
	    struct rel_relation *rel = i.rlist[i.pr2e[r]];
	    const struct var_variable **vp = rel_incidence_list(rel);

		if(rel_active(rel)){
			int nvars = rel_n_incidences(rel);	
			if(rel_included(rel)){
				for(int v=0; v < nvars; v++ ) {
					if(var_flags(vp[v]) & VAR_SVAR) {
						int vndx = var_sindex(vp[v]);
						c = i.v2pc[vndx];
						if (i.vfixed[vndx]) {
							data.push_back(IncidencePoint(r,c,IM_ACTIVE_FIXED));
						}else{
							data.push_back(IncidencePoint(r,c,IM_ACTIVE_FREE));
						}
					}
	      		}
	    	}else{ /* hollow squares */
				for(int v=0; v < nvars; v++ ) {
					if (var_flags(vp[v]) & VAR_SVAR) {
						int vndx = var_sindex(vp[v]);
						c = i.v2pc[vndx];
						if (i.vfixed[vndx]) {
							data.push_back(IncidencePoint(r,c,IM_DORMANT_FIXED));
						} else {
							data.push_back(IncidencePoint(r,c,IM_DORMANT_FREE));
						}
					}
				}
			}
	  	}
	}
	
	is_built = TRUE;
}

const int &
IncidenceMatrix::getNumRows() const{
	return i.nprow;
}

const int &
IncidenceMatrix::getNumCols() const{
	return i.npcol;
}

const vector<IncidencePoint> &
IncidenceMatrix::getIncidenceData(){
	cerr << "GET INCIDENCE DATA" << endl;
	if(!is_built){
		buildPlotData();
	}
	return data;
}

const Variable
IncidenceMatrix::getVariable(const int &col) const{
	if(!is_built)throw runtime_error("Not built");
	if(col < 0 || col >= getNumCols())throw runtime_error("Column out of range");
	int vindex = i.pc2v[col];
	struct var_variable *var = i.vlist[vindex];

	return Variable(&sim, var);
}


