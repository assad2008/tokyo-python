.. _BDB:


.. currentmodule:: tokyo.cabinet


=================================
B+ Tree Database --- :class:`BDB`
=================================

    .. versionadded:: 0.2.0


:class:`BDB`
============

.. class:: BDB

    Example::

        from tokyo.cabinet import *

        bdb = BDB()

        # if need be, you should call tune/setcache/setxmsiz/setdfunit before
        # open, ex. with default values:
        bdb.tune(0, 0, 0, -1, -1, 0)
        bdb.setcache(0, 0)
        bdb.setxmsiz(0)
        bdb.setdfunit(0)

        # open the database
        bdb.open("casket.tcb", BDBOWRITER | BDBOCREAT)

        # store records
        for key, value in [("foo", "hop"), ("bar", "step"), ("baz", "jump")]:
            bdb[key] = value

        # retrieve one record
        print(bdb["foo"])

        # traverse records
        for key in bdb:
            print(key, bdb[key])

        # close the database
        bdb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must be either :class:`str` (Python2) or
        :class:`bytes` (Python3).

    .. seealso::
        `The B+ Tree Database API
        <http://1978th.net/tokyocabinet/spex-en.html#tcbdbapi>`_


    .. describe:: len(bdb)

        Return the number of records in the database *bdb*.


    .. describe:: bdb[key]

        Return the value of *bdb*'s **first record** corresponding to *key*.
        Raises :exc:`KeyError` if *key* is not in the database.


    .. describe:: bdb[key] = value

        Set ``bdb[key]`` to *value*. If *key* has duplicates the **first
        corresponding record** is modified.


    .. describe:: del bdb[key]

        Remove ``bdb[key]`` from *bdb*.  Raises :exc:`KeyError` if *key* is not
        in the database. If *key* has duplicates the **first corresponding
        record** is deleted.


    .. describe:: key in bdb

        Return ``True`` if *bdb* has a key *key*, else ``False``.


    .. describe:: key not in bdb

        Equivalent to ``not key in bdb``.


    .. describe:: iter(bdb)

        Return an iterator over the keys of the database.

        .. note::
            Better use a :class:`BDBCursor` to iterate over a database with
            duplicates.


    .. method:: tune(lmemb, nmemb, bnum, apow, fpow, opts)

        Tune a database.

        :param lmemb: the number of members in each leaf page. If specified as 0
            or as a negative value, the default value (128) is used.
        :param nmemb: the number of members in each non-leaf page. If specified
            as 0 or as a negative value, the default value (256) is used.
        :param bnum: the number of elements in a bucket array. If specified as 0
            or as a negative value, the default value (32749) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            default value (8) is used (means: 2**8).
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            default value (10) is used (means 2**10).
        :param opts: options, see :ref:`bdb_tune_optimize_options`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: setcache(lcnum, ncnum)

        Set the cache parameters.

        :param lcnum: the maximum number of leaf nodes to be cached. If
            specified as 0 or as a negative value, the default value (1024) is
            used.
        :param ncnum: the maximum number of non-leaf nodes to be cached. If
            specified as 0 or as a negative value, the default value (512) is
            used.

        .. note::
            Setting the cache parameters on an open database is an invalid
            operation.


    .. method:: setxmsiz(xmsiz)

        Set the extra mapped memory size.

        :param xmsiz: the amount of extra mapped memory (in what unit?). If
            specified as 0 or as a negative value, the extra mapped memory is
            disabled (default).

        .. note::
            Setting the extra memory size on an open database is an invalid
            operation.


    .. method:: setdfunit(dfunit)

        Set auto defragmentation's unit step number.

        :param dfunit: the unit step number(?). If specified as 0 or as a
            negative value, auto defragmentation is disabled (default).

        .. note::
            Setting this on an open database is an invalid operation.


    .. method:: setcmpfunc(callback)

        Set the compare callback function.

        :param callback: if it is an :class:`int`, it must be one of
            :ref:`bdb_setcmpfunc_constants`. Otherwise, it must be a
            :class:`callable` taking two arguments, *a* and *b*, and returning
            ``1`` if *a* is greater than *b*, ``0`` if *a* is equal to *b*, and
            ``-1`` if *a* is less than *b*.

        .. warning::

            * If *callback* raises an exception or returns anything but an
              :class:`int`, the result is ignored and ``0`` is returned instead
              (a warning message is printed to :obj:`sys.stderr`). This means
              that if an error occurs during comparison, *a* and *b* will be
              considered equal; leading to *b*'s value being **overwritten**
              with *a*'s value if this happens during a write operation.
              **USE AT YOUR OWN RISK**.

            * There's a huge performance price in setting your own compare
              callback.

        .. note::

            * The compare callback must be set before the database file is
              created, and it must be set **again** each time the database is
              opened. It's an invalid operation to not set the compare callback
              on a database that has been marked as having a custom one.

            * Setting the compare callback function on an open database is an
              invalid operation.

        .. versionadded:: 0.6.0


    .. method:: open(path, mode)

        Open a database.

        :param path: path to the database file.
        :param mode: mode, see :ref:`bdb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            BDBs are closed when garbage-collected.


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


    .. method:: get(key[, duplicate=False])

        If *duplicate* is :const:`False` (default) this is equivalent to
        ``bdb[key]``. If *duplicate* is :const:`True` this method returns a
        tuple of all the values corresponding to *key*. It will raise
        :exc:`KeyError` if *key* is not in the database.


    .. method:: remove(key[, duplicate=False])

        If *duplicate* is :const:`False` (default) this is equivalent to
        ``del bdb[key]``. If *duplicate* is :const:`True` this method removes
        all records corresponding to *key*. It will raise :exc:`KeyError` if
        *key* is not in the database.


    .. method:: put(key, value[, duplicate=False])

        If *duplicate* is :const:`False` (default) this is equivalent to
        ``bdb[key] = value``. If *duplicate* is :const:`True` and *key* is
        already in the database, this method will store a new duplicate record
        after the last corresponding record.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``hdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Concatenate a value at the end of an existing one. If there is no
        corresponding record, a new record is stored. If *key* has duplicates
        the first corresponding record is modified.


    .. method:: putdup(key, values)

        Store duplicate records in the database, this is equivalent to::

            def putdup(self, key, values):
                for value in values:
                    self.put(key, value, duplicate=True)


    .. method:: addint(key, num)

        Store an :class:`int` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as an
        :class:`int` this method raises :exc:`KeyError`.

        .. note::

            * The returned value will wrap around :const:`INT_MAX` and
              :const:`INT_MIN`. Example::

                  >>> bdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> bdb.addint('id', 1) # adding 1 to 'id' returns INT_MIN
                  -2147483648
                  >>>

            * Trying to access a value set with :meth:`addint` using :meth:`get`
              or ``bdb[key]`` will **not** return an :class:`int`. It will
              instead return the internal binary representation of the value.
              Example::

                  >>> bdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> bdb['id']
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
            :meth:`get` or ``bdb[key]`` will **not** return a :class:`float`.

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


    .. method:: range([begin=None[, end=None[, max=-1]]])

        Return a frozenset of all the keys starting with *begin*, all the keys
        starting with *end* and all the keys in between. If given, *max* is the
        maximum number of keys to fetch, if omitted or specified as a negative
        value no limit is applied. Example::

            >>> from tokyo.cabinet import *
            >>> bdb = BDB()
            >>> bdb.open("range.tcb", BDBOWRITER | BDBOCREAT)
            >>> for key, value in [("a", "a"), ("b", "b"), ("c", "c"), ("d", "d")]:
            ...     bdb[key] = value
            ...
            >>> bdb.range()
            frozenset(['a', 'c', 'b', 'd'])
            >>> bdb.range(end="c")
            frozenset(['a', 'c', 'b'])
            >>>
            >>> bdb.range("b", "d") # begin and end are inclusive
            frozenset(['c', 'b', 'd'])
            >>> bdb.range("b", "z")
            frozenset(['c', 'b', 'd'])
            >>> bdb.range("z", "b") # end < begin doesn't work
            frozenset([])
            >>> bdb.range("a", "a") # end == begin works
            frozenset(['a'])
            >>> bdb.range("A", "Z") # it is case sensitive
            frozenset([])
            >>> bdb.range("A", "z") # uppercase < lowercase
            frozenset(['a', 'c', 'b', 'd'])
            >>> bdb.range("z", "A")
            frozenset([])
            >>> bdb.close()
            >>>


    .. method:: cursor

        Return a cursor object (:class:`BDBCursor`). See `BDBCursor operations`_.


    .. method:: optimize([lmemb=0[, nmemb=0[, bnum=0[, apow=-1[, fpow=-1[, opts=255]]]]]])

        Optimize a database.

        :param lmemb: the number of members in each leaf page. If specified as 0
            or as a negative value, the current setting is kept.
        :param nmemb: the number of members in each non-leaf page. If specified
            as 0 or as a negative value, the current setting is kept.
        :param bnum: the number of elements in a bucket array. If specified as 0
            or as a negative value, the default value (twice the number of
            pages) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param opts: options, see :ref:`bdb_tune_optimize_options`. If
            specified as 255 (:const:`UINT8_MAX`), the current setting is kept.

        .. note::
            Optimizing a read only database, or during a transaction, is an
            invalid operation.


    .. attribute:: path

        The path to the database file.


    .. attribute:: size

        The size in bytes of the database file.


.. _bdb_open_modes:

:meth:`BDB.open` modes
======================

.. data:: BDBOREADER

    Open a database in read-only mode.

.. data:: BDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`BDBOWRITER` :

* .. data:: BDBOCREAT

      Create a new database file if it does not exists.

* .. data:: BDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).

* .. data:: BDBOTSYNC

      Sync the database file on every transaction.


The following constants can be combined with either :const:`BDBOREADER` or
:const:`BDBOWRITER` :

* .. data:: BDBONOLCK

      Opens the database file without file locking.

* .. data:: BDBOLCKNB

      Locking is performed without blocking.


.. _bdb_tune_optimize_options:

:meth:`BDB.tune`/:meth:`BDB.optimize` options
=============================================

.. data:: BDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: BDBTDEFLATE

    Each page is compressed with Deflate encoding.

.. data:: BDBTBZIP

    Each page is compressed with BZIP2 encoding.

.. data:: BDBTTCBS

    Each page is compressed with TCBS encoding.


.. _bdb_setcmpfunc_constants:

:meth:`BDB.setcmpfunc` compare callback constants
=================================================

.. data:: BDBCMPLEXICAL

    tccmplexical (default)

.. data:: BDBCMPDECIMAL

    tccmpdecimal

.. data:: BDBCMPINT32

    tccmpint32

.. data:: BDBCMPINT64

    tccmpint64


:class:`BDBCursor` operations
=============================


.. class:: BDBCursor

    .. warning::
        When first returned by :meth:`BDB.cursor` a cursor is not fully
        initialized. You need to call one of the positioning method
        (:meth:`first`, :meth:`last` or :meth:`jump`) before you can do anything
        useful with it.


    .. method:: first

        Move to the first record (if there is no such thing :exc:`StopIteration`
        is raised).


    .. method:: last

        Move to the last record (if there is no such thing :exc:`StopIteration`
        is raised).


    .. method:: jump(key)

        Move to the first occurrence of *key*. **If** *key* **is not in the
        database, the cursor is positioned to the next available record** (if
        there is no such thing :exc:`StopIteration` is raised).


    .. method:: prev

        Move to the previous record (if there is no such thing
        :exc:`StopIteration` is raised).


    .. method:: next

        Move to the next record (if there is no such thing :exc:`StopIteration`
        is raised).


    .. method:: put(value[, mode=BDBCPCURRENT])

        Store a value at/around the cursor's current position.

        :param mode: see :ref:`bdbcursor_put_modes`.


    .. method:: remove

        Remove the current record. After deletion the cursor is moved to the
        next available record.


    .. method:: key

        Get current key.


    .. method:: value

        Get current value.


    .. method:: item

        Get current item (a *(key, value)* tuple).


.. _bdbcursor_put_modes:

:meth:`BDBCursor.put` modes
===========================

.. data:: BDBCPCURRENT

    The current value is overwritten (default).

.. data:: BDBCPBEFORE

    A new item is stored before the current position and the cursor is moved to
    the newly inserted record.

.. data:: BDBCPAFTER

    A new item is stored after the current position and the cursor is moved to
    the newly inserted record.
