"""Run each test module in a fresh interpreter, retaining pytest's reports.

Loaded by ``a4 pytest``. ASCEND's native compiler library is process-global;
independent test modules must not share its model types or live instance handles.
Workers use the same pytest options but collect only the parent's selected nodes.
"""

import builtins
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import warnings

import pytest


WORKER_ENV = "ASCEND_PYTEST_WORKER"


def pytest_addoption(parser):
    parser.getgroup("ascend").addoption(
        "--no-isolation", action="store_true",
        help="Run test files in one process (for debugging ASCEND shared state)",
    )


@pytest.hookimpl(tryfirst=True)
def pytest_configure(config):
    specification = os.environ.get(WORKER_ENV)
    if specification:
        data = json.loads(Path(specification).read_text())
        # Config.args contains collection targets, separately from pytest options.
        # Replacing it here avoids importing unrelated test modules in the worker.
        config.args = data["nodeids"]
        config.option.maxfail = data["maxfail"]
        # Only the parent writes the combined JUnit report.
        config.option.xmlpath = None
        config.pluginmanager.register(WorkerReports(config, data["reports"]), "ascend-worker-reports")


class WorkerReports:
    def __init__(self, config, filename):
        self.config = config
        self.stream = open(filename, "w", encoding="utf-8")

    def send(self, event, **data):
        self.stream.write(json.dumps(dict(event=event, **data)) + "\n")
        self.stream.flush()  # Preserve completed reports even if native code crashes.

    def pytest_runtest_logstart(self, nodeid, location):
        self.send("start", nodeid=nodeid, location=location)

    def pytest_runtest_logfinish(self, nodeid, location):
        self.send("finish", nodeid=nodeid, location=location)

    def pytest_runtest_logreport(self, report):
        self.send("report", data=self.config.hook.pytest_report_to_serializable(config=self.config, report=report))

    def pytest_collectreport(self, report):
        if report.failed:
            self.send("collection_error", data=self.config.hook.pytest_report_to_serializable(config=self.config, report=report))

    def pytest_warning_recorded(self, warning_message, when, nodeid, location):
        self.send(
            "warning", message=str(warning_message.message),
            category=warning_message.category.__name__, filename=warning_message.filename,
            lineno=warning_message.lineno, when=when, nodeid=nodeid, location=location,
        )

    def pytest_unconfigure(self):
        self.stream.close()


def replay_reports(config, filename):
    finished = set()
    active = None
    failed = False
    if not filename.exists():
        return finished, active, failed
    for line in filename.read_text().splitlines():
        event = json.loads(line)
        kind = event["event"]
        if kind in ("start", "finish"):
            nodeid, location = event["nodeid"], tuple(event["location"])
            if kind == "start":
                active = nodeid
                config.hook.pytest_runtest_logstart(nodeid=nodeid, location=location)
            else:
                finished.add(nodeid)
                active = None
                config.hook.pytest_runtest_logfinish(nodeid=nodeid, location=location)
        elif kind in ("report", "collection_error"):
            report = config.hook.pytest_report_from_serializable(config=config, data=event["data"])
            # JSON turns skip-location tuples into lists; pytest's JUnit writer
            # requires the original tuple representation (notably on pytest 7).
            if report.skipped and isinstance(report.longrepr, list):
                report.longrepr = tuple(report.longrepr)
            failed |= report.failed
            if kind == "report":
                config.hook.pytest_runtest_logreport(report=report)
            else:
                config.hook.pytest_collectreport(report=report)
        elif kind == "warning" and event["when"] != "config":
            # Configuration/collection warnings also occur in the parent.
            if event["when"] == "collect":
                continue
            category = getattr(builtins, event["category"], UserWarning)
            message = event["message"]
            if category is UserWarning and event["category"] != "UserWarning":
                message = event["category"] + ": " + message
            warning = warnings.WarningMessage(message, category, event["filename"], event["lineno"])
            config.hook.pytest_warning_recorded.call_historic(kwargs=dict(
                warning_message=warning, when=event["when"], nodeid=event["nodeid"],
                location=event["location"],
            ))
    return finished, active, failed


def run_module(session, items, directory):
    config = session.config
    reports = directory / "reports.jsonl"
    specification = directory / "worker.json"
    maxfail = config.option.maxfail
    if maxfail:
        maxfail = max(1, maxfail - session.testsfailed)
    specification.write_text(json.dumps(dict(
        nodeids=[str(item.path) + item.nodeid[len(item.nodeid.split("::")[0]):] for item in items],
        maxfail=maxfail, reports=str(reports),
    )))
    env = os.environ.copy()
    env[WORKER_ENV] = str(specification)
    # Preserve plugin availability even for direct -p usage outside a4.
    env["PYTHONPATH"] = os.pathsep.join(filter(None, (str(Path(__file__).parent), env.get("PYTHONPATH"))))
    command = [sys.executable, "-m", "pytest", "-p", "ascend_pytest", *config.invocation_params.args]
    with tempfile.TemporaryFile(mode="w+", encoding="utf-8") as output:
        process = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT)
        output.seek(0)
        log = output.read()
    finished, active, failed = replay_reports(config, reports)
    terminal = config.pluginmanager.getplugin("terminalreporter")
    if config.option.capture == "no" and terminal:
        terminal.write(log)
    incomplete = len(finished) != len(items) and not session.shouldfail
    if process.returncode not in (0, 1) or (process.returncode == 1 and not failed) or incomplete:
        # A worker crash must fail the run, not silently discard the rest of a file.
        item = next((item for item in items if item.nodeid == active), None)
        if item is None:
            item = next((item for item in items if item.nodeid not in finished), items[-1])
        description = (
            f"Isolated pytest worker for {item.path.name} exited with code {process.returncode}.\n"
            f"Completed {len(finished)} of {len(items)} selected tests.\n{log[-16000:]}"
        )
        config.hook.pytest_runtest_logreport(report=pytest.TestReport(
            nodeid=item.nodeid, location=item.location, keywords={}, outcome="failed",
            longrepr=description, when="teardown", sections=[], duration=0,
        ))


@pytest.hookimpl(tryfirst=True)
def pytest_runtestloop(session):
    config = session.config
    if (os.environ.get(WORKER_ENV) or config.getoption("no_isolation")
            or config.option.collectonly or config.option.usepdb or config.option.trace):
        return None
    if session.testsfailed and not config.option.continue_on_collection_errors:
        raise session.Interrupted(f"{session.testsfailed} error(s) during collection")
    modules = {}
    for item in session.items:
        modules.setdefault(item.path, []).append(item)
    for items in modules.values():
        with tempfile.TemporaryDirectory(prefix="ascend-pytest-") as directory:
            run_module(session, items, Path(directory))
        if session.shouldfail:
            raise session.Failed(session.shouldfail)
        if session.shouldstop:
            raise session.Interrupted(session.shouldstop)
    return True
