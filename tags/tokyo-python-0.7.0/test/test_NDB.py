import unittest
import sys

from tokyo.cabinet import NDB, Error


class NDBTest(unittest.TestCase):

    def setUp(self):
        self.db = NDB()

    def tearDown(self):
        self.db = None


class NDBTestDict(NDBTest):

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


class NDBTestIter(NDBTest):

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


class NDBTestPut(NDBTest):

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


class NDBTestMisc(NDBTest):

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


class NDBTestNullBytes(NDBTest):

    def test_iterkeys(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual([b"a\0b", b"cd"], sorted(list(self.db.iterkeys())))

    def test_itervalues(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual([b"ab", b"c\0d"], sorted(list(self.db.itervalues())))

    def test_iteritems(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual({b"a\0b": b"ab", b"cd": b"c\0d"},
                         dict(self.db.iteritems()))

    def test_contains(self):
        self.db[b"a\0b"] = b"ab"
        self.assertTrue(self.db.__contains__(b"a\0b"))
        self.assertTrue(b"a\0b" in self.db)

    def test_getitem(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual(b"ab", self.db.__getitem__(b"a\0b"))
        self.assertEqual(b"ab", self.db[b"a\0b"])
        self.assertEqual(b"c\0d", self.db.__getitem__(b"cd"))
        self.assertEqual(b"c\0d", self.db[b"cd"])

    def test_setitem(self):
        self.db.__setitem__(b"a\0b", b"ab")
        self.db.__setitem__(b"cd", b"c\0d")
        self.assertEqual(b"ab", self.db[b"a\0b"])
        self.assertEqual(b"c\0d", self.db[b"cd"])

    def test_get(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual(b"ab", self.db.get(b"a\0b"))
        self.assertEqual(b"c\0d", self.db.get(b"cd"))

    def test_remove(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual(len(self.db), 2)
        self.db.remove(b"a\0b")
        self.assertEqual(len(self.db), 1)

    def test_put(self):
        self.db.put(b"a\0b", b"ab")
        self.db.put(b"cd", b"c\0d")
        self.assertEqual(b"ab", self.db[b"a\0b"])
        self.assertEqual(b"c\0d", self.db[b"cd"])

    def test_putkeep(self):
        self.db.putkeep(b"a\0b", b"ab")
        self.db.putkeep(b"cd", b"c\0d")
        self.assertRaises(KeyError, self.db.putkeep, b"a\0b", b"g\0h")

    def test_putcat(self):
        self.db.putcat(b"a\0b", b"ab")
        self.db.putcat(b"a\0b", b"c\0d")
        self.assertEqual(self.db[b"a\0b"], b"abc\0d")

    def test_searchkeys(self):
        self.db[b"a\0b"] = b"ab"
        self.db[b"cd"] = b"c\0d"
        self.assertEqual(self.db.searchkeys(b"a"), frozenset((b"a\0b",)))
        self.assertEqual(self.db.searchkeys(b"a\0"), frozenset((b"a\0b",)))


all_tests = (
             "NDBTestDict",
             "NDBTestIter",
             "NDBTestPut",
             "NDBTestMisc",
             "NDBTestNullBytes",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
