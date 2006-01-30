#ifndef ASCXX_INCIDENCEMATRIX_H
#define ASCXX_INCIDENCEMATRIX_H

#include <vector>

#include "simulation.h"

extern "C"{
#include <solver/incidence.h>
}

typedef enum{
	IM_NULL=0, IM_ACTIVE_FIXED, IM_ACTIVE_FREE, IM_DORMANT_FIXED, IM_DORMANT_FREE
} IncidencePointType;

class IncidencePoint{
public:
	IncidencePoint(const int&row, const int&col, const IncidencePointType &type);
	IncidencePoint(const IncidencePoint &);
	IncidencePoint();

	int row;
	int col;
	IncidencePointType type;
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
	slv_system_structure *sys;

	std::vector<IncidencePoint> data;
	incidence_vars_t i;
	bool is_built;

	void buildPlotData();
public:
	explicit IncidenceMatrix(Simulation &sim);
	~IncidenceMatrix();

	const std::vector<IncidencePoint> &getIncidenceData();
	const int &getNumRows() const;
	const int &getNumCols() const;
	const Variable getVariable(const int &row) const;
};

#endif // ASCXX_INCIDENCEMATRIX_H
