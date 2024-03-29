#include "incidencematrix.h"

#include <stdexcept>
#include <iostream>
using namespace std;

#include "variable.h"
#include "relation.h"

extern "C"{
#include <ascend/linear/mtx.h>
#include <ascend/system/slv_client.h>
}

//#define INCIDENCEMATRIX_DEBUG
#ifdef INCIDENCEMATRIX_DEBUG
# define MSG CONSOLE_DEBUG
# define ERRMSG CONSOLE_DEBUG
#else
# define MSG(...) 
# define ERRMSG CONSOLE_DEBUG
#endif

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

	//cerr << "BUILDPLOTDATA" << endl;

	slv_system_t sys = sim.getSystem();

	//cerr << "GOT SYSTEM DATA" << endl;

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
	if(col < 0 || col >= getNumCols())throw range_error("Column out of range");
	int vindex = i.pc2v[col];
	struct var_variable *var = i.vlist[vindex];

	return Variable(&sim, var);
}

const Relation
IncidenceMatrix::getRelation(const int &row) const{
	if(!is_built)throw runtime_error("Not built");
	if(row < 0 || row >= getNumRows())throw range_error("Row out of range");
	int rindex = i.pr2e[row];
	struct rel_relation *rel = i.rlist[rindex];
	return Relation(&sim, rel);
}

const int
IncidenceMatrix::getBlockRow(const int &row) const{
	if(!is_built)throw runtime_error("Not built");
	if(row < 0 || row >= getNumRows())throw range_error("Row out of range");
	const mtx_block_t *bb = slv_get_solvers_blocks(sim.getSystem());
	for(int i=0; i < bb->nblocks; ++i){
		if(row >= bb->block[i].row.low && row <= bb->block[i].row.high){
			return i;
		}
	}
	return -1;
}

/**
	Returns location of specified block
	@param block the block number
	@return vector(ve row-low, col-low, row-high, col-high)
*/
const vector<int>
IncidenceMatrix::getBlockLocation(const int &block) const{
	if(!is_built)throw runtime_error("Not built");
	const mtx_block_t *bb = slv_get_solvers_blocks(sim.getSystem());
	if(block < 0 || block >= bb->nblocks){
		throw range_error("Invalid block number");
	}
	vector<int> v;
	v.push_back(bb->block[block].row.low);
	v.push_back(bb->block[block].col.low);
	v.push_back(bb->block[block].row.high);
	v.push_back(bb->block[block].col.high);
	return v;
}

const BlockStatusType
IncidenceMatrix::getBlockStatus(const int &block) const{
	if(!is_built)throw runtime_error("Not build");
	SolverStatus st;
	st.getSimulationStatus(sim);
	
	if(st.isConverged() || st.getCurrentBlockNum() > block){
		return IM_CONVERGED;
	}

	if(st.getCurrentBlockNum() < block){
		return IM_NOT_YET_ATTEMPTED;
	}

	if(st.hasExceededIterationLimit())return IM_OVER_ITER;
	if(st.hasExceededTimeLimit())return IM_OVER_TIME;
	return IM_DIVERGED;
}

const vector<Variable>
IncidenceMatrix::getBlockVars(const int &block){
	if(!is_built){
		buildPlotData();
	}
	vector<Variable> v;
	const mtx_block_t *bb = slv_get_solvers_blocks(sim.getSystem());
	if(block < 0 || block >= bb->nblocks){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Block out of range");
		return v;
	}
	int low = bb->block[block].col.low;
	int high = bb->block[block].col.high;
	for(int j=low; j<=high; ++j){
		v.push_back(getVariable(j));
	}
	return v;
}

const vector<Relation>
IncidenceMatrix::getBlockRels(const int &block){
	MSG("...");
	if(!is_built){
		buildPlotData();
	}
	vector<Relation> v;
	const mtx_block_t *bb = slv_get_solvers_blocks(sim.getSystem());
	if(block < 0 || block >= bb->nblocks){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Block out of range");
		return v;
	}
	int low = bb->block[block].row.low;
	int high = bb->block[block].row.high;
	for(int j=low; j<=high; ++j){
		v.push_back(getRelation(j));
	}
	MSG("...");
	return v;
}
	
const int
IncidenceMatrix::getNumBlocks(){
	if(!is_built){
		buildPlotData();
	}
	const mtx_block_t *bb = slv_get_solvers_blocks(sim.getSystem());
	return bb->nblocks;
}


