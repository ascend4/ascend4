#!/usr/bin/env python3
"""Compare full IDA solve costs on small linear ODEs; see AUTO-SELECTION.md.

Run via ./a4 script solvers/ida/benchmarks/small_dae.py -- --help.
"""
import argparse
import json
import math
import os
from pathlib import Path
import statistics
import sys
import tempfile
import time


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--n', type=int, required=True)
    ap.add_argument('--pattern', choices=['band', 'dense'], required=True)
    ap.add_argument('--solver', choices=['DENSE', 'KLU'], required=True)
    ap.add_argument('--repeats', type=int, default=10)
    ap.add_argument('--warmups', type=int, default=2)
    args = ap.parse_args()
    if args.n < 1 or args.repeats < 1 or args.warmups < 0:
        ap.error('n and repeats must be positive; warmups must be nonnegative')

    sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_NOW)
    import ascpy

    eqs = []
    for i in range(1, args.n + 1):
        neighbours = [j for j in range(1, args.n + 1)
                      if j != i and (args.pattern == 'dense' or abs(i-j) == 1)]
        rhs = ''.join(f' - 0.1*y[{j}]' for j in neighbours)
        diag = 1 + 0.1 * len(neighbours)
        eqs.append(f'e{i}: der(y[{i}]) + {diag:.17g}*y[{i}]{rhs} = 0;')
    equations = '\n'.join(eqs)
    source = f'''REQUIRE "ivpsystem.a4l";
MODEL crossover;
t IS_A solver_var;
INDEPENDENT t;
y[1..{args.n}] IS_A solver_var;
{equations}
METHODS
METHOD reset_values;
t := 0;
y[1..{args.n}] := 1;
END reset_values;
METHOD on_load;
FIX t;
RUN reset_values;
END on_load;
END crossover;
'''
    lib = ascpy.Library()
    with tempfile.TemporaryDirectory(prefix='ida-auto-dae-') as tmp:
        model = Path(tmp) / 'model.a4c'
        model.write_text(source)
        lib.load(str(model))
    typ = lib.findType('crossover')
    sim = typ.getSimulation('sim', True)
    sim.setSolver(ascpy.Solver('QRSlv'))
    integ = ascpy.Integrator(sim)
    integ.setEngine('IDA')
    integ.analyse()
    params = integ.getParameters()
    for param in params:
        name = param.getName()
        if name == 'linsolver': param.setStrValue(args.solver)
        elif name == 'atolvect': param.setBoolValue(False)
        elif name in ('atol', 'rtol'): param.setRealValue(1e-8)
    integ.setParameters(params)
    integ.setMaxSubSteps(10000)
    integ.setLinearTimesteps(ascpy.Units('s'), 0, 1, 10)
    reporter = ascpy.IntegratorReporterNull(integ)
    integ.setReporter(reporter)
    times, errors = [], []
    for _ in range(args.warmups + args.repeats):
        sim.run(typ.getMethod('reset_values'))
        start = time.perf_counter()
        integ.solve()
        times.append(time.perf_counter() - start)
        values = [v.getValue() for v in sim.getallVariables()
                  if v.getName().startswith('y[')]
        if len(values) != args.n or not all(math.isfinite(v) for v in values):
            raise RuntimeError('Missing or non-finite state values')
        errors.append(max(abs(v - math.exp(-1)) for v in values))
    if max(errors) > 1e-6:
        raise RuntimeError(f'Unexpected endpoint error: {max(errors)}')
    measured = times[args.warmups:]
    print(json.dumps(dict(n=args.n, pattern=args.pattern, solver=args.solver,
                          repeats=args.repeats, warmups=args.warmups,
                          median_s=statistics.median(measured),
                          min_s=min(measured), max_s=max(measured),
                          max_error=max(errors))))


if __name__ == '__main__':
    main()
