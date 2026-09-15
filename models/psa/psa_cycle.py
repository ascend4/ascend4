#!/usr/bin/env python3
"""Solve and report isothermal PSA reconstructions and optional timing LPs.

Run from the repository: ./a4 script models/psa/psa_cycle.py
"""
from pathlib import Path
import argparse
import math


def model_type(name):
    import ascpy
    lib = ascpy.Library()
    # The compiler library is process-global (including in a GUI session).
    # Requiring an already-loaded module through this wrapper raises an error.
    try:
        typ = lib.findType(name)
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
        typ = lib.findType(name)
    return typ


def solve(equalisations=0):
    import ascpy
    if equalisations not in (0, 1):
        raise ValueError('Only zero or one pressure equalisation is implemented')
    typ = model_type('psa_cycle' if equalisations == 0 else 'psa_cycle_1pe')
    sim = typ.getSimulation('psa_report', True)
    sim.checkDimensions()
    sim.solve(ascpy.Solver('QRSlv'), ascpy.SolverReporter())
    if not sim.getStatus().isConverged():
        raise RuntimeError('PSA cycle did not converge')
    sim.run(next(m for m in typ.getMethods() if str(m.getName()) == 'self_test'))
    m = sim.getModel()
    def value(x): return x.getRealValue()
    result = dict(
        equalisations=equalisations, nbed=m.nbed.getIntValue(),
        carbon=value(m.bed.Mcarbon), recovery=value(m.recovery), period=value(m.period),
        feed=value(m.feed), ads_feed=value(m.ads.feed), fr_feed=value(m.fr.feed),
        gross=value(m.ads.product), purge=value(m.purge.hydrogen_in), product=value(m.product),
        wasteA=value(m.wasteA), wasteH=value(m.wasteH),
        Ffeed=value(m.Ffeed), Fproduct=value(m.Fproduct),
        states={key: tuple(value(getattr(m.state[key], field)) for field in ('gA', 'gH', 'sA'))
                for key in (('pressurised', 'loaded', 'blown_down', 'regenerated') if not equalisations else
                            ('pressurised', 'loaded', 'equalised_down', 'blown_down', 'regenerated', 'equalised_up'))})
    if equalisations:
        result['pe'] = dict(P=value(m.pe.P), transferA=value(m.pe.transferA), transferH=value(m.pe.transferH))
    return result


def schedule(result, durations, solver='HiGHS'):
    """Find a cyclic layout for explicit, positive non-adsorption times in s.

    Durations are externally supplied requirements, not inferred kinetics.
    Returned standby is real idle time, never credited as production time.
    """
    import ascpy
    npe = result['equalisations']
    if npe not in (0, 1) or result['nbed'] != npe + 2:
        raise ValueError('Timing supports only the two/three-bed zero/one-PE cases')
    stages = ['ads', 'bd', 'purge', 'fr'] if npe == 0 else ['ads', 'ed', 'bd', 'purge', 'eu', 'fr']
    if set(durations) != set(stages[1:]):
        raise ValueError(f'Specify durations in seconds for {stages[1:]}')
    if not all(math.isfinite(v) and v > 0 for v in durations.values()):
        raise ValueError('Processing durations must be finite and positive')
    if not math.isfinite(result['period']) or result['period'] <= 0:
        raise ValueError('Cycle period must be finite and positive')
    if (not math.isfinite(result['gross']) or result['gross'] <= 0
            or not math.isfinite(result['purge']) or result['purge'] < 0):
        raise ValueError('Invalid gross hydrogen production or purge consumption')
    if npe and durations['ed'] != durations['eu']:
        raise ValueError('Coupled PE donor/receiver processing durations must match')
    typ = model_type(f'psa_timing_{npe}pe')
    sim = typ.getSimulation('psa_timing', True)
    m = sim.getModel()
    m.period.setRealValueWithUnits(result['period'], 's')
    m.gross.setRealValueWithUnits(result['gross'], 'mol')
    m.purge_use.setRealValueWithUnits(result['purge'], 'mol')
    for k, name in enumerate(stages[1:], 2):
        m.actual[k].setRealValueWithUnits(durations[name], 's')
    sim.checkDimensions()
    sim.solve(ascpy.Solver(solver), ascpy.SolverReporter())
    if not sim.getStatus().isConverged():
        raise RuntimeError('No converged timing solution for the specified durations')
    sim.run(next(method for method in typ.getMethods() if str(method.getName()) == 'self_test'))
    timing = dict(D=m.D.getRealValue(), period=result['period'], nbed=result['nbed'],
                  gross=result['gross'], purge=result['purge'],
                  margin=m.margin.getRealValue(), solver=solver,
                  stages=[dict(name=name, start=m.boundary[k-1].getRealValue(),
                               end=m.boundary[k].getRealValue(), actual=m.actual[k].getRealValue(),
                               idle=m.idle[k].getRealValue()) for k, name in enumerate(stages, 1)])
    validate_timing(timing)
    return timing


def validate_timing(timing):
    """Check the returned event layout independently, including cross-cycle pairing."""
    def close(a, b): return math.isclose(a, b, rel_tol=1e-8, abs_tol=1e-7)
    period, D, beds = timing['period'], timing['D'], timing['nbed']
    stages = timing['stages']
    names = ['ads', 'bd', 'purge', 'fr'] if beds == 2 else ['ads', 'ed', 'bd', 'purge', 'eu', 'fr']
    if beds not in (2, 3) or [s['name'] for s in stages] != names:
        raise ValueError('Unexpected timing topology')
    numbers = [period, D, timing['margin'], timing['gross'], timing['purge']] + [s[k] for s in stages for k in ('start', 'end', 'actual', 'idle')]
    if not all(math.isfinite(x) for x in numbers) or period <= 0 or D <= 0:
        raise ValueError('Invalid timing values')
    if timing['margin'] < -1e-7 or any(s['idle'] < timing['margin']-1e-7 for s in stages[1:]):
        raise ValueError('Invalid minimum standby allowance')
    if not close(period, beds*D) or not close(stages[0]['start'], 0) or not close(stages[-1]['end'], period):
        raise ValueError('Cycle timing does not close')
    previous = 0
    for s in stages:
        if (not close(s['start'], previous) or s['actual'] <= 0 or s['idle'] < -1e-7
                or not close(s['end']-s['start'], s['actual']+s['idle'])):
            raise ValueError('Invalid processing/standby accounting')
        previous = s['end']
    if not close(stages[0]['actual'], D) or not close(stages[0]['idle'], 0):
        raise ValueError('Standby cannot count as continuous production')
    purge_time = next(s['actual'] for s in stages if s['name'] == 'purge')
    if (timing['gross'] <= 0 or timing['purge'] < 0
            or timing['gross']*purge_time + 1e-6 < timing['purge']*D):
        raise ValueError('Insufficient unbuffered hydrogen supply during purge')
    if beds == 3:
        ed, eu = stages[1], stages[4]
        if not close(ed['actual'], eu['actual']) or not close(ed['end']-ed['start'], eu['end']-eu['start']):
            raise ValueError('PE processing/allocated durations do not match')
        # Donor on bed b pairs with receiver on bed b-1, modulo the cycle.
        for bed in range(beds):
            donor = (bed*D + ed['start']) % period
            receiver = (((bed-1) % beds)*D + eu['start']) % period
            separation = abs(donor-receiver)
            if not (close(separation, 0) or close(separation, period)):
                raise ValueError('PE events are not synchronised across beds')


def report(r):
    print(f"Isothermal PSA reconstruction: {r['nbed']} beds, {r['equalisations']} pressure equalisation(s)")
    print('Fixed-design material balances; pure H2 product is ASSUMED, not verified.')
    print(f"Dry carbon per bed: {r['carbon']:.6f} kg")
    if 'pe' in r:
        print(f"Equalised pressure: {r['pe']['P']/1e5:.6f} bar absolute")
        print(f"Internal PE transfer: {r['pe']['transferA']:.6f} mol CH4, {r['pe']['transferH']:.6f} mol H2")
    print('\nBoundary inventories per bed / mol:')
    print(f"{'State':16} {'Gas CH4':>12} {'Gas H2':>12} {'Adsorbed CH4':>15}")
    for key, (a, h, s) in r['states'].items():
        print(f'{key:16} {a:12.6f} {h:12.6f} {s:15.6f}')
    print('\nTransfers per bed per cycle / mol:')
    for label, key in [('Adsorption feed', 'ads_feed'), ('Repressurisation feed', 'fr_feed'),
                       ('Total external feed', 'feed'), ('Gross H2 product', 'gross'),
                       ('Internal H2 purge', 'purge'), ('Net H2 product', 'product'),
                       ('External CH4 waste', 'wasteA'), ('External H2 waste', 'wasteH')]:
        print(f'{label:25} {r[key]:12.6f}')
    print(f"\nNet hydrogen recovery: {100*r['recovery']:.4f}%")
    print(f"Plant-average feed: {r['Ffeed']:.6f} mol/s")
    print(f"Plant-average net H2 product: {r['Fproduct']:.6f} mol/s")
    print(f"Throughput-implied cycle period: {r['period']:.6f} s (operation timings not verified)")


def report_timing(t):
    print(f"\nConditional timing solution ({t['solver']}), all times in s:")
    print('Feasible for the SUPPLIED processing durations and unbuffered H2 purge supply; kinetics not validated.')
    print(f"Bed phase shift: {t['D']:.6f}; minimum non-adsorption standby: {t['margin']:.6f}")
    print(f"{'Stage':8} {'Start':>12} {'End':>12} {'Processing':>12} {'Standby':>12}")
    for s in t['stages']:
        values = [0. if abs(s[k]) < .5e-6 else s[k] for k in ('start', 'end', 'actual', 'idle')]
        print(f"{s['name']:8}" + ''.join(f'{v:13.6f}' for v in values))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--equalisations', type=int, choices=(0, 1), default=0)
    parser.add_argument('--timing', action='store_true', help='Solve a timing LP for explicit processing times')
    parser.add_argument('--timing-solver', choices=('HiGHS', 'Gurobi'), default='HiGHS')
    for operation in ('blowdown', 'purge', 'repressurisation', 'equalisation'):
        parser.add_argument(f'--{operation}-time', type=float, help='Specified processing duration in seconds')
    args = parser.parse_args(argv)
    supplied_times = (args.blowdown_time, args.purge_time, args.repressurisation_time, args.equalisation_time)
    if not args.timing and any(v is not None for v in supplied_times):
        parser.error('Processing times require --timing')
    if not args.equalisations and args.equalisation_time is not None:
        parser.error('--equalisation-time requires --equalisations 1')
    durations = dict(bd=args.blowdown_time, purge=args.purge_time, fr=args.repressurisation_time)
    if args.equalisations:
        durations.update(ed=args.equalisation_time, eu=args.equalisation_time)
    if args.timing and any(v is None or not math.isfinite(v) or v <= 0 for v in durations.values()):
        parser.error('Timing requires positive --blowdown-time, --purge-time, --repressurisation-time'
                     ' and, for one PE, --equalisation-time (all seconds)')
    result = solve(args.equalisations)
    report(result)
    if args.timing:
        report_timing(schedule(result, durations, args.timing_solver))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
