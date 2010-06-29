.. _TDB:


.. currentmodule:: tokyo.cabinet


===============================
Table Database --- :class:`TDB`
===============================

    .. versionadded:: 0.4.0


:class:`TDB`
============

.. class:: TDB

    Example::

        from tokyo.cabinet import *

        tdb = TDB()

        # if need be, you should call tune/setcache/setxmsiz/setdfunit before
        # open, ex. with default values:
        tdb.tune(0, -1, -1, 0)
        tdb.setcache(0, 0, 0)
        tdb.setxmsiz(0)
        tdb.setdfunit(0)

        # open the database
        tdb.open("casket.tct", TDBOWRITER | TDBOCREAT)

        # store records
        for key, value in [
                           ("foo", {"age": "30", "name": "John", "sex": "m"}),
                           ("bar", {"age": "56", "name": "Paul", "sex": "m"}),
                           ("baz", {"age": "22", "name": "Ella", "sex": "f"})
                          ]:
            tdb[key] = value

        # retrieve one record
        print(tdb["foo"])

        # traverse records
        for key in tdb:
            print(key, tdb[key])

        # close the database
        tdb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* must be either :class:`str` (Python2) or :class:`bytes` (Python3)
        and *value* must be a :class:`dict`.

        All items in *value* must be pairs of :class:`str` (Python2) or
        :class:`bytes` (Python3). Empty keys in *value* **are not allowed**.

    .. seealso::
        `The Table Database API
        <http://1978th.net/tokyocabinet/spex-en.html#tctdbapi>`_


    .. describe:: len(tdb)

        Return the number of records in the database *tdb*.


    .. describe:: tdb[key]

        Return the value of *tdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: tdb[key] = value

        Set ``tdb[key]`` to *value*.


    .. describe:: del tdb[key]

        Remove ``tdb[key]`` from *tdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in tdb

        Return ``True`` if *tdb* has a key *key*, else ``False``.


    .. describe:: key not in tdb

        Equivalent to ``not key in tdb``.


    .. describe:: iter(tdb)

        Return an iterator over the keys of the database.


    .. method:: tune(bnum, apow, fpow, opts)

        Tune a database.

        :param bnum: the number of elements in a bucket array. If specified as
            0 or as a negative value, the default value (131071) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            default value (4) is used (means: 2**4).
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            default value (10) is used (means 2**10).
        :param opts: options, see :ref:`tdb_tune_optimize_options`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: setcache(rcnum, lcnum, ncnum)

        Set the cache size.

        :param rcnum: the maximum number of records to be cached. If specified
            as 0 or as a negative value, caching is disabled (default).
        :param lcnum: the maximum number of leaf nodes to be cached. If
            specified as 0 or as a negative value, the default value (4096) is
            used (for indexes).
        :param ncnum: the maximum number of non-leaf nodes to be cached. If
            specified as 0 or as a negative value, the default value (512) is
            used (for indexes).

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
        :param mode: mode, see :ref:`tdb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            TDBs are closed when garbage-collected.


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

        Return the value corresponding to *key*. Equivalent to ``tdb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del tdb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``tdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``tdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Merge a value with an existing one, **does not override existing items**
        in current value. If there is no corresponding record, a new record is
        stored.


    .. method:: sync

        Flush modifications to the database file.


    .. method:: iterkeys

        Return an iterator over the database’s keys.

        .. versionadded:: 0.6.1


    .. method:: itervalues

        Return an iterator over the database’s values.

        .. versionadded:: 0.6.1


    .. method:: iteritems

        Return an iterator over the database’s items (``(key, value)`` pairs).

        .. versionadded:: 0.6.1


    .. method:: searchkeys(prefix[, max])

        Return a frozenset of keys starting with *prefix*. If given, *max* is
        the maximum number of keys to fetch, if omitted or specified as a
        negative value no limit is applied.


    .. method:: setindex(column, type)

        Add an index to a column.

        :param column: name of the column. An empty string means *key*.
        :type column: :class:`str` (Python2)/:class:`bytes` (Python3)
        :param type: index type, see :ref:`tdb_setindex_types`.


    .. method:: uid

        Return a new unique id.


    .. method:: query

        Return a query object (:class:`TDBQuery`). See `Querying a Table
        Database --- TDBQuery`_.


    .. staticmethod:: metasearch(queries, type)

        Combine queries and return the result set as a tuple of keys.

        :param queries: a sequence of :class:`TDBQuery`.
        :param type: type of combination, see :ref:`tdb_metasearch_types`.


    .. method:: optimize([bnum=0[, apow=-1[, fpow=-1[, opts=255]]]])

        Optimize a database.

        :param bnum: the number of elements in a bucket array. If specified as 0
            or as a negative value, the default value (twice the number of
            records) is used.
        :param apow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param fpow: (power of 2) TODO. If specified as a negative value, the
            current setting is kept.
        :param opts: options, see :ref:`tdb_tune_optimize_options`. If
            specified as 255 (:const:`UINT8_MAX`), the current setting is kept.

        .. note::
            Optimizing a read only database, or during a transaction, is an
            invalid operation.


    .. attribute:: path

        The path to the database file.


    .. attribute:: size

        The size in bytes of the database file.


.. _tdb_open_modes:

:meth:`TDB.open` modes
======================

.. data:: TDBOREADER

    Open a database in read-only mode.

.. data:: TDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`TDBOWRITER` :

* .. data:: TDBOCREAT

      Create a new database file if it does not exists.

* .. data:: TDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).

* .. data:: TDBOTSYNC

      Sync the database file on every transaction.


The following constants can be combined with either :const:`TDBOREADER` or
:const:`TDBOWRITER` :

* .. data:: TDBONOLCK

      Opens the database file without file locking.

* .. data:: TDBOLCKNB

      Locking is performed without blocking.


.. _tdb_tune_optimize_options:

:meth:`TDB.tune`/:meth:`TDB.optimize` options
=============================================

.. data:: TDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: TDBTDEFLATE

    Each record is compressed with Deflate encoding.

.. data:: TDBTBZIP

    Each record is compressed with BZIP2 encoding.

.. data:: TDBTTCBS

    Each record is compressed with TCBS encoding.


.. _tdb_setindex_types:

:meth:`TDB.setindex` types
==========================

.. data:: TDBITLEXICAL

    String index.

.. data:: TDBITDECIMAL

    Numeric index.

.. data:: TDBITTOKEN

    Token index.

.. data:: TDBITQGRAM

    Q-gram index.

.. data:: TDBITOPT

    Optimize the index.

.. data:: TDBITVOID

    Remove the index.

.. data:: TDBITKEEP

    Fail if the index already exists.


.. _tdb_metasearch_types:

:meth:`TDB.metasearch` types
============================

.. data:: TDBMSUNION

    Union.

.. data:: TDBMSISECT

    Intersection.

.. data:: TDBMSDIFF

    Difference.


Querying a Table Database --- :class:`TDBQuery`
===============================================


.. class:: TDBQuery

    When first returned by :meth:`TDB.query` a query result set potentially
    includes all keys contained in the database. You can narrow down a search by
    calling :meth:`filter` and limit the number of results with :meth:`limit`.


    .. method:: filter(column, condition, expr)

        Filter the result set on the condition expressed by the parameters.

        :param column: name of the column. An empty string means *key*.
        :type column: :class:`str` (Python2)/:class:`bytes` (Python3)
        :param condition: see :ref:`tdbquery_filter_conditions`.
        :param expr: expression.
        :type expr: :class:`str` (Python2)/:class:`bytes` (Python3)

        .. note::
            Calling :meth:`filter` multiple times with different conditions is
            equivalent to applying a logical ``AND`` between the conditions.


    .. method:: sort(column, type)

        Sort the result set.

        :param column: name of the column. An empty string means *key*.
        :type column: :class:`str` (Python2)/:class:`bytes` (Python3)
        :param type: sort type (and direction), see :ref:`tdbquery_sort_types`.


    .. method:: limit([max[, skip]])

        Limit the number of keys in the result set.

        :param max: the maximum number of keys to return. If specified as a
            negative value no limit is applied.
        :param skip: the number of keys to skip (from the beginning). If
            specified as a 0 or as a negative value no skipping is done.


    .. method:: search

        Execute the query and return the result set as a tuple of keys.


    .. method:: remove

        Remove all records corresponding to the result set from the database.


    .. method:: process(callback)

        Apply *callback* to each record in the result set.

        :param callback: *callback* must be a callable that accept a pair *key*,
         *value* as its arguments. *callback* can return one, or a combination,
         of :ref:`these constants <tdbquery_process_constants>` to
         trigger post-processing or to stop iterating.


    .. method:: count

        Return the length of the result set.


    .. attribute:: hint

        TODO.


.. _tdbquery_filter_conditions:

:meth:`TDBQuery.filter` conditions
==================================

*value* refers to ``tdb[key][column]`` if *column* is not an empty string
otherwise *value* is *key* (that will need a better description).

String conditions :

.. data:: TDBQCSTREQ

    ::

        value == expr

.. data:: TDBQCSTRINC

    ::

        expr in value #substring test

.. data:: TDBQCSTRBW

    ::

        value.startswith(expr)

.. data:: TDBQCSTREW

    ::

        value.endswith(expr)

.. data:: TDBQCSTRAND

    if *expr* is expressed as ``"expr1,expr2"``::

        expr1 in value and expr2 in value

.. data:: TDBQCSTROR

    if *expr* is expressed as ``"expr1,expr2"``::

        expr1 in value or expr2 in value

.. data:: TDBQCSTROREQ

    if *expr* is expressed as ``"expr1,expr2"``::

        value == expr1 or value == expr2

.. data:: TDBQCSTRRX

    *expr* is a regular expression.

Numeric conditions :

.. data:: TDBQCNUMEQ

    ::

        value == expr

.. data:: TDBQCNUMGT

    ::

        value > expr

.. data:: TDBQCNUMGE

    ::

        value >= expr

.. data:: TDBQCNUMLT

    ::

        value < expr

.. data:: TDBQCNUMLE

    ::

        value <= expr

.. data:: TDBQCNUMBT

    if *expr* is expressed as ``"expr1,expr2"``::

        value >= min(expr1, expr2) and value <= max(expr1, expr2)

.. data:: TDBQCNUMOREQ

    if *expr* is expressed as ``"expr1,expr2"``::

        value == expr1 or value == expr2

Full-text search :

.. data:: TDBQCFTSPH

    TODO.

.. data:: TDBQCFTSAND

    TODO.

.. data:: TDBQCFTSOR

    TODO.

.. data:: TDBQCFTSEX

    TODO.

All conditions above can be combined with the following :

.. data:: TDBQCNEGATE

    Negate the condition.

.. data:: TDBQCNOIDX

    Do not use the column index (if one has been set).


.. _tdbquery_sort_types:

:meth:`TDBQuery.sort` types
===========================

.. data:: TDBQOSTRASC

    Alphabetical, ascending.

.. data:: TDBQOSTRDESC

    Alphabetical, descending.

.. data:: TDBQONUMASC

    Numerical, ascending.

.. data:: TDBQONUMDESC

    Numerical, descending.


.. _tdbquery_process_constants:

:meth:`TDBQuery.process` post-processing constants
==================================================

.. data:: TDBQPPUT

    Store a modified *value*.

.. data:: TDBQPOUT

    Remove the record.

.. data:: TDBQPSTOP

    Stop iterating over the records.
