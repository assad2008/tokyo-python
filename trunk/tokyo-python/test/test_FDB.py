import unittest
import sys
import os
import tempfile

from tokyo.cabinet import (FDBOREADER, FDBOWRITER, FDBOCREAT, FDB, Error,
                           INT_MAX, INT_MIN)


class FDBTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(), "tmp_tc_test.tcf")
        self.db = FDB()
        self.db.open(self.path, FDBOWRITER | FDBOCREAT)

    def tearDown(self):
        self.db.close()
        os.remove(self.path)
        self.db = None


class FDBTestDict(FDBTest):

    def test_contains(self):
        self.assertRaises(TypeError, self.db.__contains__)
        self.assertRaises(TypeError, self.db.__contains__, b"a")
        self.assertTrue(not (1 in self.db))
        self.assertTrue(1 not in self.db)
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.assertTrue(1 in self.db)
        self.assertTrue(2 in self.db)
        self.assertTrue(3 not in self.db)

    def test_len(self):
        self.assertEqual(len(self.db), 0)
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.assertEqual(len(self.db), 2)

    def test_getitem(self):
        self.assertRaises(TypeError, self.db.__getitem__)
        self.assertRaises(TypeError, self.db.__getitem__, b"a")
        self.assertRaises(KeyError, self.db.__getitem__, 1)
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.assertEqual(self.db[1], b"a")
        self.assertEqual(self.db[2], b"b")
        self.db[3] = b"c"
        self.db[4] = b"d"
        self.assertEqual(self.db[3], b"c")
        self.assertEqual(self.db[4], b"d")

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__)
        self.assertRaises(TypeError, self.db.__setitem__, 1)
        self.assertRaises(TypeError, self.db.__setitem__, b"a", 1)
        self.assertRaises(TypeError, self.db.__setitem__, 1, 1)
        self.assertRaises(Error, self.db.__setitem__, 0, b"a")
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: b"a", 2: b"b"})
        self.db[3] = b"c"
        self.db[1] = b"d"
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: b"d", 2: b"b", 3: b"c"})
        del self.db[2]
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {1: b"d", 3: b"c"})

    def test_clear(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.assertEqual(len(self.db), 2)
        self.db.clear()
        self.assertEqual(len(self.db), 0)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {})


class FDBTestIter(FDBTest):

    def test_iter(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.db[3] = b"c"
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
        self.db[4] = b"d"
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        del self.db[4]
        self.db[4] = b"e"
        self.assertRaises(Error, next, i)

    def test_iterkeys(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.db[3] = b"c"
        self.assertEqual([1, 2, 3], sorted(list(self.db.iterkeys())))

    def test_itervalues(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.db[3] = b"c"
        self.assertEqual([b"a", b"b", b"c"], sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        self.db[3] = b"c"
        self.assertEqual({1: b"a", 2: b"b", 3: b"c"}, dict(self.db.iteritems()))


class FDBTestPut(FDBTest):

    def test_put(self):
        self.db.put(1, b"a")
        self.db.put(2, b"b")
        self.assertEqual(self.db[1], b"a")
        self.assertEqual(self.db[2], b"b")
        self.db.put(3, b"c")
        self.db.put(1, b"d")
        self.assertEqual(self.db[3], b"c")
        self.assertEqual(self.db[1], b"d")

    def test_putkeep(self):
        self.db.putkeep(1, b"a")
        self.assertRaises(KeyError, self.db.putkeep, 1, b"a")

    def test_putcat(self):
        self.db.putcat(1, b"a")
        self.db.putcat(1, b"b")
        self.assertEqual(self.db[1], b"ab")


class FDBTestTransaction(FDBTest):

    def test_commit(self):
        self.db.begin()
        for x in range(1, 3):
            self.db[x] = str(x).encode()
        self.db.commit()
        self.assertEqual(len(self.db), 2)

    def test_abort(self):
        self.db.begin()
        for x in range(1, 11):
            self.db[x] = str(x).encode()
        self.db.abort()
        self.assertEqual(len(self.db), 0)

    def test_tx_commit(self):
        self.db.begin()
        try:
            for x in range(2):
                self.db[1] = str(x).encode()
        except KeyError:
            self.db.abort()
        else:
            self.db.commit()
        self.assertEqual(len(self.db), 1)

    def test_tx_abort(self):
        self.db.begin()
        try:
            for x in range(2):
                self.db.putkeep(1, str(x).encode())
        except KeyError:
            self.db.abort()
        else:
            self.db.commit()
        self.assertEqual(len(self.db), 0)

    def test_tx_abort2(self):
        for x in range(2):
            try:
                self.db.begin()
                self.db.putkeep(1, str(x).encode())
            except KeyError:
                self.db.abort()
            else:
                self.db.commit()
        self.assertEqual(len(self.db), 1)


class FDBTestMisc(FDBTest):

    def test_path(self):
        self.assertEqual(self.path, self.db.path)

    def test_size(self):
        self.assertEqual(os.stat(self.path).st_size, self.db.size)

    def test_copy(self):
        self.db[1] = b"a"
        self.db[2] = b"b"
        path = os.path.join(tempfile.gettempdir(), "tmp_tc_test2.tcf")
        self.db.copy(path)
        db = FDB()
        db.open(path, FDBOREADER)
        self.assertEqual(len(self.db), len(db))
        self.assertEqual(self.db.size, db.size)
        db.close()
        os.remove(path)

    def test_range(self):
        for x in range(1, 5):
            self.db[x] = str(x).encode()
        self.assertEqual(self.db.range(), frozenset((1, 2, 3, 4)))
        self.assertEqual(self.db.range(2, 10), frozenset((2, 3, 4)))
        self.assertEqual(self.db.range(10, 2), frozenset())
        self.assertEqual(self.db.range(1, 1), frozenset((1,)))
        self.assertEqual(self.db.range(max=2), frozenset((1, 2)))

    def test_addint(self):
        self.assertEqual(self.db.addint(100, 0), 0)
        self.assertEqual(self.db.addint(100, 1), 1)
        self.db[100] = b"aa"
        self.assertRaises(KeyError, self.db.addint, 100, 1)

    def test_addint_max(self):
        self.assertEqual(self.db.addint(100, INT_MAX), INT_MAX)
        self.assertEqual(self.db.addint(100, 1), INT_MIN)
        self.assertEqual(self.db.addint(100, 1), INT_MIN + 1)

    def test_adddouble(self):
        self.assertEqual(self.db.adddouble(200, 1.0), 1.0)
        self.assertEqual(self.db.adddouble(200, -1.5), -0.5)
        self.db[200] = b"aa"
        self.assertRaises(KeyError, self.db.adddouble, 200, 1.0)


class FDBTestNullBytes(FDBTest):

    def test_itervalues(self):
        self.db[1] = b"ab"
        self.db[2] = b"c\0d"
        self.assertEqual([b"ab", b"c\0d"], sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[1] = b"ab"
        self.db[2] = b"c\0d"
        self.assertEqual({1: b"ab", 2: b"c\0d"}, dict(self.db.iteritems()))

    def test_getitem(self):
        self.assertRaises(TypeError, self.db.__getitem__, b"b\0c")
        self.db[1] = b"ab"
        self.db[2] = b"c\0d"
        self.assertEqual(b"c\0d", self.db.__getitem__(2))
        self.assertEqual(b"c\0d", self.db[2])

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__, b"b\0c", b"bc")
        self.db.__setitem__(1, b"ab")
        self.db.__setitem__(2, b"c\0d")
        self.assertEqual(b"c\0d", self.db[2])

    def test_get(self):
        self.assertRaises(TypeError, self.db.get, b"b\0c")
        self.db[1] = b"ab"
        self.db[2] = b"c\0d"
        self.assertEqual(b"c\0d", self.db.get(2))

    def test_remove(self):
        self.assertRaises(TypeError, self.db.remove, b"b\0c")

    def test_put(self):
        self.assertRaises(TypeError, self.db.put, b"b\0c", b"bc")
        self.db.put(1, b"ab")
        self.db.put(2, b"c\0d")
        self.assertEqual(b"c\0d", self.db[2])

    def test_putkeep(self):
        self.assertRaises(TypeError, self.db.putkeep, b"b\0c", b"bc")
        self.db.putkeep(1, b"ab")
        self.db.putkeep(2, b"c\0d")
        self.assertRaises(KeyError, self.db.putkeep, 2, b"g\0h")

    def test_putcat(self):
        self.assertRaises(TypeError, self.db.putcat, b"b\0c", b"bc")
        self.db.putcat(1, b"ab")
        self.db.putcat(1, b"c\0d")
        self.assertEqual(self.db[1], b"abc\0d")

    def test_range(self):
        self.assertRaises(TypeError, self.db.range, b"b\0c")
        self.assertRaises(TypeError, self.db.range, upper=b"c\0d")
        self.assertRaises(TypeError, self.db.range, b"b\0c", b"c\0d")

    def test_addint(self):
        self.assertRaises(TypeError, self.db.addint, b"b\0c", 1)

    def test_adddouble(self):
        self.assertRaises(TypeError, self.db.adddouble, b"b\0c", 1.0)


all_tests = (
             "FDBTestDict",
             "FDBTestIter",
             "FDBTestPut",
             "FDBTestTransaction",
             "FDBTestMisc",
             "FDBTestNullBytes",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
