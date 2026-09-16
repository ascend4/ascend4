"""End-to-end `a4 solvers` licensing checks (requires a built Gurobi plugin).

Opt in with ASCEND_TEST_GUROBI=1; the configured-license test can contact the
license service. Captured vendor/other solver output is never echoed on failure.
"""
import os
import pathlib
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.environ.get("ASCEND_TEST_GUROBI") == "1",
                     "Set ASCEND_TEST_GUROBI=1 to run licensed listing tests")
class GurobiListing(unittest.TestCase):
    def listing(self, env):
        root = pathlib.Path(__file__).resolve().parents[1]
        result = subprocess.run([str(root / "a4"), "solvers"], cwd=root,
                                env=env, capture_output=True, text=True, timeout=45)
        self.assertEqual(result.returncode, 0, "a4 solvers failed")
        lines = [line.strip() for line in result.stdout.splitlines()
                 if line.strip().startswith("Gurobi:")]
        self.assertEqual(len(lines), 1, "Expected one Gurobi listing entry")
        return lines[0], result.stdout + result.stderr

    def test_configured_license(self):
        line, _ = self.listing(os.environ.copy())
        self.assertRegex(line, r"^Gurobi: \d+\.\d+\.\d+ \(licensed; LICENSEID(?:=[1-9]\d*| unavailable)\)$")

    def test_missing_license(self):
        with tempfile.TemporaryDirectory(prefix="ascend-license-listing-") as path:
            license_file = pathlib.Path(path) / "missing.lic"
            env = dict(os.environ, GRB_LICENSE_FILE=str(license_file))
            line, output = self.listing(env)
            self.assertIn("license unavailable (Gurobi error 10009)", line)
            self.assertNotIn("LICENSEID=", line)
            self.assertNotIn(str(license_file), output)

    def test_malformed_license_does_not_leak(self):
        with tempfile.TemporaryDirectory(prefix="ascend-license-listing-") as path:
            license_file = pathlib.Path(path) / "invalid.lic"
            canary = "ASCEND_TEST_PRIVATE_LICENSE_CONTENT"
            license_file.write_text(canary + "\n", encoding="utf-8")
            env = dict(os.environ, GRB_LICENSE_FILE=str(license_file))
            line, output = self.listing(env)
            self.assertIn("license unavailable (Gurobi error 10009)", line)
            self.assertNotIn("LICENSEID=", line)
            self.assertNotIn(canary, output)
            self.assertNotIn(str(license_file), output)


if __name__ == "__main__":
    unittest.main()
