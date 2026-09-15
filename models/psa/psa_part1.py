#!/usr/bin/env python3
"""Report Part I source consistency and conditional operation comparisons.

./a4 script models/psa/psa_part1.py [--json] [--adsorption-heat J_PER_MOL]
No design optimisation, closed cycle or inferred economics is performed.
"""
import argparse
import json
import math
from pathlib import Path


def solve(adsorption_heat=20920):
    import ascpy
    if not math.isfinite(adsorption_heat) or adsorption_heat < 0:
        raise ValueError('Adsorption heat must be finite and nonnegative (J/mol)')
    lib = ascpy.Library()
    try:
        typ = lib.findType('psa_part1')
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
        typ = lib.findType('psa_part1')
    sim = typ.getSimulation('part1_report', True)
    m = sim.getModel()
    m.Hads.setRealValueWithUnits(adsorption_heat, 'J/mol')
    sim.checkDimensions()
    sim.solve(ascpy.Solver('QRSlv'), ascpy.SolverReporter())
    if not sim.getStatus().isConverged():
        raise RuntimeError('Part I conditional operation audit did not converge')
    def v(x): return x.getRealValue()
    data = m.data
    result = dict(source='Smith and Westerberg (1991), Part I, Tables 1-5',
                  scope='Independent operation audits, NOT closed cycles or optimisation',
                  assumptions=dict(Hads_J_mol=adsorption_heat, adsorption_initial_K=298,
                                   post_blowdown_K=298, initial_solid_mol=0,
                                   K='Hypothesis: methane LRC infinite-dilution mass-loading slope',
                                   amounts='Table 4 numbers interpreted as mol'),
                  Ffeed_mol_s=v(m.Ffeed), cases=[])
    for name in ('pe0', 'pe1', 'pe2', 'pe3'):
        npe, n = data.npe[name].getIntValue(), data.nbed[name].getIntValue()
        amounts = {s: v(data.amount[s][name]) for s in ('ads_feed', 'fr_feed', 'gross_H2', 'purge_H2')}
        times = {str(i): v(data.tau[i][name]) for i in range(1, 10)}
        feed, D, period = v(m.feed_total[name]), v(data.D[name]), v(m.period[name])
        recovery = v(m.recovery_from_amounts[name])
        # Last printed digits: ads/gross amounts +/-0.5 mol, fr/purge
        # +/-0.05 mol, reported recovery +/-0.005. No tolerance fitting.
        recovery_lo = (amounts['gross_H2']-.5-amounts['purge_H2']-.05)/(.95*(feed+.55))
        recovery_hi = (amounts['gross_H2']+.5-amounts['purge_H2']+.05)/(.95*(feed-.55))
        reported = v(data.recovery[name])
        # Coupled pair starts: sum slots between donor k and receiver 10-k.
        sync = [sum(times[str(j)] for j in range(k, 10-k))
                - data.J[k-1][name].getIntValue()*D for k in range(2, 2+npe)]
        result['cases'].append(dict(
            name=name, npe=npe, nbed=n, J=[data.J[i][name].getIntValue() for i in range(1, 4)],
            d_m=v(data.size['d'][name]), L_m=v(data.size['L'][name]), phi=v(data.phi[name]),
            pressures_Pa={p: (v(data.P[p][name]) if v(data.p_psi[p][name]) else None)
                          for p in ('P2', 'P3', 'P4', 'P5', 'P9')},
            D_s=D, period_s=period, slots_s=times, published_amounts_mol=amounts,
            reported_recovery=reported,
            published_cost_table3_USD_year=v(data.cost_table3[name])*v(data.year),
            published_cost_table5_USD_year=v(data.cost_table5[name])*v(data.year),
            checks=dict(recovery_from_amounts=recovery,
                        recovery_interval=[recovery_lo, recovery_hi],
                        recovery_agrees_with_rounding=not (recovery_hi < reported-.005
                                                          or recovery_lo > reported+.005),
                        feed_over_FD=feed/(v(m.Ffeed)*D),
                        feed_over_Fperiod=feed/(v(m.Ffeed)*period),
                        slot_closure_error_s=v(m.slot_sum[name])-period,
                        paired_start_errors_s=sync,
                        adsorption_slot_over_D=times['1']/D),
            conditional_audit=dict(Tads_K=v(m.ads[name].Te), Tdes_K=v(m.des[name].Te),
                                   ads_feed_mol=v(m.ads[name].feed), gross_H2_mol=v(m.ads[name].product),
                                   purge_H2_mol=v(m.des[name].hydrogen_in),
                                   yout=v(m.des[name].yout), K_m3_mol=v(m.des[name].K))))
    return result


def report(result):
    print('PART I: PUBLISHED-DATA CHECKS (not optimisation results)')
    print('Source: Tables 1-5, Smith & Westerberg (1991); amounts interpreted as mol.')
    print('PE   Rec printed / eq31   rounding?   feed/(F D)  feed/(F period)  tau1/D')
    for c in result['cases']:
        t = c['checks']
        print(f"{c['npe']:2d}   {c['reported_recovery']:.2f} / {t['recovery_from_amounts']:.6f}"
              f"      {'yes' if t['recovery_agrees_with_rounding'] else 'NO':3s}"
              f"        {t['feed_over_FD']:.6f}       {t['feed_over_Fperiod']:.6f}"
              f"       {t['adsorption_slot_over_D']:.4f}")
    print('\nEquation (37) prints F*period; the amounts instead approximately match F*D.')
    print('Slots tau1 > D in the 0/1-PE cases do not fit our earlier tau1=D timing LP.')
    print('Published cost targets at 3.5 USD/kgmol (Table 5), NOT computed economics:')
    print('  '+', '.join(f"{c['npe']} PE: {c['published_cost_table5_USD_year']/1e6:.4f} MUSD/year"
                        for c in result['cases']))
    print('\nCONDITIONAL PRINTED-EQUATION AUDITS: computed / Table 4 (mol)')
    print(f"Hads={result['assumptions']['Hads_J_mol']:g} J/mol; K uses the Henry-slope HYPOTHESIS.")
    print('Initial bed and post-blowdown temperatures=298 K; initial solid loading=0.')
    print('PE    adsorption feed       gross H2          purge H2     Tads/Tdes (K)')
    for c in result['cases']:
        a, p = c['conditional_audit'], c['published_amounts_mol']
        print(f"{c['npe']:2d}    {a['ads_feed_mol']:8.2f}/{p['ads_feed']:6.1f}"
              f"    {a['gross_H2_mol']:8.2f}/{p['gross_H2']:6.1f}"
              f"    {a['purge_H2_mol']:7.2f}/{p['purge_H2']:5.1f}"
              f"    {a['Tads_K']:.2f}/{a['Tdes_K']:.2f}")
    print('No recovery is inferred from these incomplete operation audits.')
    print('Missing: operation-to-operation thermal closure, PE, journal K interpretation,')
    print('design constraints and economics. Reported optima are not imposed or fitted.')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adsorption-heat', type=float, default=20920, metavar='J_PER_MOL',
                        help='Explicit caloric assumption (default: Cen-Yang 20920 J/mol)')
    parser.add_argument('--json', action='store_true', help='Machine-readable SI results and assumptions')
    args = parser.parse_args(argv)
    try:
        result = solve(args.adsorption_heat)
    except (ValueError, RuntimeError) as exc:
        parser.exit(1, f'Part I benchmark: {exc}\n')
    if args.json:
        print(json.dumps(result, indent=2, allow_nan=False))
    else:
        report(result)


if __name__ == '__main__':
    main()
