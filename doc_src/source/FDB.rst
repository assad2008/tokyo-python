.. _FDB:


.. currentmodule:: tokyo.cabinet


======================================
Fixed-length Database --- :class:`FDB`
======================================

    .. versionadded:: 0.3.0


:class:`FDB`
============

.. class:: FDB()

    Example::

        from tokyo.cabinet import *

        fdb = FDB()

        # if need be, you should call tune before open, ex. with default values:
        fdb.tune(0, 0)

        # open the database
        fdb.open("casket.tcf", FDBOWRITER | FDBOCREAT)

        # store records
        for key, value in [(1, "hop"), (2, "step"), (3, "jump")]:
            fdb[key] = value

        # retrieve one record
        print(fdb[1])

        # traverse records
        for key in fdb:
            print(key, fdb[key])

        # close the database
        fdb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*:

        * Python2: *key* must be :class:`long`/:class:`int` and *value* must be
          :class:`str`.
        * Python3: *key* must be :class:`int` and *value* must be :class:`bytes`.

        On top of that *key* **must always be > 0**. See :ref:`fdb_special_keys`
        for exceptions to this rule.

    .. seealso::
        `The Fixed-length Database API
        <http://fallabs.com/tokyocabinet/spex-en.html#tcfdbapi>`_


    .. describe:: len(fdb)

        Return the number of records in the database *fdb*.


    .. describe:: fdb[key]

        Return the value of *fdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: fdb[key] = value

        Set ``fdb[key]`` to *value*.


    .. describe:: del fdb[key]

        Remove ``fdb[key]`` from *fdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in fdb

        Return ``True`` if *fdb* has a key *key*, else ``False``.


    .. describe:: key not in fdb

        Equivalent to ``not key in fdb``.


    .. describe:: iter(fdb)

        Return an iterator over the keys of the database.


    .. method:: tune(width, size)

        Tune a database.

        :param width: the max lenght (in bytes) of the value of each record.
            Values longer than this parameter will be truncated. If specified as
            0 or as a negative value, the default value (255) is used.
        :param size: the max size (in bytes) of the database file. It will be
            aligned to the system page size. If specified as 0 or as a negative
            value, the default value (268435456) is used.

        See :ref:`fdb_tune_optimize_note`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: open(path, mode)

        Open a database.

        :param path: path to the database file.
        :param mode: mode, see :ref:`fdb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            FDBs are closed when garbage-collected.


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

        Return the value corresponding to *key*. Equivalent to ``fdb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del fdb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``fdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``fdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Concatenate a value at the end of an existing one. If there is no
        corresponding record, a new record is stored.


    .. method:: addint(key, num)

        Store an :class:`int` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as an
        :class:`int` this method raises :exc:`KeyError`.

        .. note::

            * The returned value will wrap around :const:`INT_MAX` and
              :const:`INT_MIN`. Example::

                  >>> fdb.addint(100, INT_MAX) # setting key 100 to INT_MAX
                  2147483647
                  >>> fdb.addint(100, 1) # adding 1 to key 100 returns INT_MIN
                  -2147483648
                  >>>

            * Trying to access a value set with :meth:`addint` using :meth:`get`
              or ``fdb[key]`` will **not** return an :class:`int`. It will
              instead return the internal binary representation of the value.
              Example::

                  >>> fdb.addint(100, INT_MAX) # setting key 100 to INT_MAX
                  2147483647
                  >>> fdb[100]
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
            :meth:`get` or ``fdb[key]`` will **not** return a :class:`float`.

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


    .. method:: range([lower=FDBIDMIN[, upper=FDBIDMAX[, max=-1]]])

        Return a frozenset of keys. If given, *max* is the maximum number of
        keys to fetch, if omitted or specified as a negative value no limit is
        applied.


    .. method:: optimize([width=0[, size=0]])

        Optimize a database.

        :param width: the max lenght (in bytes) of the value of each record.
            Values longer than this parameter will be truncated. If specified as
            0 or as a negative value, the current setting is kept.
        :param size: the max size (in bytes) of the database file. It will be
            aligned to the system page size. If specified as 0 or as a negative
            value, the current setting is kept.

        See :ref:`fdb_tune_optimize_note`.

        .. note::
            Optimizing a read only database, or during a transaction, is an
            invalid operation.


    .. attribute:: path

        The path to the database file.


    .. attribute:: size

        The size in bytes of the database file.


.. _fdb_tune_optimize_note:

Note on tuning/optimizing a fixed-length database
=================================================

The maximum length (and highest possible key) of a database is in direct
relation to the *width* and *size* parameters and can be determined using the
following function::

    import os

    PAGESIZE = os.sysconf("SC_PAGESIZE")

    def fdb_max_len(width, size):
        diff = size & (PAGESIZE - 1)
        if diff > 0: # align to page size
            size = size + PAGESIZE - diff
        # 256 is the database header size
        return int((size - 256) / (width + 1))


.. _fdb_open_modes:

:meth:`FDB.open` modes
======================

.. data:: FDBOREADER

    Open a database in read-only mode.

.. data:: FDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`FDBOWRITER` :

* .. data:: FDBOCREAT

      Create a new database file if it does not exists.

* .. data:: FDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).

* .. data:: FDBOTSYNC

      Sync the database file on every transaction.


The following constants can be combined with either :const:`FDBOREADER` or
:const:`FDBOWRITER` :

* .. data:: FDBONOLCK

      Opens the database file without file locking.

* .. data:: FDBOLCKNB

      Locking is performed without blocking.


.. _fdb_special_keys:

Special *keys*
==============

.. data:: FDBIDMIN (-1)

    The lowest *key* currently in the database.

.. data:: FDBIDPREV (-2)

   :const:`FDBIDMIN` - 1

.. data:: FDBIDMAX (-3)

    The highest *key* currently in the database.

.. data:: FDBIDNEXT (-4)

   :const:`FDBIDMAX` + 1
