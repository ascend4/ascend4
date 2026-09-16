#!/usr/bin/env python3
"""Published cyclic scheduling minima, solver parity and periodic event checks.

./a4 script models/psa/test/test_psa_scheduling.py
HiGHS is required. Gurobi is exercised whenever built, with its normal licence.
"""
from copy import deepcopy
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace
import runpy

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('psa_scheduling_reporter',
                                             ROOT/'models/psa/psa_scheduling.py')
reporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reporter)

# Smith (1991), sections 4.5, 4.7, and Table 7-2; these are assertions,
# never model inputs. Table 7-2 uses zeros for ABSENT pairs, omitted here.
PUBLISHED = dict(oxy=(2, [1]), oxy_continuous=(4, [2]),
                 seven=(4, [2, 1]), seven_purge=(4, [2, 1]),
                 pe0=(2, []), pe1=(3, [1]), pe2=(4, [2, 1]), pe3=(5, [3, 2, 1]))


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run using ./a4 and built ascpy')
class CyclicScheduling(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.ascpy = ascpy
        # Library initialisation registers solver plugins, before getIndex().
        cls.library = ascpy.Library()

    def available(self, solver):
        try:
            self.ascpy.Solver(solver).getIndex()
        except RuntimeError:
            if solver == 'HiGHS':
                raise
            self.skipTest(f'{solver} not built')

    def compare(self, r, case):
        N, J = PUBLISHED[case]
        self.assertEqual(r['N'], N)
        self.assertEqual([round(p['J']) for p in r['pairs']], J)
        reporter.validate(r)
        self.assertIsNotNone(r['primary_bound'])
        self.assertAlmostEqual(r['primary_bound'], N, places=6)
        if case == 'oxy_continuous':
            self.assertAlmostEqual(r['Jfeed'], 1)
        if case.startswith('oxy'):
            self.assertGreaterEqual(r['period'], 240-1e-6)

    def published_cases(self, solver):
        self.available(solver)
        for case in PUBLISHED:
            with self.subTest(case=case, solver=solver):
                r = reporter.solve(case, solver)
                self.compare(r, case)
                self.assertAlmostEqual(min(o['p'] for o in r['operations'].values()),
                                       1 if case == 'oxy_continuous' else .5, places=6)

    def test_published_highs(self):
        self.published_cases('HiGHS')

    def test_published_gurobi(self):
        self.published_cases('Gurobi')

    def insufficient_beds(self, solver):
        self.available(solver)
        for case, (N, _) in PUBLISHED.items():
            with self.subTest(case=case, solver=solver):
                sim = reporter.prepare(case, solver, max_beds=N-1)
                try:
                    sim.solve(self.ascpy.Solver(solver), self.ascpy.SolverReporter())
                except RuntimeError:
                    pass  # Accept only the solver's explicit infeasible status below.
                status = sim.getStatus()
                self.assertFalse(status.isConverged())
                self.assertTrue(status.isInconsistent(), 'Must prove infeasibility, not just fail')
                self.assertFalse(status.hasExceededTimeLimit())
                self.assertFalse(status.hasResidualCalculationErrors())

    def test_insufficient_beds_highs(self):
        self.insufficient_beds('HiGHS')

    def test_insufficient_beds_gurobi(self):
        self.insufficient_beds('Gurobi')

    def test_epsilon_and_search_bound_sensitivity(self):
        for case in PUBLISHED:
            for eps in (.001, .1):
                with self.subTest(case=case, epsilon=eps):
                    r = reporter.solve(case, epsilon=eps, max_beds=12)
                    self.compare(r, case)

    def test_primary_schedule_without_tie_break(self):
        for case in PUBLISHED:
            with self.subTest(case=case):
                self.compare(reporter.solve(case, balance=False), case)

    def test_physical_dimensions_and_time_scale(self):
        sim = reporter.prepare()
        s = sim.getModel().schedule
        for instance in (s.D, s.period, s.slot[1].start, s.slot[1].finish,
                         s.slot[1].processing, s.slot[1].standby):
            self.assertEqual(instance.getDimensions(), self.ascpy.Units('s').getDimensions())
        for shift in (30, 120):
            r = reporter.solve(shift=shift)
            self.compare(r, 'pe3')
            self.assertAlmostEqual(r['period'], 5*shift)
            self.assertAlmostEqual(r['operations'][2]['processing'], shift/2)

    def test_reject_invalid_inputs(self):
        for kw in (dict(epsilon=0), dict(epsilon=.5), dict(epsilon=float('nan')),
                   dict(max_beds=0), dict(max_beds=2.5), dict(shift=-1),
                   dict(shift=float('inf')), dict(case='unknown'), dict(solver='QRSlv')):
            with self.subTest(options=kw), self.assertRaises(ValueError):
                reporter.prepare(**kw)

    def test_event_checker_rejects_corruption(self):
        original = reporter.solve()
        mutations = (
            lambda r: r.update(N=4.5),
            lambda r: r.update(period=299),
            lambda r: r['pairs'][0].update(J=0),
            lambda r: r['pairs'][0].update(J=2),
            lambda r: r['operations'][3].update(a=2),
            lambda r: r['operations'][3].update(p=float('nan')),
            lambda r: r['operations'][2].update(processing=29),
            lambda r: r.update(continuous=[2]),  # Half-shift steps leave real product gaps.
            lambda r: r.update(compressor=True, Jfeed=2),
            lambda r: r.update(ordering=[(2, 1)]),
        )
        for i, mutate in enumerate(mutations):
            r = deepcopy(original)
            mutate(r)
            with self.subTest(mutation=i), self.assertRaises(ValueError):
                reporter.validate(r)

    def test_periodic_wrap_and_standby(self):
        self.assertEqual(list(reporter.segments(4.75, .5, 5)), [(4.75, 5), (0., .25)])
        r = reporter.solve()
        # Construct real standby without relying on a solver's arbitrary
        # primary-optimum timings: shorten BOTH partners, keeping allocations.
        for k in (r['pairs'][0]['donor'], r['pairs'][0]['receiver']):
            op = r['operations'][k]
            op['s'] += op['p']/2
            op['p'] /= 2
            op['processing'] = op['p']*r['D']
            op['standby'] = op['s']*r['D']
        events = reporter.validate(r)
        self.assertTrue(any(e[4] for e in events))
        # Every lane includes one period of processing plus standby.
        for bed in range(r['N']):
            self.assertAlmostEqual(sum(b-a for a, b, j, _, _ in events if j == bed), r['N'])

    def test_extpy_registration_does_not_run_cli(self):
        registered = []
        with patch.dict('sys.modules', extpy=SimpleNamespace(registermethod=registered.append)):
            runpy.run_path(str(ROOT/'models/psa/psa_scheduling.py'), run_name='__main__')
        self.assertEqual([fn.__name__ for fn in registered], ['psa_scheduling_gantt'])

    def test_gui_refuses_dirty_schedule(self):
        browser = SimpleNamespace(sim=SimpleNamespace(isSolveDirty=lambda: True))
        with patch.dict('sys.modules', extpy=SimpleNamespace(getbrowser=lambda: browser)):
            with self.assertRaisesRegex(RuntimeError, 'Solve the cyclic'):
                reporter.psa_scheduling_gantt(None)

    def test_headless_plot(self):
        if importlib.util.find_spec('matplotlib') is None:
            self.skipTest('Matplotlib not installed')
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        for case in ('pe3', 'oxy_continuous'):
            r = reporter.solve(case)
            fig = reporter.plot_schedule(r)
            try:
                self.assertEqual(len(fig.axes[0].get_yticklabels()), r['N'])
                self.assertIn('chosen scale', fig.axes[0].get_title(loc='left'))
                with tempfile.TemporaryDirectory(prefix='psa-scheduling-') as tmp:
                    path = Path(tmp)/'schedule.png'
                    fig.savefig(path)
                    self.assertGreater(path.stat().st_size, 10000)
            finally:
                plt.close(fig)


if __name__ == '__main__':
    unittest.main()
