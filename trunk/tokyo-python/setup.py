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
from os import name as platform_name
from ctypes.util import find_library
from sys import argv
from distutils.command.build_ext import build_ext as _build_ext
from distutils.core import setup, Extension


curr_ver = python_version()
min_vers = {2: "2.6.4", 3: "3.1.1"}
if curr_ver < min_vers[int(curr_ver[0])]:
    raise SystemExit("Aborted: tokyo-python requires Python2 >= {0[2]} "
                     "OR Python3 >= {0[3]}".format(min_vers))

if platform_name != "posix":
    raise SystemExit("Aborted: platform '{0}' not supported".format(platform_name))


cabinet_ext = Extension("tokyo.cabinet", ["src/cabinet.c"],
                        libraries=["tokyocabinet", "z", "bz2", "rt", "pthread",
                                   "m", "c"])
tyrant_ext = Extension("tokyo.tyrant", ["src/tyrant.c"],
                       libraries=["tokyotyrant", "tokyocabinet", "z", "bz2",
                                  "resolv", "nsl", "dl", "rt", "pthread", "m", "c"])


class build_ext(_build_ext):
    def finalize_options(self):
        _build_ext.finalize_options(self)
        if "sdist" not in argv:
            if not find_library("tokyocabinet"):
                raise SystemExit(
                    "Aborted: tokyo-python requires Tokyo Cabinet to be installed.\n"
                    "See http://1978th.net/tokyocabinet/ for more information.")
            if find_library("tokyotyrant"):
                self.extensions.append(tyrant_ext)


setup(
      name="tokyo-python",
      version="0.6.1",
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
      ext_modules=[cabinet_ext],
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
