import unittest

import test_cabinet


all_tests = [test_cabinet]
opts_tests = ("test_tyrant",)

for name in opts_tests:
    try:
        test = __import__(name)
    except ImportError:
        pass
    else:
        all_tests.append(test)


suite = unittest.TestSuite((test.suite for test in all_tests))

if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite)
