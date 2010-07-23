.. _tokyo.dystopia:


**********************************************************
:mod:`tokyo.dystopia` --- Python Tokyo Dystopia interface.
**********************************************************

    .. versionadded:: 0.7.0

.. module:: tokyo.dystopia
    :platform: POSIX
    :synopsis: Python Tokyo Dystopia interface.


Tokyo Dystopia is a full-text search system. You can search lots of records for
some records including specified patterns. The characteristic of Tokyo Dystopia
is the following.

- High performance of search
- High scalability of target documents
- Perfect recall ratio by character N-gram method
- Phrase matching, prefix matching, suffix matching, and token matching
- Multilingualism with Unicode
- Layered Architecture of APIs

.. todo::
    Rewrite the previous section (copy/pasted from http://1978th.net/tokyodystopia/).

.. seealso::
    `Fundamental Specifications of Tokyo Dystopia Version 1
    <http://1978th.net/tokyodystopia/spex.html>`_ for more information about
    Tokyo Dystopia.


.. exception:: Error

    Raised when an error specific to Tokyo Dystopia happens.


.. function:: version

    Returns the version string of the underlying Tokyo Dystopia library.


.. toctree::
    :maxdepth: 1

    IDB
