#!/usr/bin/env python3
"""Spatial/temporal convergence checks for the ASCEND PSA operation model.

An exactly translating, nonadsorbing smooth pulse separates spatial error
from IDA time-integration error. Real adsorption/purge refinements then
measure profile differences, inventories, outlet integrals and undershoots.
No assertion that the boundary closures deliver global fourth-order accuracy.
"""
import argparse
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

import psa_dynamic as dynamic


def gaussian_cells(n, length=1.982, shift=0):
    """Analytical cell averages, with zero inflow before x=0."""
    centre, width, amplitude = .35*length, .07*length, .02
    dx = length/n
    def primitive(x):
        x = max(0, min(length, x))
        return amplitude*width*math.sqrt(math.pi)/2*math.erf((x-centre)/width)
    return [(primitive((i+1)*dx-shift)-primitive(i*dx-shift))/dx for i in range(n)]


def advection(n, stencil, rtol=1e-9, atol=1e-11):
    initial = dict(y=gaussian_cells(n), q=[0.0]*n)
    r = dynamic.run_operation(n, stencil, initial=initial, changes=dict(k=0, yin=0),
                              duration=1, samples=2, rtol=rtol, atol=atol)
    area = math.pi*r['d']**2/4
    velocity = r['F']/(area*r['eps']*r['P']/(dynamic.R*r['T']))
    exact = gaussian_cells(n, r['L'], shift=velocity)
    errors = [abs(a-b) for a, b in zip(r['rows'][-1]['y'], exact)]
    return dict(n=n, stencil=stencil, mean_absolute_error=sum(errors)/n,
                max_error=max(errors), audit=dynamic.validate(r))


def coarsen(values, n):
    if len(values) % n:
        raise ValueError('Nested cell-average grids required')
    ratio = len(values)//n
    return [sum(values[i*ratio:(i+1)*ratio])/ratio for i in range(n)]


def compare(coarse, fine):
    result = {}
    for operation in ('adsorption', 'purge'):
        a, b = coarse[operation], fine[operation]
        pa, pb = a['rows'][-1], b['rows'][-1]
        result[operation] = dict(
            mean_y_difference=sum(abs(x-y) for x, y in zip(pa['y'], coarsen(pb['y'], a['n'])))/a['n'],
            mean_q_difference=sum(abs(x-y) for x, y in zip(pa['q'], coarsen(pb['q'], a['n'])))/a['n'],
            inventory_difference=abs(pa['inventory']-pb['inventory']),
            outlet_moles_difference=abs(pa['discharged']-pb['discharged']))
    return result


def evaluate(task, n, stencil):
    if task == 'advection': return advection(n, stencil)
    if task == 'tight-advection': return advection(n, stencil, rtol=1e-11, atol=1e-13)
    if task == 'operations': return dynamic.adsorption_purge(n, stencil)
    if task == 'css':
        import psa_dynamic_cycle as cycle
        return cycle.solve(n, stencil)
    raise ValueError('Unknown convergence task')


def isolated(task, n, stencil):
    # ASCEND's current Python wrapper does not own/delete simulation trees.
    # Bound a large sweep's memory without clearing a caller's shared Library.
    # Each case runs with the same interpreter and inherited ./a4 environment.
    print(f'Checking {task}: {n} cells, stencil {stencil}', flush=True)
    with tempfile.TemporaryDirectory(prefix='psa-grid-') as directory:
        output = Path(directory)/'result.json'
        command = [sys.executable, str(Path(__file__).resolve()), '--worker', task,
                   '--cells', str(n), '--stencil', str(stencil), '--output', str(output)]
        completed = subprocess.run(command, capture_output=True, text=True)
        if completed.returncode:
            raise RuntimeError(f'{task}, n={n}, stencil={stencil} failed:\n{completed.stderr[-4000:]}')
        return json.loads(output.read_text())


def grid_check(grids=(25, 50, 100), stencils=(1, 5), include_cycles=False):
    if len(grids) < 2 or any(b <= a or b % a for a, b in zip(grids, grids[1:])):
        raise ValueError('Use at least two increasing nested grids')
    report = dict(grids=list(grids), stencils={})
    for stencil in stencils:
        pulse = [isolated('advection', n, stencil) for n in grids]
        for a, b in zip(pulse, pulse[1:]):
            b['observed_order'] = math.log(a['mean_absolute_error']/b['mean_absolute_error'])/math.log(b['n']/a['n'])
        operations = [isolated('operations', n, stencil) for n in grids]
        entry = dict(advection=pulse,
                     operation_audits=[dict(n=n, **{op: dynamic.validate(r[op]) for op in ('adsorption', 'purge')})
                                       for n, r in zip(grids, operations)],
                     operation_differences=[compare(a, b) for a, b in zip(operations, operations[1:])])
        # Tighten time tolerances at fixed spatial resolution; this does not
        # change the spatial stencil or claim to cure its oscillations.
        tight = isolated('tight-advection', grids[-1], stencil)
        entry['time_refinement'] = dict(normal_error=pulse[-1]['mean_absolute_error'],
                                        tight_error=tight['mean_absolute_error'])
        if include_cycles:
            cycles = [isolated('css', n, stencil) for n in grids]
            entry['css'] = [dict(n=r['n'], metrics=r['metrics'], convergence=r['history'][-1],
                                 audits={op: dynamic.validate(r[op]) for op in ('adsorption', 'purge')})
                            for r in cycles]
            entry['css_differences'] = [compare(a, b) for a, b in zip(cycles, cycles[1:])]
        report['stencils'][str(stencil)] = entry
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--grids', nargs='+', type=int, default=[25, 50, 100])
    parser.add_argument('--cycles', action='store_true')
    parser.add_argument('--output', type=Path, default=Path('psa_dynamic_convergence.json'))
    parser.add_argument('--worker', choices=('advection', 'tight-advection', 'operations', 'css'), help=argparse.SUPPRESS)
    parser.add_argument('--cells', type=int, default=50, help=argparse.SUPPRESS)
    parser.add_argument('--stencil', type=int, choices=(1, 5), default=1, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.worker:
        result = evaluate(args.worker, args.cells, args.stencil)
        args.output.write_text(json.dumps(result, allow_nan=False)+'\n')
        return
    report = grid_check(args.grids, include_cycles=args.cycles)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False)+'\n')
    for stencil, result in report['stencils'].items():
        errors = ', '.join('{n}: {mean_absolute_error:.3g}'.format(**r) for r in result['advection'])
        print(f'Stencil {stencil}: pulse errors {errors}')
    print(args.output.resolve())


if __name__ == '__main__':
    main()
