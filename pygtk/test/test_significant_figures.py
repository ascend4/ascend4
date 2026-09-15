"""GUI coverage for configurable significant-figure display."""

from pathlib import Path

import pytest


pytestmark = pytest.mark.gui


def find_child(model, parent, name):
	child = model.iter_children(parent)
	while child is not None:
		if model.get_value(child, 0) == name:
			return child
		child = model.iter_next(child)
	raise AssertionError("No child named %r" % name)


def row_instance(model_view, row):
	path = model_view.sort_model.get_value(row, 8)
	return model_view.otank[path][1]


def test_significant_figures_menu_formats_and_persists(browser, drain_gtk):
	assert browser.significant_figures is None
	assert browser.significant_figures_menu_items[None].get_active()
	assert browser.format_real_value(1.23456789012345) == "1.23456789012345"

	browser.significant_figures_menu_items[4].activate()
	drain_gtk()

	assert browser.significant_figures == 4
	assert browser.prefs.getStringPref("Browser", "significant_figures") == "4"
	assert browser.format_real_value(1.23456789012345) == "1.235"
	assert browser.format_display_value("12345.6789 kPa") == "1.235e+04 kPa"
	assert browser.format_real_value(1.23456789012345, full_precision=True) == (
		"1.23456789012345"
	)

	browser.prefs.save_preferences()
	from preferences import Preferences
	Preferences._instance = None
	assert Preferences().getStringPref("Browser", "significant_figures") == "4"


@pytest.mark.solver
def test_tree_and_observer_edit_full_precision_values(
	browser, wait_until, drain_gtk
):
	from gi.repository import Gtk

	browser.significant_figures_menu_items[4].activate()
	model_file = Path(__file__).with_name("gui_significant_figures.a4c")
	browser.library.load(str(model_file))
	browser.do_sim(browser.library.findType("gui_significant_figures"))

	if not hasattr(browser, "solver") or str(browser.solver.getName()) != "QRSlv":
		if not browser.set_solver("QRSlv"):
			pytest.skip("QRSlv is not available in this build")

	browser.prefs.setBoolPref("SolverReporter", "show_popup", False)
	browser.solvebutton.emit("clicked")
	wait_until(
		lambda: browser._get_simulation_status_message().startswith("🟢"),
		description="significant-figures model to solve",
	)

	model_view = browser.modelview
	sorted_model = model_view.sort_model
	root = sorted_model.get_iter_first()
	fixed_row = find_child(sorted_model, root, "fixed_value")
	solved_row = find_child(sorted_model, root, "solved_value")
	fixed_instance = row_instance(model_view, fixed_row)
	solved_instance = row_instance(model_view, solved_row)

	assert sorted_model.get_value(fixed_row, 2) == "1.235"
	assert sorted_model.get_value(solved_row, 2) == "0.4115"
	assert fixed_instance.getRealValue() == pytest.approx(1.23456789012345)

	for row, instance in (
		(fixed_row, fixed_instance), (solved_row, solved_instance)
	):
		entry = Gtk.Entry()
		model_view.cell_editing_started_callback(
			model_view.valuerenderer,
			entry,
			sorted_model.get_path(row).to_string(),
		)
		assert entry.get_text() == browser.get_instance_display_value(
			instance, full_precision=True
		)
		assert entry.get_text() != sorted_model.get_value(row, 2)

	observer = browser.create_observer()
	observer.add_instance(fixed_instance)
	column = observer.cols[0]
	assert column.display_value(fixed_instance.getRealValue()) == "1.235"
	observer_entry = Gtk.Entry()
	observer.on_view_cell_editing_started(
		observer.renderers[0], observer_entry, "0", column
	)
	assert observer_entry.get_text() == "1.23456789012345"

	browser.significant_figures_menu_items[None].activate()
	drain_gtk()
	root = sorted_model.get_iter_first()
	fixed_row = find_child(sorted_model, root, "fixed_value")
	assert sorted_model.get_value(fixed_row, 2) == "1.23456789012345"
