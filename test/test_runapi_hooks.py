#!/usr/bin/env python3
"""METHOD configuration and time units across native and CLI integration.

Run with ./a4 pytest test/test_runapi_hooks.py after building ascpy and IDA.
"""
import importlib.util
import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
MODEL = ROOT / 'models/test/ida/runapi_hooks.a4c'


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class RunApiHooks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(MODEL))
        if 'IDA' not in ascpy.Integrator.getEngines():
            raise unittest.SkipTest('IDA is not built')

    def setUp(self):
        self.manager = self.ascpy.SolverHooksManager.Instance()
        self.old_hooks = self.manager.getHooks()
        self.simulations = []
        self.unit_overrides = []

    def tearDown(self):
        self.manager.setHooks(self.old_hooks)
        for inst in self.unit_overrides:
            inst.clearDisplayUnitsOverride()
        for sim in self.simulations:
            sim.invalidateSystem()

    def display_minutes(self, sim):
        inst = sim.getModel().t
        inst.setDisplayUnitsOverride('min')
        self.unit_overrides.append(inst)

    def simulation(self, run=True):
        sim = self.lib.findType('runapi_hooks').getSimulation('hook_case', run)
        self.simulations.append(sim)
        return sim

    def run_method(self, sim, name):
        sim.run(sim.getType().getMethod(name))

    def assert_options(self, integ, rtol=3e-7):
        self.assertEqual(integ.getParameterValue('rtol'), rtol)
        self.assertEqual(integ.getParameterValue('atol'), 7e-9)
        self.assertFalse(integ.getParameterValue('atolvect'))
        self.assertEqual(integ.getParameterValue('maxord'), 2)
        self.assertEqual(integ.getParameterValue('linsolver'), 'DENSE')

    def test_separate_methods_preserve_observations_focus_and_options(self):
        sim = self.simulation()
        self.run_method(sim, 'update_options')
        hooks = sim.getSolverHooks()
        self.assertEqual(len(hooks.getObservedVars(sim)), 1)
        self.assertEqual(len(hooks.getStudyPrintVars(sim)), 1)
        self.assertEqual(hooks.getIntegratorName(sim), 'IDA')
        integ = self.ascpy.Integrator(sim)
        integ.setEngine('IDA')
        hooks.applyIntegratorOptions(sim, integ)
        self.assert_options(integ, rtol=2e-7)
        del integ

        # A new instance (even with the same name) must not inherit settings.
        fresh = self.simulation(run=False)
        hooks.assign(fresh)
        self.assertEqual(hooks.getIntegratorName(fresh), '')
        self.assertEqual(len(hooks.getObservedVars(fresh)), 0)

    def test_engine_override_does_not_replay_other_engine_options(self):
        if 'LSODE' not in self.ascpy.Integrator.getEngines():
            self.skipTest('LSODE is not built')
        sim = self.simulation()
        hooks = sim.getSolverHooks()
        integ = self.ascpy.Integrator(sim)
        integ.setEngine('LSODE')
        default_rtol = integ.getParameterValue('rtol')
        hooks.applyIntegratorOptions(sim, integ)
        self.assertEqual(integ.getParameterValue('rtol'), default_rtol)
        self.run_method(sim, 'select_lsode')
        hooks.applyIntegratorOptions(sim, integ)
        self.assertEqual(integ.getParameterValue('rtol'), default_rtol)
        del integ

    def test_native_method_uses_base_times_with_minute_display(self):
        sim = self.simulation()
        self.display_minutes(sim)
        self.run_method(sim, 'integrate_case')
        self.assertAlmostEqual(sim.getModel().t.getRealValue(), 180)
        self.assertAlmostEqual(sim.getModel().y.getRealValue(), 60, places=5)

    def test_timestep_setters_convert_minutes(self):
        for setter in ('setLinearTimesteps', 'setLogTimesteps'):
            with self.subTest(setter=setter):
                sim = self.simulation()
                integ = self.ascpy.Integrator(sim)
                integ.setEngine('IDA')
                integ.addObservedInstance(sim.getModel().y)
                reporter = self.ascpy.IntegratorReporterNull(integ)
                integ.setReporter(reporter)
                # Exercise replacement of the sample list as well.
                integ.setLinearTimesteps(self.ascpy.Units('s'), 120, 180, 4)
                for start, stop, steps in ((2, 3, 0), (3, 2, 4), (2, float('nan'), 4)):
                    with self.assertRaises(RuntimeError):
                        getattr(integ, setter)(self.ascpy.Units('min'), start, stop, steps)
                getattr(integ, setter)(self.ascpy.Units('min'), 2, 3, 4)
                integ.analyse()
                integ.solve()
                self.assertAlmostEqual(sim.getModel().t.getRealValue(), 180)
                self.assertAlmostEqual(sim.getModel().y.getRealValue(), 60, places=5)
                del reporter, integ

    def test_initial_equations_see_requested_start(self):
        for engine in ('IDA', 'LSODE'):
            if engine not in self.ascpy.Integrator.getEngines():
                continue
            with self.subTest(engine=engine):
                sim = self.lib.findType('runapi_initial_time').getSimulation('initial_time', True)
                self.simulations.append(sim)
                integ = self.ascpy.Integrator(sim)
                integ.setEngine(engine)
                integ.addObservedInstance(sim.getModel().y)
                reporter = self.ascpy.IntegratorReporterNull(integ)
                integ.setReporter(reporter)
                integ.setLinearTimesteps(self.ascpy.Units('min'), 2, 3, 4)
                integ.analyse()
                integ.solve()
                self.assertAlmostEqual(sim.getModel().y.getRealValue(), 180, places=5)
                del reporter, integ

    def test_cli_method_integration_preserves_settings(self):
        import runmodel
        cli = runmodel.CliSolverHooks(self.ascpy)
        self.manager.setHooks(cli.hooks)
        sim = self.simulation()
        self.display_minutes(sim)
        real_reporter = runmodel.CliIntegratorReporter

        def reporter(ascpy, sim, integ, **kwargs):
            self.assert_options(integ)
            return real_reporter(ascpy, sim, integ, **kwargs)

        with patch.object(runmodel, 'CliIntegratorReporter', side_effect=reporter), \
                patch.object(runmodel, '_print_table') as table:
            self.run_method(sim, 'integrate_case')
        self.assertTrue(cli.did_integrate(sim))
        rows = table.call_args.args[1]
        self.assertAlmostEqual(rows[0][0], 2)
        self.assertAlmostEqual(rows[-1][0], 3)
        self.assertAlmostEqual(rows[-1][1], 60, places=5)

    def test_forced_cli_inherits_and_overrides_bounds_independently(self):
        # METHOD supplies 120..180 s; only explicitly supplied CLI values
        # are interpreted in --units. Include zero to guard against truth tests.
        cases = [
            ({'units': 'min'}, 120, 180),
            ({'units': 'min', 'start': 0}, 0, 60),
            ({'units': 'min', 'duration': 2}, 120, 240),
            ({'units': 'min', 'start': 1, 'duration': 2}, 60, 180),
        ]
        for options, start, stop in cases:
            with self.subTest(options=options), tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / 'trajectory.tsv'
                command = [sys.executable, str(ROOT / 'ascxx/runmodel.py'), str(MODEL),
                           '--model', 'runapi_hooks_auto', '--integrate', '--no-test',
                           '--output', str(output)]
                for key, value in options.items():
                    command.extend(['--' + key, str(value)])
                completed = subprocess.run(command, capture_output=True, text=True, timeout=30)
                self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
                with output.open() as stream:
                    headers, *rows = csv.reader(stream, delimiter='\t')
                time_units = headers[0].split('[')[1].rstrip(']')
                conversion = self.ascpy.Units(time_units).getConversion()
                self.assertEqual(len(rows), 5)
                self.assertAlmostEqual(float(rows[0][0]) * conversion, start)
                self.assertAlmostEqual(float(rows[-1][0]) * conversion, stop)
                self.assertAlmostEqual(float(rows[-1][1]), stop-start, places=5)

    def test_restoring_default_hooks_after_temporary_cli_hooks(self):
        import runmodel
        cli = runmodel.CliSolverHooks(self.ascpy)
        self.manager.setHooks(cli.hooks)
        self.manager.setHooks(self.old_hooks)
        sim = self.simulation()
        self.run_method(sim, 'update_options')
        self.assertEqual(sim.getSolverHooks().getIntegratorName(sim), 'IDA')

    def test_explicit_integration_uses_method_engine_without_request(self):
        import runmodel
        if 'LSODE' not in self.ascpy.Integrator.getEngines():
            self.skipTest('LSODE is not built')
        sim = self.simulation()
        self.run_method(sim, 'select_lsode')
        real_reporter = runmodel.CliIntegratorReporter

        def reporter(ascpy, sim, integ, **kwargs):
            self.assertEqual(integ.getName(), 'LSODE')
            return real_reporter(ascpy, sim, integ, **kwargs)

        with patch.object(runmodel, 'CliIntegratorReporter', side_effect=reporter), \
                patch.object(runmodel, '_print_table'):
            runmodel._run_integration(self.ascpy, sim, engine=None, start=2,
                                     duration=1, steps=4, units_token='min',
                                     output=None, plot=False, microstates='endpoints')
        self.assertAlmostEqual(sim.getModel().t.getRealValue(), 180)


if __name__ == '__main__':
    unittest.main()
