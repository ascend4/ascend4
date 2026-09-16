#!/usr/bin/env python3
"""Journal-cycle tests: convergence/equation fidelity, NOT historical agreement."""
import importlib.util
from contextlib import redirect_stdout
from io import StringIO
import json
import itertools
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
R, PSI = 8.31446261815324, 6894.757293168
VBASE = math.pi*.45**2*2.24/4


def load_bounds():
    spec = importlib.util.spec_from_file_location('part1_heat_bounds', ROOT/'models/psa/psa_part1_bounds.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def root(f, lo, hi):
    fl = f(lo)
    if fl*f(hi) > 0:
        raise ValueError('Unbracketed reference root')
    for _ in range(85):
        mid = (lo+hi)/2
        if fl*f(mid) <= 0:
            hi = mid
        else:
            lo, fl = mid, f(mid)
    return (lo+hi)/2


def state(T, P, A, B, s, V, H=20920, Tref=298, share=1):
    C = V*.56*804000
    U = share*C*(T-Tref)+s*(36.8*(T-Tref)-H)
    U += A*(36.8*(T-Tref)-R*T)+B*(29.3*(T-Tref)-R*T)
    return dict(T=T, P=P, gA=A, gH=B, sA=s, n=A+B, U=U)


def blowdown(start, Pv, V, H=20920, Tref=298):
    cp, C = .05*36.8+.95*29.3, V*.56*804000
    Tg = start['T']*(Pv/start['P'])**(R/cp)
    nf = Pv*V*.44/(R*Tg)
    Tf = (C*start['T']+nf*cp*Tg)/(C+nf*cp)
    x = start['gA']/start['n']
    final = state(Tf, nf*R*Tf/(V*.44), x*nf, (1-x)*nf, start['sA'], V, H, Tref)
    k = R/(cp-R)
    Eout = (x*36.8+(1-x)*29.3)*(start['T']*start['n']/(k+1)
            *(1-(nf/start['n'])**(k+1))-Tref*(start['n']-nf))
    return final, Tg, Eout


def repressurise(start, Pv, V, Psource=300*PSI, Tfeed=298, H=20920, Tref=298):
    cp, C = .05*36.8+.95*29.3, V*.56*804000
    TB = start['T']*(Pv/start['P'])**(R/29.3)
    TF = Tfeed*(Pv/Psource)**(R/cp)
    feed = (Pv*V*.44/R-start['gH']*TB)/TF
    T = (C*start['T']+start['gH']*cp*TB+feed*cp*TF)/(C+(start['gH']+feed)*cp)
    B = start['gH']+.95*feed
    final = state(T, B*R*T/(V*.44), 0, B, start['sA']+.05*feed, V, H, Tref)
    return final, feed, TB, TF


def reference(valve=False, H=20920, Kmult=1, Psource=300, Tref=298, scale=1):
    V = VBASE*scale
    C, M, vg = V*.56*804000, V*.56*800, V*.44
    cp, phi = .05*36.8+.95*29.3, .75
    def props(T, P):
        qs = (-.76+40539/T)*1e-3/(R*273.15/101325)
        B = math.exp(-10.245+1756/T)
        a = B*.05*P/PSI
        return qs*a/(1+a), .016043*qs*B*R*T/PSI
    T0, s0, P0 = 298., 5*scale, 225*PSI
    for iteration in range(2000):
        start = state(T0, P0, 0, P0*vg/(R*T0), s0, V, H, Tref)
        def solid(T): return phi*.95*M*props(T, P0)[0]
        Ta = root(lambda T: phi*C*(T-T0)-solid(T)*H
                  -(solid(T)-s0)*36.8*(298-T)-.95*(solid(T)-s0)/.05*29.3*(298-T0), 220, 650)
        sa, na = solid(Ta), P0*phi*vg/(R*Ta)
        hot = state(Ta, P0, .05*na, .95*na, sa, V, H, Tref, phi)
        cold = state(T0, P0, 0, P0*(1-phi)*vg/(R*T0), 0, V, H, Tref, 1-phi)
        A, B = hot['gA'], hot['gH']+cold['gH']
        Tm = (hot['U']+cold['U']+(C+(sa+A)*36.8+B*29.3)*Tref+sa*H)
        Tm /= C+sa*36.8+A*(36.8-R)+B*(29.3-R)
        mixed = state(Tm, (A+B)*R*Tm/vg, A, B, sa, V, H, Tref)
        Pvb = 15*PSI if valve else root(lambda p: blowdown(mixed, p, V, H, Tref)[0]['P']-15*PSI,
                                       .1*PSI, 15*PSI)
        blown, Tg, Eb = blowdown(mixed, Pvb, V, H, Tref)
        qref = props(Ta, P0)[0]*.016043
        def yd(T): return .95**2*qref*R*T/(Kmult*props(T, P0)[1]*blown['P'])
        def supply(T): return sa*(1-yd(T))/yd(T)+blown['P']*vg/(R*T)
        Td = root(lambda T: C*(blown['T']-T)-sa*H-sa*36.8*(T-blown['T'])
                  -supply(T)*29.3*(T-350), 200, 500)
        purged = state(Td, blown['P'], 0, blown['P']*vg/(R*Td), 0, V, H, Tref)
        Pvr = 225*PSI if valve else root(lambda p: repressurise(purged, p, V, Psource*PSI, H=H, Tref=Tref)[0]['P']-225*PSI,
                                        blown['P'], Psource*PSI*3)
        finish, fr, TB, TF = repressurise(purged, Pvr, V, Psource*PSI, H=H, Tref=Tref)
        ads = (sa-s0)/.05+na
        gross = .95*(sa-s0)/.05+P0*phi*vg/(R*T0)
        purge = supply(Td)
        purgeA, purgeH = blown['gA']+sa, blown['gH']+purge-purged['gH']
        Ep = (purgeA*36.8+purgeH*29.3)*(Td-Tref)
        Ea_in, Ea_out = ads*cp*(298-Tref), gross*29.3*(T0-Tref)
        Er = fr*cp*(298-Tref)
        heater = purge*29.3*(350-T0)
        defects = dict(ads=hot['U']+cold['U']-start['U']-Ea_in+Ea_out,
                       bd=blown['U']-mixed['U']+Eb,
                       purge=purged['U']-blown['U']-purge*29.3*(350-Tref)+Ep,
                       fr=finish['U']-purged['U']-Er)
        result = dict(states=dict(pressurised=start, mixed=mixed, blown_down=blown, regenerated=purged),
                      hot=hot, cold=cold, ads_feed=ads, fr_feed=fr, gross=gross, purge=purge,
                      feed=ads+fr, product=gross-purge, recovery=(gross-purge)/(.95*(ads+fr)),
                      Pvb=Pvb, Pvr=Pvr, Tgas=Tg, TB=TB, TF=TF, Eb=Eb,
                      defects=defects, heater=heater,
                      energy_defect=(gross-purge)*29.3*(T0-Tref)+Eb+Ep-Ea_in-Er-heater)
        if abs(finish['T']-T0)<1e-10 and abs(finish['sA']-s0)<1e-10*scale and abs(finish['P']-P0)<1e-6:
            return result
        T0, s0, P0 = finish['T'], finish['sA'], finish['P']
    raise RuntimeError('Independent journal cycle failed to converge')


class PartISourceHeatBounds(unittest.TestCase):
    def setUp(self):
        self.bounds = load_bounds()
        self.inputs = dict(d=.45, L=2.24, P=225*PSI, ads_feed=1610., fr_feed=95.6, gross_H2=1533.)

    @staticmethod
    def inverse(d,L,P,ads_feed,fr_feed,gross_H2):
        """Independent exact-point back-substitution, not interval arithmetic."""
        V=math.pi*d*d*L/4
        M,C=V*.56*800,V*.56*804000
        def q(T):
            a=math.exp(-10.245+1756/T)*.05*P/PSI
            return (-.76+40539/T)*a/(1+a)*1e-3/(R*273.15/101325)
        Ta=root(lambda T: .75*.95*M*q(T)-.05*fr_feed-.05*(ads_feed-P*.75*V*.44/(R*T)),220,650)
        s=.75*.95*M*q(Ta)
        T0=P*.75*V*.44/(R*(gross_H2-.95*(s-.05*fr_feed)/.05))
        heat=(.75*C*(Ta-T0)-(s-.05*fr_feed)*36.8*(298-Ta)
              -.95*(s-.05*fr_feed)/.05*29.3*(298-T0))/s
        return dict(adsorption_final_K=Ta,adsorption_initial_K=T0,heat_J_mol=heat)

    def test_exact_point_and_conservative_rounding_enclosure(self):
        check=self.bounds.zero_pe_heat_check(**self.inputs)
        exact=self.inverse(**self.inputs)
        for key,value in exact.items():
            self.assertAlmostEqual(check['centre'][key],value,delta=1e-5)
        self.assertAlmostEqual(exact['heat_J_mol'],1118.02657694,delta=1e-5)
        enclosure=check['rounding_enclosure']['heat_J_mol']
        self.assertLess(enclosure[0],0)  # Do not truncate an inconsistent inference to physical bounds.
        self.assertGreater(enclosure[1],14000)
        self.assertLess(enclosure[1],15000)
        self.assertTrue(check['reference_heat_excluded'])
        self.assertIn('NOT sufficient',check['scope'])

    def test_rounding_corners_and_interior_are_enclosed(self):
        # This verifies implementation; the enclosure argument is monotonic
        # root bounds + interval arithmetic, NOT a claim that corners suffice.
        check=self.bounds.zero_pe_heat_check(**self.inputs)['rounding_enclosure']
        half=dict(d=.005,L=.005,P=.05*PSI,ads_feed=.5,fr_feed=.05,gross_H2=.5)
        for offsets in itertools.product((-1,0,1),repeat=6):
            inputs={key:value+offset*half[key] for (key,value),offset in zip(self.inputs.items(),offsets)}
            point=self.inverse(**inputs)
            for key,value in point.items():
                self.assertLessEqual(check[key][0],value)
                self.assertGreaterEqual(check[key][1],value)

    def test_invalid_source_and_interval_division(self):
        for value in (0,-1,math.nan,math.inf,.001):
            with self.assertRaises(ValueError):
                self.bounds.zero_pe_heat_check(**dict(self.inputs,d=value))
        with self.assertRaises(ValueError): self.bounds._div((1,2),(-1,1))
        with self.assertRaises(ValueError):
            self.bounds.zero_pe_heat_check(**dict(self.inputs,ads_feed=1e9))


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class PartICycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(ROOT/'models/psa/test/part1_cycle.a4c'))

    def setUp(self):
        self.make('psa_part1_cycle')

    def make(self, name):
        self.typ = self.lib.findType(name)
        self.sim = self.typ.getSimulation('part1_cycle_check', True)
        self.m = self.sim.getModel()

    def solve(self, solver='QRSlv'):
        self.sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
        self.assertTrue(self.sim.getStatus().isConverged())

    def close(self, actual, expected):
        self.assertTrue(math.isclose(actual, expected, rel_tol=2e-7, abs_tol=3e-5), f'{actual} != {expected}')

    def compare(self, **changes):
        r = reference(**changes)
        values = dict(ads_feed=self.m.ads.feed, fr_feed=self.m.fr.feed, gross=self.m.ads.product,
                      purge=self.m.des.hydrogen_in, Pvb=self.m.bd.Pvalve, Pvr=self.m.fr.Pvalve,
                      Tgas=self.m.bd.Tgas, TB=self.m.fr.TB, TF=self.m.fr.TF, Eb=self.m.bd.Eout)
        for name in ('feed','product','recovery','energy_defect','heater'):
            values[name] = getattr(self.m, name)
        for name, instance in values.items():
            with self.subTest(quantity=name): self.close(instance.getRealValue(), r[name])
        for name, instance in dict(ads=self.m.ads_defect, bd=self.m.bd.energy_defect,
                                   purge=self.m.purge_defect, fr=self.m.fr.energy_defect).items():
            self.close(instance.getRealValue(), r['defects'][name])
        for instance, expected in ([(self.m.state[k], s) for k,s in r['states'].items()]
                                   + [(self.m.hot,r['hot']), (self.m.cold,r['cold'])]):
            for field, value in expected.items():
                self.close(getattr(instance,field).getRealValue(),value)

    def check_balances(self):
        def v(x): return x.getRealValue()
        m=self.m
        self.close(v(m.y)*v(m.feed),v(m.wasteA))
        self.close((1-v(m.y))*v(m.feed),v(m.product)+v(m.wasteH))
        self.close(v(m.fr.feed), (v(m.state['pressurised'].n)-v(m.state['regenerated'].n))/(1-v(m.y)))
        self.close(v(m.ads.sA0),v(m.des.sAe)+v(m.y)*v(m.fr.feed))
        self.close(v(m.energy_defect),v(m.ads_defect)+v(m.bd.energy_defect)+v(m.purge_defect)+v(m.fr.energy_defect))
        self.assertGreater(abs(v(m.energy_defect)), 1e4)  # This is not an energy-conserving reconstruction.

    def test_default_closed_cycle(self):
        self.solve(); self.compare(); self.check_balances()
        self.sim.run(next(m for m in self.typ.getMethods() if str(m.getName())=='self_test'))
        self.assertLess(self.m.bd.Pvalve.getRealValue(),101325)
        self.assertGreater(self.m.ads.sA0.getRealValue(),0)

    def test_valve_pressure_interpretation(self):
        self.make('psa_part1_cycle_valve')
        self.solve(); self.compare(valve=True); self.check_balances()
        self.close(self.m.bd.Pvalve.getRealValue(),15*PSI)
        self.assertGreater(self.m.des.P.getRealValue(),15*PSI)
        self.assertNotAlmostEqual(self.m.ads.P.getRealValue()/PSI,225,places=1)

    def test_standalone_blowdown(self):
        self.make('psa_part1_blowdown_check')
        for Pv in (15,60,120,225):
            self.m.bd.Pvalve.setRealValueWithUnits(Pv*PSI,'Pa'); self.solve()
            s=state(310,225*PSI,.05*225*PSI*.35*.44/(R*310),.95*225*PSI*.35*.44/(R*310),60,.35)
            expected,Tg,E=blowdown(s,Pv*PSI,.35)
            for key,value in expected.items(): self.close(getattr(self.m.finish,key).getRealValue(),value)
            self.close(self.m.bd.Tgas.getRealValue(),Tg); self.close(self.m.bd.Eout.getRealValue(),E)

    def test_standalone_repressurisation(self):
        self.make('psa_part1_repressurisation_check')
        for Pv in (100,175,225,15):
            self.m.fr.Pvalve.setRealValueWithUnits(Pv*PSI,'Pa'); self.solve()
            s=state(290,15*PSI,0,15*PSI*.35*.44/(R*290),0,.35)
            expected,feed,TB,TF=repressurise(s,Pv*PSI,.35)
            for key,value in expected.items(): self.close(getattr(self.m.finish,key).getRealValue(),value)
            for instance,value in ((self.m.fr.feed,feed),(self.m.fr.TB,TB),(self.m.fr.TF,TF)):
                self.close(instance.getRealValue(),value)

    def test_reference_temperature_invariance(self):
        self.solve(); original=self.m.energy_defect.getRealValue()
        self.m.bed.Tref.setRealValueWithUnits(350,'K')
        self.solve(); self.compare(Tref=350); self.check_balances()
        self.close(self.m.energy_defect.getRealValue(),original)

    def test_explicit_assumption_sensitivity(self):
        for key, value, unit, name, kwargs in (
            ('Psource',250*PSI,'Pa',None,dict(Psource=250)),
            ('K_multiplier',1.2,None,None,dict(Kmult=1.2)),
            ('H',18000,'J/mol','bed',dict(H=18000)),
            ('V',2*VBASE,'m^3','bed',dict(scale=2))):
            with self.subTest(**kwargs):
                self.make('psa_part1_cycle')
                self.solve()  # Continue from the baseline; avoid claiming cold-start robustness.
                inst=getattr(getattr(self.m,name) if name else self.m,key)
                if unit: inst.setRealValueWithUnits(value,unit)
                else: inst.setRealValue(value)
                self.solve(); self.compare(**kwargs); self.check_balances()

    def test_dimensions(self):
        self.sim.checkDimensions()
        for inst,unit in ((self.m.bd.Pvalve,'Pa'),(self.m.fr.TF,'K'),(self.m.energy_defect,'J'),
                          (self.m.des.K,'m^3/mol'),(self.m.fr.feed,'mol')):
            self.assertEqual(inst.getDimensions(),self.ascpy.Units(unit).getDimensions())

    def test_inverse_diagnostics_against_independent_cycle(self):
        for method,two_targets in (('fit_purge',False),('fit_adsorption_purge',True)):
            with self.subTest(method=method):
                self.make('psa_part1_cycle'); self.solve()
                self.sim.run(next(x for x in self.typ.getMethods() if str(x.getName())==method))
                self.solve(); self.sim.checkDimensions(); self.check_balances()
                H=self.m.bed.H.getRealValue(); K=self.m.K_multiplier.getRealValue()
                self.compare(H=H,Kmult=K)
                self.close(self.m.des.hydrogen_in.getRealValue(),47.7)
                self.assertGreater(abs(self.m.fr.feed.getRealValue()-95.6),.05)
                self.assertGreater(abs(self.m.ads.product.getRealValue()-1533),.5)
                if two_targets:
                    self.close(self.m.ads.feed.getRealValue(),1610)
                    self.assertTrue(3500 < H < 3550)
                    self.assertTrue(.85 < K < .86)
                else:
                    self.close(H,20920)
                    self.assertTrue(.76 < K < .77)
                    self.assertLess(self.m.ads.feed.getRealValue(),1400)
                self.sim.run(next(x for x in self.typ.getMethods() if str(x.getName())=='on_load'))
                self.solve(); self.compare()

    def test_bounds_LRC_matches_ASCEND_and_fits_are_not_baseline(self):
        self.solve()
        carbon=self.m.ads.carbon
        self.close(load_bounds().loading(carbon.T.getRealValue(),self.m.ads.P.getRealValue()),
                   carbon.q.getRealValue())
        spec=importlib.util.spec_from_file_location('inverse_part1_report',ROOT/'models/psa/psa_part1_cycle.py')
        driver=importlib.util.module_from_spec(spec); spec.loader.exec_module(driver)
        for mode in ('purge','adsorption-purge'):
            result=driver.solve(fit=mode)
            self.assertEqual(result['calibration']['mode'],mode)
            self.assertEqual(result['amount_comparison']['purge_H2']['role'],'fitted target')
            self.assertEqual(result['amount_comparison']['fr_feed']['role'],'independent check')
            self.assertFalse(result['amount_comparison']['fr_feed']['within_printed_rounding'])
            self.assertTrue(result['source_adsorption_heat_check']['reference_heat_excluded'])
            output=StringIO()
            with redirect_stdout(output): driver.report(result)
            self.assertIn('INVERSE DIAGNOSTIC',output.getvalue())
            self.assertIn('held-out',output.getvalue())
            self.assertIn('Not a feasible heat range',output.getvalue())
            self.assertEqual(json.loads(json.dumps(result,allow_nan=False))['calibration']['mode'],mode)
        baseline=driver.solve()
        self.assertEqual(baseline['calibration']['fitted_amounts'],[])
        self.close(baseline['assumptions']['K_multiplier'],1)
        self.close(baseline['assumptions']['adsorption_heat_J_mol'],20920)
        self.close(baseline['amounts_mol']['ads_feed'],1310.48335058)

    def test_valve_fit_negative_heat_is_exposed(self):
        spec=importlib.util.spec_from_file_location('valve_inverse_report',ROOT/'models/psa/psa_part1_cycle.py')
        driver=importlib.util.module_from_spec(spec); spec.loader.exec_module(driver)
        result=driver.solve(pressure_basis='valve',fit='adsorption-purge')
        self.assertLess(result['assumptions']['adsorption_heat_J_mol'],0)
        self.assertTrue(any('NEGATIVE' in text for text in result['warnings']))
        expected=reference(valve=True,H=result['assumptions']['adsorption_heat_J_mol'],
                           Kmult=result['assumptions']['K_multiplier'])
        self.close(result['amounts_mol']['ads_feed'],expected['ads_feed'])
        self.close(result['amounts_mol']['fr_feed'],expected['fr_feed'])
        self.close(result['amounts_mol']['purge_H2'],expected['purge'])

    def test_report_keeps_warnings_and_diagnostics(self):
        spec=importlib.util.spec_from_file_location('closed_part1_report',ROOT/'models/psa/psa_part1_cycle.py')
        driver=importlib.util.module_from_spec(spec); spec.loader.exec_module(driver)
        result=driver.solve()
        self.close(result['amounts_mol']['fr_feed'],95.6534687717)
        self.assertEqual(result['published_amounts_mol']['fr_feed'],95.6)
        self.assertTrue(any('below atmospheric' in warning for warning in result['warnings']))
        self.assertTrue(any('energy-accounting defect' in warning for warning in result['warnings']))
        self.assertLess(abs(result['energy_defect_sum_error_J']),.01)
        output=StringIO()
        with redirect_stdout(output): driver.report(result)
        self.assertIn('NOT compensating heat duties',output.getvalue())
        self.assertIn('unsourced',output.getvalue())
        self.assertIn('WARNING',output.getvalue())
        self.assertEqual(json.loads(json.dumps(result,allow_nan=False))['pressure_basis'],'reconciled')
        for kwargs in (dict(source_pressure=0),dict(K_multiplier=-1),dict(adsorption_heat=math.nan),
                       dict(pressure_basis='unknown'),dict(fit='unknown')):
            with self.assertRaises(ValueError): driver.solve(**kwargs)

    def test_ipopt_neighbouring_K(self):
        try: self.ascpy.Solver('IPOPT').getIndex()
        except RuntimeError: self.skipTest('IPOPT not built')
        self.make('psa_part1_cycle_ipopt_check')
        self.m.K_multiplier.setRealValue(1.01); self.solve()
        self.m.K_multiplier.setRealValue(1); self.solve('IPOPT')
        self.compare(); self.check_balances()


if __name__=='__main__': unittest.main()
