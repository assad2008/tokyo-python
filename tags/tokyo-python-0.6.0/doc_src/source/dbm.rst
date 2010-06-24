.. _tokyo.dbm:


*****************************************************************************
:mod:`tokyo.dbm` --- :mod:`dbm` compatible interface to :mod:`tokyo.cabinet`.
*****************************************************************************

    .. versionadded:: 0.5.0

.. module:: tokyo.dbm
    :platform: POSIX
    :synopsis: dbm-compatible interface to tokyo.cabinet.


.. exception:: error

    Raised when an error specific to tokyo.dbm happens.


.. function:: whichdb(filename)

    This function attempts to guess which of the 4 database types available —
    :class:`TCHDB`, :class:`TCBDB`, :class:`TCFDB` or :class:`TCTDB` — should be
    used to open a given file. Returns the type, raises :exc:`error` if
    *filename* is not a Tokyo Cabinet file or if its type is unknown.


.. function:: open(filename[, flag="r"[, mode=0o666]])

    Open the database file *filename* and return a corresponding object. If the
    database file already exists, :func:`whichdb` is used to determine its type;
    if it does not exist, :func:`open` will try to deduce the database type from
    the *filename* extension; if this doesn't work, :class:`TCHDB` will be used.

    The optional *flag* argument can be:

    +---------+-------------------------------------------+
    | Value   | Meaning                                   |
    +=========+===========================================+
    | ``'r'`` | Open existing database for reading only   |
    |         | (default)                                 |
    +---------+-------------------------------------------+
    | ``'w'`` | Open existing database for reading and    |
    |         | writing                                   |
    +---------+-------------------------------------------+
    | ``'c'`` | Open database for reading and writing,    |
    |         | creating it if it doesn't exist           |
    +---------+-------------------------------------------+
    | ``'n'`` | Always create a new, empty database, open |
    |         | for reading and writing                   |
    +---------+-------------------------------------------+

    The optional *mode* argument is the Unix mode of the file. It defaults to
    octal ``0o666`` (and will be modified by the prevailing umask).


The :mod:`tokyo.dbm` database types are wrappers for their corresponding
:mod:`tokyo.cabinet` type. In addition to their counterparts methods and
attributes, they  implement :class:`collections.MutableMapping` and should
behave, for all intents and purposes, like :class:`dict`.


.. class:: TCHDB

    Wrapper for :class:`tokyo.cabinet.HDB`.


.. class:: TCBDB

    Wrapper for :class:`tokyo.cabinet.BDB`.


    .. method:: get(key[, default=None, [, duplicate=False]])

        Return the value for *key* if *key* is in the database, else *default*.
        If *default* is not given, it defaults to ``None``, so that this method
        never raises a :exc:`KeyError`. If *key* has duplicates and *duplicate*
        is :const:`False` (default), return the value of the **first record**.
        If *duplicate* is :const:`True` this method returns a tuple of all the
        values corresponding to *key*.


.. class:: TCFDB

    Wrapper for :class:`tokyo.cabinet.FDB`.


.. class:: TCTDB

    Wrapper for :class:`tokyo.cabinet.TDB`.
