.. _HDB:


.. currentmodule:: tokyo.cabinet


==============================
Hash Database --- :class:`HDB`
==============================


:class:`HDB`
============

.. class:: HDB

    Example::

        from tokyo.cabinet import *

        hdb = HDB()

        # if need be, you should call tune/setcache/setxmsiz/setdfunit before
        # open, ex. with default values:
        hdb.tune(0, -1, -1, 0)
        hdb.setcache(0)
        hdb.setxmsiz(0)
        hdb.setdfunit(0)

        # open the database
        hdb.open("casket.tch", HDBOWRITER | HDBOCREAT)

        # store records
        for key, value in [("foo", "hop"), ("bar", "step"), ("baz", "jump")]:
            hdb[key] = value

        # retrieve one record
        print(hdb["foo"])

        # traverse records
        for key in hdb:
            print(key, hdb[key])

        # close the database
        hdb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must be either :class:`str` (Python2) or
        :class:`bytes` (Python3).

    .. seealso::
        `The Hash Database API
        <http://fallabs.com/tokyocabinet/spex-en.html#tchdbapi>`_


    .. describe:: len(hdb)

        Return the number of records in the database *hdb*.


    .. describe:: hdb[key]

        Return the value of *hdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: hdb[key] = value

        Set ``hdb[key]`` to *value*.


    .. describe:: del hdb[key]

        Remove ``hdb[key]`` from *hdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in hdb

        Return ``True`` if *hdb* has a key *key*, else ``False``.


    .. describe:: key not in hdb

        Equivalent to ``not key in hdb``.


    .. describe:: iter(hdb)

        Return an iterator over the keys of the database.


    .. method:: tune(bnum, apow, fpow, opts)

        Tune a database.

        :param bnum: the number of elements in a bucket array. If specified as
            0 or as a negative value, the default value (131071) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            default value (4) is used (means: 2**4).
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            default value (10) is used (means 2**10).
        :param opts: options, see :ref:`hdb_tune_optimize_options`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: setcache(rcnum)

        Set the cache size.

        :param rcnum: the maximum number of records to be cached. If specified
            as 0 or as a negative value, caching is disabled (default).

        .. note::
            Setting the cache size on an open database is an invalid operation.


    .. method:: setxmsiz(xmsiz)

        Set the extra mapped memory size.

        :param xmsiz: the amount of extra mapped memory (in what unit?). If
            specified as 0 or as a negative value, the extra mapped memory is
            disabled. Default is 67108864 (unit?).

        .. note::
            Setting the extra memory size on an open database is an invalid
            operation.


    .. method:: setdfunit(dfunit)

        Set auto defragmentation's unit step number.

        :param dfunit: the unit step number(?). If specified as 0 or as a
            negative value, auto defragmentation is disabled (default).

        .. note::
            Setting this on an open database is an invalid operation.


    .. method:: open(path, mode)

        Open a database.

        :param path: path to the database file.
        :param mode: mode, see :ref:`hdb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            HDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database file.

        :param path: path to the destination file.


    .. method:: begin

        Begin a transaction.


    .. method:: commit

        Commit a transaction.


    .. method:: abort

        Abort a transaction.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``hdb[key]``.

        .. versionadded:: 0.2.0


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del hdb[key]``.

        .. versionadded:: 0.2.0


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``hdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``hdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Concatenate a value at the end of an existing one. If there is no
        corresponding record, a new record is stored.


    .. method:: putasync(key, value)

        Store a record in the database in an asynchronous fashion. Records
        passed to this method are accumulated into an inner buffer and written
        to the file when synced.


    .. method:: addint(key, num)

        Store an :class:`int` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as an
        :class:`int` this method raises :exc:`KeyError`.

        .. note::

            * The returned value will wrap around :const:`INT_MAX` and
              :const:`INT_MIN`. Example::

                  >>> hdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> hdb.addint('id', 1) # adding 1 to 'id' returns INT_MIN
                  -2147483648
                  >>>

            * Trying to access a value set with :meth:`addint` using :meth:`get`
              or ``hdb[key]`` will **not** return an :class:`int`. It will
              instead return the internal binary representation of the value.
              Example::

                  >>> hdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> hdb['id']
                  '\xff\xff\xff\x7f'
                  >>>

        .. versionadded:: 0.5.0


    .. method:: adddouble(key, num)

        Store a :class:`float` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as a
        :class:`float` this method raises :exc:`KeyError`.

        .. note::

            Trying to access a value set with :meth:`adddouble` using
            :meth:`get` or ``hdb[key]`` will **not** return a :class:`float`.

        .. versionadded:: 0.5.0


    .. method:: sync

        Flush modifications to the database file.


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


    .. method:: optimize([bnum=0[, apow=-1[, fpow=-1[, opts=255]]]])

        Optimize a database.

        :param bnum: the number of elements in a bucket array. If specified as 0
            or as a negative value, the default value (twice the number of
            records) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param opts: options, see :ref:`hdb_tune_optimize_options`. If
            specified as 255 (:const:`UINT8_MAX`), the current setting is kept.

        .. note::
            Optimizing a read only database, or during a transaction, is an
            invalid operation.


    .. attribute:: path

        The path to the database file.


    .. attribute:: size

        The size in bytes of the database file.


.. _hdb_open_modes:

:meth:`HDB.open` modes
======================

.. data:: HDBOREADER

    Open a database in read-only mode.

.. data:: HDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`HDBOWRITER` :

* .. data:: HDBOCREAT

      Create a new database file if it does not exists.

* .. data:: HDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).

* .. data:: HDBOTSYNC

      Sync the database file on every transaction.


The following constants can be combined with either :const:`HDBOREADER` or
:const:`HDBOWRITER` :

* .. data:: HDBONOLCK

      Opens the database file without file locking.

* .. data:: HDBOLCKNB

      Locking is performed without blocking.


.. _hdb_tune_optimize_options:

:meth:`HDB.tune`/:meth:`HDB.optimize` options
=============================================

.. data:: HDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: HDBTDEFLATE

    Each record is compressed with Deflate encoding.

.. data:: HDBTBZIP

    Each record is compressed with BZIP2 encoding.

.. data:: HDBTTCBS

    Each record is compressed with TCBS encoding.
