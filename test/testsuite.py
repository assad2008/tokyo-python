import unittest
import sys


class TestSuite(unittest.TestSuite):

    failureException = Exception
    shortDescription = lambda self: None

    def setUp(self):
        "Hook method for setting up the test suite before running it."
        pass

    def tearDown(self):
        "Hook method for deconstructing the test suite after running it."
        pass

    def _exc_info(self):
        """Return a version of sys.exc_info() with the traceback frame
           minimised; usually the top level of the traceback frame is not
           needed.
        """
        return sys.exc_info()

    def run(self, result):
        try:
            self.setUp()
        except KeyboardInterrupt:
            raise
        except:
            result.addError(self, self._exc_info())
            return
        try:
            unittest.TestSuite.run(self, result)
        finally:
            try:
                self.tearDown()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, self._exc_info())
