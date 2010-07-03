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


#ifndef _TOKYO_PYTHON_H
#define _TOKYO_PYTHON_H


#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <tcutil.h>


/*******************************************************************************
* objects
*******************************************************************************/

/* Error */
static PyObject *Error;


/*******************************************************************************
* utilities
*******************************************************************************/

/* error handling utils */
PyObject *
set_error(PyObject *type, const char *message)
{
    PyErr_SetString(type, message);
    return NULL;
}

#define set_key_error(key) set_error(PyExc_KeyError, key)

PyObject *
set_stopiteration_error(void)
{
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
}


#if PY_MAJOR_VERSION >= 3
/* PyUnicode to char * */
char *
PyUnicode_AS_STRING(PyObject *unicode)
{
    PyObject *tmp;
    char *result;

    tmp = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(unicode),
                               PyUnicode_GET_SIZE(unicode), NULL);
    if (!tmp) {
        return NULL;
    }
    result = PyBytes_AS_STRING(tmp);
    Py_DECREF(tmp);
    return result;
}

char *
PyUnicode_AsString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_SetString(PyExc_TypeError, "a string is required");
        return NULL;
    }
    return PyUnicode_AS_STRING(unicode);
}

#define PyString_AsString PyUnicode_AsString
#define PyString_FromString PyUnicode_FromString
#define PyInt_FromLong PyLong_FromLong
#endif


/* convert a TCLIST to a fozenset */
PyObject *
tclist_to_frozenset(TCLIST *result)
{
    const void *value;
    int len, i, value_size;
    PyObject *pyresult, *pyvalue;

    pyresult = PyFrozenSet_New(NULL);
    if (!pyresult) {
        return NULL;
    }
    len = tclistnum(result);
    for (i = 0; i < len; i++) {
        value = tclistval(result, i, &value_size);
        pyvalue = PyBytes_FromStringAndSize((char *)value,
                                            (Py_ssize_t)value_size);
        if (!pyvalue) {
            Py_DECREF(pyresult);
            return NULL;
        }
        if (PySet_Add(pyresult, pyvalue)) {
            Py_DECREF(pyvalue);
            Py_DECREF(pyresult);
            return NULL;
        }
        Py_DECREF(pyvalue);
    }
    return pyresult;
}


/* convert a TCLIST to a tuple */
PyObject *
tclist_to_tuple(TCLIST *result)
{
    const void *value;
    int len, i, value_size;
    PyObject *pyresult, *pyvalue;

    len = tclistnum(result);
    pyresult = PyTuple_New((Py_ssize_t)len);
    if (!pyresult) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        value = tclistval(result, i, &value_size);
        pyvalue = PyBytes_FromStringAndSize((char *)value,
                                            (Py_ssize_t)value_size);
        if (!pyvalue) {
            Py_DECREF(pyresult);
            return NULL;
        }
        PyTuple_SET_ITEM(pyresult, (Py_ssize_t)i, pyvalue);
    }
    return pyresult;
}


/* convert a TCMAP to a dict */
PyObject *
tcmap_to_dict(TCMAP *result)
{
    const void *key, *value;
    int key_size, value_size;
    PyObject *pyresult, *pykey, *pyvalue;

    pyresult = PyDict_New();
    if (!pyresult) {
        return NULL;
    }
    tcmapiterinit(result);
    while ((key = tcmapiternext(result, &key_size)) != NULL) {
        value = tcmapget(result, key, key_size, &value_size);
        pykey = PyBytes_FromStringAndSize((char *)key, (Py_ssize_t)key_size);
        pyvalue = PyBytes_FromStringAndSize((char *)value,
                                            (Py_ssize_t)value_size);
        if (!(pykey && pyvalue)) {
            Py_XDECREF(pykey);
            Py_XDECREF(pyvalue);
            Py_DECREF(pyresult);
            return NULL;
        }
        if (PyDict_SetItem(pyresult, pykey, pyvalue)) {
            Py_DECREF(pykey);
            Py_DECREF(pyvalue);
            Py_DECREF(pyresult);
            return NULL;
        }
        Py_DECREF(pykey);
        Py_DECREF(pyvalue);
    }
    return pyresult;
}


/* convert a dict to a TCMAP */
TCMAP *
dict_to_tcmap(PyObject *pyitems)
{
    TCMAP *items;
    PyObject *pykey, *pyvalue;
    char *key, *value;
    Py_ssize_t key_size, value_size, pos = 0;

    if (!PyDict_Check(pyitems)) {
        set_error(PyExc_TypeError, "a dict is required");
        return NULL;
    }
    items = tcmapnew();
    if (!items) {
        set_error(Error, "could not create TCMAP, memory issue?");
        return NULL;
    }
    while (PyDict_Next(pyitems, &pos, &pykey, &pyvalue)) {
        if (PyBytes_AsStringAndSize(pykey, &key, &key_size) ||
            PyBytes_AsStringAndSize(pyvalue, &value, &value_size)) {
            tcmapdel(items);
            return NULL;
        }
        tcmapput(items, (void *)key, (int)key_size,
                 (void *)value, (int)value_size);
    }
    return items;
}


/* merge dict args */
PyObject *
merge_put_args(const char *name, PyObject *pyvalue, PyObject *kwargs)
{
    if (pyvalue) {
        if (!PyDict_Check(pyvalue)) {
            return set_error(PyExc_TypeError, "a dict is required");
        }
        if (kwargs) {
            if (PyDict_Merge(pyvalue, kwargs, 1)) {
                return NULL;
            }
        }
        return pyvalue;
    }
    else if (kwargs) {
        return kwargs;
    }
    else {
        return PyErr_Format(PyExc_TypeError, "%s() takes at least 2 arguments",
                            name);
    }
}


#endif /* _TOKYO_PYTHON_H */
