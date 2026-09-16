#!/usr/bin/env python3
"""IDA reconstruction of Smith's isothermal adsorption and purge operations.

./a4 script models/psa/psa_dynamic.py --cells 50 --stencil 5 --output psa_dynamic.png
The default starts clean, adsorbs for 69 s, then purges countercurrently for
31 s. It is NOT the thesis's three-equalisation cyclic-steady-state example.
"""
import argparse
import math
from pathlib import Path

R = 8.31446261815324


def build(n=50, stencil=5):
    import ascpy
    if isinstance(n, bool) or int(n) != n or n < 4 or stencil not in (1, 5):
        raise ValueError('At least four integer cells and stencil 1 or 5 are required')
    lib = ascpy.Library()
    try:
        lib.findType('psa_dynamic_bed')
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
    name = f'psa_dynamic_grid_{n}_{stencil}'
    try:
        typ = lib.findType(name)
    except RuntimeError:
        # Only the discretisation is supplied here; physical data stay in ASCEND.
        lib.loadString(f'MODEL {name} WHERE (n >= 4; stencil = 1 OR stencil = 5;) '
                       f'REFINES psa_dynamic_bed(n :== {n}; stencil :== {stencil};); '
                       f'END {name};', name)
        typ = lib.findType(name)
    return typ.getSimulation('dynamic_bed', True)


def _operation_duration(n, operation, initial, duration, samples, rtol, atol):
    """Validate caller inputs before changing a supplied simulation."""
    if operation not in ('adsorption', 'purge'):
        raise ValueError('Operation must be adsorption or purge')
    duration = (69 if operation == 'adsorption' else 31) if duration is None else duration
    if not math.isfinite(duration) or duration <= 0 or samples < 1 or int(samples) != samples:
        raise ValueError('Positive duration and integer sample count required')
    if not 0 < rtol < 1 or not 0 < atol < 1:
        raise ValueError('Positive integration tolerances below one required')
    if initial is not None and (set(initial) != {'y', 'q'}
            or any(len(initial[key]) != n for key in ('y', 'q'))
            or any(not math.isfinite(v) for key in ('y', 'q') for v in initial[key])):
        raise ValueError('Initial profiles must contain n finite y and q values')
    return duration


def _set_operating_inputs(m, changes):
    """Apply dimensional inputs, then check the resulting operating point."""
    units = dict(P='Pa', F='mol/s', k='1/s', T='K', rho='kg/m^3', L='m', d='m')
    for key, value in (changes or {}).items():
        if key not in (*units, 'yin', 'eps') or not math.isfinite(value):
            raise ValueError(f'Invalid operating input {key}')
        if key in units:
            getattr(m, key).setRealValueWithUnits(value, units[key])
        else:
            getattr(m, key).setRealValue(value)
    for key in ('P', 'T', 'rho', 'L', 'd'):
        if getattr(m, key).getRealValue() <= 0:
            raise ValueError(f'{key} must be positive')
    if (m.F.getRealValue() < 0 or m.k.getRealValue() < 0
            or not 0 < m.eps.getRealValue() < 1 or not 0 <= m.yin.getRealValue() <= 1):
        raise ValueError('Invalid flow, rate, void fraction or inlet composition')


def _integrator(sim, duration, samples, rtol, atol):
    import ascpy
    integ = ascpy.Integrator(sim)
    integ.setEngine('IDA')
    params = integ.getParameters()
    for param in params:
        if param.getName() == 'rtol':
            param.setRealValue(rtol)
        elif param.getName() == 'atol':
            param.setRealValue(atol)
        elif param.getName() == 'atolvect':
            param.setBoolValue(False)
        elif param.getName() == 'safeeval':
            param.setBoolValue(True)
    integ.setParameters(params)
    integ.setMaxSubSteps(20000)
    integ.setLinearTimesteps(ascpy.Units('s'), 0, duration, samples)
    return integ


def run_operation(n=50, stencil=5, operation='adsorption', initial=None,
                  duration=None, samples=69, rtol=1e-7, atol=1e-9, changes=None,
                  simulation=None):
    """Integrate a single constant-P, constant-flow operation in flow coordinates.

    Initial y and q arrays are in flow direction. No profiles are clipped;
    linear high-order spatial undershoots are included in diagnostics.
    """
    import ascpy
    duration = _operation_duration(n, operation, initial, duration, samples, rtol, atol)
    sim = simulation if simulation is not None else build(n, stencil)
    if simulation is not None:
        sim.invalidateSystem()
        sim.runDefaultMethod()
    m = sim.getModel()
    if m.n.getIntValue() != n or m.stencil.getIntValue() != stencil:
        raise ValueError('Reused simulation has a different grid or stencil')
    if operation == 'purge':
        sim.run(next(method for method in m.getType().getMethods() if str(method.getName()) == 'purge'))
    _set_operating_inputs(m, changes)
    if initial is not None:
        for i in range(1, n+1):
            m.y0[i].setRealValue(initial['y'][i-1])
            m.q0[i].setRealValueWithUnits(initial['q'][i-1], 'mol/kg')
    sim.checkDimensions()
    sim.build()
    integ = _integrator(sim, duration, samples, rtol, atol)
    rows = []

    class Recorder(ascpy.IntegratorReporterCxx):
        def initOutput(self):
            return 1

        def updateStatus(self):
            return 1

        def closeOutput(self):
            return 1

        def recordObservedValues(self):
            # Never throw Python exceptions through an IDA C callback.
            rows.append(dict(t=m.t.getRealValue(),
                y=[m.y[i].getRealValue() for i in range(1, n+1)],
                q=[m.q[i].getRealValue() for i in range(1, n+1)],
                admitted=m.admitted.getRealValue(), discharged=m.discharged.getRealValue(),
                inventory=m.inventory.getRealValue(), error=m.balance_error.getRealValue()))
            return 1

    rec = Recorder(integ)
    integ.setReporter(rec)
    try:
        integ.analyse()
        integ.solve()
    finally:
        del rec, integ
        sim.invalidateSystem()
    if not rows or abs(rows[-1]['t']-duration) > 1e-7*max(1, duration):
        raise RuntimeError('IDA did not reach the requested end time')
    result = dict(n=n, stencil=stencil, operation=operation, rows=rows,
        **{key: getattr(m, key).getRealValue() for key in
           ('P', 'T', 'F', 'k', 'yin', 'eps', 'rho', 'L', 'd', 'volume', 'carbon_mass')},
        qscale=m.reference.q.getRealValue(), qsat=m.reference.qsat.getRealValue(),
        affinity=m.reference.affinity.getRealValue(), p_ref=m.reference.p_ref.getRealValue(),
        x=[m.x[i].getRealValue() for i in range(1, n+1)])
    validate(result)
    return result


def validate(r, tolerance=1e-5):
    """Independent inventory/boundary-integral audit, never a clipping step."""
    if not r['rows'] or abs(r['rows'][0]['t']) > 1e-10:
        raise ValueError('Operation must include its initial state at time zero')
    C = r['P']/(R*r['T'])
    initial = None
    previous = -1.0
    for row in r['rows']:
        if (not math.isfinite(row['t']) or row['t'] < previous
                or len(row['y']) != r['n'] or len(row['q']) != r['n']
                or any(not math.isfinite(v) for key in ('y', 'q') for v in row[key])
                or any(not math.isfinite(row[key]) for key in ('admitted', 'discharged', 'inventory', 'error'))):
            raise ValueError('Invalid dynamic profile or time sequence')
        inventory = r['volume']/r['n']*sum(r['eps']*C*y+(1-r['eps'])*r['rho']*q
                                         for y, q in zip(row['y'], row['q']))
        if initial is None:
            initial = inventory
        scale = max(1., abs(initial), abs(row['admitted']))
        if (abs(inventory-row['inventory']) > tolerance*scale
                or abs(inventory-initial-row['admitted']+row['discharged']) > tolerance*scale
                or abs(row['error']) > tolerance*scale):
            raise ValueError('Methane inventory/flow balance failed')
        if abs(row['admitted']-r['F']*r['yin']*row['t']) > tolerance*scale:
            raise ValueError('Inlet amount does not match specified boundary flux')
        previous = row['t']
    return dict(min_y=min(min(row['y']) for row in r['rows']),
                max_y=max(max(row['y']) for row in r['rows']),
                min_q=min(min(row['q']) for row in r['rows']),
                max_q=max(max(row['q']) for row in r['rows']),
                negative_solid_moles=max(r['carbon_mass']/r['n']*
                    sum(-min(q, 0) for q in row['q']) for row in r['rows']),
                max_balance_error=max(abs(row['error']) for row in r['rows']))


def adsorption_purge(n=50, stencil=5):
    """Exploratory two-operation run with explicit frozen-solid blowdown map.

    This uses the Chapter 5 gas-averaging convention, not a reconstructed
    three-PE sequence. Gas removed on depressurisation is reported explicitly.
    No purity, recovery or cyclic-steady-state reproduction is claimed.
    """
    ads = run_operation(n, stencil)
    final = ads['rows'][-1]
    average_y = sum(final['y'])/n
    purge = run_operation(n, stencil, operation='purge',
                         initial=dict(y=[average_y]*n, q=list(reversed(final['q']))), samples=62)
    removed = (ads['P']-purge['P'])*ads['eps']*ads['volume']/(8.31446261815324*ads['T'])*average_y
    if abs(final['inventory']-purge['rows'][0]['inventory']-removed) > 1e-5:
        raise RuntimeError('Frozen-solid depressurisation inventory mismatch')
    return dict(adsorption=ads, purge=purge, blowdown_methane=removed)


def plot(result):
    import matplotlib.pyplot as plt
    fig, axes = plt.subplots(2, 2, figsize=(11, 7), layout='constrained')
    ads, purge = result['adsorption'], result['purge']
    for r, label, color in ((ads, 'Adsorption', '#236ba8'), (purge, 'Purge', '#b65a22')):
        final = r['rows'][-1]
        reverse = r['operation'] == 'purge'
        gas = list(reversed(final['y'])) if reverse else final['y']
        solid = list(reversed(final['q'])) if reverse else final['q']
        axes[0, 0].plot(r['x'], [y/ads['yin'] for y in gas], label=label, color=color)
        axes[0, 1].plot(r['x'], [q/ads['qscale'] for q in solid], label=label, color=color)
        axes[1, 0].plot([row['t'] for row in r['rows']],
                        [row['y'][-1] for row in r['rows']], label=label, color=color)
        axes[1, 1].plot([row['t'] for row in r['rows']],
                        [row['inventory'] for row in r['rows']], label=label, color=color)
    axes[0, 0].set_ylabel('Gas methane / feed methane')
    axes[0, 1].set_ylabel('Solid loading / feed-equilibrium loading')
    axes[1, 0].set_ylabel('Trace-model outlet methane mole fraction')
    if max(validate(r)['max_y'] for r in (ads, purge)) > 1:
        axes[1, 0].axhline(1, color='black', linestyle=':', linewidth=1)
        axes[1, 0].set_title('Warning: trace approximation predicts y > 1', fontsize=10)
    axes[1, 1].set_ylabel('Bed methane inventory / mol')
    for ax in axes[0]:
        ax.set_xlabel('Distance from adsorption inlet / m')
    for ax in axes[1]:
        ax.set_xlabel('Time since operation start / s')
    for ax in axes.flat:
        ax.grid(alpha=.25)
        ax.legend()
    fig.suptitle(f"IDA: clean-bed adsorption then countercurrent purge; {ads['n']} cells, "
                 f"stencil {ads['stencil']}\nNot cyclic steady state; no pressure equalisation")
    return fig


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cells', type=int, default=50)
    parser.add_argument('--stencil', '--order', type=int, choices=(1, 5), default=5,
                        help='Spatial stencil (1: upwind; 5: five-point interior). --order is a legacy alias.')
    parser.add_argument('--output', type=Path, default=Path('psa_dynamic.png'))
    args = parser.parse_args(argv)
    result = adsorption_purge(args.cells, args.stencil)
    for name in ('adsorption', 'purge'):
        r = result[name]
        audit = validate(r)
        print(f"{name}: inventory {r['rows'][-1]['inventory']:.6g} mol; "
              f"outlet yCH4 {r['rows'][-1]['y'][-1]:.6g}; audit {audit}")
    print(f"Frozen-solid blowdown removes {result['blowdown_methane']:.6g} mol CH4")
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig = plot(result)
    try:
        fig.savefig(args.output, dpi=160)
    finally:
        plt.close(fig)
    print(f'Plot: {args.output.resolve()}')


if __name__ == '__main__':
    main()
