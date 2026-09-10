"""GUI coverage for ASCEND's embedded IPython console."""

import pytest


pytestmark = pytest.mark.gui


def buffer_text(console):
	text_buffer = console.get_buffer()
	return text_buffer.get_text(
		text_buffer.get_start_iter(), text_buffer.get_end_iter(), False
	)


def test_embedded_ipython_executes_with_browser_namespace(browser, wait_until):
	from ipython_view import IPythonView

	console = browser.consoletext
	assert isinstance(console, IPythonView)
	assert console.IP.user_ns["browser"] is browser

	wait_until(
		lambda: buffer_text(console).startswith("In ["),
		description="initial IPython prompt",
	)
	console.updateNamespace({"ascend_console_test_value": 40})
	console.get_buffer().insert_at_cursor("ascend_console_test_value + 2")
	console._processLine()

	wait_until(
		lambda: "42" in buffer_text(console),
		description="IPython expression result",
	)
	assert console.IP.user_ns["_"] == 42
	assert "Out[" in buffer_text(console)
