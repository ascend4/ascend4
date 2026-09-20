#!/usr/bin/env python3
"""Exercise the GUI's real instance-status data across system rebuilds."""
import importlib.util
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(importlib.util.find_spec("ascpy"), "Run with ./a4 pytest and built ascpy")
class DerivativeInterfaces(unittest.TestCase):
    def test_gui_status_survives_build_and_teardown(self):
        import ascpy

        lib = ascpy.Library()
        lib.load(str(ROOT / "models/test/ida/der_coexist.a4c"))
        sim = lib.findType("der_coexist").getSimulation("derivative_gui_status", True)
        x = sim.getModel().alias_x
        derivative = x.der
        try:
            self.assertEqual(derivative.getStatus(), ascpy.ASCXX_INST_STATUS_UNKNOWN)
            sim.setSolver(ascpy.Solver("QRSlv"))
            self.assertEqual(derivative.getStatus(), ascpy.ASCXX_INST_STATUS_UNKNOWN)
            sim.solve(ascpy.Solver("QRSlv"), ascpy.SolverReporter())
            sim.processVarStatus()
            statuses = (ascpy.ASCXX_VAR_FIXED, ascpy.ASCXX_VAR_SOLVED)
            self.assertEqual((x.getStatus(), derivative.getStatus()), statuses)

            # These are instance handles, intentionally retained across the
            # destruction and rebuilding of solver-system mappings.
            sim.invalidateSystem()
            self.assertEqual((x.getStatus(), derivative.getStatus()), statuses)
            sim.build()
            self.assertEqual((x.getStatus(), derivative.getStatus()), statuses)
            self.assertEqual(x.der.getStatus(), statuses[1])
        finally:
            sim.invalidateSystem()

    def check_initial(self, engine, retry=False):
        import ascpy

        lib = getattr(type(self), "initial_library", None)
        if lib is None:
            lib = ascpy.Library()
            lib.load(str(ROOT / "models/test/ida/initial.a4c"))
            type(self).initial_library = lib
        if engine not in ascpy.Integrator.getEngines():
            # Only LSODE and IDA are automatically discovered at present.
            plugin = engine.lower()
            folder = ROOT / "solvers" / plugin
            if any(folder.glob(f"*{plugin}_ascend.*")):
                lib.loadString(f'IMPORT "{plugin}";', f"load_{plugin}")
            else:
                self.skipTest(f"{engine} integrator is not built")
        self.assertIn(engine, ascpy.Integrator.getEngines())
        model = "ida_initial_retry" if retry else "ida_initial_decay"
        sim = lib.findType(model).getSimulation(f"initial_{engine}_{retry}", True)
        sim.setSolver(ascpy.Solver("QRSlv"))
        y = sim.getModel().y
        derivative = y.der
        # Unlike instance handles, these wrap system-owned var_variable
        # objects: replacing the system would invalidate them.
        variables = list(sim.getallVariables())
        expected_names = [v.getName() for v in variables]
        integ = ascpy.Integrator(sim)
        integ.setEngine(engine)
        integ.addObservedInstance(y)
        integ.setLinearTimesteps(ascpy.Units("s"), 0, 1, 20)
        owner = self
        seen = []
        callback_errors = []

        class Reporter(ascpy.IntegratorReporterCxx):
            def initOutput(self):
                return 1

            def closeOutput(self):
                return 1

            def updateStatus(self):
                return 1

            def recordObservedValues(self):
                # Do not throw through a C integrator callback: some engines
                # cannot unwind a SWIG director exception safely.
                try:
                    owner.assertEqual(str(sim.getSolver().getName()), "QRSlv")
                    owner.assertEqual([v.getName() for v in variables], expected_names)
                    owner.assertTrue(all(math.isfinite(v.getValue()) for v in variables))
                    owner.assertEqual(integ.getNumObservedItems(), 1)
                    owner.assertAlmostEqual(integ.getObservedInstance(0).getRealValue(), y.getRealValue())
                    sim.processVarStatus()
                    seen.append(y.getRealValue())
                except Exception as error:
                    callback_errors.append(str(error))
                return 1

        reporter = Reporter(integ)
        integ.setReporter(reporter)
        try:
            integ.analyse()
            if retry:
                with self.assertRaises(RuntimeError):
                    integ.solve()
                self.assertEqual([v.getName() for v in variables], expected_names)
                self.assertTrue(math.isfinite(derivative.getRealValue()))
                sim.processVarStatus()
                sim.getModel().rhs.setRealValue(1)
                y.setRealValue(1)
            integ.solve()
            self.assertEqual(callback_errors, [])
            self.assertTrue(seen)
            self.assertAlmostEqual(y.getRealValue(), math.exp(-4), delta=3e-4)
            self.assertEqual([v.getName() for v in variables], expected_names)
            self.assertTrue(math.isfinite(derivative.getRealValue()))
        finally:
            # Release the integrator before its borrowed system.
            del reporter, integ
            sim.invalidateSystem()

    def test_initial_ida(self):
        self.check_initial("IDA")

    def test_initial_retry_ida(self):
        self.check_initial("IDA", retry=True)

    def test_initial_lsode(self):
        self.check_initial("LSODE")

    def test_initial_dopri5(self):
        self.check_initial("DOPRI5")

    def test_initial_radau5(self):
        self.check_initial("RADAU5")


if __name__ == "__main__":
    unittest.main()
