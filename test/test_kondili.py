"""Event-level validation and reporting tests; run with ./a4 pytest."""
import copy
import importlib.util
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'models'))
import kondili


def small_result():
    return dict(times=[0,.5,1], value=10,
        recipes={'split':dict(duration=1, inputs={'feed':1}, outputs={'early':(.5,.5),'late':(.5,1)})},
        units={'vessel':dict(batches=[dict(task='split',start=0,mass=10)],held=[10,5,0])},
        stocks={'feed':dict(initial=10,capacity=None,price=0,values=[0,0,0]),
                'early':dict(initial=0,capacity=5,price=1,values=[0,5,5]),
                'late':dict(initial=0,capacity=None,price=1,values=[0,0,5])})


class EventChecks(unittest.TestCase):
    def test_staggered_release(self):
        kondili.validate_results(small_result())

    def test_bad_inventory(self):
        r=small_result(); r['stocks']['early']['values'][1]=0
        with self.assertRaisesRegex(ValueError,'Stock balance'): kondili.validate_results(r)

    def test_bad_holdup(self):
        r=small_result(); r['units']['vessel']['held'][1]=0
        with self.assertRaisesRegex(ValueError,'Equipment mass'): kondili.validate_results(r)

    def test_capacity(self):
        r=small_result(); r['stocks']['early']['capacity']=4
        with self.assertRaisesRegex(ValueError,'capacity'): kondili.validate_results(r)

    def test_overlap(self):
        r=small_result(); r['units']['vessel']['batches'] *= 2
        with self.assertRaisesRegex(ValueError,'Overlapping'): kondili.validate_results(r)

    def test_off_grid(self):
        r=small_result(); r['recipes']['split']['outputs']['early']=(.5,.3)
        with self.assertRaisesRegex(ValueError,'off the time grid'): kondili.validate_results(r)

    def test_objective_and_nan(self):
        for value in (11,float('nan')):
            r=small_result(); r['value']=value
            with self.assertRaisesRegex(ValueError,'Terminal value'): kondili.validate_results(r)

    def test_bad_recipe(self):
        r=small_result(); r['recipes']['split']['inputs']['feed']=float('nan')
        with self.assertRaisesRegex(ValueError,'mass fractions'): kondili.validate_results(r)

    def test_stale_gui(self):
        browser=SimpleNamespace(sim=Mock())
        browser.sim.isSolveDirty.return_value=True
        with patch.dict(sys.modules,{'extpy':SimpleNamespace(getbrowser=lambda:browser)}):
            with self.assertRaisesRegex(RuntimeError,'Solve the model'): kondili.kondili_plot(None)


@unittest.skipUnless(importlib.util.find_spec('matplotlib'),'Matplotlib unavailable')
class PlotChecks(unittest.TestCase):
    def test_plot_and_report(self):
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        from io import StringIO
        r=small_result()
        fig=kondili.plot_results(r,'HiGHS')
        self.addCleanup(plt.close,fig)
        self.assertEqual(len(fig.axes),4)
        self.assertEqual(len(fig.axes[0].patches),1)
        self.assertEqual(list(fig.axes[0].lines[0].get_xdata()),[.5])
        self.assertEqual(fig.axes[0].get_xlabel(),'Elapsed time / h')
        self.assertTrue(any('kg' in t.get_text() for t in fig.axes[0].texts))
        with patch('sys.stdout',new_callable=StringIO) as out:
            kondili.print_results(r)
            self.assertIn('early, 0.5, 5',out.getvalue())
            self.assertIn('late, 1, 5',out.getvalue())
        with tempfile.TemporaryDirectory() as tmp:
            for ext in ('png','svg'):
                path=Path(tmp)/f'chart.{ext}'
                fig.savefig(path)
                self.assertGreater(path.stat().st_size,1000)


@unittest.skipUnless(importlib.util.find_spec('ascpy') and importlib.util.find_spec('matplotlib'),
                     'Run via ./a4 pytest with built ascpy and Matplotlib')
class AscendIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.library=ascpy.Library()
        try: ascpy.Solver('HiGHS').getIndex()
        except RuntimeError: raise unittest.SkipTest('HiGHS adapter not built')

    def test_both_cases(self):
        for case,value in [('original',2744.375),('no-bc-storage',2210.625)]:
            with self.subTest(case=case):
                r=kondili.solve_case('HiGHS',case)
                self.assertAlmostEqual(r['value'],value,places=5)
                self.assertEqual(len(r['stocks']),9)
                self.assertEqual(len(r['units']),4)
                if case!='original':
                    self.assertTrue(all(abs(v)<1e-6 for v in r['stocks']['Int_BC']['values']))
                # Verify reconstructed material events detect corrupt output.
                bad=copy.deepcopy(r); bad['stocks']['Product_2']['values'][-1]+=1
                with self.assertRaises(ValueError): kondili.validate_results(bad)

    def test_cli(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'schedule.png'
            self.assertEqual(kondili.main(['--case','no-bc-storage','--output',str(path)]),0)
            self.assertGreater(path.stat().st_size,1000)

    def test_invalid_component_parameters(self):
        import ascpy
        lib=ascpy.Library()
        lib.load(str(ROOT/'models/test/mip/stn_tests.a4c'))
        for name in ('stn_off_grid','stn_bad_yield'):
            with self.subTest(model=name),self.assertRaisesRegex(RuntimeError,'instantiation'):
                lib.findType(name).getSimulation(name,False)

    def test_extpy_method(self):
        import ascpy
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        if not (ROOT/'models/johnpye/extpy/libextpy_ascend.so').exists():
            self.skipTest('extpy not built')
        lib=ascpy.Library()
        lib.load(str(ROOT/'models/kondili_plot.a4c'))
        typ=lib.findType('kondili_plot_highs')
        sim=typ.getSimulation('gui_stn',True)
        sim.solve(ascpy.Solver('HiGHS'),ascpy.SolverReporter())
        browser=SimpleNamespace(sim=sim,reporter=Mock())
        registry=ascpy.Registry(); registry.set('browser',browser)
        self.addCleanup(registry.set,'browser',None)
        self.addCleanup(plt.close,'all')
        loader=SimpleNamespace(load_matplotlib=Mock())
        with patch.dict(sys.modules,{'loading':loader}),patch.object(plt,'show') as show:
            sim.run(next(m for m in typ.getMethods() if str(m.getName())=='plot'))
            loader.load_matplotlib.assert_called_once_with(throw=True)
            show.assert_called_once_with(block=False)
        self.assertEqual(len(plt.gcf().axes),10)


if __name__=='__main__': unittest.main()
