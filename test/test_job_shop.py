"""Schedule validation, plotting and optional ascpy/extpy integration.

Run with ./a4 pytest test/test_job_shop.py -q
The integration tests skip only when ascpy/HiGHS/extpy is unavailable.
"""
from dataclasses import replace
import importlib.util
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "models"))
import job_shop


def reference_schedule():
    # Published schedule, in minutes; the optimiser may place slack differently.
    return [job_shop.Operation(*row) for row in [
        (1, "Paper_1", "Blue",   0, 42, 45, 87),
        (2, "Paper_1", "Yellow", 1, 87, 10, 97),
        (3, "Paper_2", "Green",  0,  0, 10, 10),
        (4, "Paper_2", "Blue",   3, 10, 20, 30),
        (5, "Paper_2", "Yellow", 4, 30, 34, 64),
        (6, "Paper_3", "Yellow", 0,  0, 28, 28),
        (7, "Paper_3", "Blue",   6, 30, 12, 42),
        (8, "Paper_3", "Green",  7, 42, 17, 59),
    ]]


class ScheduleValidation(unittest.TestCase):
    def test_reference_and_alternative_optimum(self):
        ops = reference_schedule()
        job_shop.validate_schedule(ops, 97)
        ops[-1] = replace(ops[-1], start=80, finish=97)
        job_shop.validate_schedule(ops, 97)

    def test_machine_overlap(self):
        ops = reference_schedule()
        ops[0] = replace(ops[0], start=40, finish=85)
        with self.assertRaisesRegex(ValueError, "Overlapping"):
            job_shop.validate_schedule(ops, 97)

    def test_precedence(self):
        ops = reference_schedule()
        ops[1] = replace(ops[1], start=85, finish=95)
        with self.assertRaisesRegex(ValueError, "Precedence"):
            job_shop.validate_schedule(ops, 97)

    def test_invalid_times(self):
        for changes in ({"start": -1}, {"duration": 0}, {"finish": 100},
                        {"start": float("nan")}, {"duration": float("inf")}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                ops = reference_schedule()
                ops[0] = replace(ops[0], **changes)
                job_shop.validate_schedule(ops, 97)

    def test_invalid_makespan(self):
        for makespan in (0, 96, 98, float("nan"), float("inf")):
            with self.subTest(makespan=makespan), self.assertRaises(ValueError):
                job_shop.validate_schedule(reference_schedule(), makespan)

    def test_missing_or_duplicate_operations(self):
        ops = reference_schedule()
        for invalid in ([], ops + [ops[0]], ops[1:]):
            with self.assertRaises(ValueError):
                job_shop.validate_schedule(invalid, 97)

    def test_display_roundoff(self):
        for value in (0, 1e-15, -1e-15):
            self.assertEqual(job_shop.time_label(value), "0")
        self.assertEqual(job_shop.time_label(12.5), "12.5")

    def test_gui_refuses_unsolved_or_stale_results(self):
        browser = SimpleNamespace(sim=Mock())
        for dirty, converged in ((True, True), (False, False)):
            browser.sim.isSolveDirty.return_value = dirty
            browser.sim.getStatus.return_value.isConverged.return_value = converged
            with patch.dict(sys.modules, {"extpy": SimpleNamespace(getbrowser=lambda: browser)}):
                with self.assertRaisesRegex(RuntimeError, "Solve the job shop"):
                    job_shop.job_shop_gantt(None)


@unittest.skipUnless(importlib.util.find_spec("matplotlib"), "Matplotlib not installed")
class GanttPlot(unittest.TestCase):
    def test_two_labelled_panels_and_file_output(self):
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig = job_shop.plot_schedule(reference_schedule(), 97, "HiGHS")
        self.addCleanup(plt.close, fig)
        self.assertEqual(len(fig.axes), 2)
        self.assertIn("Makespan: 97 min", fig._suptitle.get_text())
        self.assertIn("Elapsed time / min", fig.axes[1].get_xlabel())
        for ax in fig.axes:
            self.assertEqual(len(ax.patches), 8)
            self.assertEqual(len(ax.get_yticklabels()), 3)
            self.assertEqual(len(ax.get_legend().get_texts()), 3)
            self.assertEqual(list(ax.lines[0].get_xdata()), [97, 97])
            self.assertTrue(any("0–10" in t.get_text() for t in ax.texts))
        with tempfile.TemporaryDirectory(prefix="ascend-gantt-test-") as tmp:
            for suffix in ("png", "svg"):
                output = Path(tmp) / f"schedule.{suffix}"
                fig.savefig(output)
                self.assertGreater(output.stat().st_size, 1000)


@unittest.skipUnless(importlib.util.find_spec("ascpy") and importlib.util.find_spec("matplotlib"),
                     "Run via ./a4 script with built ascpy and Matplotlib")
class AscendIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.library = ascpy.Library()  # Initialise compiler before loading plugins.
        try:
            ascpy.Solver("HiGHS").getIndex()
        except RuntimeError:
            raise unittest.SkipTest("HiGHS adapter not built")

    def test_driver_highs(self):
        with tempfile.TemporaryDirectory(prefix="ascend-job-shop-cli-") as tmp:
            output = Path(tmp) / "schedule.png"
            self.assertEqual(job_shop.main(["--solver", "HiGHS", "--output", str(output)]), 0)
            self.assertGreater(output.stat().st_size, 1000)

    def test_extpy_method(self):
        import ascpy
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        # Exercise the real ASCEND IMPORT and EXTERNAL call, replacing only the
        # browser and its interactive backend so this also works on headless CI.
        if not (ROOT / "models/johnpye/extpy/libextpy_ascend.so").exists():
            self.skipTest("extpy shared library not built")
        library = ascpy.Library()
        library.load(str(ROOT / "models/job_shop_plot.a4c"))
        model_type = library.findType("job_shop_plot_highs")
        sim = model_type.getSimulation("gui_schedule", True)
        sim.solve(ascpy.Solver("HiGHS"), ascpy.SolverReporter())
        ops, makespan = job_shop.read_schedule(sim.getModel())
        self.assertEqual(len(ops), 8)
        self.assertAlmostEqual(makespan, 97)
        browser = SimpleNamespace(sim=sim, reporter=Mock())
        registry = ascpy.Registry()
        registry.set("browser", browser)
        self.addCleanup(registry.set, "browser", None)
        self.addCleanup(plt.close, "all")
        loader = SimpleNamespace(load_matplotlib=Mock())
        with patch.dict(sys.modules, {"loading": loader}), patch.object(plt, "show") as show:
            sim.run(next(m for m in model_type.getMethods() if str(m.getName()) == "gantt"))
            loader.load_matplotlib.assert_called_once_with(throw=True)
            show.assert_called_once_with(block=False)
        browser.reporter.reportNote.assert_called_once()
        self.assertEqual(len(plt.gcf().axes), 2)


if __name__ == "__main__":
    unittest.main()
