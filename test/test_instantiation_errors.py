#!/usr/bin/env python3
"""Instantiation failures must raise Python errors, not abort the process.

Run with ./a4 script test/test_instantiation_errors.py (no solver required).
Each case runs in a subprocess so an assertion regression fails the test
without killing the rest of the test runner. Recovery is checked inside
that SAME subprocess, not by starting a fresh compiler after failure.
"""
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]


def probe(name):
    import ascpy

    check = unittest.TestCase()
    lib = ascpy.Library()
    lib.load(str(ROOT / 'models/test/compiler/instantiation_errors.a4c'))
    reporter = ascpy.getReporter()
    messages = []

    def capture(severity, filename, line, message):
        messages.append(message)
        return 0

    reporter.setPythonErrorCallback(capture)
    try:
        with check.assertRaisesRegex(RuntimeError, name):
            # Running on_load must never be attempted for an invalid instance.
            lib.findType(name).getSimulation('rejected', True)
        if name != 'wrapper_positive':
            check.assertTrue(any('Failed requirement: (n > 0)' in m for m in messages),
                             f'Lost WHERE diagnostic: {messages}')
        for attempt in range(3):
            # A leftover diagnostic tree would buffer this message instead of
            # delivering it immediately, or contaminate the next instantiation.
            messages.clear()
            reporter.reportNote('instantiation-recovery-marker')
            check.assertTrue(any('instantiation-recovery-marker' in m for m in messages))
            messages.clear()
            good = lib.findType('wrapper_good_root').getSimulation('recovered', True)
            check.assertEqual(good.getModel().n.getIntValue(), 1)
            check.assertEqual(good.getModel().x.getRealValue(), 42)
            check.assertFalse(any('Failed requirement' in m for m in messages))
        print('recovery checks passed')
    finally:
        reporter.clearPythonErrorCallback()


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 and built ascpy')
class InstantiationErrors(unittest.TestCase):
    def check_probe(self, name):
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--probe', name],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0,
                         f'{name}: exit {result.returncode}\n{result.stdout}\n{result.stderr}')
        self.assertIn('recovery checks passed', result.stdout)

    def test_failed_root_where(self):
        self.check_probe('wrapper_bad_root')

    def test_failed_child_where(self):
        self.check_probe('wrapper_bad_child')

    def test_unreduced_parameterised_root(self):
        # This rejection returns NULL without a structured WHERE diagnostic.
        self.check_probe('wrapper_positive')


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--probe':
        probe(sys.argv[2])
    else:
        unittest.main()
