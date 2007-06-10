#include "solverstatus.h"
#include "simulation.h"

extern "C"{
#include <solver/solver.h>
}

SolverStatus::SolverStatus(){
	// do nothing else
}

SolverStatus::SolverStatus(const SolverStatus &old) : s(old.s){
	// just that
}

void
SolverStatus::getSimulationStatus(Simulation &sim){
	slv_get_status(sim.getSystem(), &s);
}

const bool
SolverStatus::isOK() const{
	return s.ok;
}

const bool
SolverStatus::isOverDefined() const{
	return s.over_defined!=0;
}

const bool
SolverStatus::isUnderDefined() const{
	return s.under_defined!=0;
}

const bool
SolverStatus::isStructurallySingular() const{
	return s.struct_singular!=0;
}

const bool
SolverStatus::isReadyToSolve() const{
	return s.ready_to_solve!=0;
}

const bool
SolverStatus::isConverged() const{
	return s.converged!=0;
}
const bool
SolverStatus::isDiverged() const{
	return s.diverged!=0;
}

const bool
SolverStatus::isInconsistent() const{
	return s.inconsistent!=0;
}

const bool
SolverStatus::hasResidualCalculationErrors() const{
	return s.calc_ok!=0;
}

const bool
SolverStatus::hasExceededIterationLimit() const{
	return s.iteration_limit_exceeded!=0;
}
const bool
SolverStatus::hasExceededTimeLimit() const{
	return s.time_limit_exceeded!=0;
}

const bool
SolverStatus::isInterrupted() const{
	return s.panic!=0;
}

const int
SolverStatus::getIterationNum() const{
	return s.iteration;
}

// block stuff....

const int
SolverStatus::getNumBlocks() const{
	return s.block.number_of;
}

const int
SolverStatus::getCurrentBlockNum() const{
	return s.block.current_block;
}
const int
SolverStatus::getCurrentBlockSize() const{
	return s.block.current_size;
}
const int
SolverStatus::getCurrentBlockIteration() const{
	return s.block.iteration;
}

const int
SolverStatus::getNumConverged() const{
	return s.block.previous_total_size;
}
const int
SolverStatus::getNumJacobianEvals() const{
	return s.block.jacs;
}
const int
SolverStatus::getNumResidualEvals() const{
	return s.block.funcs;
}

const double
SolverStatus::getBlockResidualRMS() const{
	return s.block.residual;
}

	
