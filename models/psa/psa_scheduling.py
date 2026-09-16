#!/usr/bin/env python3
"""Solve Smith's cyclic PSA scheduling MIP and draw a periodic bed Gantt chart.

./a4 script models/psa/psa_scheduling.py --case pe3 --solver HiGHS --output psa.png
Operation data and constraints live in ASCEND; Python checks and presents results.
"""
import argparse
import math
from pathlib import Path
import sys

CASES = {name: 'psa_scheduling_' + name for name in
         ('pe0', 'pe1', 'pe2', 'oxy', 'oxy_continuous', 'seven', 'seven_purge')}
CASES['pe3'] = 'psa_scheduling'
TITLES = dict(oxy='Oxy-Rich', oxy_continuous='Oxy-Rich: continuous product and feed',
              seven='Seven-operation cycle', seven_purge='Seven-operation cycle: purge constraint',
              **{f'pe{n}': f'Hydrogen PSA: {n} pressure equalisation(s)' for n in range(4)})


def prepare(case='pe3', solver='HiGHS', epsilon=.01, max_beds=8, shift=None):
    """Return a fresh simulation; no source optimum is supplied to the MIP."""
    import ascpy
    if case not in CASES or solver not in ('HiGHS', 'Gurobi'):
        raise ValueError('Unknown case or solver')
    if not math.isfinite(epsilon) or not 0 < epsilon < .5:
        raise ValueError('epsilon must lie strictly between zero and 0.5')
    if (isinstance(max_beds, bool) or not math.isfinite(max_beds)
            or int(max_beds) != max_beds or max_beds < 1):
        raise ValueError('max_beds must be a positive integer')
    if shift is not None and (not math.isfinite(shift) or shift <= 0):
        raise ValueError('Reporting shift must be positive seconds')
    library = ascpy.Library()
    try:
        typ = library.findType(CASES[case])
    except RuntimeError:
        library.load(str(Path(__file__).with_suffix('.a4c')))
        typ = library.findType(CASES[case])
    sim = typ.getSimulation('cyclic_schedule', True)
    s = sim.getModel().schedule
    s.epsilon.setRealValue(epsilon)
    for variable in [s.N, s.Jfeed] + [s.J[l] for l in s.data.pair.getSetValue()]:
        variable.upper_bound.setRealValue(max_beds)
    if shift is not None:
        s.D.setRealValueWithUnits(shift, 's')
    sim.setSolver(ascpy.Solver(solver))
    params = sim.getParameters()
    for param in params:
        if param.getName() == 'threads':
            param.setIntValue(1)
        elif param.getName() in ('mip_rel_gap', 'mip_abs_gap'):
            param.setRealValue(0.0)
    sim.setParameters(params)
    sim.checkDimensions()
    return sim


def solve(case='pe3', solver='HiGHS', balance=True, **options):
    """Minimise beds, then optionally maximise the shortest processing step.

    The second objective is ONLY a transparent presentation tie-break. N is
    fixed at its proven optimum first; no economic or kinetic claim is made.
    """
    import ascpy
    sim = prepare(case, solver, **options)
    sim.solve(ascpy.Solver(solver), ascpy.SolverReporter())
    status = sim.getStatus()
    if not status.isConverged():
        raise RuntimeError(f'{solver}: minimum-bed solve did not converge')
    result = read_schedule(sim.getModel())
    optimum = result['N']
    bound = status.getMipDualBound() if status.hasMipDualBound() else None
    if bound is None or not math.isfinite(bound) or abs(bound - optimum) > 1e-6:
        raise RuntimeError('Minimum bed count has not been proved')
    if balance:
        s = sim.getModel().schedule
        s.N.setRealValue(optimum)
        s.N.setFixed(True)
        s.presentation_weight.setRealValue(1)
        sim.solve(ascpy.Solver(solver), ascpy.SolverReporter())
        if not sim.getStatus().isConverged():
            raise RuntimeError(f'{solver}: timing tie-break did not converge')
        result = read_schedule(sim.getModel())
        if result['N'] != optimum:
            raise RuntimeError('Timing tie-break changed the bed count')
    result.update(case=case, solver=solver, balanced=balance, minimum_proven=True,
                  primary_bound=bound)
    return result


def read_schedule(model):
    """Read normalised and dimensional times, retaining raw integers for checks."""
    d, s = model.data, model.schedule
    real = lambda x: x.getRealValue()
    result = dict(N=real(s.N), D=real(s.D), period=real(s.period),
                  epsilon=real(s.epsilon), Jfeed=real(s.Jfeed),
                  continuous=list(d.continuous.getSetValue()),
                  short=list(d.short.getSetValue()), compressor=d.compressor.getBoolValue(),
                  feed_op=d.feed_op.getIntValue(),
                  ordering=[(d.longer[l].getIntValue(), d.shorter[l].getIntValue())
                            for l in d.ordering.getSetValue()],
                  pairs=[dict(donor=d.donor[l].getIntValue(),
                              receiver=d.receiver[l].getIntValue(), J=real(s.J[l]))
                         for l in sorted(d.pair.getSetValue())],
                  operations={k: dict(label=str(d.label[k].getSymbolValue()),
                                      **{name: real(getattr(s.slot[k], name)) for name in
                                         ('a', 'b', 'p', 's', 'tau', 'start', 'finish',
                                          'processing', 'standby')})
                              for k in sorted(d.op.getSetValue())})
    validate(result)
    result['N'] = round(result['N'])
    return result


def segments(start, duration, period):
    """Clip a periodic interval to [0, period], including wrap-around pieces."""
    a = start % period
    b = a + duration
    if min(b, period) - a > 1e-9:
        yield a, min(b, period)
    if b > period + 1e-9:
        yield 0.0, b - period


def validate(r, tol=1e-6):
    """Check scalar constraints AND expanded periodic events on every bed.

    Midpoints between all event boundaries exhaust the intervals on which
    concurrency is constant: this is not approximate time-grid sampling.
    Units for event checks are D; dimensional outputs are checked separately.
    """
    def require(test, message):
        if not test:
            raise ValueError(message)

    N, D, eps = (r[k] for k in ('N', 'D', 'epsilon'))
    require(all(math.isfinite(r[k]) for k in ('N', 'D', 'epsilon', 'period', 'Jfeed')),
            'Non-finite scheduling result')
    require(N >= 1 and abs(N-round(N)) < tol, 'Non-integer bed count')
    N = round(N)
    require(D > 0 and 0 < eps < .5, 'Invalid time scale or epsilon')
    require(abs(r['period']/D-N) < tol, 'Incorrect physical period')
    ops = r['operations']
    require(list(ops) == list(range(1, len(ops)+1)) and len(ops) >= 2,
            'Route must contain consecutive operation indices')
    end = 0.0
    for k, op in ops.items():
        require(all(math.isfinite(v) for name, v in op.items() if name != 'label'),
                'Non-finite operation time')
        a, b, p, idle = (op[key] for key in ('a', 'b', 'p', 's'))
        require(abs(a-end) < tol and abs(b-a-p-idle) < tol, f'Broken route at operation {k}')
        require(abs(op['tau']-p-idle) < tol, 'Incorrect allocated duration')
        require(p >= eps-tol and idle >= -tol, 'Nonpositive processing or negative standby')
        for physical, normal in (('start', 'a'), ('finish', 'b'),
                                 ('processing', 'p'), ('standby', 's')):
            require(abs(op[physical]/D-op[normal]) < tol, 'Incorrect dimensional time')
        end = b
    require(abs(end-N) < tol, 'Route does not close after N shifts')
    for k in r['short']:
        require(ops[k]['p'] <= 1-eps+tol, 'Short operation exceeds one shift')
    for longer, shorter in r['ordering']:
        require(ops[longer]['p'] >= ops[shorter]['p']-tol, 'Duration ordering violated')
    for pair in r['pairs']:
        J = pair['J']
        require(math.isfinite(J) and 1 <= J+tol and J <= N-1+tol
                and abs(J-round(J)) < tol, 'Invalid integer bed offset')
        donor, receiver = (ops[pair[key]] for key in ('donor', 'receiver'))
        require(pair['donor'] < pair['receiver'], 'Pairs must point forward along the route')
        require(abs(receiver['a']-donor['a']-J) < tol, 'Incorrect pair separation')
        for bed in range(N):
            partner = (bed-round(J)) % N
            difference = donor['a']+bed-receiver['a']-partner
            require(partner != bed and abs(difference/N-round(difference/N)) < tol
                    and abs(donor['p']-receiver['p']) < tol, 'Unmatched bed transfer event')

    # Expand actual processing and standby, not merely the allocated slots.
    events = []
    for bed in range(N):
        for k, op in ops.items():
            for idle, offset, duration in ((False, 0, op['p']), (True, op['p'], op['s'])):
                for a, b in segments(op['a']+bed+offset, max(0, duration), N):
                    events.append((a, b, bed, k, idle))
    boundaries = sorted({0.0, float(N)} | {x for a, b, *_ in events for x in (a, b)})
    jf = r['Jfeed']
    require(abs(jf-round(jf)) < tol, 'Non-integer feed concurrency')
    require(jf >= 1-tol if r['compressor'] else abs(jf) < tol, 'Invalid feed concurrency')
    for a, b in zip(boundaries, boundaries[1:]):
        if b-a <= tol:
            continue
        t = (a+b)/2
        active = [e for e in events if e[0] <= t < e[1]]
        for bed in range(N):
            require(sum(e[2] == bed for e in active) == 1, 'Bed overlap or unallocated time')
        for k in r['continuous']:
            require(any(e[3] == k and not e[4] for e in active), 'Interruption of continuous operation')
        if r['compressor']:
            require(sum(e[3] == r['feed_op'] and not e[4] for e in active) == round(jf),
                    'Feed compressor concurrency changes within the period')
    return events


def plot_schedule(r):
    """Return a figure without selecting a backend, saving or showing it."""
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch
    events = validate(r)
    ops, D, N = r['operations'], r['D']/60, r['N']
    colors = {k: '#91c9a0' if 'Adsorption' in op['label'] else
              '#eab881' if 'purge' in op['label'].lower() or 'Blowdown' in op['label']
              else '#d9ce8c' for k, op in ops.items()}
    for pair, color in zip(r['pairs'], ('#80b8db', '#b9a1d5', '#df9fab')):
        colors[pair['donor']] = colors[pair['receiver']] = color
    fig, ax = plt.subplots(figsize=(12, 3.0 + .55*N), layout='constrained')
    for a, b, bed, k, idle in events:
        ax.barh(bed, (b-a)*D, left=a*D, height=.65,
                color='#eeeeee' if idle else colors[k], edgecolor='#555555',
                linewidth=.7, hatch='///' if idle else None)
        if not idle and b-a > .13:
            ax.text((a+b)*D/2, bed, str(k), ha='center', va='center', fontsize=10)
    ax.set_yticks(range(N), [f'Bed {b+1}' for b in range(N)])
    ax.set_ylim(N-.35, -.7)
    ax.set_xlim(0, N*D)
    ax.set_xticks([i*D for i in range(N+1)])
    ax.set_xlabel('Elapsed time / min (one repeating cycle; numbers are route operations)')
    ax.grid(axis='x', linestyle=':', alpha=.5)
    ax.set_axisbelow(True)
    ax.spines[['top', 'right']].set_visible(False)
    integers = ', '.join(str(round(p['J'])) for p in r['pairs']) or 'none'
    heading = TITLES.get(r.get('case'), 'PSA cyclic schedule')
    proof = 'Minimum' if r.get('minimum_proven') else 'Solved'
    ax.set_title(f'{heading} — {proof.lower()} beds: {N}; pair offsets J: {integers}\n'
                 f'D = {D:g} min (chosen scale), period = {N*D:g} min'
                 + ('; max-min timing tie-break' if r.get('balanced') else ''), loc='left')
    pair_labels = {k: f' (pair {l})' for l, pair in enumerate(r['pairs'], 1)
                   for k in (pair['donor'], pair['receiver'])}
    handles = [Patch(facecolor=colors[k], edgecolor='#555555',
                     label=f"{k}: {op['label'].replace('_', ' ')}{pair_labels.get(k, '')}")
               for k, op in ops.items()]
    handles.append(Patch(facecolor='#eeeeee', hatch='///', edgecolor='#555555', label='Standby'))
    ax.legend(handles=handles, loc='upper center', bbox_to_anchor=(.5, -.16),
              ncol=3, frameon=False, fontsize=9)
    return fig


def psa_scheduling_gantt(self):
    """Plot the current solved schedule from the GUI (run Solve first)."""
    import extpy
    browser = extpy.getbrowser()
    if browser is None or browser.sim.isSolveDirty() or not browser.sim.getStatus().isConverged():
        raise RuntimeError('Solve the cyclic scheduling model before running gantt')
    r = read_schedule(self)
    import loading
    loading.load_matplotlib(throw=True)
    import matplotlib.pyplot as plt
    fig = plot_schedule(r)
    plt.show(block=False)
    browser.reporter.reportNote(f"PSA schedule: {r['N']} beds; time scale is chosen, not kinetic.")
    return fig


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=sorted(CASES), default='pe3')
    parser.add_argument('--solver', choices=('HiGHS', 'Gurobi'), default='HiGHS')
    parser.add_argument('--output', type=Path, default=Path('psa_scheduling.png'))
    parser.add_argument('--show', action='store_true')
    parser.add_argument('--no-balance', action='store_true', help='Skip the timing tie-break')
    args = parser.parse_args(argv)
    try:
        r = solve(args.case, args.solver, balance=not args.no_balance)
        print(f"\n{args.case}, {args.solver}: minimum beds = {r['N']}; "
              f"J = {[round(p['J']) for p in r['pairs']]}")
        print(f"Chosen shift D = {r['D']:g} s; period = {r['period']:g} s. "
              'These are NOT kinetic predictions.')
        print('Op  Operation                         Start / s  Process / s  Standby / s')
        for k, op in r['operations'].items():
            print(f"{k:2}  {op['label']:32}  {op['start']:9.3f}  "
                  f"{op['processing']:11.3f}  {op['standby']:11.3f}")
        if r['compressor']:
            print(f"Continuous feed: {round(r['Jfeed'])} bed(s) feeding at every instant.")
        import matplotlib
        if not args.show:
            matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        fig = plot_schedule(r)
        try:
            fig.savefig(args.output, dpi=160)
            print(f'Gantt chart: {args.output.resolve()}')
            if args.show:
                plt.show()
        finally:
            plt.close(fig)
    except (ImportError, RuntimeError, ValueError, OSError) as exc:
        print(f'psa_scheduling: {exc}', file=sys.stderr)
        return 1
    return 0


# extpy executes imports as __main__: register instead of running the CLI.
if 'extpy' in sys.modules:
    sys.modules['extpy'].registermethod(psa_scheduling_gantt)
elif __name__ == '__main__':
    sys.exit(main())
