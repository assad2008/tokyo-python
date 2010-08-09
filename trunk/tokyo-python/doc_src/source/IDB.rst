.. _IDB:


.. currentmodule:: tokyo.dystopia


=================================
Indexed Database --- :class:`IDB`
=================================


:class:`IDB`
============

.. class:: IDB

    Example::

        from tokyo.dystopia import *

        idb = IDB()

        # if need be, you should call tune/setcache before open,
        # ex. with default values:
        idb.tune(0, 0, 0, 0)
        idb.setcache(0, 0)

        # open the database
        idb.open("casket.tdi", IDBOWRITER | IDBOCREAT)

        # store records
        for key, value in [(1, "hop"), (2, "step"), (3, "jump")]:
            idb[key] = value

        # retrieve one record
        print(idb[1])

        # traverse records
        for key in idb:
            print(key, idb[key])

        # close the database
        idb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*:

        * Python2: *key* must be :class:`long`/:class:`int` and *value* must be
          :class:`str` or :class:`unicode`.
        * Python3: *key* must be :class:`int` and *value* must be :class:`bytes`
          or :class:`str`.

        Values will always be returned as UTF-8 encoded unicode objects.

        On top of that *key* **must always be > 0**.

    .. seealso::
        `Tokyo Dystopia Core API
        <http://1978th.net/tokyodystopia/spex.html#dystopiaapi>`_


    .. describe:: len(idb)

        Return the number of records in the database *idb*.


    .. describe:: idb[key]

        Return the value of *idb*'s record corresponding to *key*. Raises
        :exc:`KeyError` if *key* is not in the database.


    .. describe:: idb[key] = value

        Set ``idb[key]`` to *value*.


    .. describe:: del idb[key]

        Remove ``idb[key]`` from *idb*.  Raises :exc:`KeyError` if *key* is not
        in the database.


    .. describe:: key in idb

        Return ``True`` if *idb* has a key *key*, else ``False``.


    .. describe:: key not in idb

        Equivalent to ``not key in idb``.


    .. describe:: iter(idb)

        Return an iterator over the keys of the database.


    .. method:: tune(ernum, etnum, iusiz, opts)

        Tune a database.

        :param ernum: the expected number of records to be stored. If specified
            as 0 or as a negative value, the default value (1000000) is used.
        :param etnum: the expected number of tokens to be stored. If specified
            as 0 or as a negative value, the default value (1000000) is used.
        :param iusiz: the unit size of each index file(?). If specified as 0 or
            as a negative value, the default value (536870912) is used.
        :param opts: options, see :ref:`idb_tune_options`.

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
        :param mode: mode, see :ref:`idb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            IDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database directory.

        :param path: path to the destination directory.


    .. method:: get(key)

        Return the value corresponding to *key*. Equivalent to ``idb[key]``.


    .. method:: remove(key)

        Delete a record from the database. Equivalent to ``del idb[key]``.


    .. method:: put(key, value)

        Store a record in the database. Equivalent to ``idb[key] = value``.


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
        :param mode: mode, see :ref:`idb_search_modes`.

        Conditions can be expressed in two ways:

        * When *mode* is given, it must be one of :ref:`idb_search_modes`.
        * If *mode* is omitted or specified as a negative value, *expr* can then
          be used to express more complex conditions using
          :ref:`idb_search_compound_expression`.

        .. seealso::
            'Compound Expression of Search' at `Tokyo Dystopia documentation
            <http://1978th.net/tokyodystopia/spex.html#dystopiaapi>`_.


    .. method:: optimize

        Optimize a database.

        .. note::
            Optimizing a read only database is an invalid operation.


    .. attribute:: path

        The path to the database directory.


    .. attribute:: size

        The size in bytes of the database.


.. _idb_open_modes:

:meth:`IDB.open` modes
======================

.. data:: IDBOREADER

    Open a database in read-only mode.

.. data:: IDBOWRITER

    Open a database in read-write mode.


The following constants can only be combined with :const:`IDBOWRITER` :

* .. data:: IDBOCREAT

      Create a new database file if it does not exists.

* .. data:: IDBOTRUNC

      Create a new database file even if one already exists (truncates existing
      file).


The following constants can be combined with either :const:`IDBOREADER` or
:const:`IDBOWRITER` :

* .. data:: IDBONOLCK

      Opens the database file without file locking.

* .. data:: IDBOLCKNB

      Locking is performed without blocking.


.. _idb_tune_options:

:meth:`IDB.tune` options
========================

.. data:: IDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: IDBTDEFLATE

    Each page is compressed with Deflate encoding.

.. data:: IDBTBZIP

    Each page is compressed with BZIP2 encoding.

.. data:: IDBTTCBS

    Each page is compressed with TCBS encoding.


.. _idb_search_modes:

:meth:`IDB.search` modes
========================

.. data:: IDBSSUBSTR

    ::

        expr in value #substring test

.. data:: IDBSPREFIX

    ::

        value.startswith(expr)

.. data:: IDBSSUFFIX

    ::

        value.endswith(expr)

.. data:: IDBSFULL

    ::

        value == expr

.. data:: IDBSTOKEN

    ::

        expr in value.split()

.. data:: IDBSTOKPRE

    ::

        v.startswith(expr) for v in value.split()

.. data:: IDBSTOKSUF

    ::

        v.endswith(expr) for v in value.split()


.. _idb_search_compound_expression:

:meth:`IDB.search` mini language
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
 ``'[[[[expr'``             ::

                                value.startswith(expr)
 ``'expr]]]]'``             ::

                                value.endswith(expr)
 ``'[[expr]]'``             ::

                                expr in value.split()
 ``'[[expr*]]'``            ::

                                v.startswith(expr) for v in value.split()
 ``'[[*expr]]'``            ::

                                v.endswith(expr) for v in value.split()
=========================  =====================================================

The expressions above can be combined with ``||`` and/or ``&&`` (``||`` has a
higher order of precedence).

Example::

    >>> from tokyo.dystopia import *
    >>> presidents = [(1, 'Washington, George'), (2, 'Adams, John'),
    ...               (3, 'Jefferson, Thomas'), (4, 'Madison, James'),
    ...               (5, 'Monroe, James'), (6, 'Adams, John Quincy'),
    ...               (7, 'Jackson, Andrew'), (8, 'Van Buren, Martin'),
    ...               (9, 'Harrison, William Henry'), (10, 'Tyler, John'),
    ...               (11, 'Polk, James Knox'), (12, 'Taylor, Zachary'),
    ...               (13, 'Fillmore, Millard'), (14, 'Pierce, Franklin'),
    ...               (15, 'Buchanan, James'), (16, 'Lincoln, Abraham'),
    ...               (17, 'Johnson, Andrew'), (18, 'Grant, Ulysses S.'),
    ...               (19, 'Hayes, Rutherford Birchard'), (20, 'Garfield, James Abram'),
    ...               (21, 'Arthur, Chester Alan'), (22, 'Cleveland, Grover'),
    ...               (23, 'Harrison, Benjamin'), (24, 'Cleveland, Grover'),
    ...               (25, 'McKinley, William'), (26, 'Roosevelt, Theodore'),
    ...               (27, 'Taft, William Howard'), (28, 'Wilson, Woodrow'),
    ...               (29, 'Harding, Warren Gamaliel'), (30, 'Coolidge, Calvin'),
    ...               (31, 'Hoover, Herbert Clark'), (32, 'Roosevelt, Franklin Delano'),
    ...               (33, 'Truman, Harry'), (34, 'Eisenhower, Dwight David'),
    ...               (35, 'Kennedy, John Fitzgerald'), (36, 'Johnson, Lyndon Baines'),
    ...               (37, 'Nixon, Richard Milhous'), (38, 'Ford, Gerald Rudolph'),
    ...               (39, 'Carter, James Earl Jr.'), (40, 'Reagan, Ronald Wilson'),
    ...               (41, 'Bush, George Herbert Walker'), (42, 'Clinton, William Jefferson'),
    ...               (43, 'Bush, George Walker'), (44, 'Obama, Barack Hussein')]
    >>> idb = IDB()
    >>> idb.open("presidents.tdi", IDBOWRITER | IDBOCREAT)
    >>> for k, v in presidents:
    ...     idb[k] = v
    ...
    >>> idb.search('earl james')
    frozenset([39L])
    >>> idb.search('"earl james"')
    frozenset([])
    >>> idb.search('"james earl"')
    frozenset([39L])
    >>> idb.search('earl || james')
    frozenset([4L, 5L, 39L, 11L, 15L, 20L])
    >>> idb.search('in]]]]')
    frozenset([8L, 44L, 30L, 14L, 23L])
    >>> idb.search('in]]]] && am')
    frozenset([44L, 23L])
    >>> idb.search('in]]]] && [[am]]')
    frozenset([])
    >>> idb.search('in]]]] && [[am*]]')
    frozenset([])
    >>> idb.search('in]]]] && [[*am]]')
    frozenset([])
    >>> idb.search('in]]]] && [[ob*]]')
    frozenset([44L])
    >>> idb.close()
    >>>
