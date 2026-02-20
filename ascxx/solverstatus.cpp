#include "solverstatus.h"
#include "simulation.h"

#include <stdexcept>

extern "C"{
#include <ascend/solver/solver.h>
}

SolverStatus::SolverStatus(){
	// do nothing else
}

SolverStatus::SolverStatus(const SolverStatus &old) : s(old.s){
	// just that
}

void
SolverStatus::getSimulationStatus(Simulation &sim){
	int res = slv_get_status(sim.getSystem(), &s);
	if(res)throw std::runtime_error("Solver returned error when status requested.");
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
	return s.calc_ok==0;
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

const double
SolverStatus::getCpuElapsed() const{
	return s.cpu_elapsed;
}

const int
SolverStatus::getKind() const{
	return (int)s.kind;
}

const bool
SolverStatus::isNLP() const{
	return s.kind == SLV_STATUS_NLP;
}

const bool
SolverStatus::isLP() const{
	return s.kind == SLV_STATUS_LP;
}

const bool
SolverStatus::isMIP() const{
	return s.kind == SLV_STATUS_MIP;
}

const bool
SolverStatus::hasLpObjective() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return lp->have_objective;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->lp.have_objective;
}

const double
SolverStatus::getLpObjective() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp && lp->have_objective)return lp->objective_value;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->lp.have_objective) ? mip->lp.objective_value : 0.0;
}

const bool
SolverStatus::hasLpPrimalStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return lp->have_primal_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->lp.have_primal_status;
}

const int
SolverStatus::getLpPrimalStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return (int)lp->primal_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip ? (int)mip->lp.primal_status : (int)SLV_SOLUTION_STATUS_UNKNOWN;
}

const bool
SolverStatus::hasLpDualStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return lp->have_dual_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->lp.have_dual_status;
}

const int
SolverStatus::getLpDualStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return (int)lp->dual_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip ? (int)mip->lp.dual_status : (int)SLV_SOLUTION_STATUS_UNKNOWN;
}

const bool
SolverStatus::hasLpBasisStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return lp->have_basis_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->lp.have_basis_status;
}

const int
SolverStatus::getLpBasisStatus() const{
	const slv_status_lp_t *lp = slv_status_lp(&s);
	if(lp != NULL)return (int)lp->basis_status;
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip ? (int)mip->lp.basis_status : (int)SLV_BASIS_STATUS_UNKNOWN;
}

const bool
SolverStatus::hasMipPrimalBound() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_primal_bound;
}

const double
SolverStatus::getMipPrimalBound() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_primal_bound) ? mip->primal_bound : 0.0;
}

const bool
SolverStatus::hasMipDualBound() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_dual_bound;
}

const double
SolverStatus::getMipDualBound() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_dual_bound) ? mip->dual_bound : 0.0;
}

const bool
SolverStatus::hasMipGap() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_gap;
}

const double
SolverStatus::getMipGap() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_gap) ? mip->gap : 0.0;
}

const bool
SolverStatus::hasMipAbsGap() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_abs_gap;
}

const double
SolverStatus::getMipAbsGap() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_abs_gap) ? mip->abs_gap : 0.0;
}

const bool
SolverStatus::hasMipNodeCount() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_node_count;
}

const long long
SolverStatus::getMipNodeCount() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_node_count) ? mip->node_count : -1;
}

const bool
SolverStatus::hasMipTotalLpIterations() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return mip && mip->have_total_lp_iterations;
}

const int
SolverStatus::getMipTotalLpIterations() const{
	const slv_status_mip_t *mip = slv_status_mip(&s);
	return (mip && mip->have_total_lp_iterations) ? mip->total_lp_iterations : -1;
}

// block stuff....

const int
SolverStatus::getNumBlocks() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->number_of : 0;
}

const int
SolverStatus::getCurrentBlockNum() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->current_block : 0;
}
const int
SolverStatus::getCurrentBlockSize() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->current_size : 0;
}
const int
SolverStatus::getCurrentBlockIteration() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->iteration : 0;
}

const int
SolverStatus::getNumConverged() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->previous_total_size : 0;
}
const int
SolverStatus::getNumJacobianEvals() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->jacs : 0;
}
const int
SolverStatus::getNumResidualEvals() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->funcs : 0;
}

const double
SolverStatus::getBlockResidualRMS() const{
	const struct slv__block_status_structure *block = slv_status_block(&s);
	return block ? block->residual : 0.0;
}

	
