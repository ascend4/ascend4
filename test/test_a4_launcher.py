"""License discovery in the generated launcher; run after `scons a4`."""
import pathlib
import runpy
import tempfile
import unittest


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
