import unittest

import test_HDB
import test_MDB
import test_BDB
import test_NDB
import test_FDB
import test_TDB


all_tests = (
             test_HDB,
             test_MDB,
             test_BDB,
             test_NDB,
             test_FDB,
             test_TDB,
            )

suite = unittest.TestSuite((test.suite for test in all_tests))

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
