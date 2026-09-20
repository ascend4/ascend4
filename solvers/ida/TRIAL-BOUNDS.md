# Recoverable IDA trial-bound diagnostics

The U4 packing study on ASCEND `885290c7` completed all 63 cases and passed
all grid/tolerance comparisons. Five logs nevertheless contained 38
`ERROR:` messages about the lower bound of `step2_product_fraction_pos`
during startup. The common U3 replay on `afbe3d1c` also reproduces one such
message in standard X31.

IDA's residual and direct-Jacobian callbacks check bounds on their tentative
input vectors before evaluating equations. An out-of-bounds value returns
positive status 1 to IDA, allowing it to reject the evaluation and retry.
The shared `slv_check_bounds` routine reports these temporary violations as
errors, which makes a successful recovery indistinguishable from a model
error to log-based qualification tools.

`slv_check_bounds_recoverable` keeps the same comparisons, bitmask and
non-mutating behaviour. It reports out-of-range trial values as notes,
including the offending value and bound. Inconsistent bounds and invalid
range arguments retain error severity. Existing users of `slv_check_bounds`
retain their original diagnostics. IDA's residual and dense-Jacobian
callbacks use the recoverable variant; terminal IDA failures remain errors.
The fboard2 sparse-Jacobian callback needs the same one-line substitution.

This does not relax variable bounds, clip trial values, change tolerances,
accept a failed integration, or establish that an accepted trajectory is
physical. Qualification still requires completed horizons, balance/domain
checks, grid/tolerance agreement and no unexpected errors. Rejected trial
notes are retained as separate diagnostics.

The portable fix is prepared on `ida-trial-bound-reporting`, based on
`origin/python3` at `6daae4f5`. The same changes plus the sparse callback are
applied to the fboard2 working build for TGA verification. Neither base
branch has been advanced by a commit for this change.

Regression: `integrator_ida.trial_bound_reporting`, using
`models/test/ida/trial_bounds.a4c`, checks lower/upper trial rejection,
unchanged values, informative note severity, legacy error severity and
inconsistent-bound errors. Full model replays compare the unchanged
physical trajectories before and after the diagnostic change.

## Verification on the integrated fboard2 build

GDB confirms that the common U3 X31 message comes from
`integrator_ida_fex`, called by IDA's Newton iteration at
`t=0.06402687245171626 s`. The corrected diagnostic prints a trial value of
`-1.8982043032143403e-07` against lower bound zero. The callback still rejects
that trial before evaluating the equations.

The focused IDA suite passes 91 tests and 3393 assertions, including
`trial_bound_terminal_failure`: the bounded trajectory `dx/dt=-1`, starting
at `x=1`, cannot continue beyond zero and still terminates with IDA error -9
and nonzero integration status. Lower/upper trial rejection and inconsistent
bounds are covered separately. The test build command on fboard2 is
`scons -j1 test-ida`, followed by `solvers/ida/test_ida` with the local shared
library directory on `LD_LIBRARY_PATH`.

Both full common U3 replays pass all numerical checks. Before the patch,
X31's message prevents the clean-log gate; after the patch, clean
qualification passes. The patched panel takes 835.97 s on six workers,
versus 834.11 s before. All 26 saved reports are byte-for-byte identical.
The report retains one recoverable trial note for X31; no unexpected errors
remain. This is a diagnostics correction, not a tolerance or kinetics change.

The first one-cell 25-point fitting profile also exercises real failures:
seven parameter points fail integration and remain excluded, while 18 points
are scored. Thus the qualification/fitting workflow continues to reject
terminal solver errors. No commits have been made for this diagnostic fix.
