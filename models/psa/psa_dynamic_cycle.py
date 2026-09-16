#!/usr/bin/env python3
"""Representative-bed CSS reconstruction of Smith (1991), thesis section 7.4.

ASCEND/IDA integrates adsorption and purge; ASCEND/QRSlv balances frozen-solid
pressure transfers. Equalisation partners are periodic images of one bed,
not a simulated start-up of five separately scheduled beds. See psa_dynamic.md.
"""
import argparse
import json
import math
from pathlib import Path

import psa_dynamic as dynamic


class Transfers:
    """Reusable ASCEND gas-volume balances, with paired internal streams."""
    def __init__(self, pe=3):
        import ascpy
        if isinstance(pe, bool) or pe not in range(4):
            raise ValueError('pe must be an integer from zero to three')
        self.pe = pe
        lib = ascpy.Library()
        try:
            lib.findType('psa_dynamic_transfer_cycle')
        except RuntimeError:
            lib.load(str(Path(__file__).with_name('psa_dynamic_transfers.a4l')))
        name = f'psa_dynamic_transfers_{pe}'
        try:
            typ = lib.findType(name)
        except RuntimeError:
            lib.loadString(f'MODEL {name} WHERE (pe >= 0; pe <= 3;) '
                           f'REFINES psa_dynamic_transfer_cycle(pe :== {pe};); END {name};', name)
            typ = lib.findType(name)
        self.sim = typ.getSimulation(name, True)

    def solve(self, ya, yp, V, T, Ph, Pl, fill_y=.05):
        import ascpy
        if not all(math.isfinite(x) for x in (ya, yp, V, T, Ph, Pl, fill_y)):
            raise ValueError('Nonfinite transfer input')
        if not (V > 0 and T > 0 and Ph > Pl > 0 and 0 <= fill_y <= 1):
            raise ValueError('Invalid transfer volume, temperature, pressure or fill composition')
        m = self.sim.getModel()
        for name, val, unit in (('V', V, 'm^3'), ('T', T, 'K'), ('Ph', Ph, 'Pa'), ('Pl', Pl, 'Pa')):
            getattr(m, name).setRealValueWithUnits(val, unit)
        for name, val in (('ya', ya), ('yp', yp), ('fill_y', fill_y)):
            getattr(m, name).setRealValue(val)
        self.sim.checkDimensions()
        self.sim.solve(ascpy.Solver('QRSlv'), ascpy.SolverReporter())
        if not self.sim.getStatus().isConverged():
            raise RuntimeError('Frozen-gas transfer balances did not converge')
        def step(obj):
            return {k: getattr(obj, k).getRealValue() for k in
                    ('P0', 'P1', 'y0', 'y1', 'yin', 'yout', 'n0', 'n1',
                     'nin', 'nout', 'm0', 'm1', 'minlet', 'moutlet')}
        result = dict(down=[step(m.down[j]) for j in range(1, self.pe+1)],
                      up=[step(m.up[j]) for j in range(self.pe, 0, -1)],
                      blowdown=step(m.blowdown), fill=step(m.fill), pe=self.pe,
                      V=V, T=T, Ph=Ph, Pl=Pl)
        validate_transfers(result)
        return result

    def close(self):
        self.sim.invalidateSystem()


def validate_transfers(r, tolerance=1e-7):
    """Independently recompute EOS, each ledger, and paired-stream equality."""
    steps = r['down'] + [r['blowdown']] + r['up'] + [r['fill']]
    for s in steps:
        defects = [s['n1']-s['n0']-s['nin']+s['nout'],
                   s['m1']-s['m0']-s['minlet']+s['moutlet'],
                   s['n0']-s['P0']*r['V']/(dynamic.R*r['T']),
                   s['n1']-s['P1']*r['V']/(dynamic.R*r['T']),
                   s['m0']-s['n0']*s['y0'], s['m1']-s['n1']*s['y1'],
                   s['minlet']-s['nin']*s['yin'], s['moutlet']-s['nout']*s['yout']]
        if not all(math.isfinite(x) and abs(x) <= tolerance*max(1, s['n0']) for x in defects):
            raise ValueError('Gas-transfer material/EOS balance failed')
        if min(s['nin'], s['nout']) < -tolerance:
            raise ValueError('Negative total-gas transfer')
    for down, up in zip(r['down'], reversed(r['up'])):
        for left, right in (('nout', 'nin'), ('moutlet', 'minlet'), ('P1', 'P1')):
            if abs(down[left]-up[right]) > tolerance*max(1, abs(down[left])):
                raise ValueError('Equalisation partners do not match')
    return max(abs(s['m1']-s['m0']-s['minlet']+s['moutlet']) for s in steps)


def solve(n=50, stencil=1, pe=3, max_cycles=60, css_tolerance=1e-6,
          initial=None, rtol=1e-7, atol=1e-9):
    """Fixed point of all operation maps, not accelerated or clipped.

    Gas pressures of identical isothermal beds follow from the coupled gas
    balances, not an imposed pressure table. Pure hydrogen supplies purge;
    feed gas supplies final repressurisation (section 7.2, operation O9).
    """
    if max_cycles < 1 or int(max_cycles) != max_cycles or not 0 < css_tolerance < 1:
        raise ValueError('Invalid CSS iteration controls')
    transfers = Transfers(pe)
    ads_sim = purge_sim = None
    state = initial or dict(y=[0.0]*n, q=[0.0]*n)
    history = []
    try:
        ads_sim = dynamic.build(n, stencil)
        purge_sim = dynamic.build(n, stencil)
        purge_sim.run(next(method for method in purge_sim.getModel().getType().getMethods()
                           if str(method.getName()) == 'purge'))
        low_pressure = purge_sim.getModel().P.getRealValue()
        for iteration in range(1, max_cycles+1):
            ads = dynamic.run_operation(n, stencil, initial=state, samples=69,
                                        rtol=rtol, atol=atol, simulation=ads_sim)
            ya = sum(ads['rows'][-1]['y'])/n
            inputs = dict(ya=ya, V=ads['eps']*ads['volume'], T=ads['T'],
                          Ph=ads['P'], Pl=low_pressure, fill_y=ads['yin'])
            # Donor withdrawals are independent of receiver composition under
            # the stated well-mixed/frozen-solid approximation. The preliminary
            # solution supplies only the blowdown endpoint for purge.
            pre = transfers.solve(yp=0, **inputs)
            p0 = dict(y=[pre['blowdown']['y1']]*n,
                      q=list(reversed(ads['rows'][-1]['q'])))
            purge = dynamic.run_operation(n, stencil, operation='purge', initial=p0,
                                          samples=62, rtol=rtol, atol=atol, simulation=purge_sim)
            yp = sum(purge['rows'][-1]['y'])/n
            ledger = transfers.solve(yp=yp, **inputs)
            new = dict(y=[ledger['fill']['y1']]*n,
                       q=list(reversed(purge['rows'][-1]['q'])))
            error = max(max(abs(a-b) for a, b in zip(new['y'], state['y']))/.05,
                        max(abs(a-b) for a, b in zip(new['q'], state['q']))/ads['qscale'])
            # Trace-methane balance over the representative cycle. Equalisation
            # streams cancel in matched pairs; blowdown leaves the process.
            end_inventory = (ledger['fill']['m1'] + ads['carbon_mass']/n*sum(new['q']))
            start_inventory = ads['rows'][0]['inventory']
            inflow = ads['rows'][-1]['admitted'] + purge['rows'][-1]['admitted'] + ledger['fill']['minlet']
            outflow = ads['rows'][-1]['discharged'] + purge['rows'][-1]['discharged'] + ledger['blowdown']['moutlet']
            defect = end_inventory-start_inventory-inflow+outflow
            if abs(defect) > 1e-5*max(1, abs(inflow)):
                raise ValueError('Full-cycle methane balance failed')
            # Check the adsorption-to-purge boundary independently too.
            removed = sum(s['moutlet'] for s in ledger['down']) + ledger['blowdown']['moutlet']
            handover = ads['rows'][-1]['inventory']-purge['rows'][0]['inventory']-removed
            if abs(handover) > 1e-5*max(1, abs(removed)):
                raise ValueError('Cycle handover lost methane')
            history.append(dict(iteration=iteration, profile_error=error,
                                methane_balance_error=defect, inventory_change=end_inventory-start_inventory,
                                adsorption_audit=dynamic.validate(ads), purge_audit=dynamic.validate(purge)))
            state = new
            if error < css_tolerance:
                break
        else:
            raise RuntimeError(f'CSS did not converge in {max_cycles} iterations; profile error {error:g}')
        sads = ads['carbon_mass']/n*sum(ads['rows'][-1]['q'])
        spur = purge['carbon_mass']/n*sum(purge['rows'][-1]['q'])
        feed = ads['F']*ads['rows'][-1]['t']
        purge_gas = purge['F']*purge['rows'][-1]['t']
        return dict(n=n, stencil=stencil, pe=pe, history=history, state=state,
                    adsorption=ads, purge=purge, transfers=ledger,
                    assumptions=['isothermal trace methane', 'frozen solid pressure steps',
                                 'well-mixed donor withdrawal', 'pure-hydrogen purge; feed-gas final fill',
                                 'representative-bed periodic partners, not multibed startup'],
                    metrics=dict(adsorption_utilisation=sads/(ads['carbon_mass']*ads['qscale']),
                                 desorption_fraction=1-spur/sads,
                                 purge_per_methane_desorbed=purge_gas/(sads-spur),
                                 mean_outlet_methane=ads['rows'][-1]['discharged']/feed,
                                 net_product_gas_trace_approx=feed-purge_gas,
                                 total_feed_gas_trace_approx=feed+ledger['fill']['nin']))
    finally:
        transfers.close()
        if ads_sim is not None:
            ads_sim.invalidateSystem()
        if purge_sim is not None:
            purge_sim.invalidateSystem()


def plot(result):
    import matplotlib.pyplot as plt
    fig = dynamic.plot(result)
    fig.suptitle(f"Representative-bed CSS: {result['pe']} equalisation pairs; "
                 f"{result['n']} cells, spatial stencil {result['stencil']}\n"
                 "Isothermal/frozen-solid; well-mixed gas-transfer reconstruction")
    fig2, (ax, convergence) = plt.subplots(1, 2, figsize=(11, 4), layout='constrained')
    tr = result['transfers']
    steps = tr['down'] + [tr['blowdown']] + tr['up'] + [tr['fill']]
    labels = ([f'PE↓{i}' for i in range(1, result['pe']+1)] + ['Blowdown'] +
              [f'PE↑{i}' for i in range(result['pe'], 0, -1)] + ['Fill'])
    ax.plot(range(len(steps)+1), [tr['Ph']/1e5]+[s['P1']/1e5 for s in steps], 'o-')
    ax.set_xticks(range(1, len(steps)+1), labels, rotation=45)
    ax.set(ylabel='Pressure / bar', title='Pressure transitions (not durations)')
    convergence.semilogy([h['iteration'] for h in result['history']],
                         [max(h['profile_error'], 1e-16) for h in result['history']], 'o-')
    convergence.set(xlabel='Fixed-point iteration', ylabel='Scaled full-profile change', title='CSS convergence')
    for panel in (ax, convergence): panel.grid(alpha=.25)
    return fig, fig2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cells', type=int, default=50)
    parser.add_argument('--stencil', type=int, choices=(1, 5), default=1)
    parser.add_argument('--pe', type=int, choices=range(4), default=3)
    parser.add_argument('--max-cycles', type=int, default=60)
    parser.add_argument('--css-tolerance', type=float, default=1e-6)
    parser.add_argument('--output', type=Path, default=Path('psa_dynamic_css.json'))
    parser.add_argument('--plot', type=Path)
    args = parser.parse_args()
    result = solve(args.cells, args.stencil, args.pe, args.max_cycles, args.css_tolerance)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False)+'\n')
    print(json.dumps(dict(metrics=result['metrics'], convergence=result['history'][-1],
                         adsorption_audit=dynamic.validate(result['adsorption']),
                         purge_audit=dynamic.validate(result['purge'])), indent=2))
    if any(dynamic.validate(result[op])['max_y'] > 1 for op in ('adsorption', 'purge')):
        print('WARNING: y(CH4) > 1: the constant-flow trace approximation is outside '
              'its physical range. Values are reported unchanged; see psa_dynamic.md.')
    if args.plot:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        for i, fig in enumerate(plot(result)):
            target = args.plot if i == 0 else args.plot.with_name(args.plot.stem+'_cycle'+args.plot.suffix)
            fig.savefig(target, dpi=160)
            plt.close(fig)


if __name__ == '__main__':
    main()
