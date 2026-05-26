#!/usr/bin/env python3
"""Static checks for the Python 3/Gtk3 canvas port."""

from __future__ import annotations

import py_compile
import re
import unittest
from pathlib import Path


CANVAS_DIR = Path(__file__).resolve().parent


class CanvasPortingTests(unittest.TestCase):
	def python_files(self):
		return sorted(CANVAS_DIR.glob("*.py"))

	def test_canvas_python_files_compile(self):
		for path in self.python_files():
			with self.subTest(path=path.name):
				py_compile.compile(str(path), doraise=True)

	def test_removed_legacy_imports_do_not_return(self):
		legacy_patterns = [
			re.compile(r"^\s*import\s+gtk\b", re.MULTILINE),
			re.compile(r"^\s*import\s+pygtk\b", re.MULTILINE),
			re.compile(r"pygtk\.require"),
			re.compile(r"gtksourceview2"),
			re.compile(r"^\s*from\s+gaphas\.state|^\s*import\s+gaphas\.state", re.MULTILINE),
			re.compile(r"from\s+gaphas\.tool\s+import\s+[A-Z]"),
			re.compile(r"=\s*file\("),
		]
		for path in self.python_files():
			if path.name == "test_py3_gtk3_port.py":
				continue
			text = path.read_text()
			for pattern in legacy_patterns:
				with self.subTest(path=path.name, pattern=pattern.pattern):
					self.assertIsNone(pattern.search(text))


if __name__ == "__main__":
	unittest.main()
