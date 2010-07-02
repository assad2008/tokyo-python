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


    .. method:: iterkeys

        Return an iterator over the database's keys.

        .. versionadded:: 0.6.1


    .. method:: itervalues

        Return an iterator over the database's values.

        .. versionadded:: 0.6.1


    .. method:: iteritems

        Return an iterator over the database's items (``(key, value)`` pairs).

        .. versionadded:: 0.6.1


    .. method:: searchkeys(prefix[, max])

        Return a frozenset of keys starting with *prefix*. If given, *max* is
        the maximum number of keys to fetch, if omitted or specified as a
        negative value no limit is applied.


    .. attribute:: size

        The size in bytes of the database.
