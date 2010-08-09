import unittest
import sys
import os
import tempfile
import shutil

from tokyo.dystopia import (IDBOREADER, IDBOWRITER, IDBOCREAT, IDB, Error,
                            IDBSSUBSTR, IDBSPREFIX, IDBSSUFFIX, IDBSFULL,
                            IDBSTOKEN, IDBSTOKPRE, IDBSTOKSUF)


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
        if sys.version_info[0] >= 3:
            self.assertTrue(type(self.db.__getitem__(1)) is str)
        else:
            self.assertTrue(type(self.db.__getitem__(1)) is unicode)

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__)
        self.assertRaises(TypeError, self.db.__setitem__, 1)
        self.assertRaises(TypeError, self.db.__setitem__, "a", 1)
        self.assertRaises(TypeError, self.db.__setitem__, 1, 1)
        self.assertRaises(OverflowError, self.db.__setitem__, 0, "a")
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

    def test_clear(self):
        self.db[1] = "a"
        self.db[2] = "b"
        self.assertEqual(len(self.db), 2)
        self.db.clear()
        self.assertEqual(len(self.db), 0)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {})


class IDBTestIter(IDBTest):

    def test_iter(self):
        self.db[1] = "a"
        self.db[2] = "b"
        self.db[3] = "c"
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
        self.db[4] = "d"
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        del self.db[4]
        self.db[4] = "e"
        self.assertRaises(Error, next, i)

    def test_iterkeys(self):
        self.db[1] = "a"
        self.db[2] = "b"
        self.db[3] = "c"
        self.assertEqual([1, 2, 3], sorted(list(self.db.iterkeys())))

    def test_itervalues(self):
        self.db[1] = "a"
        self.db[2] = "b"
        self.db[3] = "c"
        self.assertEqual(["a", "b", "c"], sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[1] = "a"
        self.db[2] = "b"
        self.db[3] = "c"
        self.assertEqual({1: "a", 2: "b", 3: "c"}, dict(self.db.iteritems()))


class IDBTestMisc(IDBTest):

    def test_path(self):
        self.assertEqual(self.path, self.db.path)

    def test_size(self):
        self.assertEqual(os.stat(os.path.join(self.path, "dystopia.tch")).st_size,
                         self.db.size)

    def test_copy(self):
        self.db[1] = "a"
        self.db[2] = "b"
        path = os.path.join(tempfile.gettempdir(), "tmp_td_test2.tdi")
        self.db.copy(path)
        db = IDB()
        db.open(path, IDBOREADER)
        self.assertEqual(len(self.db), len(db))
        self.assertEqual(dict(self.db.iteritems()), dict(db.iteritems()))
        db.close()
        shutil.rmtree(path)


class IDBTestSearch(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(sys.path[0], "presidents.tdi")
        self.db = IDB()
        self.db.open(self.path, IDBOREADER)

    def tearDown(self):
        self.db.close()
        self.db = None

    def test_search1(self):
        self.assertEqual(self.db.search("geo", IDBSSUBSTR),
                         frozenset((1, 41, 43)))
        self.assertEqual(self.db.search("bus", IDBSPREFIX),frozenset((41, 43)))
        self.assertEqual(self.db.search("son", IDBSSUFFIX), frozenset((40, 42)))
        self.assertEqual(self.db.search("washington, george", IDBSFULL),
                         frozenset((1,)))
        self.assertEqual(self.db.search("washington george", IDBSFULL),
                         frozenset())
        self.assertEqual(self.db.search("george", IDBSTOKEN),
                         frozenset((1, 41, 43)))
        self.assertEqual(self.db.search("wil", IDBSTOKPRE),
                         frozenset((9, 25, 27, 28, 40, 42)))
        self.assertEqual(self.db.search("ton,", IDBSTOKSUF), frozenset((1, 42)))
        self.assertEqual(self.db.search("ton", IDBSTOKSUF), frozenset())

    def test_search2(self):
        self.assertEqual(self.db.search("geor"), frozenset((1, 41, 43)))
        self.assertEqual(self.db.search("geor walk"), frozenset((41, 43)))
        self.assertEqual(self.db.search("geor && walk"), frozenset((41, 43)))
        self.assertEqual(self.db.search("geor || walk"), frozenset((1, 41, 43)))

    def test_search3(self):
        self.assertEqual(self.db.search('earl james'), frozenset((39,)))
        self.assertEqual(self.db.search('"earl james"'), frozenset())
        self.assertEqual(self.db.search('"james earl"'), frozenset((39,)))
        self.assertEqual(self.db.search('"ames ear"'), frozenset((39,)))

    def test_search4(self):
        self.assertEqual(self.db.search("in]]]]"),
                         frozenset((8, 14, 23, 30, 44)))
        self.assertEqual(self.db.search("[[[[roo"), frozenset((26, 32)))
        self.assertEqual(self.db.search("[[john*]]"),
                         frozenset((2, 6, 10, 17, 35, 36)))
        self.assertEqual(self.db.search("[[*am]]"),
                         frozenset((9, 16, 20, 25, 27, 42)))
        self.assertEqual(self.db.search("[[wilson]]"), frozenset((40,)))
        self.assertEqual(self.db.search("[[wilson,]]"), frozenset((28,)))
        self.assertEqual(self.db.search("[[wilson*]]"), frozenset((28, 40)))

    def test_search5(self):
        res = frozenset((28, 40))
        self.assertEqual(self.db.search("[[*wilson*]]"), res)
        self.assertEqual(self.db.search("wilson"), res)
        self.assertEqual(self.db.search("wilson", IDBSSUBSTR), res)


all_tests = (
             "IDBTestDict",
             "IDBTestIter",
             "IDBTestMisc",
             "IDBTestSearch",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
