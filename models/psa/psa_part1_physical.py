#!/usr/bin/env python3
"""Compare conservative zero-PE alternatives, without fitting published flows."""
import argparse
import importlib.util
import json
import math
from pathlib import Path

TYPES = dict(adiabatic='psa_part1_physical', henry='psa_part1_physical_henry',
             ratio='psa_part1_physical_ratio', isothermal='psa_part1_physical_isothermal')


def solve(case='adiabatic', extra_capacity=0, beta=.5):
    import ascpy
    if case not in TYPES:
        raise ValueError('Unknown physical reconstruction case')
    if not math.isfinite(extra_capacity) or extra_capacity < 0:
        raise ValueError('Extra capacity must be finite and nonnegative')
    if not math.isfinite(beta) or not 0 <= beta <= 1:
        raise ValueError('Exhaust temperature weight must lie in [0,1]')
    lib=ascpy.Library()
    try: typ=lib.findType(TYPES[case])
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
        typ=lib.findType(TYPES[case])
    sim=typ.getSimulation('part1_physical_report',True)
    m=sim.getModel()
    m.extra_capacity.setRealValue(extra_capacity)
    m.beta.setRealValue(beta)
    sim.checkDimensions()
    sim.solve(ascpy.Solver('QRSlv'),ascpy.SolverReporter())
    if not sim.getStatus().isConverged(): raise RuntimeError('Physical cycle did not converge')
    sim.run(next(x for x in typ.getMethods() if str(x.getName())=='self_test'))
    def v(x): return x.getRealValue()
    amounts=dict(ads_feed=v(m.ads.feed),fr_feed=v(m.fr.feed),gross_H2=v(m.ads.product),purge_H2=v(m.purge.hydrogen_in))
    published={key:v(m.data.amount[key]['pe0']) for key in amounts}
    duties={key:v(obj.Q) for key,obj in dict(adsorption=m.ads,blowdown=m.bd,purge=m.purge,repressurisation=m.fr).items()}
    # Two identical staggered beds: per-bed-cycle duty / phase shift gives
    # plant-average power. Positive and negative duties are NOT cancelled
    # when reporting heating/cooling; no heat recovery is assumed.
    D=v(m.period)/2
    heating=(sum(max(0,q) for q in duties.values())+max(0,v(m.heater)))/D
    cooling=(sum(max(0,-q) for q in duties.values())+max(0,-v(m.heater)))/D
    return dict(scope='Fixed-design zero-PE reconstruction; NOT economic optimisation or purity validation',
                case=case, fitted_parameters=[], extra_capacity_fraction=extra_capacity,
                beta=beta, adsorption_heat_J_mol=v(m.bed.H),
                amounts_mol=amounts,published_amounts_mol=published,
                relative_amount_errors={k:(amounts[k]-published[k])/published[k] for k in amounts},
                recovery=v(m.recovery),published_recovery=v(m.data.recovery['pe0']),
                published_amount_recovery=(published['gross_H2']-published['purge_H2'])/
                                         (.95*(published['ads_feed']+published['fr_feed'])),
                states_SI={k:{f:v(getattr(m.state[k],f)) for f in ('T','P','sA','gA','gH','U')}
                           for k in ('pressurised','mixed','blown_down','regenerated')},
                adsorption_final_K=v(m.hot.T), effective_sweep_CH4_fraction=v(m.yout),
                average_exhaust_CH4_fraction=v(m.average_yout), purge_ratio=v(m.purge_ratio),
                period_s=v(m.period), product_mol_s=v(m.Fproduct),
                operation_heat_J=duties,purge_heater_J=v(m.heater),
                mean_external_heating_W=heating,mean_external_cooling_W=cooling,
                component_residuals_mol=dict(CH4=v(m.y)*v(m.feed)-v(m.wasteA),
                                            H2=(1-v(m.y))*v(m.feed)-v(m.product)-v(m.wasteH)),
                energy_residual_J=v(m.Efeed)+v(m.heater)+v(m.Qbed)-v(m.Eproduct)-v(m.Ewaste))


def comparison():
    spec=importlib.util.spec_from_file_location('literal_part1_report',Path(__file__).with_name('psa_part1_cycle.py'))
    driver=importlib.util.module_from_spec(spec); spec.loader.exec_module(driver)
    return dict(literal=driver.solve(),alternatives=[solve('henry'),solve(),
                solve(extra_capacity=.5),solve('isothermal'),solve('ratio')])


def report(result):
    cases=result['alternatives'] if 'alternatives' in result else [result]
    print('PART I: ALTERNATIVE ZERO-PE RECONSTRUCTIONS — NO FITTED PARAMETERS')
    print('Not yet the economic design/topology optimisation; pure product is assumed.')
    print('\nCase                       Ads feed   FR feed   Gross H2   Purge H2   Recovery')
    if 'literal' in result:
        literal=result['literal']; a=literal['amounts_mol']
        print(f"{'Literal equation audit':26s} {a['ads_feed']:8.2f} {a['fr_feed']:9.2f} {a['gross_H2']:10.2f} {a['purge_H2']:10.2f} {literal['recovery']:10.5f}")
    for r in cases:
        a=r['amounts_mol']; label=r['case']+(f" +{100*r['extra_capacity_fraction']:g}% capacity" if r['extra_capacity_fraction'] else '')
        print(f"{label:26s} {a['ads_feed']:8.2f} {a['fr_feed']:9.2f} {a['gross_H2']:10.2f} {a['purge_H2']:10.2f} {r['recovery']:10.5f}")
    p=cases[0]['published_amounts_mol']
    print(f"{'Published Part I':26s} {p['ads_feed']:8.2f} {p['fr_feed']:9.2f} {p['gross_H2']:10.2f} {p['purge_H2']:10.2f} {cases[0]['published_recovery']:10.5f}")
    print('Amounts: mol/bed-cycle. Published amounts imply recovery '+f"{cases[0]['published_amount_recovery']:.5f}, not the printed 0.90.")
    for r in cases:
        print(f"\n{r['case']}, extra capacity {100*r['extra_capacity_fraction']:g}%: "
              f"energy residual {r['energy_residual_J']:.3g} J/bed-cycle; "
              f"mean external heating/cooling {r['mean_external_heating_W']/1000:.3f}/{r['mean_external_cooling_W']/1000:.3f} kW.")
        print('  Bed heat per operation (kJ): '+', '.join(f'{k}={q/1000:.2f}' for k,q in r['operation_heat_J'].items()))
    print('\nAll conservative cases use actual bed pressures, H=20.920 kJ/mol, and close CH4/H2 and energy balances.')
    print('Adiabatic uses full-LRC effective sweep; henry uses the candidate K; ratio uses the thesis 1.25 rule.')
    print('Isothermal is an actively thermostatted limit, not an adiabatic result. Heat recovery is not assumed.')


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=['all']+list(TYPES),default='all')
    parser.add_argument('--json',action='store_true')
    args=parser.parse_args(argv)
    try: result=comparison() if args.case=='all' else solve(args.case)
    except (ValueError,RuntimeError) as exc: parser.exit(1,f'Physical PSA reconstruction: {exc}\n')
    if args.json: print(json.dumps(result,indent=2,allow_nan=False))
    else: report(result)


if __name__=='__main__': main()
