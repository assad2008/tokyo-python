import unittest
import sys
import os
import tempfile

from tokyo.cabinet import (BDBOREADER, BDBOWRITER, BDBOCREAT, BDB as _BDB,
                           BDBCPBEFORE, BDBCPAFTER, Error, INT_MAX, INT_MIN)


class BDB(_BDB):

    def keys(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.key()
            c.next()

    def values(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.value()
            c.next()

    def items(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.item()
            c.next()



class BDBTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(), "tmp_tc_test.tcb")
        self.db = BDB()
        self.db.open(self.path, BDBOWRITER | BDBOCREAT)

    def tearDown(self):
        self.db.close()
        os.remove(self.path)
        self.db = None


class BDBTestDict(BDBTest):

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
        d = dict(self.db.items())
        self.assertEqual(d, {b"a": b"1", b"b": b"2"})
        self.db[b"c"] = b"3"
        self.db[b"a"] = b"4"
        d = dict(self.db.items())
        self.assertEqual(d, {b"a": b"4", b"b": b"2", b"c": b"3"})
        del self.db[b"b"]
        self.assertEqual(len(self.db), 2)
        d = dict(self.db.items())
        self.assertEqual(d, {b"a": b"4", b"c": b"3"})

    def test_clear(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.assertEqual(len(self.db), 2)
        self.db.clear()
        self.assertEqual(len(self.db), 0)
        d = dict(self.db.items())
        self.assertEqual(d, {})


class BDBTestIter(BDBTest):

    def test_iter(self):
        i = iter(self.db)
        self.assertEqual(len(i), 0)
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        self.db[b"c"] = b"3"
        i = iter(self.db)
        self.assertEqual(len(i), len(self.db))
        self.assertTrue(b"a" in i)
        self.assertEqual([b"a", b"b", b"c"], sorted(i))
        i = iter(self.db)
        a = next(i)
        b = next(i)
        c = next(i)
        self.assertRaises(StopIteration, next, i)
        self.assertEqual([b"a", b"b", b"c"], sorted((a, b, c)))
        i = iter(self.db)
        self.assertEqual(len(i), 3)
        a = next(i)
        del self.db[b"b"]
        self.db[b"d"] = b"4"
        self.assertEqual(len(i), 3)
        self.assertRaises(Error, next, i)


class BDBTestPut(BDBTest):

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

    def test_putdup(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.assertEqual(len(self.db), 2)
        self.assertEqual(self.db[b"a"], b"1")
        del self.db[b"a"]
        self.assertEqual(self.db[b"a"], b"2")


class BDBTestTransaction(BDBTest):

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


class BDBTestMisc(BDBTest):

    def test_path(self):
        self.assertEqual(self.path, self.db.path)

    def test_size(self):
        self.assertEqual(os.stat(self.path).st_size, self.db.size)

    def test_copy(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"
        path = os.path.join(tempfile.gettempdir(), "tmp_tc_test2.tcb")
        self.db.copy(path)
        db = BDB()
        db.open(path, BDBOREADER)
        self.assertEqual(len(self.db), len(db))
        self.assertEqual(self.db.size, db.size)
        db.close()
        os.remove(path)

    def test_searchkeys(self):
        self.db[b"key1"] = b"1"
        self.db[b"key2"] = b"2"
        self.db[b"key3"] = b"3"
        self.db[b"akey"] = b"4"
        self.assertEqual(self.db.searchkeys(b"k"), frozenset((b"key1", b"key2", b"key3")))
        self.assertEqual(self.db.searchkeys(b"k", 2), frozenset((b"key1", b"key2")))
        self.assertEqual(self.db.searchkeys(b"z"), frozenset())
        self.assertEqual(self.db.searchkeys(b"a"), frozenset((b"akey",)))

    def test_range(self):
        for key, value in [(b"a", b"a"), (b"b", b"b"), (b"c", b"c"), (b"d", b"d")]:
            self.db[key] = value
        self.assertEqual(self.db.range(), frozenset((b"a", b"b", b"c", b"d")))
        self.assertEqual(self.db.range(b"b", b"d"), frozenset((b"b", b"c", b"d")))
        self.assertEqual(self.db.range(b"b", b"z"), frozenset((b"b", b"c", b"d")))
        self.assertEqual(self.db.range(b"z", b"b"), frozenset())
        self.assertEqual(self.db.range(b"a", b"a"), frozenset((b"a",)))
        self.assertEqual(self.db.range(b"A", b"Z"), frozenset())
        self.assertEqual(self.db.range(b"A", b"z"), frozenset((b"a", b"b", b"c", b"d")))
        self.assertEqual(self.db.range(b"z", b"A"), frozenset())
        self.assertEqual(self.db.range(max=2), frozenset((b"a", b"b")))

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


class BDBTestDuplicate(BDBTest):

    def test_get(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.assertEqual(self.db.get(b"a"), b"1")
        self.assertEqual(self.db.get(b"a", True), (b"1", b"2"))

    def test_remove(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.remove(b"a")
        self.assertEqual(len(self.db), 1)
        self.assertEqual(self.db[b"a"], b"2")
        self.db.putdup(b"a", [b"1", b"2"])
        self.assertEqual(len(self.db), 3)
        self.db.remove(b"a", True)
        self.assertEqual(len(self.db), 0)

    def test_put(self):
        self.db.put(b"a", b"1")
        self.assertEqual(len(self.db), 1)
        self.db.put(b"a", b"1")
        self.assertEqual(len(self.db), 1)
        self.db.put(b"a", b"2", True)
        self.assertEqual(len(self.db), 2)
        self.assertEqual(self.db.get(b"a", True), (b"1", b"2"))


class BDBTestCursor(BDBTest):

    def test_basic(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.assertEqual(tuple(self.db.keys()), (b"a", b"a"))
        self.assertEqual(tuple(self.db.values()), (b"1", b"2"))
        self.assertEqual(tuple(self.db.items()), ((b"a", b"1"), (b"a", b"2")))

    def test_first_next(self):
        c = self.db.cursor()
        self.assertRaises(StopIteration, c.next)
        self.db.putdup(b"a", [b"1", b"2"])
        c.first()
        self.assertEqual(c.key(), b"a")
        self.assertEqual(c.value(), b"1")
        self.assertEqual(c.item(), (b"a", b"1"))
        c.next()
        self.assertEqual(c.key(), b"a")
        self.assertEqual(c.value(), b"2")
        self.assertEqual(c.item(), (b"a", b"2"))
        self.assertRaises(StopIteration, c.next)

    def test_last_prev(self):
        c = self.db.cursor()
        self.assertRaises(StopIteration, c.prev)
        self.db.putdup(b"a", [b"1", b"2"])
        c.last()
        self.assertEqual(c.key(), b"a")
        self.assertEqual(c.value(), b"2")
        self.assertEqual(c.item(), (b"a", b"2"))
        c.prev()
        self.assertEqual(c.key(), b"a")
        self.assertEqual(c.value(), b"1")
        self.assertEqual(c.item(), (b"a", b"1"))
        self.assertRaises(StopIteration, c.prev)

    def test_jump(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.putdup(b"c", [b"4", b"3"])
        c = self.db.cursor()
        c.jump(b"b")
        self.assertEqual(c.item(), (b"c", b"4"))
        self.assertRaises(StopIteration, c.jump, b"d")
        c.jump(b"a")
        self.assertEqual(c.item(), (b"a", b"1"))

    def test_put(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.putdup(b"c", [b"4", b"3"])
        c = self.db.cursor()
        c.jump(b"c")
        self.assertEqual(c.item(), (b"c", b"4"))
        self.assertEqual(len(self.db), 4)
        c.put(b"5")
        self.assertEqual(len(self.db), 4)
        self.assertEqual(c.item(), (b"c", b"5"))
        c.next()
        self.assertEqual(c.item(), (b"c", b"3"))

    def test_put_before(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.putdup(b"c", [b"4", b"3"])
        c = self.db.cursor()
        c.jump(b"c")
        self.assertEqual(c.item(), (b"c", b"4"))
        self.assertEqual(len(self.db), 4)
        c.put(b"5", BDBCPBEFORE)
        self.assertEqual(len(self.db), 5)
        self.assertEqual(c.item(), (b"c", b"5"))
        c.next()
        self.assertEqual(c.item(), (b"c", b"4"))

    def test_put_after(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.putdup(b"c", [b"4", b"3"])
        c = self.db.cursor()
        c.jump(b"c")
        self.assertEqual(c.item(), (b"c", b"4"))
        self.assertEqual(len(self.db), 4)
        c.put(b"5", BDBCPAFTER)
        self.assertEqual(len(self.db), 5)
        self.assertEqual(c.item(), (b"c", b"5"))
        c.next()
        self.assertEqual(c.item(), (b"c", b"3"))

    def test_remove(self):
        self.db.putdup(b"a", [b"1", b"2"])
        self.db.putdup(b"c", [b"4", b"3"])
        c = self.db.cursor()
        c.jump(b"c")
        self.assertEqual(c.item(), (b"c", b"4"))
        self.assertEqual(len(self.db), 4)
        c.remove()
        self.assertEqual(c.item(), (b"c", b"3"))
        self.assertEqual(len(self.db), 3)
        c.remove()
        self.assertEqual(len(self.db), 2)
        self.assertRaises(Error, c.item)


all_tests = (
             "BDBTestDict",
             "BDBTestIter",
             "BDBTestPut",
             "BDBTestTransaction",
             "BDBTestMisc",
             "BDBTestDuplicate",
             "BDBTestCursor",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
