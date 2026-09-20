"""Black-box tests of the per-file pytest subprocess runner."""

import os
from pathlib import Path
import subprocess
import sys
import textwrap
import xml.etree.ElementTree as ET

import pytest


TOOLS = Path(__file__).resolve().parents[1] / "tools"


def write_test(directory, filename, source):
    path = directory / filename
    path.write_text(textwrap.dedent(source))
    return path


def run_tests(directory, *arguments):
    env = os.environ.copy()
    for key in ("ASCEND_PYTEST_WORKER", "PYTEST_PLUGINS", "PYTEST_ADDOPTS"):
        env.pop(key, None)
    env["PYTEST_DISABLE_PLUGIN_AUTOLOAD"] = "1"
    env["PYTHONPATH"] = str(TOOLS)
    return subprocess.run(
        [sys.executable, "-m", "pytest", "-p", "ascend_pytest", "-q", *arguments],
        cwd=directory, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=45,
    )


@pytest.fixture
def isolated_tests(tmp_path):
    (tmp_path / "pytest.ini").write_text("[pytest]\n")
    for filename in ("test_one.py", "test_two.py"):
        write_test(tmp_path, filename, """
            import builtins
            import pytest

            @pytest.fixture(scope='module')
            def shared():
                assert not hasattr(builtins, '_ascend_isolation_probe')
                builtins._ascend_isolation_probe = True
                return []

            def test_first(shared):
                assert shared == []
                shared.append(1)

            def test_second(shared):
                assert shared == [1]
        """)
    return tmp_path


def test_fresh_process_per_file_preserves_module_fixtures(isolated_tests):
    result = run_tests(isolated_tests)
    assert result.returncode == 0, result.stdout
    assert "4 passed" in result.stdout
    shared = run_tests(isolated_tests, "--no-isolation")
    assert shared.returncode == 1
    assert "2 passed, 2 errors" in shared.stdout


def test_worker_does_not_import_unselected_files(tmp_path):
    write_test(tmp_path, "test_one.py", """
        import sys
        def test_only():
            assert 'test_two' not in sys.modules
    """)
    write_test(tmp_path, "test_two.py", """
        import sys
        def test_only():
            assert 'test_one' not in sys.modules
    """)
    result = run_tests(tmp_path)
    assert result.returncode == 0, result.stdout
    assert "2 passed" in result.stdout


@pytest.mark.parametrize("selection", [
    ("-k", "keep"), ("test_select.py::test_keep",), ("-m", "chosen"),
])
def test_selection_is_preserved(tmp_path, selection):
    write_test(tmp_path, "test_select.py", """
        import pytest
        @pytest.mark.chosen
        def test_keep():
            pass
        def test_ignore():
            assert False
    """)
    (tmp_path / "pytest.ini").write_text("[pytest]\nmarkers = chosen: selection probe\n")
    result = run_tests(tmp_path, *selection)
    assert result.returncode == 0, result.stdout
    assert "1 passed" in result.stdout


def test_reports_and_junit_include_all_files(tmp_path):
    write_test(tmp_path, "test_one.py", """
        import pytest
        import warnings
        def test_pass():
            warnings.warn('worker warning', RuntimeWarning)
        @pytest.mark.skip(reason='probe skip')
        def test_skip(): pass
        @pytest.mark.xfail(reason='probe xfail')
        def test_xfail(): assert False
    """)
    write_test(tmp_path, "test_two.py", """
        def test_fail():
            print('captured worker output')
            assert False, 'worker assertion'
    """)
    result = run_tests(tmp_path, "--junitxml=results.xml")
    assert result.returncode == 1, result.stdout
    for message in ("1 failed, 1 passed, 1 skipped, 1 xfailed", "worker assertion", "captured worker output", "worker warning"):
        assert message in result.stdout
    cases = ET.parse(tmp_path / "results.xml").findall(".//testcase")
    assert len(cases) == 4
    assert len([case for case in cases if case.find("failure") is not None]) == 1


def test_global_maxfail_stops_across_files(tmp_path):
    for name in ("one", "two", "three"):
        write_test(tmp_path, "test_" + name + ".py", """
            def test_first(): assert False
            def test_second(): assert False
        """)
    result = run_tests(tmp_path, "--maxfail=3")
    assert result.returncode == 1, result.stdout
    assert "3 failed" in result.stdout
    assert "stopping after 3 failures" in result.stdout


def test_crashed_worker_is_reported_and_next_file_runs(tmp_path):
    write_test(tmp_path, "test_one.py", """
        import os
        def test_crash(): os._exit(17)
    """)
    write_test(tmp_path, "test_two.py", "def test_pass(): pass\n")
    result = run_tests(tmp_path)
    assert result.returncode == 1, result.stdout
    assert "exited with code 17" in result.stdout
    assert "1 passed, 1 error" in result.stdout


def test_collect_only_and_empty_selection_do_not_execute(isolated_tests):
    collected = run_tests(isolated_tests, "--collect-only")
    assert collected.returncode == 0, collected.stdout
    assert "4 tests collected" in collected.stdout
    empty = run_tests(isolated_tests, "-k", "nonexistent")
    assert empty.returncode == 5, empty.stdout


def test_collection_errors_do_not_run_tests(tmp_path):
    write_test(tmp_path, "test_one.py", "raise RuntimeError('collection failure')\n")
    write_test(tmp_path, "test_two.py", "def test_fail(): assert False, 'must not run'\n")
    result = run_tests(tmp_path)
    assert result.returncode == 2, result.stdout
    assert "collection failure" in result.stdout
    assert "FAILED test_two.py" not in result.stdout
