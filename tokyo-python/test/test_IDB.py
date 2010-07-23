import unittest
import sys
import os
import tempfile
import shutil

from tokyo.dystopia import IDBOREADER, IDBOWRITER, IDBOCREAT, IDB, Error


class IDBTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(), "tmp_td_test.tdi")
        self.db = IDB()
        self.db.open(self.path, IDBOWRITER | IDBOCREAT)

    def tearDown(self):
        self.db.close()
        shutil.rmtree(self.path)
        self.db = None


class IDBTestDict(IDBTest):

    def test_contains(self):
        self.assertRaises(TypeError, self.db.__contains__)
        self.assertRaises(TypeError, self.db.__contains__, "a")
        self.assertTrue(not (1 in self.db))
        self.assertTrue(1 not in self.db)
        self.db[1] = "a"
        self.db[2] = "b"
        self.assertTrue(1 in self.db)
        self.assertTrue(2 in self.db)
        self.assertTrue(3 not in self.db)

    def test_len(self):
        self.assertEqual(len(self.db), 0)
        self.db[1] = "a"
        self.db[2] = "b"
        self.assertEqual(len(self.db), 2)

    def test_getitem(self):
        self.assertRaises(TypeError, self.db.__getitem__)
        self.assertRaises(TypeError, self.db.__getitem__, "a")
        self.assertRaises(KeyError, self.db.__getitem__, 1)
        self.db[1] = "a"
        self.db[2] = "b"
        self.assertEqual(self.db[1], "a")
        self.assertEqual(self.db[2], "b")
        self.db[3] = "c"
        self.db[4] = "d"
        self.assertEqual(self.db[3], "c")
        self.assertEqual(self.db[4], "d")

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__)
        self.assertRaises(TypeError, self.db.__setitem__, 1)
        self.assertRaises(TypeError, self.db.__setitem__, "a", 1)
        self.assertRaises(TypeError, self.db.__setitem__, 1, 1)
        self.db[1] = "a"
        self.db[2] = "b"
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: "a", 2: "b"})
        self.db[3] = "c"
        self.db[1] = "d"
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: "d", 2: "b", 3: "c"})
        del self.db[2]
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: "d", 3: "c"})

    #def test_clear(self):
    #    self.db[1] = "a"
    #    self.db[2] = "b"
    #    self.assertEqual(len(self.db), 2)
    #    self.db.clear()
    #    self.assertEqual(len(self.db), 0)
    #    d = dict(self.db.iteritems())
    #    self.assertEqual(d, {})


all_tests = (
             "IDBTestDict",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)