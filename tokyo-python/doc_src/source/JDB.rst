.. _JDB:


.. currentmodule:: tokyo.dystopia


================================
Tagged Database --- :class:`JDB`
================================


:class:`JDB`
============

.. class:: JDB

    Example::

        from tokyo.dystopia import *

        jdb = JDB()

        # if need be, you should call tune/setcache before open,
        # ex. with default values:
        jdb.tune(0, 0, 0, 0)
        jdb.setcache(0, 0)

        # open the database
        jdb.open("casket.tdj", JDBOWRITER | JDBOCREAT)

        # store records
        for key, value in [(1, ("hop", "step", "jump")),
                           (2, ("quick", "brown", "fox")),
                           (3, ("lazy", "dog"))]:
            jdb[key] = value

        # retrieve one record
        print(jdb[1])

        # traverse records
        for key in jdb:
            print(key, jdb[key])

        # close the database
        jdb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*:

        * Python2: *key* must be :class:`long`/:class:`int` and *value* must be
          a sequence of :class:`str` or :class:`unicode`.
        * Python3: *key* must be :class:`int` and *value* must be a sequence of
          :class:`bytes` or :class:`str`.

        Values will always be returned as a tuple of UTF-8 encoded unicode objects.

        On top of that *key* **must always be > 0**.

    .. seealso::
        `Tokyo Dystopia Simple API
        <http://fallabs.com/tokyodystopia/spex.html#laputaapi>`_


    .. describe:: len(jdb)

        Return the number of records in the database *jdb*.


    .. describe:: jdb[key]

        Return the value of *jdb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: jdb[key] = value

        Set ``jdb[key]`` to *value*.


    .. describe:: del jdb[key]

        Remove ``jdb[key]`` from *jdb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in jdb

        Return ``True`` if *jdb* has a key *key*, else ``False``.


    .. describe:: key not in jdb

        Equivalent to ``not key in jdb``.


    .. describe:: iter(jdb)

        Return an iterator over the keys of the database.


    .. method:: tune(ernum, etnum, iusiz, opts)

        Tune a database.

        :param ernum: the expected number of records to be stored. If specified
            as 0 or as a negative value, the default value (1000000) is used.
        :param etnum: the expected number of tokens to be stored. If specified
            as 0 or as a negative value, the default value (1000000) is used.
        :param iusiz: the unit size of each index file(?). If specified as 0 or
            as a negative value, the default value (536870912) is used.
        :param opts: options, see :ref:`jdb_tune_options`.

        .. note::
            Tuning an open database is an invalid operation.


    .. method:: setcache(icsiz, lcnum)

        Set the cache size.

        :param icsiz: the size of the token cache. If specified as 0 or as a
            negative value, the default value (134217728) is used.
        :param lcnum: the maximum number of cached leaf nodes. If specified as 0
            or as a negative value, the default value (64 for writer or 1024 for
            reader) is used.

        .. note::
            Setting the cache size on an open database is an invalid operation.


    .. method:: setfwmmax(fwmmax)

        Set the maximum number of forward matching expansion(?).

        :param fwmmax: the maximum number of forward matching expansion.

        .. note::
            Setting this on an open database is an invalid operation.


    .. method:: open(path, mode)

        Open a database.

        :param path: path to the database directory.
        :param mode: mode, see :ref:`jdb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            JDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database directory.

        :param path: path to the destination directory.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``jdb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del jdb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``jdb[key] = value``.


    .. method:: sync

        Flush modifications to the database file.


    .. method:: iterkeys

        Return an iterator over the database's keys.


    .. method:: itervalues

        Return an iterator over the database's values.


    .. method:: iteritems

        Return an iterator over the database's items (``(key, value)`` pairs).


    .. method:: search(expr[, mode])

        Search a database, return a frozenset of keys whose value match the
        expressed condition.

        :param expr: case insensitive expression.
        :param mode: mode, see :ref:`jdb_search_modes`.

        Conditions can be expressed in two ways:

        * When *mode* is given, it must be one of :ref:`jdb_search_modes`.
        * If *mode* is omitted or specified as a negative value, *expr* can then
          be used to express more complex conditions using
          :ref:`jdb_search_compound_expression`.

        .. seealso::
            'Compound Expression of Search' at `Tokyo Dystopia documentation
            <http://fallabs.com/tokyodystopia/spex.html#laputaapi>`_.


    .. method:: optimize

        Optimize a database.

        .. note::
            Optimizing a read only database is an invalid operation.


    .. attribute:: path

        The path to the database directory.


    .. attribute:: size

        The size in bytes of the database.


.. _jdb_open_modes:

:meth:`JDB.open` modes
======================

.. data:: JDBOREADER

    Open a database in read-only mode.

.. data:: JDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`JDBOWRITER` :

* .. data:: JDBOCREAT

      Create a new database file if it does not exists.

* .. data:: JDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).


The following constants can be combined with either :const:`JDBOREADER` or
:const:`JDBOWRITER` :

* .. data:: JDBONOLCK

      Opens the database file without file locking.

* .. data:: JDBOLCKNB

      Locking is performed without blocking.


.. _jdb_tune_options:

:meth:`JDB.tune` options
========================

.. data:: JDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: JDBTDEFLATE

    Each page is compressed with Deflate encoding.

.. data:: JDBTBZIP

    Each page is compressed with BZIP2 encoding.

.. data:: JDBTTCBS

    Each page is compressed with TCBS encoding.


.. _jdb_search_modes:

:meth:`JDB.search` modes
========================

.. data:: JDBSSUBSTR

    ::

        expr in v for v in value

.. data:: JDBSPREFIX

    ::

        v.startswith(expr) for v in value

.. data:: JDBSSUFFIX

    ::

        v.endswith(expr) for v in value

.. data:: JDBSFULL

    ::

        expr in value


.. _jdb_search_compound_expression:

:meth:`JDB.search` mini language
================================

=========================  =====================================================
 *expr*                     meaning
=========================  =====================================================
 ``'expr'``                 ::

                                expr in value
 ``'expr1 expr2'``          ::

                                expr1 in value and expr2 in value
 ``'"expr1 expr2"'``        ::

                                "expr1 expr2" in value
 ``'[[*expr*]]'``           ::

                                expr in v for v in value
 ``'[[expr*]]'``            ::

                                v.startswith(expr) for v in value
 ``'[[*expr]]'``            ::

                                v.endswith(expr) for v in value
=========================  =====================================================

The expressions above can be combined with ``||`` and/or ``&&`` (``||`` has a
higher order of precedence).
