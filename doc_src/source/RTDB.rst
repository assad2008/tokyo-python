.. _RTDB:


.. currentmodule:: tokyo.tyrant


=======================================
Remote Table Database --- :class:`RTDB`
=======================================


:class:`RTDB`
=============

.. class:: RTDB

    Example::

        # First you need to start a ttserver, example command (starting a
        # server on default address localhost:1978): ttserver test.tct
        # see http://1978th.net/tokyotyrant/spex.html#serverprog and
        # http://1978th.net/tokyotyrant/spex.html#tutorial for more info
        # about ttserver

        from tokyo.tyrant import *

        rtdb = RTDB()

        # if need be, you should call tune before open, ex. with default values:
        rtdb.tune(0, 0)

        # open the database
        rtdb.open() # this will open the server at localhost:1978

        # store records
        for key, value in [
                           ("foo", {"age": "30", "name": "John", "sex": "m"}),
                           ("bar", {"age": "56", "name": "Paul", "sex": "m"}),
                           ("baz", {"age": "22", "name": "Ella", "sex": "f"})
                          ]:
            rtdb[key] = value

        # retrieve one record
        print(rtdb["foo"])

        # traverse records
        for key in rtdb:
            print(key, rtdb[key])

        # close the database
        rtdb.close()

    .. warning::
        This client works only with Tokyo Tyrant daemon serving table databases.

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* must be either :class:`str` (Python2) or :class:`bytes` (Python3)
        and *value* must be a :class:`dict`.

        All items in *value* must be pairs of :class:`str` (Python2) or
        :class:`bytes` (Python3). Empty keys in *value* **are not allowed**.

    .. seealso::
        `API of the Table Extension
        <http://1978th.net/tokyotyrant/spex.html#tcrdbapi_apitbl>`_


    .. describe:: len(rtdb)

        Return the number of records in the database *rtdb*.


    .. describe:: rtdb[key]

        Return the value of *rtdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: rtdb[key] = value

        Set ``rtdb[key]`` to *value*.


    .. describe:: del rtdb[key]

        Remove ``rtdb[key]`` from *rtdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in rtdb

        Return ``True`` if *rtdb* has a key *key*, else ``False``.


    .. describe:: key not in rtdb

        Equivalent to ``not key in rtdb``.


    .. describe:: iter(rtdb)

        Return an iterator over the keys of the database.


    .. method:: tune(timeout, opts)

        Tune a database.

        :param timeout: timeout in seconds. If specified as 0 or as a negative
            value, no timeout is applied.
        :param opts: options, see :ref:`rdb_tune_options`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: open([host='localhost'[, port=1978]])

        Open a database.

        :param host: name/address of the server (defaults to ``'localhost'``).
        :param port: port number (defaults to ``1978``).


    .. method:: close

        Close the database.

        .. note::
            RTDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database file.

        :param path: path to the destination file. This path refers to the
            server, not the client, the database is **not** copied locally.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``rtdb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del rtdb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``rtdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``rtdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Merge a value with an existing one, **does not override existing items**
        in current value. If there is no corresponding record, a new record is
        stored.


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


    .. method:: itervalueskeys

        Return an iterator over the database's values' keys.

        .. versionadded:: 0.6.1


    .. method:: itervaluesvals

        Return an iterator over the database's values' values.

        .. versionadded:: 0.6.1


    .. method:: searchkeys(prefix[, max])

        Return a frozenset of keys starting with *prefix*. If given, *max* is
        the maximum number of keys to fetch, if omitted or specified as a
        negative value no limit is applied.


    .. method:: restore(path, timestamp, opts)

        Restore a database from an update log.

        :param path: path to the update log directory (relative to server).
        :param timestamp: restore from *timestamp* (in microseconds).
        :param opts: options, see :ref:`rdb_restore_setmaster_options`.


    .. method:: setmaster(host, port, timestamp, opts)

        Set the replication master of a database.

        :param host: name/address of the master server.
        :param port: port number of the master server.
        :param timestamp: start replication from *timestamp* (in microseconds).
        :param opts: options, see :ref:`rdb_restore_setmaster_options`.


    .. method:: setindex(column, type)

        Add an index to a column.

        :param column: name of the column. An empty string means *key*.
        :type column: :class:`str` (Python2)/:class:`bytes` (Python3)
        :param type: index type, see :ref:`tdb_setindex_types`.


    .. method:: uid

        Return a new unique id.


    .. method:: query

        Return a query object (:class:`RTDBQuery`). See `Querying a Remote Table
        Database --- RTDBQuery`_.


    .. staticmethod:: metasearch(queries, type)

        Combine queries and return the result set as a tuple of keys.

        :param queries: a sequence of :class:`RTDBQuery`.
        :param type: type of combination, see :ref:`tdb_metasearch_types`.


    .. method:: optimize(**kwargs)

        Optimize a database. This method only accepts keyword arguments. Each
        argument must be a :class:`str` (Python2) or :class:`bytes` (Python3)
        representation of its real value. See :meth:`tokyo.cabinet.TDB.optimize`
        for valid arguments and values. Example::

            rtdb.optimize(bnum='0', apow='-1', fpow='-1', opts='255')

        .. note::
            Optimizing a read only database is an invalid operation.


    .. attribute:: size

        The size in bytes of the database file.


    .. attribute:: status

        A :class:`dict` of status informations about the database.


Querying a Remote Table Database --- :class:`RTDBQuery`
=======================================================


.. class:: RTDBQuery

    When first returned by :meth:`RTDB.query` a query result set potentially
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

            Unfortunately, due to a Tokyo Tyrant limitation, neither *column*
            nor *expr* can contain null bytes.


    .. method:: sort(column, type)

        Sort the result set.

        :param column: name of the column. An empty string means *key*.
        :type column: :class:`str` (Python2)/:class:`bytes` (Python3)
        :param type: sort type (and direction), see :ref:`tdbquery_sort_types`.

        .. note::
            Unfortunately, due to a Tokyo Tyrant limitation, *column* cannot
            contain null bytes.


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


    .. method:: count

        Return the length of the result set.


    .. attribute:: hint

        TODO.
