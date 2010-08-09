import unittest
import sys
import os
import tempfile
import shutil

from tokyo.dystopia import (JDBOREADER, JDBOWRITER, JDBOCREAT, JDB, Error,
                            JDBSSUBSTR, JDBSPREFIX, JDBSSUFFIX, JDBSFULL)


class JDBTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(), "tmp_td_test.tdj")
        self.db = JDB()
        self.db.open(self.path, JDBOWRITER | JDBOCREAT)

    def tearDown(self):
        self.db.close()
        shutil.rmtree(self.path)
        self.db = None


class JDBTestDict(JDBTest):

    def test_contains(self):
        self.assertRaises(TypeError, self.db.__contains__)
        self.assertRaises(TypeError, self.db.__contains__, "a")
        self.assertTrue(not (1 in self.db))
        self.assertTrue(1 not in self.db)
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.assertTrue(1 in self.db)
        self.assertTrue(2 in self.db)
        self.assertTrue(3 not in self.db)

    def test_len(self):
        self.assertEqual(len(self.db), 0)
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.assertEqual(len(self.db), 2)

    def test_getitem(self):
        self.assertRaises(TypeError, self.db.__getitem__)
        self.assertRaises(TypeError, self.db.__getitem__, "a")
        self.assertRaises(KeyError, self.db.__getitem__, 1)
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.assertEqual(self.db[1], ("a", "b"))
        self.assertEqual(self.db[2], ("c", "d"))
        self.db[3] = ("e", "f")
        self.db[4] = ("g", "h")
        self.assertEqual(self.db[3], ("e", "f"))
        self.assertEqual(self.db[4], ("g", "h"))
        if sys.version_info[0] >= 3:
            for v in self.db.__getitem__(1):
                self.assertTrue(type(v) is str)
        else:
            for v in self.db.__getitem__(1):
                self.assertTrue(type(v) is unicode)

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__)
        self.assertRaises(TypeError, self.db.__setitem__, 1)
        self.assertRaises(TypeError, self.db.__setitem__, "a", (1,))
        self.assertRaises(TypeError, self.db.__setitem__, 1, (1,))
        self.assertRaises(OverflowError, self.db.__setitem__, 0, ("a",))
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: ("a", "b"), 2: ("c", "d")})
        self.db[3] = ("e", "f")
        self.db[1] = ("g", "h")
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: ("g", "h"), 2: ("c", "d"), 3: ("e", "f")})
        del self.db[2]
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: ("g", "h"), 3: ("e", "f")})

    def test_clear(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.assertEqual(len(self.db), 2)
        self.db.clear()
        self.assertEqual(len(self.db), 0)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {})


class JDBTestIter(JDBTest):

    def test_iter(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.db[3] = ("e", "f")
        i = iter(self.db)
        self.assertTrue(1 in i)
        i = iter(self.db)
        self.assertEqual([1, 2, 3], sorted(i))
        i = iter(self.db)
        a = next(i)
        b = next(i)
        c = next(i)
        self.assertRaises(StopIteration, next, i)
        self.assertEqual([1, 2, 3], sorted((a, b, c)))
        i = iter(self.db)
        a = next(i)
        del self.db[2]
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        self.db[4] = ("i", "j")
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        del self.db[4]
        self.db[4] = ("k", "l")
        self.assertRaises(Error, next, i)

    def test_iterkeys(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.db[3] = ("e", "f")
        self.assertEqual([1, 2, 3], sorted(list(self.db.iterkeys())))

    def test_itervalues(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.db[3] = ("e", "f")
        self.assertEqual([("a", "b"), ("c", "d"), ("e", "f")],
                         sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        self.db[3] = ("e", "f")
        self.assertEqual({1: ("a", "b"), 2: ("c", "d"), 3: ("e", "f")},
                         dict(self.db.iteritems()))


class JDBTestMisc(JDBTest):

    def test_path(self):
        self.assertEqual(self.path, self.db.path)

    def test_size(self):
        self.assertEqual(os.stat(os.path.join(self.path, "laputa.tch")).st_size,
                         self.db.size)

    def test_copy(self):
        self.db[1] = ("a", "b")
        self.db[2] = ("c", "d")
        path = os.path.join(tempfile.gettempdir(), "tmp_td_test2.tdj")
        self.db.copy(path)
        db = JDB()
        db.open(path, JDBOREADER)
        self.assertEqual(len(self.db), len(db))
        self.assertEqual(dict(self.db.iteritems()), dict(db.iteritems()))
        db.close()
        shutil.rmtree(path)


class JDBTestSearch(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(sys.path[0], "presidents.tdj")
        self.db = JDB()
        self.db.open(self.path, JDBOREADER)

    def tearDown(self):
        self.db.close()
        self.db = None

    def test_search1(self):
        self.assertEqual(self.db.search("geo", JDBSSUBSTR),
                         frozenset((1, 41, 43)))
        self.assertEqual(self.db.search("bus", JDBSPREFIX), frozenset((41, 43)))
        self.assertEqual(self.db.search("ton", JDBSSUFFIX), frozenset((1, 42)))
        self.assertEqual(self.db.search("washington", JDBSFULL),
                         frozenset((1,)))

    def test_search2(self):
        self.assertEqual(self.db.search("bush"), frozenset((41, 43)))
        self.assertEqual(self.db.search("johnson"), frozenset((17, 36)))
        self.assertEqual(self.db.search("johnson andrew"), frozenset((17,)))
        self.assertEqual(self.db.search("johnson && andrew"), frozenset((17,)))
        self.assertEqual(self.db.search("johnson || andrew"),
                         frozenset((7, 17, 36)))

    def test_search3(self):
        self.assertEqual(self.db.search('james earl jr.'), frozenset(()))
        self.assertEqual(self.db.search('"james earl jr."'), frozenset((39,)))

    def test_search4(self):
        self.assertEqual(self.db.search("[[john*]]"),
                         frozenset((2, 6, 10, 17, 35, 36)))
        self.assertEqual(self.db.search("[[*am]]"), frozenset((16, 20, 25)))
        self.assertEqual(self.db.search("[[*wilson*]]"), frozenset((28, 40)))

    def test_search5(self):
        res = frozenset((28, 40))
        self.assertEqual(self.db.search("[[*wilson*]]"), res)
        self.assertEqual(self.db.search("wilson || reagan"), res)
        self.assertEqual(self.db.search("wilson", JDBSSUBSTR), res)


all_tests = (
             "JDBTestDict",
             "JDBTestIter",
             "JDBTestMisc",
             "JDBTestSearch",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
