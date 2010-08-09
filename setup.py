################################################################################
#
# Copyright (c) 2010, Malek Hadj-Ali
# All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
################################################################################


from platform import python_version
from os import name as os_name
from ctypes import cdll, c_char_p
from ctypes.util import find_library
from sys import argv
from distutils.command.build_ext import build_ext as _build_ext
from distutils.core import setup, Extension


curr_py_ver = python_version()
min_py_vers = {2: "2.6.4", 3: "3.1.1"}
if curr_py_ver < min_py_vers[int(curr_py_ver[0])]:
    raise SystemExit("Aborted: tokyo-python requires Python2 >= {0[2]} "
                     "OR Python3 >= {0[3]}".format(min_py_vers))

if os_name != "posix":
    raise SystemExit("Aborted: os '{0}' not supported".format(os_name))


class TokyoPythonExt(object):
    def __init__(self, required, libname, name, min_ver, ver_char_p, url,
                 *args, **kwargs):
        self.required = required
        self.libname = libname
        self.name = name
        self.min_ver = min_ver
        self.ver_char_p = ver_char_p
        self.url = url
        self.c_ext = Extension(*args, **kwargs)

err_msg = """Aborted: tokyo-python requires {0} >= {1} to be installed.
See {2} for more information."""

def check_extension(ext):
    lib = find_library(ext.libname)
    if ext.required and not lib:
        raise SystemExit(err_msg.format(ext.name, ext.min_ver, ext.url))
    elif lib:
        curr_ver = c_char_p.in_dll(cdll.LoadLibrary(lib), ext.ver_char_p).value
        if curr_ver.decode() < ext.min_ver:
            raise SystemExit(err_msg.format(ext.name, ext.min_ver, ext.url))
    return lib

cabinet_ext = TokyoPythonExt(True, "tokyocabinet", "Tokyo Cabinet", "1.4.45",
                             "tcversion", "http://fallabs.com/tokyocabinet/",
                             "tokyo.cabinet", ["src/cabinet.c"],
                             libraries=["tokyocabinet", "z", "bz2", "rt",
                                        "pthread", "m", "c"])

tyrant_ext = TokyoPythonExt(False, "tokyotyrant", "Tokyo Tyrant", "1.1.40",
                            "ttversion", "http://fallabs.com/tokyotyrant/",
                            "tokyo.tyrant", ["src/tyrant.c"],
                            libraries=["tokyotyrant", "tokyocabinet", "z", "bz2",
                                       "resolv", "nsl", "dl", "rt", "pthread",
                                       "m", "c"])

dystopia_ext = TokyoPythonExt(False, "tokyodystopia", "Tokyo Dystopia", "0.9.14",
                              "tdversion", "http://fallabs.com/tokyodystopia/",
                              "tokyo.dystopia", ["src/dystopia.c"],
                              libraries=["tokyodystopia", "tokyocabinet", "z",
                                         "bz2", "pthread", "m", "c"])

class build_ext(_build_ext):
    def finalize_options(self):
        _build_ext.finalize_options(self)
        if "sdist" not in argv:
            self.extensions = [ext.c_ext for ext in (cabinet_ext, tyrant_ext,
                                                     dystopia_ext)
                               if check_extension(ext)]


setup(
      name="tokyo-python",
      version="0.7.0",
      url="http://packages.python.org/tokyo-python/",
      download_url="http://pypi.python.org/pypi/tokyo-python/",
      description="Tokyo libraries Python interface.",
      long_description=open("README.txt", "r").read(),
      author="Malek Hadj-Ali",
      author_email="lekmalek@gmail.com",
      platforms=["POSIX"],
      license="GNU General Public License (GPL)",
      packages = ["tokyo"],
      cmdclass={"build_ext": build_ext},
      ext_modules=[cabinet_ext.c_ext],
      classifiers=[
                   "Development Status :: 4 - Beta",
                   "Intended Audience :: Developers",
                   "License :: OSI Approved :: GNU General Public License (GPL)",
                   "Operating System :: POSIX",
                   "Programming Language :: Python :: 2.6",
                   "Programming Language :: Python :: 3.1",
                   "Topic :: Software Development :: Libraries"
                  ]
     )
