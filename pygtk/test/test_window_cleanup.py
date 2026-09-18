"""Regression coverage for browser-window ownership during GUI teardown."""

from types import SimpleNamespace

import pytest

from conftest import destroy_browser_windows


pytestmark = pytest.mark.gui


def test_cleanup_preserves_windows_not_owned_by_browser(gtk_runtime, drain_gtk):
	Gtk = gtk_runtime
	builder = Gtk.Builder()
	builder.add_from_string("""
		<interface>
			<object class="GtkWindow" id="browserwin"/>
			<object class="GtkWindow" id="hidden_dialog"/>
		</interface>
	""")
	main = builder.get_object("browserwin")
	dialog = builder.get_object("hidden_dialog")
	unrelated = Gtk.Window()
	# Even a transient relationship does not transfer ownership: GTK's own
	# tooltip window can be transient for an application window.
	unrelated.set_transient_for(main)
	destroyed = set()
	for name, window in (("main", main), ("dialog", dialog), ("unrelated", unrelated)):
		window.connect("destroy", lambda _window, name=name: destroyed.add(name))
	try:
		main.show_all()
		unrelated.show_all()
		drain_gtk()
		destroy_browser_windows(SimpleNamespace(builder=builder), Gtk)
		drain_gtk()
		assert destroyed == {"main", "dialog"}
		assert unrelated.get_visible()
	finally:
		for window in (unrelated, dialog, main):
			window.destroy()
		drain_gtk()
