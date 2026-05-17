# A4SQP Failure Notes

This file records focused notes on A4SQP failures and underperforming CUTEst
or ASCEND-translated models. Keep entries short and evidence-based so we can
avoid rediscovering the same failure mode.

## CHWIRUT2LS

- CUTEst class: `SUR2-MN-3-0`, unconstrained sum-of-squares problem with 3 variables and 54 residual groups.
- IPOPT accepts objective `513.048029406865...` after 6 iterations. Its detailed log reports unscaled dual infeasibility `1.20e-9`, zero constraint violation, and "Optimal Solution Found".
- Fixed in May 2026 by tightening the least-squares core stall test. A4SQP now accepts the dedicated LM least-squares path with objective `513.0480294068655`, raw CUTEst objective-gradient infinity norm `6.74e-9`, and no SQP handoff.
- Root cause: the LS micro-solver treated `step_norm <= step_tol` as a stall even when `grad_inf > grad_tol`. On this ill-conditioned LS fit, the rejected final micro-steps were only `O(1e-9)` to `O(1e-12)` in variable space but reduced the raw gradient from `1.78e-4` to `6.74e-9`.
- Final-point diagnostic confirmed this was not a CUTEst callback mismatch. Before the fix, A4SQP and IPOPT final points differed by only `2.3e-9` in the largest component, with objective values agreeing to roundoff, but that tiny displacement was enough to leave A4SQP with a `1.78e-4` raw gradient.
- NLopt SLSQP is not a useful success reference for this case: it reports `FTOL_REACHED` at objective `14794.790154797307`, far worse than IPOPT/A4SQP, with projected gradient infinity norm `1.80e6`. The SLSQP CUTEst adapter now classifies this as `success_high_gradient` rather than a strict pass.
