#!/usr/bin/env python3
"""Real ASCEND correlation checks: ./a4 script models/psa/test/test_psa_properties.py.

Reference calculations retain Cen & Yang's original cm3(STP)/g and psi
units, then convert separately to ASCEND's SI mol/kg. No licensed solver
or external property package is required.
"""
import importlib.util
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
PSI = 6894.757293168
VM_STP = 8.31446261815324 * 273.15 / 101325  # m3/mol, explicitly 1 atm STP


def reference(T, partial_psi, coefficients=(-0.76, 40539, -10.245, 1756, 1)):
    a, b, c, d, n = coefficients
    saturation = a + b / T  # cm3(STP)/g
    B = math.exp(c + d / T)
    term = B * partial_psi**n
    loading = saturation * term / (1 + term)
    # 1 cm3/g = 0.001 m3/kg; divide by STP molar volume, NOT molar mass.
    return saturation * 1e-3 / VM_STP, loading * 1e-3 / VM_STP, B


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class AdsorptionProperties(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(ROOT / 'models/psa/psa_properties.a4c'))
        cls.lib.load(str(ROOT / 'models/psa/test/properties.a4c'))
        # QRSlv is a standard solver: an unavailable/broken plugin is a failure.
        ascpy.Solver('QRSlv').getIndex()

    def setUp(self):
        self.typ = self.lib.findType('psa_properties')
        self.sim = self.typ.getSimulation('property_check', True)
        self.model = self.sim.getModel()

    def solve(self, T=299, P=355, y=0.05):
        self.model.T.setRealValueWithUnits(T, 'K')
        self.model.P.setRealValueWithUnits(P * PSI, 'Pa')
        self.model.y.setRealValue(y)
        self.sim.solve(self.ascpy.Solver('QRSlv'), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())
        return self.model.carbon.q.getRealValue()

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-10),
                        f'{actual} != {expected}')

    def test_reference_and_model_self_test(self):
        self.solve()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))
        self.close(self.model.carbon.Hads.getRealValue(), 20920)
        self.close(self.model.carbon.Vm_stp.getRealValue(), VM_STP)

    def test_dimensions(self):
        self.sim.checkDimensions()
        for instance, units in ((self.model.T, 'K'), (self.model.P, 'Pa'),
                                (self.model.carbon.p, 'Pa'),
                                (self.model.carbon.q, 'mol/kg'),
                                (self.model.carbon.qsat, 'mol/kg'),
                                (self.model.carbon.a, 'm^3/kg'),
                                (self.model.carbon.b, 'm^3*K/kg'),
                                (self.model.carbon.Hads, 'J/mol')):
            with self.subTest(units=units, instance=str(instance.getName())):
                self.assertEqual(instance.getDimensions(), self.ascpy.Units(units).getDimensions())
        with self.assertRaises(RuntimeError):
            self.model.carbon.q.setRealValueWithUnits(1, 'kg')

    def test_temperature_pressure_and_composition_grid(self):
        # Correlation checks, not a claim of experimental validation over this grid.
        for T in (273.15, 298, 299, 350):
            for P in (15, 355):
                for y in (1e-4, 0.05, 1.0):
                    with self.subTest(T=T, P=P, y=y):
                        qsat, q, B = reference(T, P * y)
                        self.close(self.solve(T, P, y), q)
                        self.close(self.model.carbon.qsat.getRealValue(), qsat)
                        self.close(self.model.carbon.affinity.getRealValue(), B)
                        self.close(self.model.carbon.p.getRealValue(), P * PSI * y)
                        self.close(self.model.carbon.w.getRealValue(), q * 0.016043)

    def test_zero_loading_and_pressure_limits(self):
        for P, y in ((355, 0), (0, 0.05), (0, 0)):
            self.close(self.solve(P=P, y=y), 0)
        qsat, _, B = reference(299, 1)
        # Dilute (Henry) limit, q/p -> qsat*B, with pressure in psi.
        p = 1e-5
        self.close(self.solve(P=p, y=1) / p, qsat * B / (1 + B * p))
        # Mathematical saturation limit only: far outside a physical design range.
        q = self.solve(P=1e4, y=1)
        self.assertLess(q, qsat)
        self.assertGreater(q / qsat, 0.99)

    def test_expected_monotonicity(self):
        loadings = [self.solve(P=p) for p in (1, 15, 100, 355)]
        self.assertTrue(all(a < b for a, b in zip(loadings, loadings[1:])))
        loadings = [self.solve(T=T) for T in (273.15, 298, 350)]
        self.assertTrue(all(a > b for a, b in zip(loadings, loadings[1:])))

    def test_noninteger_exponent_with_pressure_units(self):
        sim = self.lib.findType('psa_fractional_property_test').getSimulation('fractional', True)
        sim.checkDimensions()
        sim.solve(self.ascpy.Solver('QRSlv'), self.ascpy.SolverReporter())
        self.assertTrue(sim.getStatus().isConverged())
        qsat, q, B = reference(298, 100, (87.68, 42392, -12.336, 1219.3, 0.97))
        self.close(sim.getModel().qsat.getRealValue(), qsat)
        self.close(sim.getModel().q.getRealValue(), q)
        self.close(sim.getModel().affinity.getRealValue(), B)

    def test_invalid_exponent_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'instantiation'):
            self.lib.findType('psa_invalid_exponent').getSimulation('invalid', False)


if __name__ == '__main__':
    unittest.main()
