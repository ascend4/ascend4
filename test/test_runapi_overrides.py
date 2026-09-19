"""Literal overrides, safe paths and the CLI's explicit preparation order."""

import csv
import math
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

import pytest

ascpy = pytest.importorskip("ascpy")
import runmodel
from runvalues import apply_overrides, resolve_instance

ROOT = Path(__file__).resolve().parents[1]
MODEL = ROOT / "models/test/ida/runapi_overrides.a4c"


@pytest.fixture
def library():
    lib = ascpy.Library()
    lib.clear()
    yield lib
    lib.clear()


@pytest.fixture
def simulation(library):
    library.load(str(MODEL))
    sim = library.findType("runapi_override_scalars").getSimulation("overrides", True)
    yield sim
    sim.invalidateSystem()


def test_typed_overrides_and_paths_preserve_fix_state(simulation):
    apply_overrides(simulation, ["T = 873.15 {K}", "x=5", "flag=TRUE",
                                "count=-7", "tag='case one'", "values[2]=3e-4",
                                "named['hydrogen']=2", "named['a=b']=4",
                                "group.amount=40{mg}", "mode='high'"])
    assert simulation.T.getRealValue() == pytest.approx(873.15)
    assert simulation.x.getRealValue() == 5
    assert simulation.x.isFixed()
    assert simulation.flag.getBoolValue() is True
    assert simulation.count.getIntValue() == -7
    assert str(simulation.tag.getSymbolValue()) == "case one"
    assert str(simulation.mode.getSelectorValue()) == "high"
    assert resolve_instance(simulation, "values[2]").getRealValue() == pytest.approx(3e-4)
    assert not resolve_instance(simulation, "values[2]").isFixed()
    assert resolve_instance(simulation, "named['hydrogen']").getRealValue() == 2
    assert resolve_instance(simulation, "named['a=b']").getRealValue() == 4
    assert simulation.group.amount.getRealValue() == pytest.approx(40e-6)


def test_bare_real_uses_base_units_and_repeated_override_uses_last(simulation):
    amount = simulation.group.amount
    amount.setDisplayUnitsOverride("mg")
    try:
        apply_overrides(simulation, ["group.amount=40{mg}", "group.amount=0.002"])
        assert amount.getRealValue() == pytest.approx(0.002)
    finally:
        amount.clearDisplayUnitsOverride()


@pytest.mark.parametrize("assignment", [
    "T=2{s}", "T=2{nonexistent_units}", "T=1e999{K}", "T=nan", "T=inf",
    "T=300{}", "T=300{*}", "T=300{ }", "T=2+3", "count=1.5",
    "flag=1", "tag=unquoted", "n=5", "values=5", "values[99]=5",
    "missing=5", "group=5", "group.amount.bad=5", "x[1]=5", "values[1:2]=5",
    "x.__class__=5", "x=__import__('os').getpid()", "x=3; SOLVE;", "x",
])
def test_bad_overrides_are_rejected(simulation, assignment):
    with pytest.raises(ValueError):
        apply_overrides(simulation, [assignment])


def test_no_solve_retains_values_without_implicit_solve_or_self_test(library):
    # self_test deliberately fails; --no-solve must not invoke it either.
    with patch.object(ascpy.Simulation, "solve", side_effect=AssertionError("unexpected solve")):
        result = runmodel.execute_model(MODEL, model="runapi_override_scalars",
                                       overrides=["group.amount=40{mg}"],
                                       runmethod=["group.double_amount", "group.double_amount"],
                                       solve=False)
    assert result.ok, result.diagnostics
    assert result.action == "methods"


def test_qualified_method_and_print_share_safe_path_resolution(library):
    result = runmodel.execute_model(MODEL, model="runapi_override_scalars",
                                   overrides=["group.amount=40{mg}", "named['a=b']=7"],
                                   runmethod="group.double_amount", solve=False,
                                   printvars=["group.amount", "named['a=b']"])
    assert result.ok, result.diagnostics
    assert result.values["named['a=b']"]["value"] == 7
    # Scalar reports use display units, so compare through the recorded units.
    mass = result.values["group.amount"]
    assert mass["value"] * ascpy.Units(mass["units"]).getConversion() == pytest.approx(80e-6)


def test_invalid_override_stops_before_post_methods_and_execution(library):
    with patch.object(runmodel, "_run_named_method") as method, \
            patch.object(ascpy.Simulation, "solve") as solve:
        result = runmodel.execute_model(MODEL, model="runapi_override_scalars",
                                       overrides=["T=1{s}"], runmethod="group.double_amount")
    assert not result.ok
    assert result.phase == "overrides"
    assert "T=1{s}" in result.diagnostics[0].message
    method.assert_not_called()
    solve.assert_not_called()


def cli(*args, action="run"):
    return subprocess.run([sys.executable, str(ROOT / "a4"), action,
                           str(MODEL), *args], capture_output=True, text=True, timeout=30)


@pytest.mark.parametrize("skip_on_load", [False, True])
@pytest.mark.parametrize("method_integration", [False, True])
def test_cli_kinetics_order_and_integration(tmp_path, skip_on_load, method_integration):
    if "IDA" not in ascpy.Integrator.getEngines():
        pytest.skip("IDA is not available")
    output = tmp_path / "result.tsv"
    # Deliberately scramble flag groups: documented phases, not flag placement,
    # determine order. Setup defaults must precede overrides; reset follows.
    args = ["--model", "runapi_override_kinetics", "--run-method", "prepare_case",
            "--set", "m_sample_init=40{mg}", "--setup-method", "defaults",
            "--run-method", "check_prepared"]
    if skip_on_load:
        args.append("--no-on-load")
    if method_integration:
        # --output currently requests explicit integration in a4 run. Check
        # METHOD-only integration without that flag, using its printed table.
        args += ["--run-method", "integrate_case", "--no-solve"]
    else:
        args += ["--integrate", "--duration", "60", "--steps", "4",
                 "--units", "s", "--output", str(output)]
    completed = cli(*args)
    assert completed.returncode == 0, completed.stdout + completed.stderr
    if method_integration:
        assert "inventory" in completed.stdout
        assert "run_status=failed" not in completed.stdout
    else:
        with output.open() as stream:
            headers, *rows = csv.reader(stream, delimiter="\t")
        mass_units = headers[1].split("[", 1)[1].rstrip("]")
        factor = ascpy.Units(mass_units).getConversion()
        assert len(rows) == 5
        assert float(rows[0][1]) * factor == pytest.approx(40e-6)
        assert float(rows[-1][1]) * factor == pytest.approx(40e-6 * math.exp(-1), rel=2e-3)
        reference = tmp_path / "wrapper.tsv"
        baseline = cli("--model", "runapi_override_wrapper", "--run-method", "legacy_case",
                       "--integrate", "--duration", "60", "--steps", "4", "--units", "s",
                       "--output", str(reference))
        assert baseline.returncode == 0, baseline.stdout + baseline.stderr
        assert output.read_text() == reference.read_text()


def test_cli_failed_override_exits_nonzero():
    completed = cli("--model", "runapi_override_scalars", "--set", "T=1{s}", "--no-solve")
    assert completed.returncode == 1
    assert "T=1{s}" in completed.stderr
    assert "run_status=failed" in completed.stdout


def test_cli_can_skip_on_load_and_implicit_execution():
    completed = cli("--model", "runapi_override_explicit_start", "--no-on-load",
                    "--set", "count=7", "--no-solve")
    assert completed.returncode == 0, completed.stdout + completed.stderr
    # Without --no-on-load, the deliberately failing METHOD must execute.
    failed = cli("--model", "runapi_override_explicit_start", "--set", "count=7", "--no-solve")
    assert failed.returncode == 1


def test_int_alias_forwards_overrides_and_methods():
    if "IDA" not in ascpy.Integrator.getEngines():
        pytest.skip("IDA is not available")
    completed = cli("--model", "runapi_override_kinetics", "--no-on-load",
                    "--setup-method", "defaults", "--set", "m_sample_init=40{mg}",
                    "--run-method", "prepare_case", "--run-method", "check_prepared",
                    "--duration", "60", "--steps", "4", "--units", "s", action="int")
    assert completed.returncode == 0, completed.stdout + completed.stderr
    assert "inventory" in completed.stdout
