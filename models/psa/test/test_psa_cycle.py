#!/usr/bin/env python3
"""Fixed-design cycle balances and an independent scalar reconstruction.

Run with ./a4 script models/psa/test/test_psa_cycle.py. QRSlv and HiGHS are required;
IPOPT and Gurobi are also exercised when built. No published optimal-cost
claim is tested here.
"""
import importlib.util
from copy import deepcopy
from contextlib import redirect_stdout
from io import StringIO
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
R = 8.31446261815324
PSI = 6894.757293168


def reference(d=.396, L=1.982, eps=.44, rho=800, T=299, PH=355, PL=15, y=.05,
              Ffeed=31.11, equalisations=0):
    """Eliminate the cycle equations explicitly, using the source's units."""
    V = math.pi*d*d*L/4
    M = (1-eps)*rho*V
    hi, lo = (p*PSI*eps*V/(R*T) for p in (PH, PL))
    a = math.exp(-10.245 + 1756/T)*y*PH
    q = (-.76 + 40539/T)*a/(1+a)*1e-3/(R*273.15/101325)
    solid_hi = .75*M*q
    solid_lo = .02*solid_hi
    # The receiver captures methane: only transferred hydrogen remains gas.
    x = .75*y
    eq = (lo+(1-x)*hi)/(2-x) if equalisations else lo
    transfer_a = x*(hi-eq) if equalisations else 0
    fr = (hi-eq)/(1-y)
    solid_start = solid_lo + transfer_a + y*fr
    gas_a_hi = .75*y*hi
    gas_a_lo = gas_a_hi*lo/hi
    ads = (solid_hi-solid_start+gas_a_hi)/y
    gross = (1-y)*ads + gas_a_hi
    purge = 1.25*(solid_hi-solid_lo)
    feed = ads+fr
    product = gross-purge
    states = {'pressurised': (0, hi, solid_start),
              'loaded': (gas_a_hi, hi-gas_a_hi, solid_hi),
              'blown_down': (gas_a_lo, lo-gas_a_lo, solid_hi),
              'regenerated': (0, lo, solid_lo)}
    if equalisations:
        states.update(equalised_down=(x*eq, (1-x)*eq, solid_hi),
                      equalised_up=(0, eq, solid_lo+transfer_a))
    return dict(Mcarbon=M, ads_feed=ads, fr_feed=fr, gross=gross, purge=purge,
                feed=feed, product=product, period=(2+equalisations)*feed/Ffeed,
                Fproduct=product*Ffeed/feed, recovery=product/((1-y)*feed),
                Pe=eq/hi*PH*PSI, transferA=transfer_a,
                transferH=(1-x)*(hi-eq), states=states)


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class FixedPSACycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(ROOT/'models/psa/psa_cycle.a4c'))
        cls.lib.load(str(ROOT/'models/psa/test/cycle.a4c'))
        spec = importlib.util.spec_from_file_location('psa_cycle_reporter', ROOT/'models/psa/psa_cycle.py')
        cls.reporter = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.reporter)

    def setUp(self):
        self.typ = self.lib.findType('psa_cycle')
        self.sim = self.typ.getSimulation('cycle_check', True)
        self.m = self.sim.getModel()

    def solve(self, solver='QRSlv', **changes):
        inputs = dict(d=(self.m.bed.d, 'm'), L=(self.m.bed.L, 'm'),
                      eps=(self.m.bed.eps, None), rho=(self.m.bed.rho, 'kg/m^3'),
                      T=(self.m.bed.T, 'K'), PH=(self.m.PH, 'Pa'), PL=(self.m.PL, 'Pa'),
                      y=(self.m.y, None), Ffeed=(self.m.Ffeed, 'mol/s'))
        for key, value in changes.items():
            instance, units = inputs[key]
            if key in ('PH', 'PL'):
                value *= PSI
            if units:
                instance.setRealValueWithUnits(value, units)
            else:
                instance.setRealValue(value)
        self.sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=1e-7, abs_tol=1e-6),
                        f'{actual} != {expected}')

    def compare_reference(self, **changes):
        ref = reference(**changes)
        values = dict(Mcarbon=self.m.bed.Mcarbon, ads_feed=self.m.ads.feed,
                      fr_feed=self.m.fr.feed, gross=self.m.ads.product,
                      purge=self.m.purge.hydrogen_in)
        for name in ('feed', 'product', 'period', 'Fproduct', 'recovery'):
            values[name] = getattr(self.m, name)
        for name, instance in values.items():
            with self.subTest(quantity=name):
                self.close(instance.getRealValue(), ref[name])
        for key, expected in ref['states'].items():
            for field, value in zip(('gA', 'gH', 'sA'), expected):
                with self.subTest(state=key, inventory=field):
                    self.close(getattr(self.m.state[key], field).getRealValue(), value)

    def test_baseline_and_self_test(self):
        self.solve()
        self.compare_reference()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))

    def test_dimensions(self):
        self.sim.checkDimensions()
        for instance, units in ((self.m.bed.Mcarbon, 'kg'), (self.m.bed.Vgas, 'm^3'),
                                (self.m.state['loaded'].sA, 'mol'),
                                (self.m.ads.feed, 'mol'), (self.m.period, 's'),
                                (self.m.Ffeed, 'mol/s')):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units(units).getDimensions())

    def test_component_and_external_balances(self):
        self.solve()
        def value(instance): return instance.getRealValue()
        def inventory(state): return value(state.sA)+value(state.gA), value(state.gH)
        m, y = self.m, value(self.m.y)
        transfers = [
            (m.ads, (y*value(m.ads.feed), (1-y)*value(m.ads.feed)), (0, value(m.ads.product))),
            (m.bd, (0, 0), (value(m.bd.wasteA), value(m.bd.wasteH))),
            (m.purge, (0, value(m.purge.hydrogen_in)), (value(m.purge.wasteA), value(m.purge.wasteH))),
            (m.fr, (y*value(m.fr.feed), (1-y)*value(m.fr.feed)), (0, 0))]
        for step, incoming, outgoing in transfers:
            for before, after, fin, fout in zip(inventory(step.start), inventory(step.finish), incoming, outgoing):
                self.close(before+fin, after+fout)
        self.close(y*value(m.feed), value(m.wasteA))
        self.close((1-y)*value(m.feed), value(m.product)+value(m.wasteH))
        # Explicitly count feed repressurisation and internal product reuse.
        self.close(value(m.feed), value(m.ads.feed)+value(m.fr.feed))
        self.close(value(m.product), value(m.ads.product)-value(m.purge.hydrogen_in))

    def test_operating_condition_grid(self):
        for T in (298, 330):
            for PH in (250, 355):
                for PL in (15, 30):
                    for y in (.02, .05):
                        with self.subTest(T=T, PH=PH, PL=PL, y=y):
                            values = dict(T=T, PH=PH, PL=PL, y=y)
                            self.solve(**values)
                            self.compare_reference(**values)

    def test_geometry_and_throughput_scaling(self):
        self.solve()
        product = self.m.product.getRealValue()
        period = self.m.period.getRealValue()
        rate = self.m.Fproduct.getRealValue()
        self.solve(d=.396*math.sqrt(2))
        self.compare_reference(d=.396*math.sqrt(2))
        self.close(self.m.product.getRealValue(), 2*product)
        self.close(self.m.period.getRealValue(), 2*period)
        self.close(self.m.Fproduct.getRealValue(), rate)
        self.solve(Ffeed=62.22)
        self.compare_reference(d=.396*math.sqrt(2), Ffeed=62.22)
        self.close(self.m.period.getRealValue(), period)
        self.close(self.m.Fproduct.getRealValue(), 2*rate)

    def test_ipopt_when_available(self):
        try:
            self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError:
            self.skipTest('IPOPT not built')
        self.sim = self.lib.findType('psa_cycle_ipopt_check').getSimulation('ipopt_cycle_check', True)
        self.m = self.sim.getModel()
        self.solve(solver='IPOPT')
        self.compare_reference()

    def test_operation_connection_contracts(self):
        for name in ('psa_cycle_wrong_bed', 'psa_cycle_unshared_purge_pressure'):
            with self.subTest(model=name), self.assertRaisesRegex(RuntimeError, 'instantiation'):
                self.lib.findType(name).getSimulation('bad_connection', False)

    def test_report(self):
        spec = importlib.util.spec_from_file_location('psa_cycle_reporter', ROOT/'models/psa/psa_cycle.py')
        reporter = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(reporter)
        results = reporter.solve()
        self.close(results['recovery'], reference()['recovery'])
        output = StringIO()
        with redirect_stdout(output):
            reporter.report(results)
        text = output.getvalue()
        for label in ('per bed / mol', 'per bed per cycle / mol', 'mol/s',
                      'Internal H2 purge', 'Repressurisation feed',
                      'ASSUMED, not verified', 'operation timings not verified'):
            self.assertIn(label, text)
        self.assertIn('88.1647%', text)

    def one_pe(self, name='psa_cycle_1pe'):
        self.typ = self.lib.findType(name)
        self.sim = self.typ.getSimulation('one_pe_check', True)
        self.m = self.sim.getModel()

    def test_one_pe_balances_and_reference(self):
        self.one_pe()
        self.test_component_and_external_balances()
        self.compare_reference(equalisations=1)
        self.sim.checkDimensions()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))
        pe = self.m.pe
        ref = reference(equalisations=1)
        for instance, key in ((pe.P, 'Pe'), (pe.transferA, 'transferA'), (pe.transferH, 'transferH')):
            self.close(instance.getRealValue(), ref[key])
        self.assertLess(pe.P.getRealValue(), (355+15)/2*PSI)
        self.assertGreater(pe.P.getRealValue(), 15*PSI)
        for field in ('sA', 'gA', 'gH'):
            for state in ref['states']:
                self.assertGreaterEqual(getattr(self.m.state[state], field).getRealValue(), -1e-7)
        self.assertGreater(self.m.recovery.getRealValue(), reference()['recovery'])
        self.assertEqual(pe.P.getDimensions(), self.ascpy.Units('Pa').getDimensions())

    def test_one_pe_condition_grid(self):
        self.one_pe()
        for T in (298, 330):
            for PH in (250, 355):
                for PL in (15, 30):
                    for y in (.02, .05):
                        with self.subTest(T=T, PH=PH, PL=PL, y=y):
                            changes = dict(T=T, PH=PH, PL=PL, y=y)
                            self.solve(**changes)
                            self.compare_reference(equalisations=1, **changes)

    def test_one_pe_ipopt_when_available(self):
        try:
            self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError:
            self.skipTest('IPOPT not built')
        self.one_pe('psa_cycle_1pe_ipopt_check')
        # Use a feasible neighbouring operating point, not atom defaults.
        # Then change pressure: IPOPT must solve a genuinely different cycle.
        self.solve(PH=250)
        self.solve(solver='IPOPT', PH=355)
        self.compare_reference(equalisations=1)

    def test_equalisation_component(self):
        # Unequal gas volumes and temperatures, pure-H2 limit, zero pressure
        # difference, and methane initially present in the receiver voids.
        for receiver_scale, receiver_T, donor_a, receiver_a, low_P in (
                (1, 299, 4, 0, 1e5), (2, 330, 4, 1, 1e5),
                (2, 299, 0, 0, 1e5), (1, 299, 0, 0, 24e5)):
            with self.subTest(scale=receiver_scale, T=receiver_T, a=donor_a, low_P=low_P):
                sim = self.lib.findType('psa_pe_unit_check').getSimulation('pe_component', True)
                m = sim.getModel()
                m.receiver.L.setRealValueWithUnits(1.982*receiver_scale, 'm')
                m.receiver.T.setRealValueWithUnits(receiver_T, 'K')
                m.ds.gA.setRealValueWithUnits(donor_a, 'mol')
                m.rs.gA.setRealValueWithUnits(receiver_a, 'mol')
                m.rs.P.setRealValueWithUnits(low_P, 'Pa')
                sim.checkDimensions()
                sim.solve(self.ascpy.Solver('QRSlv'), self.ascpy.SolverReporter())
                self.assertTrue(sim.getStatus().isConverged())
                def v(instance): return instance.getRealValue()
                cd = .44*math.pi*.396**2*1.982/4/(R*299)
                cr = cd*receiver_scale*299/receiver_T
                x = donor_a/(cd*24e5)
                expected = (cr*low_P-receiver_a+(1-x)*cd*24e5)/(cr+(1-x)*cd)
                self.close(v(m.df.P), expected)
                self.close(v(m.rf.P), expected)
                self.close(v(m.pe.transferA), x*cd*(24e5-expected))
                self.close(v(m.pe.transferH), (1-x)*cd*(24e5-expected))
                self.close(v(m.ds.sA), v(m.df.sA))
                self.close(v(m.rs.sA)+receiver_a+v(m.pe.transferA), v(m.rf.sA))
                self.close(v(m.ds.gH)+v(m.rs.gH), v(m.df.gH)+v(m.rf.gH))
                self.close(sum(v(s.gA)+v(s.sA) for s in (m.ds, m.rs)),
                           sum(v(s.gA)+v(s.sA) for s in (m.df, m.rf)))
                for state in (m.ds, m.df, m.rs, m.rf):
                    for field in ('gA', 'gH', 'sA'):
                        self.assertGreaterEqual(v(getattr(state, field)), -1e-7)

    def timing_cases(self, solver):
        try:
            self.ascpy.Solver(solver).getIndex()
        except RuntimeError:
            self.skipTest(f'{solver} not built')
        for npe in (0, 1):
            with self.subTest(solver=solver, npe=npe):
                result = self.reporter.solve(npe)
                durations = dict(bd=1., purge=31., fr=1.)
                if npe:
                    durations.update(ed=1., eu=1.)
                timing = self.reporter.schedule(result, durations, solver)
                D = result['period']/(npe+2)
                expected = min((D-33)/3, (D-2)/2) if npe else (D-33)/3
                self.close(timing['margin'], expected)
                self.close(timing['D'], D)
                self.reporter.validate_timing(timing)
                for stage in timing['stages'][1:]:
                    self.close(stage['actual'], durations[stage['name']])
                    self.assertGreaterEqual(stage['idle'], expected-1e-7)

    def test_timing_highs(self):
        self.timing_cases('HiGHS')

    def test_timing_gurobi(self):
        self.timing_cases('Gurobi')

    def test_timing_infeasible_and_bad_inputs(self):
        r = self.reporter.solve(1)
        # Total processing fits the non-adsorption window, but PE pairing
        # cannot: ED + BD + purge exceeds the inter-bed shift D.
        with self.assertRaises(RuntimeError):
            self.reporter.schedule(r, dict(ed=1, eu=1, bd=25, purge=40, fr=1))
        # A clock-feasible purge still needs sufficient concurrent H2 supply.
        with self.assertRaises(RuntimeError):
            self.reporter.schedule(r, dict(ed=1, eu=1, bd=1, purge=1, fr=1))
        for key, value in (('ed', 2), ('bd', -1), ('purge', float('nan')), ('fr', float('inf'))):
            durations = dict(ed=1, eu=1, bd=1, purge=31, fr=1)
            durations[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                self.reporter.schedule(r, durations)

    def test_timing_independent_validation(self):
        r = self.reporter.solve(1)
        timing = self.reporter.schedule(r, dict(ed=1, eu=1, bd=1, purge=31, fr=1))
        modified = deepcopy(timing)
        modified['stages'][0]['actual'] -= 1
        modified['stages'][0]['idle'] += 1
        with self.assertRaisesRegex(ValueError, 'continuous production'):
            self.reporter.validate_timing(modified)
        modified = deepcopy(timing)
        modified['purge'] = modified['gross']
        with self.assertRaisesRegex(ValueError, 'hydrogen supply'):
            self.reporter.validate_timing(modified)
        # Move EU one second later, preserving contiguity, paired durations,
        # positive standby and cyclic closure. Only cross-bed sync is broken.
        modified = deepcopy(timing)
        modified['stages'][3]['end'] += 1
        modified['stages'][3]['idle'] += 1
        modified['stages'][4]['start'] += 1
        modified['stages'][4]['end'] += 1
        modified['stages'][5]['start'] += 1
        modified['stages'][5]['idle'] -= 1
        with self.assertRaisesRegex(ValueError, 'synchronised'):
            self.reporter.validate_timing(modified)

    def test_one_pe_report_and_timing_arguments(self):
        output = StringIO()
        with redirect_stdout(output):
            self.reporter.report(self.reporter.solve(1))
        self.assertIn('90.7991%', output.getvalue())
        self.assertIn('Equalised pressure: 12.531331 bar absolute', output.getvalue())
        for args in (['--equalisations', '1', '--timing'], ['--purge-time', '31'],
                     ['--timing', '--equalisation-time', '1']):
            with self.subTest(args=args), self.assertRaises(SystemExit) as error:
                self.reporter.main(args)
            self.assertEqual(error.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
