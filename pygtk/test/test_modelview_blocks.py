"""GUI coverage for solver block labels in the main instance tree."""

from pathlib import Path

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


def child_values(model, parent, column):
	values = []
	child = model.iter_children(parent)
	while child is not None:
		values.append(model.get_value(child, column))
		child = model.iter_next(child)
	return values


def child_instance_kinds(model_view, parent):
	kinds = []
	child = model_view.sort_model.iter_children(parent)
	while child is not None:
		path = model_view.sort_model.get_value(child, 8)
		instance = model_view.otank[path][1]
		if instance.isRelation():
			kinds.append("relation")
		elif instance.getType().isRefinedSolverVar():
			kinds.append("variable")
		else:
			kinds.append("compound")
		child = model_view.sort_model.iter_next(child)
	return kinds


def find_child(model, parent, name):
	child = model.iter_children(parent)
	while child is not None:
		if model.get_value(child, 0) == name:
			return child
		child = model.iter_next(child)
	raise AssertionError("No child named %r" % name)


def test_browser_can_toggle_solver_block_column(browser, drain_gtk):
	menu_item = browser.builder.get_object("show_tree_blocks")
	column = browser.modelview.blockcolumn

	assert browser.window.get_visible()
	assert menu_item.get_active()
	assert column.get_visible()
	assert browser.modelview.tvcolumns[2].get_expand()
	assert not column.get_expand()

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


def test_relations_last_menu_is_persistent(browser, drain_gtk):
	from gi.repository import Gtk

	menu_item = browser.builder.get_object("relations_appear_last")

	assert menu_item.get_active()
	menu_item.activate()
	drain_gtk()

	assert not browser.modelview.relations_appear_last
	assert not browser.prefs.getBoolPref("Browser", "relations_appear_last", True)
	browser.modelview.sort_model.set_sort_column_id(11, Gtk.SortType.DESCENDING)
	drain_gtk()
	assert browser.prefs.getStringPref("Browser", "tree_sort") == "block"
	assert browser.prefs.getBoolPref("Browser", "tree_sort_descending")

	browser.prefs.save_preferences()
	from preferences import Preferences
	Preferences._instance = None
	reloaded = Preferences()
	assert not reloaded.getBoolPref("Browser", "relations_appear_last", True)
	assert reloaded.getStringPref("Browser", "tree_sort") == "block"
	assert reloaded.getBoolPref("Browser", "tree_sort_descending")


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
		"x": "–",
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

	root = browser.modelview.modelstore.get_iter_first()
	assert browser.modelview.modelstore.get_value(root, 7) == "0–1"


@pytest.mark.solver
def test_multivariable_block_can_put_all_relations_before_all_variables(
	browser, wait_until, drain_gtk
):
	from gi.repository import Gtk

	model_file = Path(__file__).with_name("gui_multivariable_block_tree.a4c")
	browser.library.load(str(model_file))
	browser.do_sim(browser.library.findType("gui_multivariable_block_tree"))

	if not hasattr(browser, "solver") or str(browser.solver.getName()) != "QRSlv":
		if not browser.set_solver("QRSlv"):
			pytest.skip("QRSlv is not available in this build")

	browser.prefs.setBoolPref("SolverReporter", "show_popup", False)
	browser.solvebutton.emit("clicked")
	wait_until(
		lambda: bool(browser.modelview.solver_var_blocks),
		description="multi-variable block ordering data",
	)

	model_view = browser.modelview
	sorted_model = model_view.sort_model
	sorted_model.set_sort_column_id(11, Gtk.SortType.ASCENDING)
	drain_gtk()
	root = sorted_model.get_iter_first()

	assert child_values(sorted_model, root, 7) == ["0", "0", "0", "0"]
	assert child_instance_kinds(model_view, root) == [
		"variable", "variable", "relation", "relation"
	]

	relations_last = browser.builder.get_object("relations_appear_last")
	relations_last.activate()
	drain_gtk()
	root = sorted_model.get_iter_first()
	assert child_instance_kinds(model_view, root) == [
		"relation", "relation", "variable", "variable"
	]

	# Reversing the block sequence does not reverse the meaningful ordering
	# within a simultaneous block.
	sorted_model.set_sort_column_id(11, Gtk.SortType.DESCENDING)
	drain_gtk()
	root = sorted_model.get_iter_first()
	assert child_instance_kinds(model_view, root) == [
		"relation", "relation", "variable", "variable"
	]


@pytest.mark.solver
def test_aliases_share_blocks_and_name_sort_returns_to_model_order(
	browser, wait_until, drain_gtk
):
	from gi.repository import Gtk

	model_file = Path(__file__).with_name("gui_alias_block_tree.a4c")
	browser.library.load(str(model_file))
	browser.do_sim(browser.library.findType("gui_alias_block_tree"))

	if not hasattr(browser, "solver") or str(browser.solver.getName()) != "QRSlv":
		if not browser.set_solver("QRSlv"):
			pytest.skip("QRSlv is not available in this build")

	browser.prefs.setBoolPref("SolverReporter", "show_popup", False)
	browser.solvebutton.emit("clicked")

	store = browser.modelview.modelstore
	root = store.get_iter_first()
	fixeds = find_child(store, root, "fixeds")
	stage = find_child(store, root, "stage")

	wait_until(
		lambda: bool(browser.modelview.solver_var_blocks),
		description="solver block data to refresh",
	)
	known = find_child(store, fixeds, "known")
	assert store.get_value(known, 7) == "–", (
		browser.modelview.solver_instance_key(
			browser.modelview.otank[store.get_value(known, 8)][1]
		),
		browser.modelview.solver_fixed_vars,
	)
	assert store.get_value(fixeds, 7) == "–"
	assert store.get_value(stage, 7) != ""

	alias_rows = [
		find_child(store, stage, "original"),
		find_child(store, stage, "local_alias"),
		find_child(store, stage, "merged"),
	]
	alias_labels = {store.get_value(row, 7) for row in alias_rows}
	assert alias_labels != {""}
	assert len(alias_labels) == 1
	assert len({
		browser.modelview.otank[store.get_value(row, 8)][1].getInstanceId()
		for row in alias_rows
	}) == 1

	# The Name header starts ascending: descending and then the default sort
	# expose GTK's usual three-state cycle. The default is declaration order.
	sorted_model = browser.modelview.sort_model
	sorted_root = sorted_model.get_iter_first()
	sorted_stage = find_child(sorted_model, sorted_root, "stage")
	assert child_values(sorted_model, sorted_stage, 0) == [
		"local_alias", "merged", "original", "result", "calculate_result"
	]

	# With relation grouping disabled, Name returns to a purely alphabetical
	# ordering and the same preference also changes block ordering.
	relations_last = browser.builder.get_object("relations_appear_last")
	relations_last.activate()
	drain_gtk()
	sorted_root = sorted_model.get_iter_first()
	sorted_stage = find_child(sorted_model, sorted_root, "stage")
	assert child_values(sorted_model, sorted_stage, 0) == sorted(
		child_values(sorted_model, sorted_stage, 0), key=str.casefold
	)
	relations_last.activate()
	drain_gtk()

	browser.modelview.tvcolumns[0].clicked()
	drain_gtk()
	assert sorted_model.get_sort_column_id() == (0, Gtk.SortType.DESCENDING)
	sorted_root = sorted_model.get_iter_first()
	sorted_stage = find_child(sorted_model, sorted_root, "stage")
	assert child_values(sorted_model, sorted_stage, 0)[-1] == "calculate_result"
	assert browser.prefs.getStringPref("Browser", "tree_sort") == "name"
	assert browser.prefs.getBoolPref("Browser", "tree_sort_descending")

	browser.modelview.tvcolumns[0].clicked()
	drain_gtk()
	assert sorted_model.get_sort_column_id()[0] in (
		None, Gtk.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID
	)
	assert browser.prefs.getStringPref("Browser", "tree_sort") == "model"
	sorted_root = sorted_model.get_iter_first()
	sorted_stage = find_child(sorted_model, sorted_root, "stage")
	assert child_values(sorted_model, sorted_stage, 0) == [
		"original", "local_alias", "merged", "result", "calculate_result"
	]

	# Block ascending uses the lower end of compound ranges; descending uses
	# the upper end. Fixed-only compounds remain at the corresponding edge.
	browser.modelview.blockcolumn.clicked()
	drain_gtk()
	assert sorted_model.get_sort_column_id() == (11, Gtk.SortType.ASCENDING)
	assert browser.prefs.getStringPref("Browser", "tree_sort") == "block"
	assert not browser.prefs.getBoolPref("Browser", "tree_sort_descending")
	sorted_root = sorted_model.get_iter_first()
	ascending = child_values(sorted_model, sorted_root, 10)
	assert ascending[0] == 1  # BLOCK_FIXED
	assert child_values(sorted_model, sorted_root, 11)[1:] == sorted(
		child_values(sorted_model, sorted_root, 11)[1:]
	)

	# In incidence order the outer relation occupies the first solver position;
	# the stage submodel appears once, at its first contained-variable position.
	relations_last.activate()
	drain_gtk()
	sorted_root = sorted_model.get_iter_first()
	assert child_values(sorted_model, sorted_root, 0) == [
		"fixeds", "connect", "stage"
	]
	assert child_values(sorted_model, sorted_root, 0).count("stage") == 1
	relations_last.activate()
	drain_gtk()

	browser.modelview.blockcolumn.clicked()
	drain_gtk()
	assert sorted_model.get_sort_column_id() == (11, Gtk.SortType.DESCENDING)
	assert browser.prefs.getBoolPref("Browser", "tree_sort_descending")
	sorted_root = sorted_model.get_iter_first()
	descending_kinds = child_values(sorted_model, sorted_root, 10)
	numbered_highs = [
		sorted_model.get_value(child, 12)
		for child in [
			find_child(sorted_model, sorted_root, name)
			for name in child_values(sorted_model, sorted_root, 0)
		]
		if sorted_model.get_value(child, 10) == 2
	]
	assert numbered_highs == sorted(numbered_highs, reverse=True)
	assert descending_kinds.index(1) > max(
		i for i, kind in enumerate(descending_kinds) if kind == 2
	)

	browser.modelview.blockcolumn.clicked()
	drain_gtk()
	assert sorted_model.get_sort_column_id()[0] in (
		None, Gtk.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID
	)
	sorted_root = sorted_model.get_iter_first()
	assert child_values(sorted_model, sorted_root, 0) == ["fixeds", "stage", "connect"]
