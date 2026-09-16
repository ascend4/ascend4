#!/usr/bin/env python3
"""Conservative thermal cycle checks, not historical optimum replication.

./a4 script models/psa/test/test_psa_thermal_cycle.py
The independent reference marches entire cycles using scalar operation
roots, rather than solving ASCEND's simultaneous equation system.
"""
import importlib.util
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
R = 8.31446261815324
PSI = 6894.757293168
VBASE = math.pi*.396**2*1.982/4


def root(f, lo=150., hi=600.):
    flo = f(lo)
    if flo*f(hi) > 0:
        raise ValueError('Scalar reference root is not bracketed')
    for _ in range(90):
        mid = (lo+hi)/2
        fm = f(mid)
        if flo*fm <= 0:
            hi = mid
        else:
            lo, flo = mid, fm
    return (lo+hi)/2


def reference(PH=355, PL=15, Tfeed=298, Tpurge=350, Tref=298, H=20920,
              beta=.5, scale=1, isothermal=False, volume=VBASE, eta=1, phi=.75,
              regenerated=.98, ratio=1.25, Cv=804000, Ffeed=31.11, purge_rule='ratio'):
    V = volume*scale
    vg, mass, C = V*.44, V*.56*800, V*.56*Cv
    pa, ph = 36.8, 29.3
    y = .05
    high, low = PH*PSI, PL*PSI

    def q(T):
        a = math.exp(-10.245+1756/T)*y*PH
        return (-.76+40539/T)*a/(1+a)*1e-3/(R*273.15/101325)

    def state(T, P, A, B, solid, share=1):
        U = share*C*(T-Tref)+solid*(pa*(T-Tref)-H)
        U += A*(pa*(T-Tref)-R*T)+B*(ph*(T-Tref)-R*T)
        return dict(T=T, P=P, gA=A, gH=B, sA=solid, n=A+B, U=U)

    def step(T0, s0):
        start = state(T0, high, 0, high*vg/(R*T0), s0)

        def ads(T):
            n = high*phi*vg/(R*T)
            hot = state(T, high, y*n, (1-y)*n, eta*phi*mass*q(T), phi)
            cold = state(T0, high, 0, high*(1-phi)*vg/(R*T0), 0, 1-phi)
            feed = (hot['sA']+hot['gA']-s0)/y
            gross = start['gH']+(1-y)*feed-hot['gH']-cold['gH']
            Q = hot['U']+cold['U']-start['U']-feed*(y*pa+(1-y)*ph)*(Tfeed-Tref)
            Q += gross*ph*(T0-Tref)
            return hot, cold, feed, gross, Q

        Ta = T0 if isothermal else root(lambda T: ads(T)[-1])
        hot, cold, feed_a, gross, Qa = ads(Ta)
        A, B, solid = hot['gA'], hot['gH']+cold['gH'], hot['sA']
        # Invert the affine-in-T uniform-state internal energy exactly.
        Tm = (hot['U']+cold['U']+(C+(solid+A)*pa+B*ph)*Tref+solid*H)
        Tm /= C+solid*pa+A*(pa-R)+B*(ph-R)
        mixed = state(Tm, (A+B)*R*Tm/vg, A, B, solid)
        x = A/(A+B)
        cv, S = x*pa+(1-x)*ph-R, C+solid*pa
        def bd_law(T):
            return math.log(T/Tm)-R/cv*math.log((S+low*vg/(R*T)*cv)/(S+(A+B)*cv))
        Tb = Tm if isothermal else root(bd_law)
        nb = low*vg/(R*Tb)
        blown = state(Tb, low, x*nb, (1-x)*nb, solid)
        bdA, bdH = A-blown['gA'], B-blown['gH']
        Eb = (bdA*pa+bdH*ph)*(Tm-Tref) if isothermal else mixed['U']-blown['U']
        Qb = blown['U']-mixed['U']+Eb
        removed = regenerated*solid
        purgeA = blown['gA']+removed

        def purge(T):
            final = state(T, low, 0, low*vg/(R*T), solid-removed)
            if purge_rule == 'ratio':
                sweep = ratio*removed
            else:
                qs = (-.76+40539/T)*1e-3/(R*273.15/101325)
                affinity = math.exp(-10.245+1756/T)
                target = eta*.95*q(Ta)
                if purge_rule == 'henry':
                    yout = target/(qs*affinity*PL)
                elif purge_rule == 'lrc':
                    yout = target/((qs-target)*affinity*PL)
                else: raise ValueError('Unknown reference purge closure')
                sweep = removed*(1-yout)/yout
            supply = sweep+final['gH']
            purgeH = blown['gH']+sweep
            Tout = (1-beta)*Tb+beta*T
            Eout = (purgeA*pa+purgeH*ph)*(Tout-Tref)
            Q = final['U']-blown['U']-supply*ph*(Tpurge-Tref)+Eout
            return final, supply, Eout, Q

        Tp = Tb if isothermal else root(lambda T: purge(T)[-1])
        purged, supply, Ep, Qp = purge(Tp)
        purgeH = blown['gH']+supply-purged['gH']
        def fr(T):
            n = high*vg/(R*T)
            feed = (n-purged['gH'])/(1-y)
            final = state(T, high, 0, n, purged['sA']+y*feed)
            Q = final['U']-purged['U']-feed*(y*pa+(1-y)*ph)*(Tfeed-Tref)
            return final, feed, Q

        Tr = Tfeed if isothermal else root(lambda T: fr(T)[-1])
        finish, feed_r, Qr = fr(Tr)
        feed = feed_a+feed_r
        product = gross-supply
        return dict(states={'pressurised': start, 'mixed': mixed,
                            'blown_down': blown, 'regenerated': purged}, hot=hot, cold=cold,
                    next=finish, feed=feed, product=product, recovery=product/((1-y)*feed),
                    ads_feed=feed_a, fr_feed=feed_r, gross=gross, purge=supply,
                    wasteA=bdA+purgeA, wasteH=bdH+purgeH,
                    heater=supply*ph*(Tpurge-T0), Qbed=Qa+Qb+Qp+Qr,
                    Efeed=feed*(y*pa+(1-y)*ph)*(Tfeed-Tref),
                    Eproduct=product*ph*(T0-Tref), Ewaste=Eb+Ep,
                    operation_heat=dict(adsorption=Qa,blowdown=Qb,purge=Qp,repressurisation=Qr),
                    period=2*feed/Ffeed, Fproduct=Ffeed*product/feed)

    T0, s0 = Tfeed, 10*scale
    for k in range(2000):
        result = step(T0, s0)
        next_T, next_s = result['next']['T'], result['next']['sA']
        if abs(next_T-T0) < 1e-10 and abs(next_s-s0) < 1e-10*scale:
            result['cycles'] = k+1
            return result
        T0, s0 = next_T, next_s
    raise RuntimeError('Reference cyclic steady state failed to converge')


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class ClosedThermalCycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(ROOT/'models/psa/test/thermal_cycle.a4c'))

    def setUp(self):
        self.make('psa_thermal_cycle')

    def make(self, name):
        self.typ = self.lib.findType(name)
        self.sim = self.typ.getSimulation('thermal_cycle', True)
        self.m = self.sim.getModel()

    def solve(self, solver='QRSlv', **changes):
        inputs = dict(PH=(self.m.PH, 'Pa', PSI), PL=(self.m.PL, 'Pa', PSI),
                      Tfeed=(self.m.Tfeed, 'K', 1), Tpurge=(self.m.Tpurge, 'K', 1),
                      Tref=(self.m.bed.Tref, 'K', 1), H=(self.m.bed.H, 'J/mol', 1),
                      scale=(self.m.bed.V, 'm^3', VBASE), beta=(self.m.beta, None, 1))
        for key, value in changes.items():
            instance, unit, factor = inputs[key]
            if unit:
                instance.setRealValueWithUnits(value*factor, unit)
            else:
                instance.setRealValue(value)
        self.sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=2e-7, abs_tol=2e-5),
                        f'{actual} != {expected}')

    def compare(self, **changes):
        ref = reference(**changes)
        for name in ('feed', 'product', 'recovery', 'wasteA', 'wasteH', 'heater', 'Qbed',
                     'Efeed', 'Eproduct', 'Ewaste', 'period', 'Fproduct'):
            with self.subTest(quantity=name):
                self.close(getattr(self.m, name).getRealValue(), ref[name])
        for name, instance in (('ads_feed', self.m.ads.feed), ('fr_feed', self.m.fr.feed),
                               ('gross', self.m.ads.product), ('purge', self.m.purge.hydrogen_in)):
            self.close(instance.getRealValue(), ref[name])
        pairs = [(self.m.state[key], expected) for key, expected in ref['states'].items()]
        pairs += [(self.m.hot, ref['hot']), (self.m.cold, ref['cold'])]
        for instance, expected in pairs:
            for field, value in expected.items():
                with self.subTest(state=str(instance.getName()), field=field):
                    self.close(getattr(instance, field).getRealValue(), value)

    def check_balances(self):
        def v(x): return x.getRealValue()
        m = self.m
        cpA, cpH, Tref = v(m.bed.cpA), v(m.bed.cpB), v(m.bed.Tref)
        hf = (v(m.y)*cpA+(1-v(m.y))*cpH)*(v(m.Tfeed)-Tref)
        hp = cpH*(v(m.state['pressurised'].T)-Tref)
        hpurge = cpH*(v(m.Tpurge)-Tref)
        def inventory(s): return v(s.sA)+v(s.gA), v(s.gH), v(s.U)
        start, mixed, blown, purged = (inventory(m.state[k]) for k in
                                      ('pressurised', 'mixed', 'blown_down', 'regenerated'))
        exit = tuple(a+b for a, b in zip(inventory(m.hot), inventory(m.cold)))
        operations = (
            (start, exit, (v(m.y)*v(m.ads.feed), (1-v(m.y))*v(m.ads.feed),
                           v(m.ads.feed)*hf+v(m.ads.Q)), (0, v(m.ads.product), v(m.ads.product)*hp)),
            (exit, mixed, (0, 0, 0), (0, 0, 0)),
            (mixed, blown, (0, 0, v(m.bd.Q)), (v(m.bd.wasteA), v(m.bd.wasteH), v(m.bd.Eout))),
            (blown, purged, (0, v(m.purge.hydrogen_in), v(m.purge.hydrogen_in)*hpurge+v(m.purge.Q)),
             (v(m.purge.wasteA), v(m.purge.wasteH), v(m.purge.Eout))),
            (purged, start, (v(m.y)*v(m.fr.feed), (1-v(m.y))*v(m.fr.feed),
                             v(m.fr.feed)*hf+v(m.fr.Q)), (0, 0, 0)))
        for before, after, incoming, outgoing in operations:
            for i, tol in enumerate((2e-5, 2e-5, .01)):
                self.assertAlmostEqual(before[i]+incoming[i], after[i]+outgoing[i], delta=tol)
        self.assertAlmostEqual(v(m.y)*v(m.feed), v(m.wasteA), delta=2e-5)
        self.assertAlmostEqual((1-v(m.y))*v(m.feed), v(m.product)+v(m.wasteH), delta=2e-5)
        # Independently construct plant energy, including the internal purge heater.
        heater = v(m.purge.hydrogen_in)*(hpurge-hp)
        heat = sum(v(op.Q) for op in (m.ads, m.bd, m.purge, m.fr))
        residual = v(m.feed)*hf+heater+heat-v(m.product)*hp-v(m.bd.Eout)-v(m.purge.Eout)
        self.assertAlmostEqual(residual, 0, delta=.02)

    def test_default_cycle_and_independent_marching_solution(self):
        self.solve()
        self.compare()
        self.check_balances()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName()) == 'self_test'))

    def test_dimensions(self):
        self.sim.checkDimensions()
        for instance, unit in ((self.m.bed.Csolid, 'J/K'), (self.m.bed.R, 'J/mol/K'),
                               (self.m.state['mixed'].U, 'J'), (self.m.purge.Q, 'J'),
                               (self.m.Fproduct, 'mol/s'), (self.m.period, 's')):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units(unit).getDimensions())

    def test_caloric_reference_invariance(self):
        self.solve()
        T, product = self.m.hot.T.getRealValue(), self.m.product.getRealValue()
        for Tref in (250, 350):
            self.solve(Tref=Tref)
            self.compare(Tref=Tref)
            self.check_balances()
            self.close(self.m.hot.T.getRealValue(), T)
            self.close(self.m.product.getRealValue(), product)

    def test_geometric_extensivity(self):
        self.solve()
        T, recovery = self.m.hot.T.getRealValue(), self.m.recovery.getRealValue()
        product, heat = self.m.product.getRealValue(), self.m.heater.getRealValue()
        for scale in (.5, 2):
            self.solve(scale=scale)
            self.compare(scale=scale)
            self.check_balances()
            self.close(self.m.hot.T.getRealValue(), T)
            self.close(self.m.recovery.getRealValue(), recovery)
            self.close(self.m.product.getRealValue(), product*scale)
            self.close(self.m.heater.getRealValue(), heat*scale)

    def test_zero_adsorption_heat_does_not_remove_filling_and_expansion_effects(self):
        self.solve(H=0, Tpurge=298)
        self.compare(H=0, Tpurge=298)
        self.check_balances()
        # H=0 is NOT an isothermal limit for a rigid-bed pressure cycle.
        self.assertGreater(abs(self.m.state['pressurised'].T.getRealValue()
                               - self.m.state['blown_down'].T.getRealValue()), 1)
        self.close(self.m.Qbed.getRealValue(), 0)

    def test_isothermal_limit_matches_existing_mass_cycle_with_thesis_void_charge(self):
        self.make('psa_thermal_cycle_isothermal')
        self.solve(Tfeed=299)
        self.compare(Tfeed=299, isothermal=True)
        self.check_balances()
        # Independently compare the earlier closed isothermal reconstruction.
        spec = importlib.util.spec_from_file_location('mass_cycle_test', ROOT/'models/psa/test/test_psa_cycle.py')
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        old = mod.reference()
        void = 15*PSI*VBASE*.44/(R*299)
        self.close(self.m.feed.getRealValue(), old['feed'])
        self.close(self.m.ads.product.getRealValue(), old['gross'])
        self.close(self.m.purge.hydrogen_in.getRealValue(), old['purge']+void)
        self.close(self.m.product.getRealValue(), old['product']-void)
        self.assertGreater(abs(self.m.Qbed.getRealValue()), 1000)

    def test_input_sensitivity_grid(self):
        for changes in (dict(PH=250), dict(PL=25), dict(Tfeed=308), dict(Tpurge=330),
                        dict(beta=0), dict(beta=1), dict(H=15000), dict(scale=2)):
            with self.subTest(**changes):
                self.make('psa_thermal_cycle')
                self.solve(**changes)
                self.compare(**changes)
                self.check_balances()

    def test_blowdown_exhaust_enthalpy_integral(self):
        self.solve()
        m = self.m
        def v(x): return x.getRealValue()
        start, end = m.state['mixed'], m.state['blown_down']
        ni, nf, Ti = v(start.n), v(end.n), v(start.T)
        x, S, cv = v(m.bd.x), v(m.bd.S), v(m.bd.cv)
        cp = x*v(m.bed.cpA)+(1-x)*v(m.bed.cpB)
        def h(n):
            T = Ti*((S+n*cv)/(S+ni*cv))**(R/cv)
            return cp*(T-v(m.bed.Tref))
        # Composite Simpson quadrature of enthalpy carried out, dn > 0.
        steps = 1000
        dn = (ni-nf)/steps
        integral = h(nf)+h(ni)
        integral += sum((4 if k % 2 else 2)*h(nf+k*dn) for k in range(1, steps))
        self.close(v(m.bd.Eout), integral*dn/3)

    def test_mixing_does_not_artificially_restore_adsorption_pressure(self):
        self.solve()
        self.assertGreater(abs(self.m.state['mixed'].P.getRealValue()-self.m.PH.getRealValue()), 100)
        self.assertGreater(self.m.state['mixed'].T.getRealValue(), self.m.cold.T.getRealValue())
        self.assertLess(self.m.state['mixed'].T.getRealValue(), self.m.hot.T.getRealValue())

    def test_ipopt_warm_start_at_neighbouring_pressure(self):
        try:
            self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError:
            self.skipTest('IPOPT not built')
        self.make('psa_thermal_cycle_ipopt_check')
        self.solve(PH=330)
        self.solve(solver='IPOPT', PH=355)
        self.compare()
        self.check_balances()


if __name__ == '__main__':
    unittest.main()
