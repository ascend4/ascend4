"""Structured execution, detached reports and partial integration failures."""

import copy
import csv
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

import pytest

ascpy = pytest.importorskip("ascpy")
import runmodel
from runresult import RunError, RunResult

ROOT = Path(__file__).resolve().parents[1]
MODEL = ROOT / "models/test/ida/runapi_results.a4c"


@pytest.fixture
def library():
    lib = ascpy.Library()
    lib.clear()
    yield lib
    lib.clear()


@pytest.fixture
def ida():
    if "IDA" not in ascpy.Integrator.getEngines():
        pytest.skip("IDA is not built")


@pytest.fixture
def simulation(library, ida):
    library.load(str(MODEL))
    simulations = []

    def create(name="runapi_result_ok"):
        sim = library.findType(name).getSimulation("result_test", True)
        simulations.append(sim)
        return sim

    yield create
    for sim in simulations:
        sim.invalidateSystem()


def assert_partial(result):
    assert not result.ok
    assert result.partial
    assert result.diagnostics
    rows = result.tables[0]["rows"]
    assert len(rows) >= 2
    assert rows[0]["time_raw"] == 0
    assert 0 < rows[-1]["time_raw"] < 2


def test_integration_returns_typed_detached_data(library, ida):
    library.load(str(MODEL))
    sim = library.findType("runapi_result_ok").getSimulation("detached_test", True)
    with patch.object(runmodel, "_print_table") as table, patch.object(runmodel, "_write_tsv") as write:
        result = runmodel.execute_integration(sim, duration=1, steps=4, units="s")
    assert result.ok, result.diagnostics
    table.assert_not_called()
    write.assert_not_called()
    report = result.tables[0]
    assert len(report["rows"]) == 5
    assert report["rows"][-1]["values"][0] == pytest.approx(1)
    assert report["rows"][-1]["values"][1:] == [True, 7, "sample"]
    assert isinstance(report["rows"][-1]["values"][1], bool)
    assert [col["kind"] for col in report["columns"]] == ["real", "boolean", "integer", "symbol"]
    assert all("instance" not in col for col in report["columns"])
    before = json.dumps(result.as_dict())
    # No native handle may be required to inspect or render the result.
    sim.invalidateSystem()
    library.clear()
    assert json.dumps(result.as_dict()) == before
    with patch.object(runmodel, "_print_table") as printed:
        runmodel.render_run_result(result)
    assert printed.call_args.args[1][-1][1:] == [pytest.approx(1), True, 7, "'sample'"]


def test_native_failure_retains_rows_and_exports_tsv(simulation, tmp_path):
    result = runmodel.execute_integration(simulation("runapi_result_partial"),
                                         start=0, duration=2, steps=10, units="s")
    assert_partial(result)
    output = tmp_path / "partial.tsv"
    runmodel.render_run_result(result, output=output)
    with output.open() as stream:
        header, *rows = csv.reader(stream, delimiter="\t")
    assert len(rows) == len(result.tables[0]["rows"])
    assert rows[0][2:] == ["TRUE", "7", "'sample'"]
    with pytest.raises(RunError) as failure:
        result.raise_for_status()
    assert failure.value.result is result


def test_result_records_display_and_base_time_units(simulation):
    sim = simulation()
    time = sim.getModel().t
    time.setDisplayUnitsOverride("min")
    try:
        result = runmodel.execute_integration(sim, duration=60, steps=4, units="s")
        assert result.ok, result.diagnostics
        report = result.tables[0]
        assert report["time_units"] == "min"
        assert report["time_label"] == "t [min]"
        assert report["rows"][-1]["time"] == pytest.approx(1)
        assert report["rows"][-1]["time_raw"] == pytest.approx(60)
    finally:
        time.clearDisplayUnitsOverride()


def test_setup_failure_has_diagnostics_and_no_rows(simulation):
    result = runmodel.execute_integration(simulation(), engine="does-not-exist", duration=1)
    assert not result.ok and not result.partial
    assert result.tables == []
    assert result.diagnostics[0].phase == "prepare"
    assert result.diagnostics[0].exception_type == "IndexError"


def test_callback_failure_is_not_reported_as_success(simulation):
    original = runmodel.CliIntegratorReporter._capture_row

    def fail_after_first(self):
        if self.rows:
            raise ValueError("injected observation failure")
        original(self)

    sim = simulation()
    with patch.object(runmodel.CliIntegratorReporter, "_capture_row", fail_after_first):
        result = runmodel.execute_integration(sim, duration=1, steps=4, units="s")
    assert not result.ok and result.partial
    assert any(d.phase == "reporter.recordObservedValues" and
               d.message == "injected observation failure" for d in result.diagnostics)
    # Failure should not poison later integration on this caller-owned sim.
    recovered = runmodel.execute_integration(sim, duration=1, steps=4, units="s")
    assert recovered.ok, recovered.diagnostics


def test_analysis_failure_before_output_returns_result(simulation):
    sim = simulation()
    with patch.object(ascpy.Integrator, "analyse", side_effect=RuntimeError("injected analysis failure")):
        result = runmodel.execute_integration(sim, duration=1, steps=4, units="s")
    assert not result.ok and not result.partial
    assert result.tables == []
    assert result.phase == "analyse"
    assert result.diagnostics[0].message == "injected analysis failure"


def test_solve_result_and_cli_share_execution(library):
    with patch.object(runmodel, "_print_table") as printed:
        result = runmodel.execute_model(MODEL, model="runapi_result_steady")
    assert result.ok, result.diagnostics
    assert result.action == "solve"
    assert result.values["y"]["value"] == pytest.approx(2)
    assert result.simulation_status["solver_status"] == "converged"
    printed.assert_not_called()
    before = json.dumps(result.as_dict())
    library.clear()
    assert json.dumps(result.as_dict()) == before
    with patch.object(runmodel, "execute_model", return_value=result), \
            patch.object(runmodel, "render_run_result") as render:
        assert runmodel.run_ascend_model(MODEL) is result
    render.assert_called_once()


def test_method_failure_returns_partial_result_and_restores_hooks(library, ida):
    old = ascpy.SolverHooksManager.Instance().getHooks()
    result = runmodel.execute_model(MODEL, model="runapi_result_auto")
    assert_partial(result)
    assert ascpy.SolverHooksManager.Instance().getHooks().this == old.this
    assert any(d.phase == "on_load" for d in result.diagnostics)
    assert any(d.phase == "integrate" for d in result.diagnostics)


def test_multiple_method_integrations_are_retained(library, ida):
    result = runmodel.execute_model(MODEL, model="runapi_result_twice")
    assert result.ok, result.diagnostics
    assert len(result.tables) == 2
    assert [table["rows"][-1]["time_raw"] for table in result.tables] == [1, 2]


def test_multi_table_export_cannot_overwrite_data(tmp_path):
    result = RunResult(action="integrate", tables=[{"rows": []}, {"rows": []}])
    output = tmp_path / "output.tsv"
    with pytest.raises(ValueError, match="multiple integration tables"):
        runmodel.render_run_result(result, output=output)
    assert not output.exists()


@pytest.mark.parametrize("options, phase", [
    ({"model": "unknown_model"}, "lookup"),
    ({"model": "runapi_result_steady", "runmethod": "missing_method"}, "method:missing_method"),
])
def test_model_errors_return_results_not_system_exit(library, options, phase):
    result = runmodel.execute_model(str(MODEL), **options)
    assert not result.ok
    assert result.phase == phase
    assert result.diagnostics


def test_render_filtering_does_not_discard_raw_microstates():
    result = RunResult(action="integrate", tables=[{
        "time_label": "t", "columns": [{"label": "y", "units": "", "is_real": True}],
        "rows": [{"time": 0, "time_raw": 0, "values": [v], "event": True} for v in (1, 2, 3)],
    }])
    before = copy.deepcopy(result.as_dict())
    for mode, count in (("none", 1), ("endpoints", 2), ("all", 3)):
        with patch.object(runmodel, "_print_table") as printed:
            runmodel.render_run_result(result, microstates=mode)
        assert len(printed.call_args.args[1]) == count
        assert result.as_dict() == before


def test_render_failure_keeps_execution_data(tmp_path):
    result = RunResult(action="integrate", tables=[{
        "time_label": "t", "columns": [], "rows": [{"time": 0, "values": []}],
    }])
    with patch.object(runmodel, "execute_model", return_value=result):
        with pytest.raises(RunError) as failure:
            runmodel.run_ascend_model(MODEL, output=tmp_path / "missing" / "output.tsv")
    assert failure.value.result is result
    assert result.phase == "render" and result.partial


@pytest.mark.parametrize("automatic", [False, True])
def test_cli_failure_is_nonzero_and_preserves_output(tmp_path, automatic, ida):
    output = tmp_path / "partial.tsv"
    model = "runapi_result_auto" if automatic else "runapi_result_partial"
    command = [sys.executable, str(ROOT / "ascxx/runmodel.py"), str(MODEL), "--model", model]
    if not automatic:
        command += ["--integrate", "--duration", "2", "--steps", "10", "--units", "s", "--output", str(output)]
    completed = subprocess.run(command, capture_output=True, text=True, timeout=30)
    assert completed.returncode == 1, completed.stdout + completed.stderr
    assert "run_status=failed" in completed.stdout
    if automatic:
        assert "sample" in completed.stdout
    else:
        with output.open() as stream:
            header, *rows = csv.reader(stream, delimiter="\t")
        assert len(rows) >= 2
        assert "flag" in header
