#include "incidencematrix.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
using namespace std;

#include "variable.h"
#include "relation.h"

extern "C"{
#include <ascend/general/ascMalloc.h>
#include <ascend/linear/mtx.h>
#include <ascend/system/decomp.h>
#include <ascend/system/discrete.h>
#include <ascend/system/logrel.h>
#include <ascend/system/rel.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/var.h>
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

DecompBlockSummary::DecompBlockSummary()
	: block(-1), row_low(-1), col_low(-1), row_high(-1), col_high(-1),
	rels(0), condrels(0), logrels(0), condlogrels(0), vars(0), intvars(0),
	binvars(0), semivars(0), dvars(0), booldvars(0), intdvars(0),
	symdvars(0), label("")
{
}

IncidenceMatrix::IncidenceMatrix(Simulation &sim) : sim(sim){
	// constructor
	is_built = FALSE;
	decomp_built = FALSE;
	slv_decomp_init(&decomp);
}

IncidenceMatrix::IncidenceMatrix(const IncidenceMatrix &old) : sim(old.sim){
	is_built = FALSE;
	decomp_built = FALSE;
	slv_decomp_init(&decomp);
}

IncidenceMatrix::~IncidenceMatrix(){
	if(is_built){
		free_incidence_data(&i);
	}
	slv_decomp_destroy(&decomp);
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

static string decomp_rel_name(slv_system_t sys, struct rel_relation *rel){
	char *n = rel_make_name(sys,rel);
	if(n==NULL)return "?";
	string s = n;
	ascfree(n);
	return s;
}

static string decomp_logrel_name(slv_system_t sys, struct logrel_relation *logrel){
	char *n = logrel_make_name(sys,logrel);
	if(n==NULL)return "?";
	string s = n;
	ascfree(n);
	return s;
}

static string decomp_var_name(slv_system_t sys, struct var_variable *var){
	char *n = var_make_name(sys,var);
	if(n==NULL)return "?";
	string s = n;
	ascfree(n);
	return s;
}

static string decomp_dvar_name(slv_system_t sys, struct dis_discrete *dvar){
	char *n = dis_make_name(sys,dvar);
	if(n==NULL)return "?";
	string s = n;
	ascfree(n);
	return s;
}

static IncidencePointType decomp_point_type(
		slv_system_t sys, const slv_decomp_partition_t &decomp,
		int orgrow, int orgcol
){
	int local;
	slv_decomp_row_kind_t rowkind = slv_decomp_row_kind(&decomp,orgrow,&local);
	slv_decomp_col_kind_t colkind = slv_decomp_col_kind(&decomp,orgcol,&local);
	if(colkind == slv_decomp_col_var){
		struct var_variable **vars = slv_get_solvers_var_list(sys);
		uint32 flags = var_flags(vars[local]);
		if(rowkind == slv_decomp_row_logrel
				|| rowkind == slv_decomp_row_condlogrel){
			return IM_DECOMP_BOUNDARY;
		}
		if(flags & (VAR_INTEGER | VAR_BINARY | VAR_SEMICONT)){
			return IM_DECOMP_INTEGER;
		}
		return IM_DECOMP_REAL;
	}
	if(colkind == slv_decomp_col_dvar){
		if(rowkind == slv_decomp_row_rel
				|| rowkind == slv_decomp_row_condrel){
			return IM_DECOMP_SELECTOR;
		}
		return IM_DECOMP_LOGICAL;
	}
	return IM_DECOMP_MIXED;
}

void
IncidenceMatrix::buildDecompPlotData(){
	slv_system_t sys = sim.getSystem();
	if(slv_decomp_partition(sys,&decomp)){
		throw runtime_error("IncidenceMatrix::buildDecompPlotData error calculating mixed decomposition");
	}

	vector<int> rowcur(decomp.n_rows,-1);
	vector<int> colcur(decomp.n_cols,-1);
	for(int r=0; r < decomp.n_rows; ++r){
		rowcur[decomp.row_org[r]] = r;
	}
	for(int c=0; c < decomp.n_cols; ++c){
		colcur[decomp.col_org[c]] = c;
	}

	decomp_data.clear();
	for(int i=0; i < decomp.nnz; ++i){
		int orgrow = decomp.nz_rows[i];
		int orgcol = decomp.nz_cols[i];
		if(orgrow >= 0 && orgrow < decomp.n_rows
				&& orgcol >= 0 && orgcol < decomp.n_cols
				&& rowcur[orgrow] >= 0 && colcur[orgcol] >= 0){
			decomp_data.push_back(IncidencePoint(
				rowcur[orgrow],
				colcur[orgcol],
				decomp_point_type(sys,decomp,orgrow,orgcol)
			));
		}
	}

	decomp_blocks.clear();
	for(int b=0; b < decomp.nblocks; ++b){
		DecompBlockSummary bs;
		bs.block = b;
		bs.row_low = decomp.blocks[b].row.low;
		bs.row_high = decomp.blocks[b].row.high;
		bs.col_low = decomp.blocks[b].col.low;
		bs.col_high = decomp.blocks[b].col.high;
		for(int r=bs.row_low; r <= bs.row_high; ++r){
			int local;
			switch(slv_decomp_row_kind(&decomp,decomp.row_org[r],&local)){
			case slv_decomp_row_rel: bs.rels++; break;
			case slv_decomp_row_condrel: bs.condrels++; break;
			case slv_decomp_row_logrel: bs.logrels++; break;
			case slv_decomp_row_condlogrel: bs.condlogrels++; break;
			default: break;
			}
		}
		for(int c=bs.col_low; c <= bs.col_high; ++c){
			int local;
			switch(slv_decomp_col_kind(&decomp,decomp.col_org[c],&local)){
			case slv_decomp_col_var:{
				struct var_variable **vars = slv_get_solvers_var_list(sys);
				uint32 flags = var_flags(vars[local]);
				bs.vars++;
				if(flags & VAR_INTEGER) bs.intvars++;
				if(flags & VAR_BINARY) bs.binvars++;
				if(flags & VAR_SEMICONT) bs.semivars++;
				break;
			}
			case slv_decomp_col_dvar:{
				struct dis_discrete **dvars = slv_get_solvers_dvar_list(sys);
				enum discrete_kind kind = dis_kind(dvars[local]);
				bs.dvars++;
				if(kind == e_dis_boolean_t) bs.booldvars++;
				else if(kind == e_dis_integer_t) bs.intdvars++;
				else if(kind == e_dis_symbol_t) bs.symdvars++;
				break;
			}
			default:
				break;
			}
		}
		ostringstream label;
		label << "B" << b << ": ";
		label << bs.rels << " rel";
		if(bs.condrels) label << ", " << bs.condrels << " condrel";
		if(bs.logrels) label << ", " << bs.logrels << " logrel";
		if(bs.condlogrels) label << ", " << bs.condlogrels << " condlogrel";
		label << "; " << bs.vars << " var";
		if(bs.intvars || bs.binvars || bs.semivars){
			label << " (" << bs.intvars << " int, " << bs.binvars
				<< " bin, " << bs.semivars << " semi)";
		}
		if(bs.dvars){
			label << "; " << bs.dvars << " dvar (" << bs.booldvars
				<< " bool, " << bs.intdvars << " int, " << bs.symdvars
				<< " sym)";
		}
		bs.label = label.str();
		decomp_blocks.push_back(bs);
	}

	decomp_built = TRUE;
}

const vector<IncidencePoint> &
IncidenceMatrix::getDecompIncidenceData(){
	if(!decomp_built){
		buildDecompPlotData();
	}
	return decomp_data;
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

const int
IncidenceMatrix::getDecompNumRows(){
	if(!decomp_built){
		buildDecompPlotData();
	}
	return decomp.n_rows;
}

const int
IncidenceMatrix::getDecompNumCols(){
	if(!decomp_built){
		buildDecompPlotData();
	}
	return decomp.n_cols;
}

const int
IncidenceMatrix::getDecompNumBlocks(){
	if(!decomp_built){
		buildDecompPlotData();
	}
	return decomp.nblocks;
}

const vector<int>
IncidenceMatrix::getDecompBlockLocation(const int &block){
	if(!decomp_built){
		buildDecompPlotData();
	}
	if(block < 0 || block >= decomp.nblocks){
		throw range_error("Invalid decomposition block number");
	}
	vector<int> v;
	v.push_back(decomp.blocks[block].row.low);
	v.push_back(decomp.blocks[block].col.low);
	v.push_back(decomp.blocks[block].row.high);
	v.push_back(decomp.blocks[block].col.high);
	return v;
}

const vector<DecompBlockSummary> &
IncidenceMatrix::getDecompBlockSummaries(){
	if(!decomp_built){
		buildDecompPlotData();
	}
	return decomp_blocks;
}

const string
IncidenceMatrix::getDecompRowLabel(const int &row){
	if(!decomp_built){
		buildDecompPlotData();
	}
	if(row < 0 || row >= decomp.n_rows)throw range_error("Row out of range");
	int local;
	int orgrow = decomp.row_org[row];
	switch(slv_decomp_row_kind(&decomp,orgrow,&local)){
	case slv_decomp_row_rel:
		return decomp_rel_name(sim.getSystem(),slv_get_solvers_rel_list(sim.getSystem())[local]);
	case slv_decomp_row_condrel:
		return decomp_rel_name(sim.getSystem(),slv_get_solvers_condrel_list(sim.getSystem())[local]);
	case slv_decomp_row_logrel:
		return decomp_logrel_name(sim.getSystem(),slv_get_solvers_logrel_list(sim.getSystem())[local]);
	case slv_decomp_row_condlogrel:
		return decomp_logrel_name(sim.getSystem(),slv_get_solvers_condlogrel_list(sim.getSystem())[local]);
	default:
		return "?";
	}
}

const string
IncidenceMatrix::getDecompColLabel(const int &col){
	if(!decomp_built){
		buildDecompPlotData();
	}
	if(col < 0 || col >= decomp.n_cols)throw range_error("Column out of range");
	int local;
	int orgcol = decomp.col_org[col];
	switch(slv_decomp_col_kind(&decomp,orgcol,&local)){
	case slv_decomp_col_var:
		return decomp_var_name(sim.getSystem(),slv_get_solvers_var_list(sim.getSystem())[local]);
	case slv_decomp_col_dvar:
		return decomp_dvar_name(sim.getSystem(),slv_get_solvers_dvar_list(sim.getSystem())[local]);
	default:
		return "?";
	}
}

const string
IncidenceMatrix::getDecompRowKind(const int &row){
	if(!decomp_built){
		buildDecompPlotData();
	}
	if(row < 0 || row >= decomp.n_rows)throw range_error("Row out of range");
	int local;
	switch(slv_decomp_row_kind(&decomp,decomp.row_org[row],&local)){
	case slv_decomp_row_rel: return "relation";
	case slv_decomp_row_condrel: return "conditional relation";
	case slv_decomp_row_logrel: return "logical relation";
	case slv_decomp_row_condlogrel: return "conditional logical relation";
	default: return "unknown";
	}
}

const string
IncidenceMatrix::getDecompColKind(const int &col){
	if(!decomp_built){
		buildDecompPlotData();
	}
	if(col < 0 || col >= decomp.n_cols)throw range_error("Column out of range");
	int local;
	switch(slv_decomp_col_kind(&decomp,decomp.col_org[col],&local)){
	case slv_decomp_col_var:{
		struct var_variable **vars = slv_get_solvers_var_list(sim.getSystem());
		uint32 flags = var_flags(vars[local]);
		if(flags & VAR_BINARY) return "binary solver variable";
		if(flags & VAR_INTEGER) return "integer solver variable";
		if(flags & VAR_SEMICONT) return "semicontinuous solver variable";
		return "real solver variable";
	}
	case slv_decomp_col_dvar:{
		struct dis_discrete **dvars = slv_get_solvers_dvar_list(sim.getSystem());
		switch(dis_kind(dvars[local])){
		case e_dis_boolean_t: return "boolean discrete variable";
		case e_dis_integer_t: return "integer discrete variable";
		case e_dis_symbol_t: return "symbol discrete variable";
		default: return "discrete variable";
		}
	}
	default:
		return "unknown";
	}
}

const vector<string>
IncidenceMatrix::getDecompPointLegend() const{
	vector<string> legend;
	legend.push_back("real");
	legend.push_back("integer");
	legend.push_back("selector");
	legend.push_back("logical");
	legend.push_back("boundary");
	legend.push_back("mixed");
	return legend;
}
