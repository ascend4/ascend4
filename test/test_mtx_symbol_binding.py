"""Matrix cleanup must coexist with the C11 mutex API in a dlopen client.

Run with `python3 -m unittest discover -s test -p test_mtx_symbol_binding.py`.
This deliberately loads libascend from Python: a directly linked C test can
put libascend ahead of libc and conceal the original symbol collision.
"""
import ctypes
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(sys.platform.startswith('linux'), 'ELF/Linux loader regression')
class MatrixSymbolBinding(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.library_path = ROOT/'libascend.so'
        if not cls.library_path.is_file():
            raise unittest.SkipTest('Build libascend.so first')
        if not shutil.which('nm'):
            raise unittest.SkipTest('nm is required for the exported-symbol check')
        cls.library = ctypes.CDLL(str(cls.library_path), mode=ctypes.RTLD_GLOBAL)

    def test_export_does_not_shadow_c11_mutex_destructor(self):
        output = subprocess.check_output(['nm', '-D', '--defined-only', str(self.library_path)], text=True)
        symbols = {line.split()[-1].split('@')[0] for line in output.splitlines() if len(line.split()) >= 3}
        self.assertNotIn('mtx_destroy', symbols, 'ASCEND must not export the C11 mutex destructor name')
        self.assertIn('asc_mtx_destroy', symbols)

    def test_dlopen_client_binds_matrix_cleanup_and_can_use_c11_mutexes(self):
        compiler = shutil.which('cc')
        if not compiler:
            self.skipTest('A C compiler is required for the shared-library fixture')
        with tempfile.TemporaryDirectory(prefix='ascend-mtx-binding-') as tmp:
            tmp = Path(tmp)
            source = tmp/'client.c'
            source.write_text('''
#include <stdint.h>
#include <threads.h>
#include <ascend/linear/mtx.h>
uintptr_t matrix_cleanup_address(void){ return (uintptr_t)asc_mtx_destroy; }
int exercise_both_destructors(void){
    mtx_t mutex;
    if(mtx_init(&mutex, mtx_plain) != thrd_success) return 1;
    if(mtx_lock(&mutex) != thrd_success) return 2;
    if(mtx_unlock(&mutex) != thrd_success) return 3;
    mtx_destroy(&mutex);
    mtx_matrix_t matrix = mtx_create();
    mtx_set_order(matrix, 32);
    mtx_coord_t c = {0, 0};
    mtx_set_value(matrix, &c, 2.0);
    int valid = mtx_value(matrix, &c) == 2.0;
    asc_mtx_destroy(matrix);
    return valid ? 0 : 4;
}
''')
            library = tmp/'client.so'
            subprocess.run([compiler, '-std=c11', '-shared', '-fPIC', '-pthread',
                            '-I'+str(ROOT), str(source), '-L'+str(ROOT), '-lascend',
                            '-Wl,-rpath,'+str(ROOT), '-o', str(library)], check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            client = ctypes.CDLL(str(library))
            client.matrix_cleanup_address.restype = ctypes.c_size_t
            client.exercise_both_destructors.restype = ctypes.c_int
            intended = ctypes.cast(self.library.asc_mtx_destroy, ctypes.c_void_p).value
            self.assertEqual(client.matrix_cleanup_address(), intended)
            self.assertEqual(client.exercise_both_destructors(), 0)


if __name__ == '__main__':
    unittest.main()
