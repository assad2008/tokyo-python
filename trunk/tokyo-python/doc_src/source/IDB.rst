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

        # close the database
        idb.close()

    .. note::
        For all methods taking either a *key* argument or a pair *(key, value)*,
        *key* and *value* must :class:`str`.

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


    .. method:: open(path, mode)

        Open a database.

        :param path: path to the database file.
        :param mode: mode, see :ref:`idb_open_modes`.


    .. method:: close

        Close the database.

        .. note::
            IDBs are closed when garbage-collected.


    .. method:: clear

        Remove all records from the database.


    .. method:: copy(path)

        Copy the database file.

        :param path: path to the destination file.


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


    .. attribute:: path

        The path to the database file.


    .. attribute:: size

        The size in bytes of the database file.


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


.. _idb_tune_optimize_options:

:meth:`IDB.tune`/:meth:`IDB.optimize` options
=============================================

.. data:: IDBTLARGE

    The size of the database can be larger than 2GB.

.. data:: IDBTDEFLATE

    Each record is compressed with Deflate encoding.

.. data:: IDBTBZIP

    Each record is compressed with BZIP2 encoding.

.. data:: IDBTTCBS

    Each record is compressed with TCBS encoding.


.. _idb_search_modes:

:meth:`IDB.search` modes
========================

.. data:: IDBSSUBSTR

    TODO.

.. data:: IDBSPREFIX

    TODO.

.. data:: IDBSSUFFIX

    TODO.

.. data:: IDBSFULL

    TODO.

.. data:: IDBSTOKEN

    TODO.

.. data:: IDBSTOKPRE

    TODO.

.. data:: IDBSTOKSUF

    TODO.
