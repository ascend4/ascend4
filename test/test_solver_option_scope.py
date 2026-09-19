"""METHOD solver options survive rebuilds, but not a change of solver."""

from pathlib import Path

import pytest

ascpy = pytest.importorskip("ascpy")
MODEL = Path(__file__).resolve().parents[1] / "models/test/solver_option_scope.a4c"


@pytest.fixture
def simulation():
    lib = ascpy.Library()
    lib.clear()
    lib.load(str(MODEL))
    sim = lib.findType("solver_option_scope").getSimulation("option_scope", True)
    yield sim
    sim.invalidateSystem()
    lib.clear()


def require_solver(name):
    try:
        ascpy.Solver(name).getIndex()
    except RuntimeError:
        pytest.skip(f"{name} is not available")


def run(sim, method):
    sim.run(sim.getType().getMethod(method))


def assert_qrslv_options(sim):
    assert sim.getSolver().getName() == "QRSlv"
    assert sim.getParameterValue("iterationlimit") == 123
    assert sim.getParameterValue("convopt") == "RELNOM_SCALE"


def test_same_solver_preserves_options_across_methods_and_rebuild(simulation):
    run(simulation, "configure_qrslv")
    simulation.invalidateSystem()
    run(simulation, "select_qrslv")
    run(simulation, "solve_case")
    assert_qrslv_options(simulation)
    assert simulation.getStatus().isConverged()


def test_change_does_not_replay_options_unknown_to_new_solver(simulation):
    require_solver("HiGHS")
    run(simulation, "configure_qrslv")
    run(simulation, "select_highs")
    # Neither iterationlimit nor convopt is a HiGHS option. Previously SOLVE
    # failed while trying to replay them, before it reached the new solver.
    run(simulation, "solve_case")
    assert simulation.getSolver().getName() == "HiGHS"
    assert simulation.getStatus().isConverged()
    assert simulation.x.getRealValue() == pytest.approx(2)


def test_switching_back_uses_defaults_not_previous_options(simulation):
    require_solver("HiGHS")
    defaults = {name: simulation.getParameterValue(name)
                for name in ("iterationlimit", "convopt")}
    run(simulation, "configure_qrslv")
    run(simulation, "select_highs")
    run(simulation, "select_qrslv")
    run(simulation, "solve_case")
    # Stale same-name options would be accepted silently on returning to QRSlv.
    assert {name: simulation.getParameterValue(name) for name in defaults} == defaults
    # An explicit configuration METHOD can restore the desired settings.
    run(simulation, "configure_qrslv")
    run(simulation, "solve_case")
    assert_qrslv_options(simulation)


@pytest.mark.parametrize("method, solver", [
    ("select_missing", None),
    ("select_ineligible", "LRSlv"),
])
def test_failed_selection_preserves_saved_configuration(simulation, method, solver):
    if solver:
        require_solver(solver)
    run(simulation, "configure_qrslv")
    with pytest.raises(RuntimeError):
        run(simulation, method)
    # Even if native selection changed the live system before failing its
    # eligibility check, SOLVE must recover the last successful configuration.
    run(simulation, "solve_case")
    assert_qrslv_options(simulation)
    assert simulation.getStatus().isConverged()
