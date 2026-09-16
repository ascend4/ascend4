#!/usr/bin/env python3
"""Physical balances, independent solutions, handovers and CSS regressions.

./a4 script models/psa/test/test_psa_dynamic.py
Requires the built IDA and QRSlv adapters. No commercial solver required.
"""
from copy import deepcopy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import psa_dynamic as dynamic
import psa_dynamic_checks as checks
import psa_dynamic_cycle as cycle


class DynamicPSA(unittest.TestCase):
    def test_frozen_transfers_all_pair_counts(self):
        for pe in range(4):
            with self.subTest(pe=pe):
                transfers = cycle.Transfers(pe)
                try:
                    r = transfers.solve(.03, .001, .1, 299, 355e5, 15e5)
                    # Deliberately use arbitrary pressures (Pa): this checks
                    # transfer algebra independently of the thesis data.
                    cycle.validate_transfers(r)
                    # Equal void volumes, frozen solid, common T: pressure
                    # levels must be uniformly spaced, derived independently.
                    for j, s in enumerate(r['down'], 1):
                        self.assertAlmostEqual(s['P1'], 355e5-j*(340e5)/(pe+1), delta=.01)
                        self.assertAlmostEqual(s['y1'], .03, places=9)
                    expected = (15e5*.001 + pe/(pe+1)*340e5*.03 + 340e5/(pe+1)*.05)/355e5
                    self.assertAlmostEqual(r['fill']['y1'], expected, places=9)
                    bad = deepcopy(r)
                    bad['fill']['m1'] += 1
                    with self.assertRaises(ValueError): cycle.validate_transfers(bad)
                finally:
                    transfers.close()

    def test_closed_bed_kinetics_against_scalar_rk4(self):
        n, duration = 8, .02
        r = dynamic.run_operation(n, 1, initial=dict(y=[.05]*n, q=[0.0]*n),
                                  changes=dict(F=0), duration=duration, samples=2,
                                  rtol=1e-10, atol=1e-12)
        C = r['P']/(dynamic.R*r['T'])
        beta = (1-r['eps'])*r['rho']/(r['eps']*C)
        b = r['affinity']*r['P']/r['p_ref']
        def f(q):
            y = .05-beta*q
            return r['k']*(r['qsat']*b*y/(1+b*y)-q)
        q, h = 0, duration/2000
        for _ in range(2000):
            k1 = f(q); k2 = f(q+h*k1/2); k3 = f(q+h*k2/2); k4 = f(q+h*k3)
            q += h*(k1+2*k2+2*k3+k4)/6
        for actual in r['rows'][-1]['q']:
            self.assertAlmostEqual(actual, q, delta=2e-8)
        for actual in r['rows'][-1]['y']:
            self.assertAlmostEqual(actual, .05-beta*q, delta=2e-8)
        self.assertLess(dynamic.validate(r)['max_balance_error'], 1e-7)

    def test_uniform_equilibrium_is_stationary(self):
        for stencil in (1, 5):
            base = dynamic.run_operation(8, stencil, duration=.001, samples=1)
            b = base['affinity']*base['P']/base['p_ref']
            y = .02
            q = base['qsat']*b*y/(1+b*y)
            r = dynamic.run_operation(8, stencil, initial=dict(y=[y]*8, q=[q]*8),
                                      changes=dict(yin=y), duration=.2, samples=2)
            for actual in r['rows'][-1]['y']:
                self.assertAlmostEqual(actual, y, delta=1e-7)
            for actual in r['rows'][-1]['q']:
                self.assertAlmostEqual(actual, q, delta=1e-7)

    def test_higher_order_handover_preserves_signed_inventory(self):
        r = dynamic.adsorption_purge(50, 5)
        a, p = r['adsorption'], r['purge']
        self.assertLess(min(a['rows'][-1]['q']), 0, 'Exercise a real high-order undershoot')
        for before, after in zip(reversed(a['rows'][-1]['q']), p['rows'][0]['q']):
            self.assertAlmostEqual(before, after, delta=1e-9)
        self.assertLess(dynamic.validate(a)['max_balance_error'], 1e-7)
        self.assertLess(dynamic.validate(p)['max_balance_error'], 1e-7)
        self.assertGreater(dynamic.validate(a)['negative_solid_moles'], 0)
        self.assertEqual(dynamic.validate(p)['max_y'], max(max(row['y']) for row in p['rows']))
        bad = deepcopy(p)
        bad['rows'][-1]['inventory'] += 1
        with self.assertRaises(ValueError): dynamic.validate(bad)

    def test_spatial_and_temporal_refinement(self):
        errors = {}
        for stencil in (1, 5):
            runs = [checks.advection(n, stencil) for n in (25, 50, 100)]
            errors[stencil] = [r['mean_absolute_error'] for r in runs]
            self.assertLess(errors[stencil][1], errors[stencil][0])
            self.assertLess(errors[stencil][2], errors[stencil][1])
            tight = checks.advection(100, stencil, rtol=1e-11, atol=1e-13)
            self.assertLess(abs(tight['mean_absolute_error']-errors[stencil][-1]),
                            .02*errors[stencil][-1])
        self.assertLess(errors[5][-1], errors[1][-1]/10)

    def test_three_pair_css_and_warm_restart(self):
        r = cycle.solve(25, 1)
        self.assertEqual(len(r['transfers']['down']), 3)
        self.assertEqual(len(r['transfers']['up']), 3)
        self.assertLess(r['history'][-1]['profile_error'], 1e-6)
        self.assertLess(max(abs(h['methane_balance_error']) for h in r['history']), 1e-6)
        self.assertGreater(r['metrics']['desorption_fraction'], .9)
        # This is a diagnostic of the trace-flow model's physical limitation,
        # not a physically admissible composition or permission to clip it.
        self.assertGreater(dynamic.validate(r['purge'])['max_y'], 1)
        warm = cycle.solve(25, 1, initial=r['state'], max_cycles=3)
        self.assertEqual(len(warm['history']), 1)
        self.assertAlmostEqual(warm['metrics']['adsorption_utilisation'],
                               r['metrics']['adsorption_utilisation'], delta=2e-6)
        with self.assertRaises(RuntimeError): cycle.solve(8, 1, max_cycles=1)

    def test_invalid_inputs(self):
        with self.assertRaises(ValueError): dynamic.build(3, 5)
        with self.assertRaises(ValueError): dynamic.run_operation(operation='unknown')
        with self.assertRaises(ValueError): dynamic.run_operation(initial=dict(y=[0], q=[0]))
        with self.assertRaises(ValueError): cycle.Transfers(4)
        with self.assertRaises(ValueError): checks.coarsen([0]*5, 3)

    def test_worker_rejects_invalid_arguments_before_launch(self):
        with patch.object(checks.subprocess, 'run') as launch:
            for args in (('--help', 25, 1), ('css', '--help', 1),
                         ('css', 0, 1), ('css', True, 1), ('css', 25, '--help')):
                with self.subTest(args=args), self.assertRaises(ValueError):
                    checks.isolated(*args)
            with self.assertRaises(ValueError): checks.grid_check((0, 25))
            launch.assert_not_called()

    def test_five_point_css_reference(self):
        r = cycle.solve(50, 5)
        # Regression values for THIS documented reconstruction, not values
        # fitted to the thesis. Its reported adsorption utilisation is 0.88.
        for key, expected in (('adsorption_utilisation', .8044358),
                              ('desorption_fraction', .9921673),
                              ('purge_per_methane_desorbed', 1.2452471),
                              ('mean_outlet_methane', .001199701)):
            self.assertAlmostEqual(r['metrics'][key], expected, delta=2e-5)
        # Preserve diagnostics from early fixed-point passes, even when the
        # final CSS profiles no longer exhibit material negative loadings.
        self.assertLess(r['history'][0]['adsorption_audit']['min_q'], -.001)
        self.assertGreater(r['history'][-1]['purge_audit']['max_y'], 1.1)

    def test_zero_one_two_pair_css(self):
        for pe in range(3):
            with self.subTest(pe=pe):
                r = cycle.solve(12, 1, pe=pe)
                self.assertEqual(len(r['transfers']['down']), pe)
                self.assertLess(r['history'][-1]['profile_error'], 1e-6)
                self.assertLess(abs(r['history'][-1]['methane_balance_error']), 1e-6)


if __name__ == '__main__':
    unittest.main()
