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


"""Provide a dbm-compatible interface to tokyo.cabinet."""


try:
    import builtins
except ImportError:
    import __builtin__ as builtins
import os
import struct
import collections
import tokyo.cabinet as tc


class error(tc.Error): pass


class DBMMixin(collections.MutableMapping):

    def __init__(self, filename, flag, mode):
        self.open(filename, self._flags[flag])
        try:
            um = os.umask(0)
            os.umask(um)
        except AttributeError:
            pass
        else:
            os.chmod(filename, mode & (~um))


class TCHDB(tc.HDB, DBMMixin):

    _exts = (".tch", ".hdb")
    _flags = {"r": tc.HDBOREADER,
              "w": tc.HDBOWRITER,
              "c": tc.HDBOWRITER | tc.HDBOCREAT,
              "n": tc.HDBOWRITER | tc.HDBOCREAT | tc.HDBOTRUNC}
    get = DBMMixin.get


class TCBDB(tc.BDB, DBMMixin):

    _exts = (".tcb", ".bdb")
    _flags = {"r": tc.BDBOREADER,
              "w": tc.BDBOWRITER,
              "c": tc.BDBOWRITER | tc.BDBOCREAT,
              "n": tc.BDBOWRITER | tc.BDBOCREAT | tc.BDBOTRUNC}

    def get(self, key, default=None, duplicate=False):
        try:
            return tc.BDB.get(self, key, duplicate)
        except KeyError:
            return default

    def iterkeys(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.key()
            c.next()

    def itervalues(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.value()
            c.next()

    def iteritems(self):
        c = self.cursor()
        c.first()
        while True:
            yield c.item()
            c.next()

    def keys(self):
        return list(self.iterkeys())

    def values(self):
        return list(self.itervalues())

    def items(self):
        return list(self.iteritems())


class TCFDB(tc.FDB, DBMMixin):

    _exts = (".tcf", ".fdb")
    _flags = {"r": tc.FDBOREADER,
              "w": tc.FDBOWRITER,
              "c": tc.FDBOWRITER | tc.FDBOCREAT,
              "n": tc.FDBOWRITER | tc.FDBOCREAT | tc.FDBOTRUNC}
    get = DBMMixin.get


class TCTDB(tc.TDB, DBMMixin):

    _exts = (".tct", ".tdb")
    _flags = {"r": tc.TDBOREADER,
              "w": tc.TDBOWRITER,
              "c": tc.TDBOWRITER | tc.TDBOCREAT,
              "n": tc.TDBOWRITER | tc.TDBOCREAT | tc.FDBOTRUNC}
    get = DBMMixin.get


# _db_types[0] is the default
_db_types = (TCHDB, TCBDB, TCFDB, TCTDB)
_db_exts = {}
for t in _db_types:
    _db_exts.update(((e, t) for e in t._exts))


def whichdb(filename):
    header = builtins.open(filename, "rb").read(256)
    if len(header) != 256 or not header[0:32].startswith(b"ToKyO CaBiNeT"):
        raise error("'{0}' is not a Tokyo Cabinet file".format(filename))
    try:
        return _db_types[struct.unpack("<B", header[32:33])[0]]
    except IndexError:
        raise error("unknown type of Tokyo Cabinet file")


def open(filename, flag="r", mode=0o666):
    if os.path.exists(filename):
        return whichdb(filename)(filename, flag, mode)
    elif flag in ("c", "n"):
        ext = os.path.splitext(filename)[1].lower()
        if ext in _db_exts:
            return _db_exts[ext](filename, flag, mode)
        else:
            # _db_types[0] is the default
            return _db_types[0](filename, flag, mode)
    else:
        raise error("need 'c' or 'n' flag to open new db")
