"""GUI coverage for solver block labels in the main instance tree."""

import pytest


pytestmark = pytest.mark.gui


def block_labels(browser):
	"""Return displayed block labels keyed by full instance name."""
	labels = {}
	model_view = browser.modelview
	for path, (_local_name, instance) in model_view.otank.items():
		try:
			name = str(browser.sim.getInstanceName(instance))
		except RuntimeError:
			continue
		row = model_view.modelstore.get_iter(path)
		labels[name] = model_view.modelstore.get_value(row, 7)
	return labels


def test_browser_can_toggle_solver_block_column(browser, drain_gtk):
	menu_item = browser.builder.get_object("show_tree_blocks")
	column = browser.modelview.blockcolumn

	assert browser.window.get_visible()
	assert menu_item.get_active()
	assert column.get_visible()

	# GtkCheckMenuItem.activate is the semantic equivalent of a user click and
	# emits the same activate/toggled signals without depending on coordinates.
	menu_item.activate()
	drain_gtk()

	assert not menu_item.get_active()
	assert not column.get_visible()
	assert not browser.prefs.getBoolPref("Browser", "show_tree_blocks", True)

	menu_item.activate()
	drain_gtk()

	assert menu_item.get_active()
	assert column.get_visible()


@pytest.mark.solver
def test_solve_button_updates_variable_and_relation_block_labels(
	browser, wait_until, models_dir
):
	model_file = models_dir / "johnpye" / "testlog10.a4c"
	browser.library.load(str(model_file))
	browser.do_sim(browser.library.findType("testlog10"))

	if not hasattr(browser, "solver") or str(browser.solver.getName()) != "QRSlv":
		if not browser.set_solver("QRSlv"):
			pytest.skip("QRSlv is not available in this build")

	# Keep the test non-modal, then exercise the production button callback and
	# asynchronous solve/idle-refresh path.
	browser.prefs.setBoolPref("SolverReporter", "show_popup", False)
	browser.solvebutton.emit("clicked")

	expected = {
		"x": "fixed",
		"y": "0",
		"z": "1",
		"log_10_expr": "0",
		"log_e_expr": "1",
	}

	def labels_have_updated():
		labels = block_labels(browser)
		return all(labels.get(name) == value for name, value in expected.items())

	wait_until(labels_have_updated, description="solver block labels to refresh")
	assert {name: block_labels(browser)[name] for name in expected} == expected
