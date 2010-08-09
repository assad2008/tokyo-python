.. _tokyo.tyrant:


******************************************************
:mod:`tokyo.tyrant` --- Python Tokyo Tyrant interface.
******************************************************

    .. versionadded:: 0.6.0

.. module:: tokyo.tyrant
    :platform: POSIX
    :synopsis: Python Tokyo Tyrant interface.

Tokyo Tyrant is a package of network interface to the DBM called Tokyo Cabinet.
Though the DBM has high performance, you might bother in case that multiple
processes share the same database, or remote processes access the database.
Thus, Tokyo Tyrant is provided for concurrent and remote connections to Tokyo
Cabinet.

.. todo::
    Rewrite the previous section (copy/pasted from http://fallabs.com/tokyotyrant/).

.. seealso::
    `Fundamental Specifications of Tokyo Tyrant Version 1
    <http://fallabs.com/tokyotyrant/spex.html>`_ for more information about
    Tokyo Tyrant.


.. exception:: Error

    Raised when an error specific to Tokyo Tyrant happens.


.. function:: version

    Returns the version string of the underlying Tokyo Tyrant library.


.. toctree::
    :maxdepth: 1

    RDB
    RTDB
