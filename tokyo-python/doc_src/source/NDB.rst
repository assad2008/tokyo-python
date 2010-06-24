.. _NDB:


.. currentmodule:: tokyo.cabinet


========================================
On-memory Tree Database --- :class:`NDB`
========================================

    .. versionadded:: 0.2.1


:class:`NDB`
============

.. class:: NDB()

    Example::

        from tokyo.cabinet import *

        ndb = NDB()

        # store records
        for key, value in [("foo", "hop"), ("bar", "step"), ("baz", "jump")]:
            ndb[key] = value

        # retrieve one record
        print(ndb["foo"])

        # traverse records
        for key in ndb:
            print(key, ndb[key])

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must be either :class:`str` (Python2) or
        :class:`bytes` (Python3).

    .. seealso::
        `API of On-memory Tree Database
        <http://1978th.net/tokyocabinet/spex-en.html#tcutilapi_ndbapi>`_


    .. describe:: len(ndb)

        Return the number of records in the database *ndb*.


    .. describe:: ndb[key]

        Return the value of *ndb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: ndb[key] = value

        Set ``ndb[key]`` to *value*.


    .. describe:: del ndb[key]

        Remove ``ndb[key]`` from *ndb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in ndb

        Return ``True`` if *ndb* has a key *key*, else ``False``.


    .. describe:: key not in ndb

        Equivalent to ``not key in ndb``.


    .. describe:: iter(ndb)

        Return an iterator over the keys of the database.


    .. method:: clear

        Remove all records from the database.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``ndb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del ndb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``ndb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``ndb[key] = value`` or :meth:`put`), this method raises
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

        from tokyo.cabinet import NDB as _NDB

        class NDB(_NDB):

            def keys(self):
                return (key for key in self)

            def values(self):
                return (self[key] for key in self)

            def items(self):
                return ((key, self[key]) for key in self)


    .. attribute:: size

        The size in bytes of the database.
