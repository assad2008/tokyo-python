import unittest

import test_IDB
import test_JDB


all_tests = (
             test_IDB,
             test_JDB,
            )

suite = unittest.TestSuite((test.suite for test in all_tests))

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
