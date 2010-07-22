/*******************************************************************************
*
* Copyright (c) 2010, Malek Hadj-Ali
* All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*******************************************************************************/


#include "tokyo.h"

#include <tchdb.h>
#include <tcbdb.h>
#include <tcfdb.h>
#include <tctdb.h>


/*******************************************************************************
* objects
*******************************************************************************/

/* HDB */
typedef struct {
    PyObject_HEAD
    TCHDB *hdb;
    bool changed;
} HDB;


/* MDB */
typedef struct {
    PyObject_HEAD
    TCMDB *mdb;
    bool changed;
} MDB;


/* BDB.setcmpfunc() callback consts */
enum {
    BDBCMPLEXICAL,
    BDBCMPDECIMAL,
    BDBCMPINT32,
    BDBCMPINT64
};

/* BDB */
typedef struct {
    PyObject_HEAD
    TCBDB *bdb;
    BDBCUR *cur; /* for iteration over self */
    bool changed;
} BDB;

/* BDBCursor */
typedef struct {
    PyObject_HEAD
    BDBCUR *cur;
    BDB *bdb;
} BDBCursor;


/* NDB */
typedef struct {
    PyObject_HEAD
    TCNDB *ndb;
    bool changed;
} NDB;


/* FDB */
typedef struct {
    PyObject_HEAD
    TCFDB *fdb;
    bool changed;
} FDB;


/* TDB */
typedef struct {
    PyObject_HEAD
    TCTDB *tdb;
    bool changed;
} TDB;

/* TDBQuery */
typedef struct {
    PyObject_HEAD
    TDBQRY *qry;
    TDB *tdb;
} TDBQuery;


/*******************************************************************************
* utilities
*******************************************************************************/

/* convert an int to a char */
char
int_to_char(int value)
{
    if (value < CHAR_MIN) {
        set_error(PyExc_OverflowError, "byte integer is less than minimum");
        return -1;
    }
    if (value > CHAR_MAX) {
        set_error(PyExc_OverflowError, "byte integer is greater than maximum");
        return -1;
    }
    return (char)value;
}


/*******************************************************************************
* types
*******************************************************************************/

#include "HDB.c"
#include "MDB.c"
#include "BDB.c"
#include "NDB.c"
#include "FDB.c"
#include "TDB.c"


/*******************************************************************************
* cabinet_module
*******************************************************************************/

/* cabinet_module.m_doc */
PyDoc_STRVAR(cabinet_m_doc,
"Python Tokyo Cabinet interface.\n\
\n\
TODO\n\
\n\
See also:\n\
Tokyo Cabinet documentation at:\n\
http://1978th.net/tokyocabinet/");


/* cabinet.version() -> str */
PyDoc_STRVAR(cabinet_version_doc,
"version() -> str\n\
\n\
Returns the version string of the underlying Tokyo Cabinet library.");

static PyObject *
cabinet_version(PyObject *module)
{
    return PyString_FromString(tcversion);
}


/* cabinet_module.m_methods */
static PyMethodDef cabinet_m_methods[] = {
    {"version", (PyCFunction)cabinet_version, METH_NOARGS, cabinet_version_doc},
    {NULL} /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3
/* cabinet_module */
static PyModuleDef cabinet_module = {
    PyModuleDef_HEAD_INIT,
    "tokyo.cabinet",                          /*m_name*/
    cabinet_m_doc,                            /*m_doc*/
    -1,                                       /*m_size*/
    cabinet_m_methods,                        /*m_methods*/
};
#endif


/* cabinet_module initialization */
PyObject *
init_cabinet(void)
{
    PyObject *cabinet;

    /* checking types */
    if (
        PyType_Ready(&HDBType) ||
        PyType_Ready(&HDBIterKeysType) ||
        PyType_Ready(&HDBIterValuesType) ||
        PyType_Ready(&HDBIterItemsType) ||
        PyType_Ready(&MDBType) ||
        PyType_Ready(&MDBIterKeysType) ||
        PyType_Ready(&MDBIterValuesType) ||
        PyType_Ready(&MDBIterItemsType) ||
        PyType_Ready(&BDBType) ||
        PyType_Ready(&BDBCursorType) ||
        PyType_Ready(&BDBIterKeysType) ||
        PyType_Ready(&BDBIterValuesType) ||
        PyType_Ready(&BDBIterItemsType) ||
        PyType_Ready(&NDBType) ||
        PyType_Ready(&NDBIterKeysType) ||
        PyType_Ready(&NDBIterValuesType) ||
        PyType_Ready(&NDBIterItemsType) ||
        PyType_Ready(&FDBType) ||
        PyType_Ready(&FDBIterKeysType) ||
        PyType_Ready(&FDBIterValuesType) ||
        PyType_Ready(&FDBIterItemsType) ||
        PyType_Ready(&TDBType) ||
        PyType_Ready(&TDBIterKeysType) ||
        PyType_Ready(&TDBIterValuesType) ||
        PyType_Ready(&TDBIterItemsType) ||
        PyType_Ready(&TDBIterValuesKeysType) ||
        PyType_Ready(&TDBIterValuesValsType) ||
        PyType_Ready(&TDBQueryType)
       ) {
        return NULL;
    }
    /* cabinet */
#if PY_MAJOR_VERSION >= 3
    cabinet = PyModule_Create(&cabinet_module);
#else
    cabinet = Py_InitModule3("tokyo.cabinet", cabinet_m_methods, cabinet_m_doc);
#endif
    if (!cabinet) {
        return NULL;
    }
    /* cabinet.Error object */
    Error = PyErr_NewException("tokyo.cabinet.Error", NULL, NULL);
    if (!Error) {
        Py_DECREF(cabinet);
        return NULL;
    }
    /* adding objects and constants */
    if (
        PyModule_AddObject(cabinet, "Error", Error) ||
        PyModule_AddObject(cabinet, "HDB", (PyObject *)&HDBType) ||
        PyModule_AddObject(cabinet, "MDB", (PyObject *)&MDBType) ||
        PyModule_AddObject(cabinet, "BDB", (PyObject *)&BDBType) ||
        PyModule_AddObject(cabinet, "NDB", (PyObject *)&NDBType) ||
        PyModule_AddObject(cabinet, "FDB", (PyObject *)&FDBType) ||
        PyModule_AddObject(cabinet, "TDB", (PyObject *)&TDBType) ||
        /* useful for addint */
        PyModule_AddIntMacro(cabinet, INT_MAX) ||
        PyModule_AddIntMacro(cabinet, INT_MIN) ||
        /* HDB open mode */
        PyModule_AddIntMacro(cabinet, HDBOREADER) ||
        PyModule_AddIntMacro(cabinet, HDBOWRITER) ||
        PyModule_AddIntMacro(cabinet, HDBOCREAT) ||
        PyModule_AddIntMacro(cabinet, HDBOTRUNC) ||
        PyModule_AddIntMacro(cabinet, HDBONOLCK) ||
        PyModule_AddIntMacro(cabinet, HDBOLCKNB) ||
        PyModule_AddIntMacro(cabinet, HDBOTSYNC) ||
        /* HDB tune/optimize opts */
        PyModule_AddIntMacro(cabinet, HDBTLARGE) ||
        PyModule_AddIntMacro(cabinet, HDBTDEFLATE) ||
        PyModule_AddIntMacro(cabinet, HDBTBZIP) ||
        PyModule_AddIntMacro(cabinet, HDBTTCBS) ||
        /* BDB open mode */
        PyModule_AddIntMacro(cabinet, BDBOREADER) ||
        PyModule_AddIntMacro(cabinet, BDBOWRITER) ||
        PyModule_AddIntMacro(cabinet, BDBOCREAT) ||
        PyModule_AddIntMacro(cabinet, BDBOTRUNC) ||
        PyModule_AddIntMacro(cabinet, BDBONOLCK) ||
        PyModule_AddIntMacro(cabinet, BDBOLCKNB) ||
        PyModule_AddIntMacro(cabinet, BDBOTSYNC) ||
        /* BDB tune/optimize opts */
        PyModule_AddIntMacro(cabinet, BDBTLARGE) ||
        PyModule_AddIntMacro(cabinet, BDBTDEFLATE) ||
        PyModule_AddIntMacro(cabinet, BDBTBZIP) ||
        PyModule_AddIntMacro(cabinet, BDBTTCBS) ||
        /* BDB setcmpfunc callback consts */
        PyModule_AddIntMacro(cabinet, BDBCMPLEXICAL) ||
        PyModule_AddIntMacro(cabinet, BDBCMPDECIMAL) ||
        PyModule_AddIntMacro(cabinet, BDBCMPINT32) ||
        PyModule_AddIntMacro(cabinet, BDBCMPINT64) ||
        /* BDBCursor put mode */
        PyModule_AddIntMacro(cabinet, BDBCPCURRENT) ||
        PyModule_AddIntMacro(cabinet, BDBCPBEFORE) ||
        PyModule_AddIntMacro(cabinet, BDBCPAFTER) ||
        /* FDB open mode */
        PyModule_AddIntMacro(cabinet, FDBOREADER) ||
        PyModule_AddIntMacro(cabinet, FDBOWRITER) ||
        PyModule_AddIntMacro(cabinet, FDBOCREAT) ||
        PyModule_AddIntMacro(cabinet, FDBOTRUNC) ||
        PyModule_AddIntMacro(cabinet, FDBONOLCK) ||
        PyModule_AddIntMacro(cabinet, FDBOLCKNB) ||
        PyModule_AddIntMacro(cabinet, FDBOTSYNC) ||
        /* FDB put/putkeep/putcat special keys */
        PyModule_AddIntMacro(cabinet, FDBIDMIN) ||
        PyModule_AddIntMacro(cabinet, FDBIDPREV) ||
        PyModule_AddIntMacro(cabinet, FDBIDMAX) ||
        PyModule_AddIntMacro(cabinet, FDBIDNEXT) ||
        /* TDB open mode */
        PyModule_AddIntMacro(cabinet, TDBOREADER) ||
        PyModule_AddIntMacro(cabinet, TDBOWRITER) ||
        PyModule_AddIntMacro(cabinet, TDBOCREAT) ||
        PyModule_AddIntMacro(cabinet, TDBOTRUNC) ||
        PyModule_AddIntMacro(cabinet, TDBONOLCK) ||
        PyModule_AddIntMacro(cabinet, TDBOLCKNB) ||
        PyModule_AddIntMacro(cabinet, TDBOTSYNC) ||
        /* TDB tune/optimize opts */
        PyModule_AddIntMacro(cabinet, TDBTLARGE) ||
        PyModule_AddIntMacro(cabinet, TDBTDEFLATE) ||
        PyModule_AddIntMacro(cabinet, TDBTBZIP) ||
        PyModule_AddIntMacro(cabinet, TDBTTCBS) ||
        /* TDB setindex type */
        PyModule_AddIntMacro(cabinet, TDBITLEXICAL) ||
        PyModule_AddIntMacro(cabinet, TDBITDECIMAL) ||
        PyModule_AddIntMacro(cabinet, TDBITTOKEN) ||
        PyModule_AddIntMacro(cabinet, TDBITQGRAM) ||
        PyModule_AddIntMacro(cabinet, TDBITOPT) ||
        PyModule_AddIntMacro(cabinet, TDBITVOID) ||
        PyModule_AddIntMacro(cabinet, TDBITKEEP) ||
        /* TDBQuery process callback return constants */
        PyModule_AddIntMacro(cabinet, TDBQPPUT) ||
        PyModule_AddIntMacro(cabinet, TDBQPOUT) ||
        PyModule_AddIntMacro(cabinet, TDBQPSTOP) ||
        /* TDBQuery sort type */
        PyModule_AddIntMacro(cabinet, TDBQOSTRASC) ||
        PyModule_AddIntMacro(cabinet, TDBQOSTRDESC) ||
        PyModule_AddIntMacro(cabinet, TDBQONUMASC) ||
        PyModule_AddIntMacro(cabinet, TDBQONUMDESC) ||
        /* TDBQuery select condition */
        PyModule_AddIntMacro(cabinet, TDBQCSTREQ) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTRINC) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTRBW) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTREW) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTRAND) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTROR) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTROREQ) ||
        PyModule_AddIntMacro(cabinet, TDBQCSTRRX) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMEQ) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMGT) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMGE) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMLT) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMLE) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMBT) ||
        PyModule_AddIntMacro(cabinet, TDBQCNUMOREQ) ||
        PyModule_AddIntMacro(cabinet, TDBQCFTSPH) ||
        PyModule_AddIntMacro(cabinet, TDBQCFTSAND) ||
        PyModule_AddIntMacro(cabinet, TDBQCFTSOR) ||
        PyModule_AddIntMacro(cabinet, TDBQCFTSEX) ||
        PyModule_AddIntMacro(cabinet, TDBQCNEGATE) ||
        PyModule_AddIntMacro(cabinet, TDBQCNOIDX) ||
        /* TDB metasearch type */
        PyModule_AddIntMacro(cabinet, TDBMSUNION) ||
        PyModule_AddIntMacro(cabinet, TDBMSISECT) ||
        PyModule_AddIntMacro(cabinet, TDBMSDIFF)
       ) {
        Py_DECREF(Error);
        Py_DECREF(cabinet);
        return NULL;
    }
    return cabinet;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_cabinet(void)
{
    return init_cabinet();
}
#else
PyMODINIT_FUNC
initcabinet(void)
{
    init_cabinet();
}
#endif
