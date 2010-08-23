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

#include <tcrdb.h>


/*******************************************************************************
* objects
*******************************************************************************/

/* RDBBase - not exposed */
typedef struct {
    PyObject_HEAD
    TCRDB *rdb;
    bool changed;
    PyObject *pystatus;
} RDBBase;


/* RDB */
typedef struct {
    RDBBase rdbase;
} RDB;


/* RTDB */
typedef struct {
    RDBBase rdbase;
} RTDB;

/* RTDBQuery */
typedef struct {
    PyObject_HEAD
    RDBQRY *rqry;
    RTDB *rtdb;
} RTDBQuery;


/*******************************************************************************
* types
*******************************************************************************/

#include "RDBBase.c"
#include "RDB.c"
#include "RTDB.c"


/*******************************************************************************
* tyrant_module
*******************************************************************************/

/* tyrant_module.m_doc */
PyDoc_STRVAR(tyrant_m_doc,
"Python Tokyo Tyrant interface.\n\
\n\
TODO\n\
\n\
See also:\n\
Tokyo Tyrant documentation at:\n\
http://fallabs.com/tokyotyrant/");


/* tyrant.version() -> str */
PyDoc_STRVAR(tyrant_version_doc,
"version() -> str\n\
\n\
Returns the version string of the underlying Tokyo Tyrant library.");

static PyObject *
tyrant_version(PyObject *module)
{
    return PyString_FromString(ttversion);
}


/* tyrant_module.m_methods */
static PyMethodDef tyrant_m_methods[] = {
    {"version", (PyCFunction)tyrant_version, METH_NOARGS, tyrant_version_doc},
    {NULL} /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3
/* tyrant_module */
static PyModuleDef tyrant_module = {
    PyModuleDef_HEAD_INIT,
    "tokyo.tyrant",                           /*m_name*/
    tyrant_m_doc,                             /*m_doc*/
    -1,                                       /*m_size*/
    tyrant_m_methods,                         /*m_methods*/
};
#endif


/* tyrant_module initialization */
PyObject *
init_tyrant(void)
{
    PyObject *tyrant;

    /* fill in deferred data addresses */
    RDBType.tp_base = &RDBBaseType;
    RTDBType.tp_base = &RDBBaseType;

    /* checking types */
    if (
        PyType_Ready(&RDBBaseType) ||
        PyType_Ready(&RDBBaseIterKeysType) ||
        PyType_Ready(&RDBType) ||
        PyType_Ready(&RDBIterValuesType) ||
        PyType_Ready(&RDBIterItemsType) ||
        PyType_Ready(&RTDBType) ||
        PyType_Ready(&RTDBIterValuesType) ||
        PyType_Ready(&RTDBIterItemsType) ||
        PyType_Ready(&RTDBIterValuesKeysType) ||
        PyType_Ready(&RTDBIterValuesValsType) ||
        PyType_Ready(&RTDBQueryType)
       ) {
        return NULL;
    }
    /* tyrant */
#if PY_MAJOR_VERSION >= 3
    tyrant = PyModule_Create(&tyrant_module);
#else
    tyrant = Py_InitModule3("tokyo.tyrant", tyrant_m_methods, tyrant_m_doc);
#endif
    if (!tyrant) {
        return NULL;
    }
    /* tyrant.Error object */
    Error = PyErr_NewException("tokyo.tyrant.Error", NULL, NULL);
    if (!Error || PyModule_AddObject(tyrant, "Error", Error)) {
        Py_XDECREF(Error);
        goto fail;
    }
    /* adding types and constants */
    if (
        PyModule_AddType(tyrant, "RDB", &RDBType) ||
        PyModule_AddType(tyrant, "RTDB", &RTDBType) ||
        /* RDB tune opts */
        PyModule_AddIntMacro(tyrant, RDBTRECON) ||
        /* RDB restore/setmaster opts */
        PyModule_AddIntMacro(tyrant, RDBROCHKCON)
       ) {
        goto fail;
    }
    return tyrant;

fail:
#if PY_MAJOR_VERSION >= 3
    Py_DECREF(tyrant);
#endif
    return NULL;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_tyrant(void)
{
    return init_tyrant();
}
#else
PyMODINIT_FUNC
inittyrant(void)
{
    init_tyrant();
}
#endif
