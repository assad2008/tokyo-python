import unittest
import sys
import os
import tempfile
import collections

import tokyo.dbm as tdbm
from tokyo.cabinet import HDB, BDB, FDB, TDB, Error


class DBMTest(unittest.TestCase):

    def test_error(self):
        self.assertTrue(issubclass(tdbm.error, Error))

    def test_open_default(self):
        path = os.path.join(tempfile.gettempdir(), "tmp_tdbm_test.db")
        db = tdbm.open(path, "c")
        self.assertTrue(isinstance(db, tdbm.TCHDB))
        db.close()
        os.remove(path)

    def test_open_unknown(self):
        path = os.path.join(tempfile.gettempdir(), "tmp_tdbm_test.db")
        f = open(path, "w")
        f.close()
        self.assertRaises(tdbm.error, tdbm.open, path, "c")
        os.remove(path)

    def test_open(self):
        for e, t in tdbm._db_exts.items():
            path = os.path.join(tempfile.gettempdir(),
                                "tmp_tdbm_test{0}".format(e))
            db = tdbm.open(path, "c")
            self.assertTrue(isinstance(db, t))
            db.close()
            os.remove(path)

    def test_whichdb(self):
        for e, t in tdbm._db_exts.items():
            path = os.path.join(tempfile.gettempdir(),
                                "tmp_tdbm_test{0}".format(e))
            db = tdbm.open(path, "c")
            db.close()
            self.assertEqual(tdbm.whichdb(path), t)
            os.remove(path)


class TCDBMTest(unittest.TestCase):

    def setUp(self):
        self.path = os.path.join(tempfile.gettempdir(),
                                 "tmp_tdbm_test.{0}".format(self._ext))

    def tearDown(self):
        self.db.close()
        os.remove(self.path)
        self.db = None

    def _open(self, flag):
        self.db = tdbm.open(self.path, flag)

    def test_type(self):
        self._open("c")
        self.assertTrue(isinstance(self.db, collections.MutableMapping))
        self.assertTrue(isinstance(self.db, self._tc_type))

    def test_open_r(self):
        self.assertRaises(tdbm.error, self._open, "r")
        self._open("c")
        self._add_records()
        self.assertEqual(len(self.db), 2)
        self.db.close()
        self._open("r")
        self.assertEqual(len(self.db), 2)
        self.assertRaises(Error, self._add_record)

    def test_open_w(self):
        self.assertRaises(tdbm.error, self._open, "w")
        self._open("c")
        self._add_records()
        self.assertEqual(len(self.db), 2)
        self.db.close()
        self._open("w")
        self.assertEqual(len(self.db), 2)
        self._add_record()
        self.assertEqual(len(self.db), 3)

    def test_open_c(self):
        self._open("c")
        self._add_records()
        self.assertEqual(len(self.db), 2)
        self.db.close()
        self._open("c")
        self.assertEqual(len(self.db), 2)
        self._add_record()
        self.assertEqual(len(self.db), 3)

    def test_open_n(self):
        self._open("c")
        self._add_records()
        self.assertEqual(len(self.db), 2)
        self.db.close()
        self._open("n")
        self.assertEqual(len(self.db), 0)
        self._add_record()
        self.assertEqual(len(self.db), 1)


class TCHDBTest(TCDBMTest):

    _ext = "tch"
    _tc_type = HDB

    def _add_record(self):
        self.db[b"c"] = b"3"

    def _add_records(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"

    def test_get(self):
        self._open("c")
        self.assertEqual(self.db.get(b"d"), None)


class TCBDBTest(TCDBMTest):

    _ext = "tcb"
    _tc_type = BDB

    def _add_record(self):
        self.db[b"c"] = b"3"

    def _add_records(self):
        self.db[b"a"] = b"1"
        self.db[b"b"] = b"2"

    def test_get(self):
        self._open("c")
        self.assertEqual(self.db.get(b"d"), None)
        self.assertEqual(self.db.get(b"d", duplicate=True), None)


class TCFDBTest(TCDBMTest):

    _ext = "tcf"
    _tc_type = FDB

    def _add_record(self):
        self.db[3] = b"3"

    def _add_records(self):
        self.db[1] = b"1"
        self.db[2] = b"2"

    def test_get(self):
        self._open("c")
        self.assertEqual(self.db.get(4), None)


class TCTDBTest(TCDBMTest):

    _ext = "tct"
    _tc_type = TDB

    def _add_record(self):
        self.db[b"c"] = {b"test": b"3"}

    def _add_records(self):
        self.db[b"a"] = {b"test": b"1"}
        self.db[b"b"] = {b"test": b"2"}

    def test_get(self):
        self._open("c")
        self.assertEqual(self.db.get(b"d"), None)


all_tests = (
             "DBMTest",
             "TCHDBTest",
             "TCBDBTest",
             "TCFDBTest",
             "TCTDBTest",
            )

suite = unittest.TestLoader().loadTestsFromNames(all_tests,
                                                 sys.modules[__name__])

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
