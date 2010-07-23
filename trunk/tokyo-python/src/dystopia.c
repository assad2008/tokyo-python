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

#include <dystopia.h>


/*******************************************************************************
* objects
*******************************************************************************/

/* IDB */
typedef struct {
    PyObject_HEAD
    TCIDB *idb;
    bool changed;
} IDB;


/*******************************************************************************
* utilities
*******************************************************************************/

long long
pylong_to_int64(PyObject *pyid)
{
    long long id;

    id = PyLong_AsLongLong(pyid);
    if (id < 0 && !PyErr_Occurred()) {
        set_error(PyExc_OverflowError, "key is lesser than minimum");
    }
    return id;
}


/*******************************************************************************
* types
*******************************************************************************/

#include "IDB.c"


/*******************************************************************************
* dystopia_module
*******************************************************************************/

/* dystopia_module.m_doc */
PyDoc_STRVAR(dystopia_m_doc,
"Python Tokyo Dystopia interface.\n\
\n\
TODO\n\
\n\
See also:\n\
Tokyo Dystopia documentation at:\n\
http://1978th.net/tokyodystopia/");


/* dystopia.version() -> str */
PyDoc_STRVAR(dystopia_version_doc,
"version() -> str\n\
\n\
Returns the version string of the underlying Tokyo Dystopia library.");

static PyObject *
dystopia_version(PyObject *module)
{
    return PyString_FromString(tdversion);
}


/* dystopia_module.m_methods */
static PyMethodDef dystopia_m_methods[] = {
    {"version", (PyCFunction)dystopia_version, METH_NOARGS,
     dystopia_version_doc},
    {NULL} /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3
/* dystopia_module */
static PyModuleDef dystopia_module = {
    PyModuleDef_HEAD_INIT,
    "tokyo.dystopia",                         /*m_name*/
    dystopia_m_doc,                           /*m_doc*/
    -1,                                       /*m_size*/
    dystopia_m_methods,                       /*m_methods*/
};
#endif


/* dystopia_module initialization */
PyObject *
init_dystopia(void)
{
    PyObject *dystopia;

    /* checking types */
    if (
        PyType_Ready(&IDBType)
       ) {
        return NULL;
    }
    /* dystopia */
#if PY_MAJOR_VERSION >= 3
    dystopia = PyModule_Create(&dystopia_module);
#else
    dystopia = Py_InitModule3("tokyo.dystopia", dystopia_m_methods,
                              dystopia_m_doc);
#endif
    if (!dystopia) {
        return NULL;
    }
    /* dystopia.Error object */
    Error = PyErr_NewException("tokyo.dystopia.Error", NULL, NULL);
    if (!Error) {
        Py_DECREF(dystopia);
        return NULL;
    }
    /* adding objects and constants */
    if (
        PyModule_AddObject(dystopia, "Error", Error) ||
        PyModule_AddObject(dystopia, "IDB", (PyObject *)&IDBType) ||
        /* IDB open mode */
        PyModule_AddIntMacro(dystopia, IDBOREADER) ||
        PyModule_AddIntMacro(dystopia, IDBOWRITER) ||
        PyModule_AddIntMacro(dystopia, IDBOCREAT) ||
        PyModule_AddIntMacro(dystopia, IDBOTRUNC) ||
        PyModule_AddIntMacro(dystopia, IDBONOLCK) ||
        PyModule_AddIntMacro(dystopia, IDBOLCKNB) ||
        /* IDB tune opts */
        PyModule_AddIntMacro(dystopia, IDBTLARGE) ||
        PyModule_AddIntMacro(dystopia, IDBTDEFLATE) ||
        PyModule_AddIntMacro(dystopia, IDBTBZIP) ||
        PyModule_AddIntMacro(dystopia, IDBTTCBS) ||
        /* IDB search mode */
        PyModule_AddIntMacro(dystopia, IDBSSUBSTR) ||
        PyModule_AddIntMacro(dystopia, IDBSPREFIX) ||
        PyModule_AddIntMacro(dystopia, IDBSSUFFIX) ||
        PyModule_AddIntMacro(dystopia, IDBSFULL) ||
        PyModule_AddIntMacro(dystopia, IDBSTOKEN) ||
        PyModule_AddIntMacro(dystopia, IDBSTOKPRE) ||
        PyModule_AddIntMacro(dystopia, IDBSTOKSUF)
       ) {
        Py_DECREF(Error);
        Py_DECREF(dystopia);
        return NULL;
    }
    return dystopia;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_dystopia(void)
{
    return init_dystopia();
}
#else
PyMODINIT_FUNC
initdystopia(void)
{
    init_dystopia();
}
#endif
