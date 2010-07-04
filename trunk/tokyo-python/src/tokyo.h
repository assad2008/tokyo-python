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


#define TK_PY_MAX_DB_LEN ((unsigned long long)PY_SSIZE_T_MAX)

#if SIZEOF_SIZE_T != SIZEOF_INT
#define TK_PY_SSIZE_T_IS_NOT_INT
#define TK_PY_MAX_STR_SIZE ((Py_ssize_t)INT_MAX)
#endif


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


/* convert a bytes object to a void ptr */
int
bytes_to_void(PyObject *pyvalue, void **value, int *value_size)
{
    char *tmp;
    Py_ssize_t tmp_size;

    if (PyBytes_AsStringAndSize(pyvalue, &tmp, &tmp_size)) {
        return -1;
    }
#ifdef TK_PY_SSIZE_T_IS_NOT_INT
    if (tmp_size > TK_PY_MAX_STR_SIZE) {
        set_error(PyExc_OverflowError, "string is too large");
        return -1;
    }
#endif
    *value = (void *)tmp;
    *value_size = (int)tmp_size;
    return 0;
}


/* convert a void ptr to a bytes object */
PyObject *
void_to_bytes(const void *value, int value_size)
{
    return PyBytes_FromStringAndSize((char *)value, (Py_ssize_t)value_size);
}


/* convert a TCXSTR to a bytes object */
PyObject *
tcxstr_to_bytes(TCXSTR *value)
{
    return void_to_bytes(tcxstrptr(value), tcxstrsize(value));
}


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
        pyvalue = void_to_bytes(value, value_size);
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
        pyvalue = void_to_bytes(value, value_size);
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
        pykey = void_to_bytes(key, key_size);
        pyvalue = void_to_bytes(value, value_size);
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
    Py_ssize_t pos = 0;
    void *key, *value;
    int key_size, value_size;

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
        if (bytes_to_void(pykey, &key, &key_size) ||
            bytes_to_void(pyvalue, &value, &value_size)) {
            tcmapdel(items);
            return NULL;
        }
        /* XXX: not sure about the following check
        if (strlen(key) != (size_t)key_size) {
            set_error(PyExc_TypeError, "value's keys must be without null bytes");
            tcmapdel(items);
            return NULL;
        }
        */
        tcmapput(items, key, key_size, value, value_size);
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


/* used by *DB_tp_as_mapping.mp_length */
Py_ssize_t
DB_Length(unsigned long long len)
{
    if (len > TK_PY_MAX_DB_LEN) {
        set_error(PyExc_OverflowError, "database is too large for Python!");
        return -1;
    }
    return (Py_ssize_t)len;
}


#endif /* _TOKYO_PYTHON_H */
