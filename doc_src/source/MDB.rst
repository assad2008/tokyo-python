.. _MDB:


.. currentmodule:: tokyo.cabinet


========================================
On-memory Hash Database --- :class:`MDB`
========================================

    .. versionadded:: 0.1.2


:class:`MDB`
============

.. class:: MDB([bnum])

    :param bnum: the number of elements in a bucket array. If omitted or
        specified as 0, the default value (65536) is used.

    Example::

        from tokyo.cabinet import *

        mdb = MDB()

        # store records
        for key, value in [("foo", "hop"), ("bar", "step"), ("baz", "jump")]:
            mdb[key] = value

        # retrieve one record
        print(mdb["foo"])

        # traverse records
        for key in mdb:
            print(key, mdb[key])

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must be either :class:`str` (Python2) or
        :class:`bytes` (Python3).

    .. seealso::
        `API of On-memory Hash Database
        <http://1978th.net/tokyocabinet/spex-en.html#tcutilapi_mdbapi>`_


    .. describe:: len(mdb)

        Return the number of records in the database *mdb*.


    .. describe:: mdb[key]

        Return the value of *mdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: mdb[key] = value

        Set ``mdb[key]`` to *value*.


    .. describe:: del mdb[key]

        Remove ``mdb[key]`` from *mdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in mdb

        Return ``True`` if *mdb* has a key *key*, else ``False``.


    .. describe:: key not in mdb

        Equivalent to ``not key in mdb``.


    .. describe:: iter(mdb)

        Return an iterator over the keys of the database.


    .. method:: clear

        Remove all records from the database.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``mdb[key]``.

        .. versionadded:: 0.2.0


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del mdb[key]``.

        .. versionadded:: 0.2.0


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``mdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``mdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Concatenate a value at the end of an existing one. If there is no
        corresponding record, a new record is stored.


    .. method:: searchkeys(prefix[, max])

        Return a frozenset of keys starting with *prefix*. If given, *max* is
        the maximum number of keys to fetch, if omitted or specified as a
        negative value no limit is applied.


    Methods :meth:`keys`, :meth:`values` and :meth:`items` are not yet
    implemented (mainly because I didn't settle on how to do it: should they
    return :class:`Iterable`, :class:`Iterator`, :class:`MappingView`, etc.?).
    Any help would be greatly appreciated in this matter.

    For the time being, for those of you who really need these methods, it's
    trivial to implement them in python. Here is an example using generators::

        from tokyo.cabinet import MDB as _MDB

        class MDB(_MDB):

            def keys(self):
                return (key for key in self)

            def values(self):
                return (self[key] for key in self)

            def items(self):
                return ((key, self[key]) for key in self)


    .. attribute:: size

        The size in bytes of the database.
