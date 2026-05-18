# SLSQP CUTEst Driver

This directory contains a CUTEst package driver for NLopt's
`NLOPT_LD_SLSQP`. It is intentionally separate from the ASCEND adapter in
`solvers/slsqp`: both adapters link NLopt directly and translate their source
problem representation into dense SLSQP callbacks.

Install the package hooks into a CUTEst tree:

```sh
solvers/slsqp/cutest/install_cutest_package.sh
```

Run one problem:

```sh
export CUTEST=/home/john/CUTEst
export ASCEND_ROOT=/home/john/ascend
export LD_LIBRARY_PATH=/home/john/ascend:/home/john/ascend/solvers/slsqp:${LD_LIBRARY_PATH:-}
runcutest -p slsqp -D HS21
```

The driver emits one JSON result row with problem name, dimensions,
classification, NLopt result, objective, max violation, callback counts, and
CUTEst timing/call counters.

Current limitations:

- No dedicated parallel runner yet. Use the A4SQP runner design as the template:
  isolated CUTEst worker roots are required for concurrent `runcutest` calls.
- No multipliers or SLSQP internal iteration diagnostics are available from
  NLopt.
- Constraint Jacobians are densified inside the callback, as expected for dense
  SLSQP.
