"""Exercise real SWIG array lookup without requiring a solver."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(importlib.util.find_spec('ascpy'), 'Run with ./a4 pytest and built ascpy')
class ArrayIndexing(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ascpy
        cls.lib = ascpy.Library()
        cls.lib.load(str(ROOT/'models/test/compiler/python_array_indexing.a4c'))
        cls.sim = cls.lib.findType('python_array_indexing').getSimulation('array_indexing',False)
        cls.model = cls.sim.getModel()

    def test_string_lookup_and_shared_identity(self):
        x = self.model.x
        for k,label in enumerate(('left','right','with space','10')):
            x[label].setRealValue(10+k)
            self.assertEqual(str(x[label].getName()),label)
            child = next(c for c in x.getChildren() if str(c.getName()) == label)
            self.assertEqual(child.getRealValue(),10+k)

    def test_native_symbols(self):
        for symbol in self.model.labels.getSetValue():
            self.model.x[symbol].setRealValue(42)
            self.assertEqual(self.model.x[str(symbol)].getRealValue(),42)

    def test_integer_and_nested_indices(self):
        self.model.ints[-2].setRealValue(8)
        self.assertEqual(self.model.ints[-2].getRealValue(),8)
        self.model.nested['with space'][3].setRealValue(17)
        self.assertEqual(self.model.nested['with space'][3].getRealValue(),17)

    def test_missing_symbol(self):
        with self.assertRaisesRegex(IndexError,'Invalid symbol index'):
            self.model.x['missing']

    def test_wrong_index_domain(self):
        with self.assertRaisesRegex(RuntimeError,'not a symbol-indexed array'):
            self.model.ints['3']
        with self.assertRaises(RuntimeError):
            self.model.x[10]
        with self.assertRaisesRegex(RuntimeError,'not a symbol-indexed array'):
            self.model['x']

    def test_unsupported_key_type(self):
        # SWIG reports a null wrapped value as ValueError; other unsupported
        # key types fail overload resolution with TypeError.
        with self.assertRaises((TypeError,ValueError)):
            self.model.x[None]
        for key in (('left',), 1.5):
            with self.subTest(key=key), self.assertRaises(TypeError):
                self.model.x[key]


if __name__ == '__main__':
    unittest.main()
