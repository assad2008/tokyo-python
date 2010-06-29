import unittest
import sys
import os
import tempfile

from tokyo.cabinet import (HDBOREADER, HDBOWRITER, HDBOCREAT, HDB, Error,
                           INT_MAX, INT_MIN)


class HDBTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(), "tmp_tc_test.tch")
        self.db = HDB()
        self.db.open(self.path, HDBOWRITER | HDBOCREAT)

    def tearDown(self):
        self.db.close()
        os.remove(self.path)
        self.db = None


class HDBTestDict(HDBTest):

    def test_contains(self):
        self.assertRaises(TypeError, self.db.__contains__)
        self.assertRaises(TypeError, self.db.__contains__, 1)
        self.assertTrue(not (b"a" in self.db))
        self.assertTrue(b"a" not in self.db)
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertTrue(b"a" in self.db)
        self.assertTrue(b"b" in self.db)
        self.assertTrue(b"c" not in self.db)

    def test_len(self):
        self.assertEqual(len(self.db), 0)
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertEqual(len(self.db), 2)

    def test_getitem(self):
        self.assertRaises(TypeError, self.db.__getitem__)
        self.assertRaises(TypeError, self.db.__getitem__, 1)
        self.assertRaises(KeyError, self.db.__getitem__, b"a")
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertEqual(self.db[b"a"], b"1")
        self.assertEqual(self.db[b"b"], b"2")
        self.db[b"c"] = b"3"
        self.db[b"a"] = b"4"
        self.assertEqual(self.db[b"c"], b"3")
        self.assertEqual(self.db[b"a"], b"4")

    def test_setitem(self):
        self.assertRaises(TypeError, self.db.__setitem__)
        self.assertRaises(TypeError, self.db.__setitem__, b"a")
        self.assertRaises(TypeError, self.db.__setitem__, b"a", 1)
        self.assertRaises(TypeError, self.db.__setitem__, 1, b"a")
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {b"a": b"1", b"b": b"2"})
        self.db[b"c"] = b"3"
        self.db[b"a"] = b"4"
        d = dict(self.db.iteritems())
        self.assertEqual(d, {b"a": b"4", b"b": b"2", b"c": b"3"})
        del self.db[b"b"]
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {b"a": b"4", b"c": b"3"})

    def test_clear(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertEqual(len(self.db), 2)
        self.db.clear()
        self.assertEqual(len(self.db), 0)
        d = dict(self.db.iteritems())
        self.assertEqual(d, {})


class HDBTestIter(HDBTest):

    def test_iter(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.db[b"c"] = b"3"
        i = iter(self.db)
        self.assertTrue(b"a" in i)
        i = iter(self.db)
        self.assertEqual([b"a", b"b", b"c"], sorted(i))
        i = iter(self.db)
        a = next(i)
        b = next(i)
        c = next(i)
        self.assertRaises(StopIteration, next, i)
        self.assertEqual([b"a", b"b", b"c"], sorted((a, b, c)))
        i = iter(self.db)
        a = next(i)
        del self.db[b"b"]
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        self.db[b"d"] = b"4"
        self.assertRaises(Error, next, i)
        i = iter(self.db)
        a = next(i)
        del self.db[b"d"]
        self.db[b"d"] = b"5"
        self.assertRaises(Error, next, i)

    def test_iterkeys(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.db[b"c"] = b"3"
        self.assertEqual([b"a", b"b", b"c"],
                         sorted(list(self.db.iterkeys())))

    def test_itervalues(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.db[b"c"] = b"3"
        self.assertEqual([b"1", b"2", b"3"],
                         sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.db[b"c"] = b"3"
        self.assertEqual({b"a": b"1", b"b": b"2", b"c": b"3"},
                         dict(self.db.iteritems()))


class HDBTestPut(HDBTest):

    def test_put(self):
        self.db.put(b"a", b"1")
        self.db.put(b"b", b"2")
        self.assertEqual(self.db[b"a"], b"1")
        self.assertEqual(self.db[b"b"], b"2")
        self.db.put(b"c", b"3")
        self.db.put(b"a", b"4")
        self.assertEqual(self.db[b"c"], b"3")
        self.assertEqual(self.db[b"a"], b"4")

    def test_putkeep(self):
        self.db.putkeep(b"a", b"1")
        self.assertRaises(KeyError, self.db.putkeep, b"a", b"1")

    def test_putcat(self):
        self.db.putcat(b"a", b"1")
        self.db.putcat(b"a", b"2")
        self.assertEqual(self.db[b"a"], b"12")

    def test_putasync(self):
        # TODO: use multiprocessing/threading
        self.db.putasync(b"a", b"1")
        self.db.sync()
        self.assertEqual(self.db[b"a"], b"1")


class HDBTestTransaction(HDBTest):

    def test_commit(self):
        self.db.begin()
        for x in range(2):
            x = str(x).encode()
            self.db[x] = x
        self.db.commit()
        self.assertEqual(len(self.db), 2)

    def test_abort(self):
        self.db.begin()
        for x in range(10):
            x = str(x).encode()
            self.db[x] = x
        self.db.abort()
        self.assertEqual(len(self.db), 0)

    def test_tx_commit(self):
        self.db.begin()
        try:
            for x in range(2):
                x = str(x).encode()
                self.db[b"a"] = x
        except KeyError:
            self.db.abort()
        else:
            self.db.commit()
        self.assertEqual(len(self.db), 1)

    def test_tx_abort(self):
        self.db.begin()
        try:
            for x in range(2):
                x = str(x).encode()
                self.db.putkeep(b"a", x)
        except KeyError:
            self.db.abort()
        else:
            self.db.commit()
        self.assertEqual(len(self.db), 0)

    def test_tx_abort2(self):
        for x in range(2):
            x = str(x).encode()
            try:
                self.db.begin()
                self.db.putkeep(b"a", x)
            except KeyError:
                self.db.abort()
            else:
                self.db.commit()
        self.assertEqual(len(self.db), 1)


class HDBTestMisc(HDBTest):

    def test_path(self):
        self.assertEqual(self.path, self.db.path)

    def test_size(self):
        self.assertEqual(os.stat(self.path).st_size, self.db.size)

    def test_copy(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        path = os.path.join(tempfile.gettempdir(), "tmp_tc_test2.tch")
        self.db.copy(path)
        db = HDB()
        db.open(path, HDBOREADER)
        self.assertEqual(len(self.db), len(db))
        self.assertEqual(self.db.size, db.size)
        db.close()
        os.remove(path)

    def test_searchkeys(self):
        self.db[b"key1"] = b"1"
        self.db[b"key2"] = b"2"
        self.db[b"key3"] = b"3"
        self.db[b"akey"] = b"4"
        self.assertEqual(self.db.searchkeys(b"k"),
                         frozenset((b"key1", b"key2", b"key3")))
        self.assertEqual(self.db.searchkeys(b"k", 2),
                         frozenset((b"key1", b"key2")))
        self.assertEqual(self.db.searchkeys(b"z"), frozenset())
        self.assertEqual(self.db.searchkeys(b"a"), frozenset((b"akey",)))

    def test_addint(self):
        self.assertEqual(self.db.addint(b"kint", 0), 0)
        self.assertEqual(self.db.addint(b"kint", 1), 1)
        self.db[b"kint"] = b"aa"
        self.assertRaises(KeyError, self.db.addint, b"kint", 1)

    def test_addint_max(self):
        self.assertEqual(self.db.addint(b"kint", INT_MAX), INT_MAX)
        self.assertEqual(self.db.addint(b"kint", 1), INT_MIN)
        self.assertEqual(self.db.addint(b"kint", 1), INT_MIN + 1)

    def test_adddouble(self):
        self.assertEqual(self.db.adddouble(b"kfloat", 1.0), 1.0)
        self.assertEqual(self.db.adddouble(b"kfloat", -1.5), -0.5)
        self.db[b"kfloat"] = b"aa"
        self.assertRaises(KeyError, self.db.adddouble, b"kfloat", 1.0)


all_tests = (
             "HDBTestDict",
             "HDBTestIter",
             "HDBTestPut",
             "HDBTestTransaction",
             "HDBTestMisc",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
