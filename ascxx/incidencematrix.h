#ifndef ASCXX_INCIDENCEMATRIX_H
#define ASCXX_INCIDENCEMATRIX_H

#include <vector>

#include "variable.h"
#include "relation.h"
#include "simulation.h"
#include "solverstatus.h"

#include "config.h"

extern "C"{
#include <ascend/system/incidence.h>
#include <ascend/system/decomp.h>
}

typedef enum{
	IM_NULL=0,
	IM_ACTIVE_FIXED,
	IM_ACTIVE_FREE,
	IM_DORMANT_FIXED,
	IM_DORMANT_FREE,
	IM_DECOMP_REAL,
	IM_DECOMP_INTEGER,
	IM_DECOMP_SELECTOR,
	IM_DECOMP_LOGICAL,
	IM_DECOMP_BOUNDARY,
	IM_DECOMP_MIXED
} IncidencePointType;

typedef enum{
	IM_CONVERGED=0, IM_OVER_ITER, IM_OVER_TIME, IM_DIVERGED, IM_NOT_YET_ATTEMPTED
} BlockStatusType;

class IncidencePoint{
public:
	IncidencePoint(const int&row, const int&col, const IncidencePointType &type);
	IncidencePoint(const IncidencePoint &);
	IncidencePoint();

	int row;
	int col;
	IncidencePointType type;
};

class DecompBlockSummary{
public:
	DecompBlockSummary();

	int block;
	int row_low;
	int col_low;
	int row_high;
	int col_high;
	int rels;
	int condrels;
	int logrels;
	int condlogrels;
	int vars;
	int intvars;
	int binvars;
	int semivars;
	int dvars;
	int booldvars;
	int intdvars;
	int symdvars;
	std::string label;
};

/**
	Special class for plotting incidence matrices using matplotlib

	GOAL: facilitate use of pylab 'spy2' function, but hopefully add extra
	stuff for viewing blocks and fixed/free incidences, solved/active/unsolved
	variables, etc.

	This is going to be like a C++ified copy of MtxProc.c
*/
class IncidenceMatrix{

private:
	Simulation &sim;
	slv_system_t sys;

	std::vector<IncidencePoint> data;
	std::vector<IncidencePoint> decomp_data;
	std::vector<DecompBlockSummary> decomp_blocks;
	incidence_vars_t i;
	slv_decomp_partition_t decomp;
	bool is_built;
	bool decomp_built;

	void buildPlotData();
	void buildDecompPlotData();
public:
	explicit IncidenceMatrix(Simulation &sim);
	IncidenceMatrix(const IncidenceMatrix &old);
	~IncidenceMatrix();

	const std::vector<IncidencePoint> &getIncidenceData();
	const std::vector<IncidencePoint> &getDecompIncidenceData();
	const int &getNumRows() const;
	const int &getNumCols() const;
	const Variable getVariable(const int &row) const;
	const Relation getRelation(const int &col) const;
	const int getBlockRow(const int & row) const;
	const std::vector<Variable> getBlockVars(const int &block);
	const std::vector<Relation> getBlockRels(const int &block);
	const std::vector<int> getBlockLocation(const int &block) const;
	const BlockStatusType getBlockStatus(const int &block) const;
	const int getNumBlocks();
	const int getDecompNumRows();
	const int getDecompNumCols();
	const int getDecompNumBlocks();
	const std::vector<int> getDecompBlockLocation(const int &block);
	const std::vector<DecompBlockSummary> &getDecompBlockSummaries();
	const std::string getDecompRowLabel(const int &row);
	const std::string getDecompColLabel(const int &col);
	const std::string getDecompRowKind(const int &row);
	const std::string getDecompColKind(const int &col);
	const std::vector<std::string> getDecompPointLegend() const;
};

#endif // ASCXX_INCIDENCEMATRIX_H
