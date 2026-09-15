#!/usr/bin/env python3
"""Conservative Part I alternatives: no assertion of a historical optimum."""
import importlib.util
import json
import math
from pathlib import Path
from contextlib import redirect_stdout
from io import StringIO
import unittest

ROOT=Path(__file__).resolve().parents[3]


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result


@unittest.skipUnless(importlib.util.find_spec('ascpy'),'Run with ./a4 and built ascpy')
class PartIPhysical(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.driver=module('physical_report',ROOT/'models/psa/psa_part1_physical.py')
        cls.reference=staticmethod(module('thermal_reference',ROOT/'models/psa/test/test_psa_thermal_cycle.py').reference)

    def close(self,a,b,atol=3e-5):
        self.assertTrue(math.isclose(a,b,rel_tol=2e-7,abs_tol=atol),f'{a} != {b}')

    def compare(self,case='adiabatic',extra_capacity=0,beta=.5):
        r=self.driver.solve(case,extra_capacity,beta)
        reference=self.reference(PH=225,volume=math.pi*.45**2*2.24/4,eta=.95,regenerated=1,
                                 Ffeed=31.1,purge_rule=dict(adiabatic='lrc',henry='henry',
                                 ratio='ratio',isothermal='lrc')[case],isothermal=case=='isothermal',
                                 Cv=804000*(1+extra_capacity),beta=beta)
        for key,refkey in dict(ads_feed='ads_feed',fr_feed='fr_feed',gross_H2='gross',purge_H2='purge').items():
            self.close(r['amounts_mol'][key],reference[refkey])
        for name,state in r['states_SI'].items():
            for key,value in state.items(): self.close(value,reference['states'][name][key])
        for key,value in r['operation_heat_J'].items(): self.close(value,reference['operation_heat'][key],.01)
        self.close(r['recovery'],reference['recovery'])
        self.close(r['period_s'],reference['period'])
        for error in r['component_residuals_mol'].values(): self.assertLess(abs(error),1e-5)
        self.assertLess(abs(r['energy_residual_J']),.01)
        self.assertEqual(r['fitted_parameters'],[])
        self.close(r['adsorption_heat_J_mol'],20920)
        self.assertTrue(0 < r['effective_sweep_CH4_fraction'] < 1)
        self.assertTrue(0 < r['average_exhaust_CH4_fraction'] < 1)
        self.assertGreaterEqual(r['states_SI']['blown_down']['P'],101325)
        self.assertGreater(min(s['T'] for s in r['states_SI'].values()),250)
        return r

    def test_adiabatic_full_LRC(self):
        r=self.compare()
        self.assertTrue(1300 < r['amounts_mol']['ads_feed'] < 1400)
        self.assertLess(max(abs(q) for q in r['operation_heat_J'].values()),.01)

    def test_alternative_purge_rules(self):
        henry=self.compare('henry'); ratio=self.compare('ratio'); lrc=self.driver.solve()
        self.assertLess(lrc['amounts_mol']['purge_H2'],henry['amounts_mol']['purge_H2'])
        self.close(ratio['purge_ratio'],1.25)
        self.assertNotAlmostEqual(lrc['effective_sweep_CH4_fraction'],lrc['average_exhaust_CH4_fraction'])

    def test_buffered_and_exhaust_temperature_sensitivities(self):
        for capacity,beta in ((.5,.5),(0,0),(0,1)):
            with self.subTest(capacity=capacity,beta=beta): self.compare(extra_capacity=capacity,beta=beta)

    def test_isothermal_duties_are_not_hidden(self):
        r=self.compare('isothermal')
        self.assertLess(abs(r['relative_amount_errors']['ads_feed']),.07)
        self.assertLess(abs(r['relative_amount_errors']['purge_H2']),.05)
        self.assertGreater(r['mean_external_heating_W'],35000)
        self.assertGreater(r['mean_external_cooling_W'],35000)
        self.close(r['mean_external_heating_W'],r['mean_external_cooling_W'],.01)
        self.assertLess(r['operation_heat_J']['adsorption'],-1e6)
        self.assertGreater(r['operation_heat_J']['purge'],1e6)
        for s in r['states_SI'].values(): self.close(s['T'],298)

    def test_reporting_and_invalid_inputs(self):
        r=self.driver.comparison()
        self.assertEqual(len(r['alternatives']),5)
        self.assertEqual(json.loads(json.dumps(r,allow_nan=False))['literal']['calibration']['mode'],'none')
        stream=StringIO()
        with redirect_stdout(stream): self.driver.report(r)
        for phrase in ('NO FITTED PARAMETERS','Not yet the economic','actively thermostatted','heating/cooling'):
            self.assertIn(phrase,stream.getvalue())
        for kwargs in (dict(case='bad'),dict(extra_capacity=-1),dict(extra_capacity=math.inf),
                       dict(beta=math.nan),dict(beta=2)):
            with self.assertRaises(ValueError): self.driver.solve(**kwargs)


if __name__=='__main__': unittest.main()
