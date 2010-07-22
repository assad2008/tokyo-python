.. _tokyo.cabinet:


********************************************************
:mod:`tokyo.cabinet` --- Python Tokyo Cabinet interface.
********************************************************

.. module:: tokyo.cabinet
    :platform: POSIX
    :synopsis: Python Tokyo Cabinet interface.

Tokyo Cabinet is a library of routines for managing a database. The database is
a simple data file containing records, each is a pair of a key and a value.
Every key and value is serial bytes with variable length. Both binary data and
character string can be used as a key and a value. There is neither concept of
data tables nor data types. Records are organized in hash table, B+ tree, or
fixed-length array.
Tokyo Cabinet is developed as the successor of GDBM and QDBM on the following
purposes. They are achieved and Tokyo Cabinet replaces conventional DBM products.

- improves space efficiency: smaller size of database file.
- improves time efficiency: faster processing speed.
- improves parallelism: higher performance in multi-thread environment.
- improves usability: simplified API.
- improves robustness: database file is not corrupted even under catastrophic
  situation.
- supports 64-bit architecture: enormous memory space and database file are
  available.

.. todo::
    Rewrite the previous section (copy/pasted from http://1978th.net/tokyocabinet/).

.. seealso::
    `Fundamental Specifications of Tokyo Cabinet Version 1
    <http://1978th.net/tokyocabinet/spex-en.html>`_ for more information about
    Tokyo Cabinet.


.. exception:: Error

    Raised when an error specific to Tokyo Cabinet happens.


.. function:: version

    Returns the version string of the underlying Tokyo Cabinet library.


.. data:: INT_MAX

.. data:: INT_MIN

    The largest positive and negative integers as defined by the platform
    (mainly used for testing :meth:`addint`).

    .. versionadded:: 0.5.0


.. toctree::
    :maxdepth: 1

    HDB
    MDB
    BDB
    NDB
    FDB
    TDB
