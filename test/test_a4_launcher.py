"""License discovery and test display setup; run after `scons a4`."""
import contextlib
import io
import os
import pathlib
import runpy
import sys
import tempfile
import unittest
from unittest import mock


LAUNCHER = pathlib.Path(__file__).resolve().parents[1] / "a4"


class PytestDisplayLauncher(unittest.TestCase):
    def launch(self, arguments, environment=None, missing=(), returncode=0):
        """Exercise real argument parsing without starting a display or pytest."""
        stderr = io.StringIO()
        with contextlib.ExitStack() as stack:
            stack.enter_context(mock.patch.dict(os.environ, environment or {}, clear=True))
            stack.enter_context(mock.patch.object(sys, "argv", [str(LAUNCHER)] + arguments))
            for name in ("getuid", "geteuid", "getgid", "getegid"):
                stack.enter_context(mock.patch.object(os, name, return_value=1000, create=True))
            which = stack.enter_context(mock.patch(
                "shutil.which", side_effect=lambda name: None if name in missing else "/usr/bin/" + name
            ))
            run = stack.enter_context(mock.patch(
                "subprocess.run", return_value=mock.Mock(returncode=returncode)
            ))
            stack.enter_context(contextlib.redirect_stderr(stderr))
            # The launcher suppresses tracebacks unless --debug is given.
            stack.enter_context(mock.patch.object(sys, "tracebacklimit", 1000, create=True))
            with self.assertRaises(SystemExit) as exit_result:
                runpy.run_path(str(LAUNCHER), run_name="__main__")
        return run, which, exit_result.exception.code, stderr.getvalue()

    def test_default_uses_private_display_with_or_without_desktop(self):
        for inherited in ({}, {"DISPLAY": ":42", "WAYLAND_DISPLAY": "wayland-0", "GDK_BACKEND": "wayland"}):
            with self.subTest(environment=inherited):
                run, _, code, stderr = self.launch(["pytest", "test/", "-q"], inherited)
                self.assertEqual(code, 0)
                command = run.call_args.args[0]
                self.assertEqual(command[:2], ["/usr/bin/xvfb-run", "-a"])
                self.assertEqual(command[3:], ["-m", "pytest", "test/", "-q"])
                env = run.call_args.kwargs["env"]
                self.assertEqual(env["GDK_BACKEND"], "x11")
                self.assertNotIn("WAYLAND_DISPLAY", env)
                self.assertIn("ascend_pytest", env["PYTEST_PLUGINS"].split(","))
                self.assertEqual(pathlib.Path(env["PYTHONPATH"].split(os.pathsep)[0]), LAUNCHER.parent / "tools")
                self.assertIn("private Xvfb display", stderr)

    def test_visible_preserves_display_and_needs_no_xvfb(self):
        inherited = {"DISPLAY": ":42", "WAYLAND_DISPLAY": "wayland-0", "GDK_BACKEND": "wayland"}
        run, which, code, stderr = self.launch(
            ["pytest", "--visible", "test/", "-q"], inherited,
            missing=("xvfb-run", "Xvfb", "xauth"),
        )
        self.assertEqual(code, 0)
        which.assert_not_called()
        self.assertEqual(run.call_args.args[0][1:], ["-m", "pytest", "test/", "-q"])
        for key, value in inherited.items():
            self.assertEqual(run.call_args.kwargs["env"][key], value)
        self.assertNotIn("private Xvfb display", stderr)

    def test_missing_display_tools_fail_without_desktop_fallback(self):
        for missing in ("xvfb-run", "Xvfb", "xauth"):
            with self.subTest(missing=missing):
                run, _, code, _ = self.launch(["pytest"], {"DISPLAY": ":42"}, missing=(missing,))
                run.assert_not_called()
                self.assertIn("missing: " + missing, code)
                self.assertIn("sudo apt-get install xvfb xauth", code)
                self.assertIn("--visible", code)

    def test_pytest_options_and_separator_are_forwarded(self):
        for arguments in (["-q", "-k", "some_test", "test/"], ["--", "-q", "test/"], ["test/", "--tb=short"]):
            with self.subTest(arguments=arguments):
                run, _, code, _ = self.launch(["pytest", "--visible"] + arguments)
                self.assertEqual(code, 0)
                expected = arguments[1:] if arguments[0] == "--" else arguments
                self.assertEqual(run.call_args.args[0][1:], ["-m", "pytest"] + expected)

    def test_help_is_forwarded_with_and_without_visible(self):
        for visible in ([], ["--visible"]):
            for help_flag in ("-h", "--help"):
                with self.subTest(visible=visible, help_flag=help_flag):
                    run, _, code, _ = self.launch(["pytest"] + visible + [help_flag])
                    self.assertEqual(code, 0)
                    self.assertEqual(run.call_args.args[0][-3:], ["-m", "pytest", help_flag])

    def test_display_wraps_debugger_not_the_other_way_around(self):
        run, _, code, _ = self.launch(["--gdb1", "pytest", "test/"])
        self.assertEqual(code, 0)
        self.assertEqual(run.call_args.args[0][:3], ["/usr/bin/xvfb-run", "-a", "gdb"])
        self.assertEqual(run.call_args.args[0][-1], "--no-isolation")

    def test_existing_pytest_plugins_are_preserved(self):
        run, _, code, _ = self.launch(["pytest"], {"PYTEST_PLUGINS": "other_plugin,ascend_pytest"})
        self.assertEqual(code, 0)
        self.assertEqual(run.call_args.kwargs["env"]["PYTEST_PLUGINS"], "other_plugin,ascend_pytest")

    def test_non_pytest_commands_do_not_use_xvfb(self):
        run, which, code, _ = self.launch(["solvers"])
        self.assertEqual(code, 0)
        which.assert_not_called()
        self.assertEqual(pathlib.Path(run.call_args.args[0][-1]).name, "listsolvers.py")
        self.assertIsNone(run.call_args.kwargs["env"])

    def test_test_runner_exit_code_is_preserved(self):
        _, _, code, _ = self.launch(["pytest"], returncode=5)
        self.assertEqual(code, 5)


class GurobiLicenseDiscovery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        launcher = pathlib.Path(__file__).resolve().parents[1] / "a4"
        cls.configure = staticmethod(runpy.run_path(str(launcher))["configure_gurobi_license"])

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="ascend-license-test-")
        self.addCleanup(self.temp.cleanup)
        self.user_home = pathlib.Path(self.temp.name)
        self.license_file = self.user_home / ".config" / "ascend" / "gurobi.lic"
        self.license_file.parent.mkdir(parents=True)

    def test_drop_in(self):
        self.license_file.touch()
        env = {}
        self.assertTrue(self.configure(env, self.user_home))
        self.assertEqual(env, {"GRB_LICENSE_FILE": str(self.license_file)})

    def test_missing_preserves_native_search(self):
        env = {}
        self.assertFalse(self.configure(env, self.user_home))
        self.assertNotIn("GRB_LICENSE_FILE", env)

    def test_directory_is_not_a_license(self):
        self.license_file.mkdir()
        env = {}
        self.assertFalse(self.configure(env, self.user_home))
        self.assertNotIn("GRB_LICENSE_FILE", env)

    def test_explicit_override_is_preserved(self):
        self.license_file.touch()
        env = {"GRB_LICENSE_FILE": "/chosen/gurobi.lic"}
        self.assertFalse(self.configure(env, self.user_home))
        self.assertEqual(env["GRB_LICENSE_FILE"], "/chosen/gurobi.lic")

    def test_explicit_empty_override_is_preserved(self):
        self.license_file.touch()
        env = {"GRB_LICENSE_FILE": ""}
        self.assertFalse(self.configure(env, self.user_home))
        self.assertEqual(env["GRB_LICENSE_FILE"], "")

    def test_native_home_file_is_left_to_gurobi(self):
        (self.user_home / "gurobi.lic").touch()
        env = {}
        self.assertFalse(self.configure(env, self.user_home))
        self.assertNotIn("GRB_LICENSE_FILE", env)


if __name__ == "__main__":
    unittest.main()
