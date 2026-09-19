#!/usr/bin/env python3
"""Run via ./a4 script solvers/ida/benchmarks/tga.py -- --help.

Uses fboard2 model sources with the current runtime. The fixture is an archival
X49 parameter set, not the later U3/U4 case in SPARSE-JACOBIAN.md.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import time


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--models', type=Path, required=True, help='fboard2 models directory')
    ap.add_argument('--cells', type=int, required=True)
    ap.add_argument('--radial', type=int, default=3)
    ap.add_argument('--solver', choices=['DENSE', 'KLU'], required=True)
    ap.add_argument('--end', type=float, default=3)
    ap.add_argument('--samples', type=int, default=30)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if args.cells < 1 or args.radial < 1 or args.end <= 0 or args.samples < 1:
        ap.error('cells, radial, end and samples must be positive')
    os.environ['ASCENDLIBRARY'] = str(args.models.resolve()) + os.pathsep + os.environ.get('ASCENDLIBRARY', '')
    sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_NOW)
    import ascpy

    source = Path(__file__).with_name('x49_legacy.a4c.in').read_text()
    source = source.replace('@CELLS@', str(args.cells)).replace('@RADIAL@', str(args.radial))
    start = time.perf_counter()
    lib = ascpy.Library()
    with tempfile.TemporaryDirectory(prefix='ida-tga-') as tmp:
        model = Path(tmp) / 'case.a4c'
        model.write_text(source)
        lib.load(str(model))
    typ = lib.findType('batch_case_X49')
    sim = typ.getSimulation('sim', True)
    sim.run(typ.getMethod('setup_X49'))
    sim.setSolver(ascpy.Solver('QRSlv'))
    integ = ascpy.Integrator(sim)
    integ.setEngine('IDA')
    integ.analyse()
    params = integ.getParameters()
    for param in params:
        name = param.getName()
        if name == 'linsolver': param.setStrValue(args.solver)
        elif name == 'rtol': param.setRealValue(1e-6)
        elif name == 'atolvect': param.setBoolValue(False)
        elif name == 'atol': param.setRealValue(1e-8)
        elif name == 'stats': param.setBoolValue(True)
    integ.setParameters(params)
    integ.setMaxSubSteps(100000)
    integ.setLinearTimesteps(ascpy.Units('s'), 0, args.end, args.samples)
    trajectory = []

    class Reporter(ascpy.IntegratorReporterCxx):
        def initOutput(self): return 1
        def closeOutput(self): return 1
        def updateStatus(self): return 1
        def recordObservedValues(self):
            trajectory.append([integ.getCurrentTime(), sim.getModel().core.reduction_degree.getRealValue()])
            return 1

    reporter = Reporter(integ)
    integ.setReporter(reporter)
    ready = time.perf_counter()
    integ.solve()
    finished = time.perf_counter()
    repo = Path(__file__).resolve().parents[3]
    runtime = repo / 'solvers/ida/libida_ascend.so'
    result = dict(cells=args.cells, radial=args.radial, solver=args.solver,
                  n=integ.getNumVars(), end=args.end, samples=args.samples,
                  rtol=1e-6, atol=1e-8, prepare_s=ready-start, solve_s=finished-ready,
                  model_sha256=hashlib.sha256(source.encode()).hexdigest(),
                  runtime_sha256=hashlib.sha256(runtime.read_bytes()).hexdigest(),
                  trajectory=trajectory,
                  final={v.getName(): v.getValue() for v in sim.getallVariables()})
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ('trajectory', 'final')}))


if __name__ == '__main__':
    main()
