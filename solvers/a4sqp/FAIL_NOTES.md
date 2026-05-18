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

## DISC2

- CUTEst class: `LQR2-MY-29-23`, linear objective with 17 quadratic equalities and 6 quadratic inequalities.
- IPOPT and NLopt SLSQP solve the CUTEst instance to objective `1.5625` with constraint violation below `1.5e-9`.
- Added `models/test/a4sqp/disc2.a4c` as a direct ASCEND translation. It reproduces the A4SQP failure shape: without a per-QP guard the run can sit inside HiGHS QP; with `qp_time_limit=2` it advances and exposes stationarity/globalization issues.
- GDB interruption of the ASCEND run showed the stall in `Highs::run -> solveqp -> Quass::solve`, called from `a4sqp_qp_solve_highs_options`, not in ASCEND relation evaluation.
- With `qp_time_limit=2`, the ASCEND translation reaches near the IPOPT/SLSQP objective in the exact-Hessian path (`eps` around `1.5736`) but then stalls with `alpha=2.44141e-4`, tiny steps, max violation around `1e-7`, and dual/KKT residual around `0.917`. At that point all six quadratic inequality rows are classified inactive, so the immediate issue is equality-constraint stationarity/multiplier handling rather than bound handling.
- IPOPT's accepted point has the first five boundary equalities active by construction and also has interior-disc inequalities 10 and 11 active. The A4SQP exact-Hessian stall reports `rel_active=0` for the six inequalities, so it is close to feasible but on the wrong active set; this is a plausible explanation for the high stationarity residual.
- The BFGS path progresses more steadily but was still short of the solution in focused local runs (`eps` around `1.44` to `1.54`, with nonzero constraint violation depending on the iteration limit). A SOC plus active-bound-release CUTEst profile can get very close to the accepted objective/feasibility, but still fails the KKT test.
- The current best A4SQP profile for the CUTEst SIF instance is not active-bound release; it is `second_order_correction=TRUE` plus `reduced_gradient_polish_mode=FALLBACK` with exact-Lagrangian Hessian. In a focused run this reached objective `1.5625000000762186`, max violation `2.4e-12`, KKT `3.3e-6`, and `acceptable_success`. The final active set matches IPOPT/SLSQP: boundary rows 1-5 active and interior rows 10-11 active.
- Interpretation: DISC2 exposes a globalization gap rather than a derivative/sign error. Plain SQP reaches feasibility on the wrong active set; SOC improves feasibility near the optimum; reduced-gradient polish then makes objective progress along the feasible manifold until the missing interior inequalities become active.
- The DISC2 model includes explicit probe methods (`a4sqp`, `a4sqp_probe`, `a4sqp_bfgs`, `a4sqp_obj`, `a4sqp_soc`) so the exact-Hessian stall and BFGS/SOC behavior can be reproduced with `./a4 run models/test/a4sqp/disc2.a4c --run METHOD --progress --print eps`.
- This case currently points to two separate issues: A4SQP should not allow an individual QP solve to monopolize the entire NLP run, and the SQP globalization/exact-Hessian/multiplier-recovery path is not recovering clean stationarity near the optimum.
