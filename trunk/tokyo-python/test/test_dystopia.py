import unittest

import test_IDB


all_tests = (
             test_IDB,
            )

suite = unittest.TestSuite((test.suite for test in all_tests))

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
