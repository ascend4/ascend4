#!/usr/bin/env python3
"""Audit printed thermal equations, NOT validate a closed PSA cycle.

Run with ./a4 script models/psa/test/test_psa_thermal.py. Independent scalar roots use
only the standard library, with original psi/cm3(STP)/g property data.
"""
import importlib.util
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
R = 8.31446261815324
PSI = 6894.757293168
MW = .016043


def bisect(f, lo, hi):
    flo, fhi = f(lo), f(hi)
    if flo*fhi > 0:
        raise ValueError('Reference root not bracketed')
    for _ in range(100):
        mid = (lo+hi)/2
        fm = f(mid)
        if flo*fm <= 0:
            hi = mid
        else:
            lo, flo = mid, fm
    return (lo+hi)/2


def properties(T, p_psi):
    qsat = (-.76+40539/T)*1e-3/(R*273.15/101325)
    affinity = math.exp(-10.245+1756/T)
    activity = affinity*p_psi
    return qsat*activity/(1+activity), qsat*MW*affinity*R*T/PSI


def reference(PH=225, T0=298, Tfeed=298, Tdes0=298, TinB=350, H=20920, initial=0, scale=1):
    V = math.pi*.45**2*2.24/4*scale
    C, mass = V*.56*804000, V*.56*800
    def solid(T): return .75*.95*mass*properties(T, .05*PH)[0]
    def ads_energy(T):
        s = solid(T)
        return s*H+(s-initial)*36.8*(Tfeed-T)+.95*(s-initial)/.05*29.3*(Tfeed-T0)-.75*C*(T-T0)
    Ta = bisect(ads_energy, 240, 650)
    sa = solid(Ta)
    va = PH*PSI*.75*V*.44/(R*Ta)
    feed = (sa-initial)/.05+va
    product = .95*(sa-initial)/.05+PH*PSI*.75*V*.44/(R*T0)
    qref = properties(Ta, .05*PH)[0]*MW
    def exhaust(T): return .95**2*qref*R*T/(properties(T, 0)[1]*15*PSI)
    def purge(T): return sa*(1-exhaust(T))/exhaust(T)+15*PSI*V*.44/(R*T)
    def des_energy(T):
        return C*(Tdes0-T)-sa*H-sa*36.8*(T-Tdes0)-purge(T)*29.3*(T-TinB)
    Td = bisect(des_energy, 200, 500)
    return dict(Tads=Ta, Tdes=Td, solid=sa, void_feed=va, feed=feed, product=product,
                K=properties(Td, 0)[1], yout=exhaust(Td), purge=purge(Td),
                void_hydrogen=15*PSI*V*.44/(R*Td))


def thesis_purge_reference(V, solid, P=15*PSI, T0=298, TinB=350,
                           H=20920, phi=.98, ratio=1.25):
    """Eliminate (6.21)-(6.23) into a quadratic in temperature.

    Independent analytic solution, not an iterative ASCEND solve.
    Inputs and returned amounts are SI (mol, m3, Pa, J/mol, K).
    """
    removed = phi*solid
    sweep = ratio*removed
    void_factor = P*V*.44/R
    capacity = V*.56*804000+removed*36.8
    aa = capacity+sweep*29.3
    bb = capacity*T0-removed*H+sweep*29.3*TinB-void_factor*29.3
    cc = void_factor*29.3*TinB
    disc = math.sqrt(bb*bb+4*aa*cc)
    # Stable positive root also when bb is negative.
    T = (bb+disc)/(2*aa) if bb >= 0 else 2*cc/(disc-bb)
    return dict(T=T, removed=removed, remaining=solid-removed,
                sweep=sweep, void=void_factor/T, hydrogen=sweep+void_factor/T)


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class ThermalEquationAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        try:
            cls.lib.findType('psa_thesis_purge_check')
        except RuntimeError:
            cls.lib.load(str(ROOT/'models/psa/test/thermal.a4c'))

    def setUp(self):
        self.typ = self.lib.findType('psa_thermal')
        self.sim = self.typ.getSimulation('thermal_audit', True)
        self.m = self.sim.getModel()

    def solve(self, solver='QRSlv', **changes):
        inputs = dict(PH=(self.m.ads.P, 'Pa', PSI), T0=(self.m.ads.T0, 'K', 1),
                      Tfeed=(self.m.ads.Tfeed, 'K', 1), Tdes0=(self.m.des.T0, 'K', 1),
                      TinB=(self.m.des.TinB, 'K', 1), H=(self.m.bed.H, 'J/mol', 1),
                      initial=(self.m.ads.sA0, 'mol', 1),
                      scale=(self.m.bed.V, 'm^3', math.pi*.45**2*2.24/4))
        for key, value in changes.items():
            instance, unit, multiplier = inputs[key]
            instance.setRealValueWithUnits(value*multiplier, unit)
        self.sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=1e-7, abs_tol=1e-8),
                        f'{actual} != {expected}')

    def compare(self, **changes):
        expected = reference(**changes)
        instances = dict(Tads=self.m.ads.Te, Tdes=self.m.des.Te, solid=self.m.ads.sAe,
                         void_feed=self.m.ads.void_feed, feed=self.m.ads.feed,
                         product=self.m.ads.product, K=self.m.des.K, yout=self.m.des.yout,
                         purge=self.m.des.hydrogen_in, void_hydrogen=self.m.des.void_hydrogen)
        for key, instance in instances.items():
            with self.subTest(quantity=key):
                self.close(instance.getRealValue(), expected[key])

    def test_baseline_reference_and_self_test(self):
        self.solve()
        self.compare()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))

    def test_dimensions(self):
        self.sim.checkDimensions()
        for instance, unit in ((self.m.bed.Cv, 'J/m^3/K'), (self.m.bed.H, 'J/mol'),
                               (self.m.bed.cpA, 'J/mol/K'), (self.m.ads.Te, 'K'),
                               (self.m.des.K, 'm^3/mol'), (self.m.ads.feed, 'mol')):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units(unit).getDimensions())

    def test_pressure_feed_temperature_grid(self):
        for PH in (150, 225, 355):
            for Tfeed in (288, 298, 318):
                with self.subTest(PH=PH, Tfeed=Tfeed):
                    self.solve(PH=PH, Tfeed=Tfeed)
                    self.compare(PH=PH, Tfeed=Tfeed)

    def test_printed_energy_balances_in_joules(self):
        self.solve(initial=5)
        self.compare(initial=5)
        def v(x): return x.getRealValue()
        a, d, b = self.m.ads, self.m.des, self.m.bed
        storage = v(a.Vads)*(1-v(b.eps))*v(b.Cv)*(v(a.Te)-v(a.T0))
        heat = v(a.sAe)*v(b.H)+(v(a.sAe)-v(a.sA0))*v(b.cpA)*(v(a.Tfeed)-v(a.Te))
        heat += (1-v(a.y))*(v(a.feed)-v(a.void_feed))*v(b.cpB)*(v(a.Tfeed)-v(a.T0))
        self.assertAlmostEqual(storage, heat, delta=1e-4)
        storage = v(b.V)*(1-v(b.eps))*v(b.Cv)*(v(d.T0)-v(d.Te))
        heat = v(d.removed)*v(b.H)+v(d.removed)*v(b.cpA)*(v(d.Te)-v(d.T0))
        heat += v(d.hydrogen_in)*v(b.cpB)*(v(d.Te)-v(d.TinB))
        self.assertAlmostEqual(storage, heat, delta=1e-4)

    def test_adsorption_two_zone_inventory_balance(self):
        self.solve()
        def v(x): return x.getRealValue()
        a, b = self.m.ads, self.m.bed
        initial_h = v(a.P)*v(b.V)*v(b.eps)/(R*v(a.T0))
        tail_h = (1-.75)*initial_h
        final_h = (1-v(a.y))*v(a.void_feed)+tail_h
        self.close(initial_h+(1-v(a.y))*v(a.feed), final_h+v(a.product))
        self.close(v(a.sA0)+v(a.y)*v(a.feed), v(a.sAe)+v(a.y)*v(a.void_feed))

    def test_zero_heat_isothermal_limit(self):
        self.solve(H=0, TinB=298)
        self.compare(H=0, TinB=298)
        self.close(self.m.ads.Te.getRealValue(), 298)
        self.close(self.m.des.Te.getRealValue(), 298)

    def test_scale_and_henry_slope(self):
        self.solve(scale=2)
        self.compare(scale=2)
        T = self.m.des.Te.getRealValue()
        # Finite dilute concentration, not the formula used by the component.
        partial_psi = 1e-6
        q, _ = properties(T, partial_psi)
        slope = q*MW/(partial_psi*PSI/(R*T))
        self.close(self.m.des.K.getRealValue(), slope)

    def test_hotter_purge_reduces_hydrogen_requirement(self):
        self.solve(TinB=330)
        cool_use = self.m.des.hydrogen_in.getRealValue()
        self.solve(TinB=370)
        self.compare(TinB=370)
        self.assertLess(self.m.des.hydrogen_in.getRealValue(), cool_use)

    def test_ipopt_from_neighbouring_point(self):
        try:
            self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError:
            self.skipTest('IPOPT not built')
        self.sim = self.lib.findType('psa_thermal_ipopt_check').getSimulation('thermal_ipopt', True)
        self.m = self.sim.getModel()
        self.solve(PH=200)
        self.solve(solver='IPOPT', PH=225)
        self.compare()

    def test_pressure_change_timing_law(self):
        sim = self.lib.findType('psa_pressure_time_check').getSimulation('pressure_time', True)
        m = sim.getModel()
        sim.checkDimensions()
        self.assertEqual(m.a_pc.getDimensions(), self.ascpy.Units('s/Pa').getDimensions())
        for high, low, coefficient in ((20e5, 1e5, 1e-5), (1e5, 1e5, 1e-5), (20e5, 1e5, 0)):
            m.Phigh.setRealValueWithUnits(high, 'Pa')
            m.Plow.setRealValueWithUnits(low, 'Pa')
            m.a_pc.setRealValueWithUnits(coefficient, 's/Pa')
            sim.solve(self.ascpy.Solver('QRSlv'), self.ascpy.SolverReporter())
            self.assertTrue(sim.getStatus().isConverged())
            self.close(m.duration.getRealValue(), coefficient*(high-low)+1)


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class ThesisPurgeAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        # Library storage is global: another audit class may have loaded it.
        try:
            cls.lib.findType('psa_thesis_purge_check')
        except RuntimeError:
            cls.lib.load(str(ROOT/'models/psa/test/thermal.a4c'))

    def setUp(self):
        self.make('psa_thesis_thermal')

    def make(self, name):
        self.typ = self.lib.findType(name)
        self.sim = self.typ.getSimulation('thesis_purge', True)
        self.m = self.sim.getModel()

    def solve(self, solver='QRSlv'):
        self.sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=1e-7, abs_tol=1e-7),
                        f'{actual} != {expected}')

    def check_purge(self, phi=.98, ratio=1.25):
        def v(x): return x.getRealValue()
        b, d, g = self.m.bed, self.m.des, self.m.gas
        ref = thesis_purge_reference(v(b.V), v(d.sA0), P=v(d.P), T0=v(d.T0),
                                     TinB=v(d.TinB), H=v(b.H), phi=phi, ratio=ratio)
        for name, instance in dict(T=d.Te, removed=d.removed, remaining=d.sAe,
                                   sweep=d.sweep_hydrogen, void=d.void_hydrogen,
                                   hydrogen=d.hydrogen_in).items():
            with self.subTest(quantity=name):
                self.close(v(instance), ref[name])
        self.close(v(g.initialA)+v(g.initialH), v(d.P)*v(b.V)*v(b.eps)/(R*v(d.T0)))
        self.close(v(g.finalH), v(d.P)*v(b.V)*v(b.eps)/(R*v(d.Te)))
        self.close(v(g.initialA)+v(d.sA0), v(g.finalA)+v(d.sAe)+v(g.wasteA))
        self.close(v(g.initialH)+v(d.hydrogen_in), v(g.finalH)+v(g.wasteH))
        self.close(v(g.wasteH), v(g.initialH)+ratio*v(d.removed))
        self.close(v(g.yout), v(g.wasteA)/(v(g.wasteA)+v(g.wasteH)))
        self.assertGreaterEqual(v(g.wasteA), -1e-8)
        self.assertGreaterEqual(v(g.wasteH), -1e-8)
        # Check the PRINTED energy balance in joules, not a global-cycle claim.
        storage = v(b.V)*(1-v(b.eps))*v(b.Cv)*(v(d.T0)-v(d.Te))
        heat = v(d.removed)*(v(b.H)+v(b.cpA)*(v(d.Te)-v(d.T0)))
        heat += v(d.hydrogen_in)*v(b.cpB)*(v(d.Te)-v(d.TinB))
        # The implemented residual is in K (QRSlv tolerance 1e-8 K).
        self.assertAlmostEqual(storage, heat,
                               delta=max(1e-4, v(b.V)*(1-v(b.eps))*v(b.Cv)*1e-8))

    def test_baseline_and_independent_adsorption_root(self):
        self.solve()
        self.check_purge()
        V = math.pi*.396**2*1.982/4
        def solid(T): return .75*V*.56*800*properties(T, .05*355)[0]
        T = bisect(lambda T: .75*V*.56*804000*(T-298)
                   - solid(T)*(20920+36.8*(298-T)), 298, 400)
        self.close(self.m.ads.Te.getRealValue(), T)
        self.close(self.m.ads.sAe.getRealValue(), solid(T))
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))

    def test_dimensions(self):
        self.sim.checkDimensions()
        for instance, unit in ((self.m.des.Te, 'K'), (self.m.des.sweep_hydrogen, 'mol'),
                               (self.m.gas.wasteA, 'mol'), (self.m.bed.H, 'J/mol')):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units(unit).getDimensions())

    def test_initial_gas_is_displaced_not_deleted_or_subtracted_twice(self):
        for x in (0, .0375, .25, 1):
            with self.subTest(initial_CH4_fraction=x):
                self.m.x0.setRealValue(x)
                self.solve()
                self.check_purge()
                # No equilibrium exhaust formula: initial composition changes
                # total exhaust but NOT thesis (6.22)'s supplied H2 amount.
                if x == 0:
                    supply = self.m.des.hydrogen_in.getRealValue()
                self.close(self.m.des.hydrogen_in.getRealValue(), supply)

    def test_pressure_temperature_and_loading_grid(self):
        for P in (1e4, 1e5, 3e5):
            for T in (280, 298, 330):
                for solid in (0, 25, 100):
                    with self.subTest(P=P, T=T, solid=solid):
                        self.make('psa_thesis_purge_check')
                        self.m.des.P.setRealValueWithUnits(P, 'Pa')
                        self.m.des.T0.setRealValueWithUnits(T, 'K')
                        self.m.des.sA0.setRealValueWithUnits(solid, 'mol')
                        try:
                            self.solve()
                        except RuntimeError as exc:
                            self.fail(f'P={P}, T0={T}, solid={solid}: {exc}')
                        self.check_purge(phi=.5, ratio=2)

    def test_zero_heat_isothermal_limit_and_nonzero_initial_gas(self):
        self.m.bed.H.setRealValueWithUnits(0, 'J/mol')
        self.m.des.TinB.setRealValueWithUnits(298, 'K')
        self.solve()
        self.check_purge()
        self.close(self.m.ads.Te.getRealValue(), 298)
        self.close(self.m.des.Te.getRealValue(), 298)
        self.close(self.m.gas.initialA.getRealValue()+self.m.gas.initialH.getRealValue(),
                   self.m.gas.finalH.getRealValue())

    def test_geometry_scaling(self):
        self.solve()
        T = self.m.des.Te.getRealValue()
        H = self.m.des.hydrogen_in.getRealValue()
        self.m.bed.V.setRealValueWithUnits(2*self.m.bed.V.getRealValue(), 'm^3')
        self.solve()
        self.check_purge()
        self.close(self.m.des.Te.getRealValue(), T)
        self.close(self.m.des.hydrogen_in.getRealValue(), 2*H)

    def test_hot_purge_changes_void_charge_not_empirical_ratio(self):
        self.solve()
        T = self.m.des.Te.getRealValue()
        sweep = self.m.des.sweep_hydrogen.getRealValue()
        supply = self.m.des.hydrogen_in.getRealValue()
        self.m.des.TinB.setRealValueWithUnits(370, 'K')
        self.solve()
        self.check_purge()
        self.close(self.m.des.sweep_hydrogen.getRealValue(), sweep)
        self.assertGreater(self.m.des.Te.getRealValue(), T)
        self.assertLess(self.m.des.hydrogen_in.getRealValue(), supply)

    def test_ipopt_from_neighbouring_point(self):
        try:
            self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError:
            self.skipTest('IPOPT not built')
        self.make('psa_thesis_thermal_ipopt_check')
        self.m.ads.P.setRealValueWithUnits(330*PSI, 'Pa')
        self.solve()
        self.m.ads.P.setRealValueWithUnits(355*PSI, 'Pa')
        self.solve('IPOPT')
        self.check_purge()


if __name__ == '__main__':
    unittest.main()
