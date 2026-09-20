"""Runtime HSL discovery with IPOPT >= 3.14 (select via PKG_CONFIG_PATH).

Run: python3 -m unittest discover -s test -p test_ipopt_hsl.py
The fixture libraries export symbols only; no numerical routines are called.
"""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(sys.platform.startswith("linux"), "Linux shared-library fixtures")
class IpoptHSLDiscovery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not all(shutil.which(tool) for tool in ("pkg-config", "c++", "cc")):
            raise unittest.SkipTest("Requires pkg-config and C/C++ compilers")
        if subprocess.call(["pkg-config", "--atleast-version=3.14", "ipopt"]) != 0:
            raise unittest.SkipTest("Requires IPOPT >= 3.14")
        cls.temp = tempfile.TemporaryDirectory(prefix="ascend-ipopt-hsl-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.path = Path(cls.temp.name)
        source = cls.path / "query.cpp"
        source.write_text('''
#include "ipopt_hsl.h"
#include <IpLinearSolvers.h>
#include <iostream>
#include <string>
int main(){
    std::cout << (IpoptGetAvailableLinearSolvers(1) & IPOPTLINEARSOLVER_ALLHSL) << std::endl;
    std::cout << (IpoptGetAvailableLinearSolvers(0) & IPOPTLINEARSOLVER_ALLHSL) << std::endl;
    std::string library;
    while(std::getline(std::cin, library)){
        std::cout << asc_ipopt_hsl_available(library == "default" ? NULL : library.c_str()) << std::endl;
    }
}
''')
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "--libs", "ipopt"], text=True))
        cls.runner = cls.path / "query"
        subprocess.run(["c++", "-I" + str(ROOT / "solvers/ipopt"), str(source),
                        str(ROOT / "solvers/ipopt/ipopt_hsl.cpp"), "-o", str(cls.runner)]
                       + flags, check=True, capture_output=True)

    def setUp(self):
        self.fixture = tempfile.TemporaryDirectory(dir=self.path)
        self.addCleanup(self.fixture.cleanup)
        self.directory = Path(self.fixture.name)
        env = dict(os.environ)
        env["LD_LIBRARY_PATH"] = str(self.directory) + ":" + env.get("LD_LIBRARY_PATH", "")
        self.process = subprocess.Popen([str(self.runner)], env=env, text=True,
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.addCleanup(self.close_runner)
        self.linked = int(self.process.stdout.readline())
        self.capabilities = int(self.process.stdout.readline())

    def close_runner(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)
        self.process.stdout.close()

    def library(self, name, symbols):
        source = self.directory / (name + ".c")
        source.write_text("\n".join("void " + symbol + "(void) {}" for symbol in symbols))
        library = self.directory / name
        subprocess.run(["cc", "-shared", "-fPIC", str(source), "-o", str(library)],
                       check=True, capture_output=True)
        return library

    def query(self, library):
        self.process.stdin.write(str(library) + "\n")
        self.process.stdin.flush()
        return int(self.process.stdout.readline())

    def test_missing_library_keeps_only_linked_routines(self):
        self.assertEqual(self.query(self.directory / "missing.so"), self.linked)

    def test_incomplete_routine_is_not_advertised(self):
        library = self.library("partial.so", ["ma27ad_", "ma27bd_", "ma27cd_"])
        self.assertEqual(self.query(library), self.linked)

    def test_complete_routines_and_configured_path(self):
        if self.capabilities & 0x003 != 0x003:
            self.skipTest("IPOPT does not support both MA27 and MA57")
        library = self.library("custom.so", ["ma27ad_", "ma27bd_", "ma27cd_", "ma27id_",
                                              "ma57ad_", "ma57bd_", "ma57cd_", "ma57ed_", "ma57id_"])
        self.assertEqual(self.query(library), self.linked | 0x001 | 0x002)

    def test_drop_in_and_removal_without_restart(self):
        if not self.capabilities & 0x001:
            self.skipTest("IPOPT does not support MA27")
        # The library does not exist when the query process starts.
        library = self.library("libhsl.so", ["ma27ad_", "ma27bd_", "ma27cd_", "ma27id_"])
        self.assertEqual(self.query("default"), self.linked | 0x001)
        library.unlink()
        self.assertEqual(self.query(library), self.linked)


if __name__ == "__main__":
    unittest.main()
