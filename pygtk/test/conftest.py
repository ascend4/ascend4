"""Fixtures for ASCEND's PyGTK tests."""

from __future__ import annotations

import sys
import time
from pathlib import Path

import pytest


TEST_DIR = Path(__file__).resolve().parent
PYGTK_DIR = TEST_DIR.parent
REPO_ROOT = PYGTK_DIR.parent
ASCXX_DIR = REPO_ROOT / "ascxx"
MODELS_DIR = REPO_ROOT / "models"
GLADE_DIR = PYGTK_DIR / "glade"

for path in (ASCXX_DIR, PYGTK_DIR):
	path_string = str(path)
	if path_string not in sys.path:
		sys.path.insert(0, path_string)


def drain_gtk_events():
	"""Process all GTK/GLib work which is ready without blocking."""
	from gi.repository import GLib

	context = GLib.MainContext.default()
	while context.pending():
		context.iteration(False)


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
	outcome = yield
	report = outcome.get_result()
	setattr(item, "report_" + report.when, report)


@pytest.fixture(scope="session")
def gtk_runtime():
	gi = pytest.importorskip("gi", reason="PyGObject is required for GUI tests")
	gi.require_version("Gtk", "3.0")
	from gi.repository import Gtk

	available, _arguments = Gtk.init_check(None)
	if not available:
		pytest.skip(
			"GTK could not open a display; install pytest-xvfb or run under xvfb-run"
		)
	return Gtk


@pytest.fixture
def wait_until():
	def wait(predicate, timeout=20, description="GUI condition"):
		deadline = time.monotonic() + timeout
		while time.monotonic() < deadline:
			drain_gtk_events()
			if predicate():
				return
			time.sleep(0.01)
		drain_gtk_events()
		raise AssertionError("Timed out waiting for %s" % description)

	return wait


@pytest.fixture
def drain_gtk():
	return drain_gtk_events


@pytest.fixture
def models_dir():
	return MODELS_DIR


def capture_window(window, filename):
	"""Save a rendered GTK window to PNG, if it has been realised."""
	from gi.repository import Gdk

	gdk_window = window.get_window()
	if gdk_window is None:
		return False
	width = window.get_allocated_width()
	height = window.get_allocated_height()
	if width <= 0 or height <= 0:
		return False
	pixbuf = Gdk.pixbuf_get_from_window(gdk_window, 0, 0, width, height)
	if pixbuf is None:
		return False
	pixbuf.savev(str(filename), "png", [], [])
	return True


@pytest.fixture
def browser(gtk_runtime, monkeypatch, tmp_path, request):
	"""Create a real Browser while isolating its per-user files."""
	monkeypatch.setenv("HOME", str(tmp_path))
	monkeypatch.setenv("MPLCONFIGDIR", str(tmp_path / "matplotlib"))
	monkeypatch.setenv("ASCENDLIBRARY", str(MODELS_DIR))
	monkeypatch.setattr(sys, "argv", ["ascend-gui-test", "--no-auto-sim"])

	try:
		import ascpy
		import preferences
		from gtkbrowser import Browser
	except (ImportError, SystemExit) as error:
		pytest.skip(
			"ASCEND's in-tree Python runtime is unavailable; use './a4 pytest': %s"
			% error
		)

	# Preferences is a singleton, so force each test to read its isolated HOME.
	preferences.Preferences._instance = None
	app = Browser(librarypath=str(MODELS_DIR), assetspath=str(GLADE_DIR))
	app.window.show_all()
	drain_gtk_events()

	yield app

	call_report = getattr(request.node, "report_call", None)
	if call_report is not None and call_report.failed:
		artifact_dir = request.config.cache.mkdir("gui-screenshots")
		filename = artifact_dir / (request.node.name + ".png")
		if capture_window(app.window, filename):
			print("GUI failure screenshot: %s" % filename)

	try:
		ascpy.setSolverInterrupt(True)
	except Exception:
		pass
	try:
		app.reporter.clearPythonErrorCallback()
		app.library.clear()
	except Exception:
		pass
	for window in gtk_runtime.Window.list_toplevels():
		window.destroy()
	drain_gtk_events()
	preferences.Preferences._instance = None
