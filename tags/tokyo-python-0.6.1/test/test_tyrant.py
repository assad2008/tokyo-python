import unittest

import test_RDB
import test_RTDB


all_tests = (
             test_RDB,
             test_RTDB,
            )

suite = unittest.TestSuite((test.suite for test in all_tests))

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
