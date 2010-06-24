.. _RDB:


.. currentmodule:: tokyo.tyrant


================================
Remote Database --- :class:`RDB`
================================


:class:`RDB`
============

.. class:: RDB

    Example::

        # First you need to start a ttserver, example commands (starting a
        # server on default address localhost:1978):
        # - on-memory hash database: ttserver
        # - on-memory tree database: ttserver +
        # - hash database: ttserver test.tch
        # - B+ tree database: ttserver test.tcb
        # see http://1978th.net/tokyotyrant/spex.html#serverprog and
        # http://1978th.net/tokyotyrant/spex.html#tutorial for more info
        # about ttserver

        from tokyo.tyrant import *

        rdb = RDB()

        # if need be, you should call tune before open, ex. with default values:
        rdb.tune(0, 0)

        # open the database
        rdb.open() # this will open the server at localhost:1978

        # store records
        for key, value in [("foo", "hop"), ("bar", "step"), ("baz", "jump")]:
            rdb[key] = value

        # retrieve one record
        print(rdb["foo"])

        # traverse records
        for key in rdb:
            print(key, rdb[key])

        # close the database
        rdb.close()

    .. warning::
        This client works only with Tokyo Tyrant daemon serving one of the
        following:

        * hash databases.
        * on-memory hash databases.
        * B+ tree databases.
        * on-memory tree databases.

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must be either :class:`str` (Python2) or
        :class:`bytes` (Python3).

    .. seealso::
        `Remote Database API
        <http://1978th.net/tokyotyrant/spex.html#tcrdbapi>`_


    .. describe:: len(rdb)

        Return the number of records in the database *rdb*.


    .. describe:: rdb[key]

        Return the value of *rdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: rdb[key] = value

        Set ``rdb[key]`` to *value*.


    .. describe:: del rdb[key]

        Remove ``rdb[key]`` from *rdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in rdb

        Return ``True`` if *rdb* has a key *key*, else ``False``.


    .. describe:: key not in rdb

        Equivalent to ``not key in rdb``.


    .. describe:: iter(rdb)

        Return an iterator over the keys of the database.


    .. method:: tune(timeout, opts)

        Tune a database.

        :param timeout: timeout in seconds. If specified as 0 or as a negative
            value, no timeout is applied.
        :param opts: options, see `RDB.tune() options`_.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: open([host='localhost'[, port=1978]])

        Open a database.

        :param host: name/address of the server (defaults to ``'localhost'``).
        :param port: port number (defaults to ``1978``).


    .. method:: close

        Close the database.

        .. note::
            RDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database file.

        :param path: path to the destination file. This path refers to the
            server, not the client, the database is **not** copied locally.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``rdb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del rdb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``rdb[key] = value``.


    .. method:: putkeep(key, value)

        Store a record in the database, unlike the standard forms
        (``rdb[key] = value`` or :meth:`put`), this method raises
        :exc:`KeyError` if *key* is already in the database.


    .. method:: putcat(key, value)

        Concatenate a value at the end of an existing one. If there is no
        corresponding record, a new record is stored.


    .. method:: putnb(key, value)

        Non-blocking :meth:`put`.


    .. method:: addint(key, num)

        Store an :class:`int` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as an
        :class:`int` this method raises :exc:`KeyError`.

        .. note::

            * The returned value will wrap around :const:`INT_MAX` and
              :const:`INT_MIN`. Example::

                  >>> rdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> rdb.addint('id', 1) # adding 1 to 'id' returns INT_MIN
                  -2147483648
                  >>>

            * Trying to access a value set with :meth:`addint` using :meth:`get`
              or ``rdb[key]`` will **not** return an :class:`int`. It will
              instead return the internal binary representation of the value.
              Example::

                  >>> rdb.addint('id', INT_MAX) # setting 'id' to INT_MAX
                  2147483647
                  >>> rdb['id']
                  '\xff\xff\xff\x7f'
                  >>>


    .. method:: adddouble(key, num)

        Store a :class:`float` in the database. If *key* is not in the database,
        this method stores *num* in the database and returns it. If *key* is
        already in the database, then it will add *num* to its current value and
        return the result. If *key* exists but its value cannot be treated as a
        :class:`float` this method raises :exc:`KeyError`.

        .. note::

            Trying to access a value set with :meth:`adddouble` using
            :meth:`get` or ``rdb[key]`` will **not** return a :class:`float`.


    .. method:: sync

        Flush modifications to the database file.


    .. method:: searchkeys(prefix[, max])

        Return a frozenset of keys starting with *prefix*. If given, *max* is
        the maximum number of keys to fetch, if omitted or specified as a
        negative value no limit is applied.


    .. method:: restore(path, timestamp, opts)

        Restore a database from an update log.

        :param path: path to the update log directory (relative to server).
        :param timestamp: restore from *timestamp* (in microseconds).
        :param opts: options, see `RDB.restore()/RDB.setmaster() options`_.


    .. method:: setmaster(host, port, timestamp, opts)

        Set the replication master of a database.

        :param host: name/address of the master server.
        :param port: port number of the master server.
        :param timestamp: start replication from *timestamp* (in microseconds).
        :param opts: options, see `RDB.restore()/RDB.setmaster() options`_.


    .. method:: optimize(**kwargs)

        Optimize a database. This method only accepts keyword arguments. Each
        argument must be a :class:`str` (Python2) or :class:`bytes` (Python3)
        representation of its real value. See :meth:`tokyo.cabinet.HDB.optimize`
        and :meth:`tokyo.cabinet.BDB.optimize`, respectively, for valid
        arguments and values. Examples::

            # optimizing a hash database:
            rdb.optimize(bnum='0', apow='-1', fpow='-1', opts='255')

            # optimizing a B+ tree database:
            rdb.optimize(lmemb='0', nmemb='0', bnum='0', apow='-1', fpow='-1', opts='255')

        .. note::
            Optimizing a read only database is an invalid operation.


    Methods :meth:`keys`, :meth:`values` and :meth:`items` are not yet
    implemented (mainly because I didn't settle on how to do it: should they
    return :class:`Iterable`, :class:`Iterator`, :class:`MappingView`, etc.?).
    Any help would be greatly appreciated in this matter.

    For the time being, for those of you who really need these methods, it's
    trivial to implement them in python. Here is an example using generators::

        from tokyo.tyrant import RDB as _RDB

        class RDB(_RDB):

            def keys(self):
                return (key for key in self)

            def values(self):
                return (self[key] for key in self)

            def items(self):
                return ((key, self[key]) for key in self)


    .. attribute:: size

        The size in bytes of the database file.


    .. attribute:: status

        A :class:`dict` of status informations about the database.


.. _rdb_tune_options:

:meth:`RDB.tune` options
========================

.. data:: RDBTRECON

    Try to reconnect automatically if the connection is lost.


.. _rdb_restore_setmaster_options:

:meth:`RDB.restore`/:meth:`RDB.setmaster` options
=================================================

.. data:: RDBROCHKCON

    Consistency checking.
