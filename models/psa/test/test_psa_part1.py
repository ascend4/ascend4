#!/usr/bin/env python3
"""Part I source deck and equation checks; NOT historical optimum validation."""
from contextlib import redirect_stdout
import importlib.util
from io import StringIO
import json
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
R, PSI = 8.31446261815324, 6894.757293168


def root(f, lo, hi):
    flo = f(lo)
    if flo*f(hi) > 0:
        raise ValueError('Reference root not bracketed')
    for _ in range(90):
        mid = (lo+hi)/2
        if flo*f(mid) <= 0:
            hi = mid
        else:
            lo, flo = mid, f(mid)
    return (lo+hi)/2


def reference(d, L, PH, phi, H=20920):
    """Independent scalar solution using source's original psi/STP units."""
    V = math.pi*d*d*L/4
    M, C = V*.56*800, V*.56*804000
    def q(T):
        a = math.exp(-10.245+1756/T)*.05*PH
        return (-.76+40539/T)*a/(1+a)*1e-3/(R*273.15/101325)
    def solid(T): return phi*.95*M*q(T)
    Ta = root(lambda T: phi*C*(T-298)-solid(T)*(H+36.8*(298-T)), 240, 650)
    s = solid(Ta)
    feed = s/.05+PH*PSI*phi*V*.44/(R*Ta)
    gross = .95*s/.05+PH*PSI*phi*V*.44/(R*298)
    def K(T):
        qsat = (-.76+40539/T)*1e-3/(R*273.15/101325)
        return .016043*qsat*math.exp(-10.245+1756/T)*R*T/PSI
    def yout(T): return .95**2*q(Ta)*.016043*R*T/(K(T)*15*PSI)
    def purge(T): return s*(1-yout(T))/yout(T)+15*PSI*V*.44/(R*T)
    Td = root(lambda T: C*(298-T)-s*H-s*36.8*(T-298)-purge(T)*29.3*(T-350), 200, 500)
    return dict(Tads_K=Ta, Tdes_K=Td, ads_feed_mol=feed, gross_H2_mol=gross,
                purge_H2_mol=purge(Td), yout=yout(Td), K_m3_mol=K(Td))


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class PartIBenchmark(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        spec = importlib.util.spec_from_file_location('part1_report', ROOT/'models/psa/psa_part1.py')
        cls.driver = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.driver)
        cls.result = cls.driver.solve()

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=1e-7, abs_tol=1e-6),
                        f'{actual} != {expected}')

    def test_published_case_transcription(self):
        # Independently listed Table 4 rows, in increasing PE order.
        expected = (
            (.45, 2.24, 225, 54.8, 93.1, 1610, 95.6, 1533, 47.7, .90, -1329200),
            (.40, 1.98, 356, 64.7, 70.1, 1953, 59.1, 1861, 4.5, .95, -1390100),
            (.39, 1.98, 358.7, 70.1, 70.1, 2137, 45, 2035, 4.5, .96, -1396400),
            (.40, 1.97, 360.5, 74.2, 74.2, 2272, 37, 2164, 4.5, .97, -1395300))
        for npe, (case, row) in enumerate(zip(self.result['cases'], expected)):
            d, L, P, D, tau1, ads, fr, gross, purge, rec, cost = row
            self.assertEqual(case['npe'], npe)
            self.assertEqual(case['nbed'], npe+2)
            self.assertEqual(case['J'], list(range(npe, 0, -1))+[0]*(3-npe))
            self.close(case['phi'], (.75, .85, .90, .95)[npe])
            for actual, value in ((case['d_m'], d), (case['L_m'], L),
                                  (case['pressures_Pa']['P9'], P*PSI), (case['D_s'], D),
                                  (case['slots_s']['1'], tau1), (case['reported_recovery'], rec),
                                  (case['published_cost_table5_USD_year'], cost)):
                self.close(actual, value)
            for key, value in zip(('ads_feed', 'fr_feed', 'gross_H2', 'purge_H2'), (ads, fr, gross, purge)):
                self.close(case['published_amounts_mol'][key], value)
        self.close(self.result['Ffeed_mol_s'], 31.1)
        # Do not silently "correct" the unusual but clearly printed 332.2.
        self.close(self.result['cases'][2]['pressures_Pa']['P2'], 332.2*PSI)
        self.assertIsNone(self.result['cases'][0]['pressures_Pa']['P2'])

    def test_rounding_does_not_explain_reported_recoveries(self):
        expected = (.91667078111, .971228662, .97954556, .98447676)
        for case, approx in zip(self.result['cases'], expected):
            p, t = case['published_amounts_mol'], case['checks']
            calculated = (p['gross_H2']-p['purge_H2'])/(.95*(p['ads_feed']+p['fr_feed']))
            self.close(t['recovery_from_amounts'], calculated)
            self.assertAlmostEqual(calculated, approx, delta=1e-6)
            self.assertFalse(t['recovery_agrees_with_rounding'])
            self.assertGreater(t['recovery_interval'][0], case['reported_recovery']+.005)

    def test_time_and_throughput_conventions(self):
        for case in self.result['cases']:
            n, t = case['nbed'], case['checks']
            self.assertLess(abs(t['slot_closure_error_s']), .5)
            self.assertLess(abs(t['feed_over_FD']-1), .001)
            self.close(t['feed_over_Fperiod'], t['feed_over_FD']/n)
            for error in t['paired_start_errors_s']:
                self.assertLess(abs(error), .5)
        self.assertGreater(self.result['cases'][0]['checks']['adsorption_slot_over_D'], 1.69)
        self.assertGreater(self.result['cases'][1]['checks']['adsorption_slot_over_D'], 1.08)

    def test_independent_operation_roots(self):
        for c in self.result['cases']:
            expected = reference(c['d_m'], c['L_m'], c['pressures_Pa']['P9']/PSI, c['phi'])
            for key, value in expected.items():
                with self.subTest(case=c['name'], quantity=key):
                    self.close(c['conditional_audit'][key], value)

    def test_assumption_changes_do_not_modify_source_targets(self):
        changed = self.driver.solve(18000)
        for old, new in zip(self.result['cases'], changed['cases']):
            self.assertEqual(old['published_amounts_mol'], new['published_amounts_mol'])
            self.assertEqual(old['checks'], new['checks'])
            self.assertEqual(old['published_cost_table5_USD_year'], new['published_cost_table5_USD_year'])
            self.assertNotEqual(old['conditional_audit']['Tads_K'], new['conditional_audit']['Tads_K'])
            expected = reference(new['d_m'], new['L_m'], new['pressures_Pa']['P9']/PSI, new['phi'], H=18000)
            for key, value in expected.items():
                self.close(new['conditional_audit'][key], value)

    def test_dimensions_and_self_test(self):
        lib = self.ascpy.Library()
        typ = lib.findType('psa_part1')
        sim = typ.getSimulation('part1_dimensions', True)
        sim.checkDimensions()
        m = sim.getModel()
        for instance, unit in ((m.data.amount['ads_feed']['pe0'], 'mol'),
                               (m.data.P['P9']['pe0'], 'Pa'), (m.data.size['d']['pe0'], 'm'),
                               (m.data.cost_table5['pe0'], 'USD/yr'), (m.Hads, 'J/mol')):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units(unit).getDimensions())
        sim.solve(self.ascpy.Solver('QRSlv'), self.ascpy.SolverReporter())
        self.assertTrue(sim.getStatus().isConverged())
        sim.run(next(method for method in typ.getMethods() if str(method.getName()) == 'self_test'))

    def test_report_and_json_preserve_scope_and_absent_values(self):
        output = StringIO()
        with redirect_stdout(output):
            self.driver.report(self.result)
        text = output.getvalue()
        for label in ('not optimisation', 'HYPOTHESIS', 'No recovery is inferred', 'Table 4', 'NO'):
            self.assertIn(label, text)
        result = json.loads(json.dumps(self.result, allow_nan=False))
        self.assertIsNone(result['cases'][0]['pressures_Pa']['P2'])
        self.assertNotIn('recovery', result['cases'][0]['conditional_audit'])
        for heat in (-1, math.nan, math.inf):
            with self.assertRaises(ValueError):
                self.driver.solve(heat)


if __name__ == '__main__':
    unittest.main()
