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


/* convert a TCLIST to a fozenset */
PyObject *
tclist_to_frozenset(TCLIST *result)
{
    int len, i;
    PyObject *pyresult, *pyvalue;

    pyresult = PyFrozenSet_New(NULL);
    if (!pyresult) {
        return NULL;
    }
    len = tclistnum(result);
    for (i = 0; i < len; i++) {
        pyvalue = PyBytes_FromString(tclistval2(result, i));
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
    int len, i;
    PyObject *pyresult, *pyvalue;

    len = tclistnum(result);
    pyresult = PyTuple_New((Py_ssize_t)len);
    if (!pyresult) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        pyvalue = PyBytes_FromString(tclistval2(result, i));
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
    const char *key;
    PyObject *pyresult, *pykey, *pyvalue;

    pyresult = PyDict_New();
    if (!pyresult) {
        return NULL;
    }
    tcmapiterinit(result);
    while ((key = tcmapiternext2(result)) != NULL) {
        pykey = PyBytes_FromString(key);
        pyvalue = PyBytes_FromString(tcmapget2(result, key));
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
    const char *key, *value;

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
        key = PyBytes_AsString(pykey);
        value = PyBytes_AsString(pyvalue);
        if (!(key && value)) {
            tcmapdel(items);
            return NULL;
        }
        tcmapput2(items, key, value);
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


/*******************************************************************************
* DBIter
*******************************************************************************/

/* DBIter */
typedef struct {
    PyObject_HEAD
    PyObject *db;
} DBIter;


/* DBIter_tp_traverse */
static int
DBIter_tp_traverse(DBIter *self, visitproc visit, void *arg)
{
    Py_VISIT(self->db);
    return 0;
}


/* DBIter_tp_clear */
static int
DBIter_tp_clear(DBIter *self)
{
    Py_CLEAR(self->db);
    return 0;
}


/* DBIter_tp_dealloc */
static void
DBIter_tp_dealloc(DBIter *self)
{
    DBIter_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* DBIter_tp_new */
static PyObject *
DBIter_tp_new(PyTypeObject *type, PyObject *db)
{
    DBIter *self = (DBIter *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->db */
    Py_INCREF(db);
    self->db = db;
    return (PyObject *)self;
}


/* DBIter.__length_hint__ */
PyDoc_STRVAR(DBIter_length_hint_doc,
"Private method returning an estimate of len(list(hdb)).");

static PyObject *
DBIter_length_hint(DBIter *self)
{
    return PyLong_FromSsize_t(PyMapping_Length(self->db));
}


/* DBIter_tp_methods */
static PyMethodDef DBIter_tp_methods[] = {
    {"__length_hint__", (PyCFunction)DBIter_length_hint, METH_NOARGS,
     DBIter_length_hint_doc},
    {NULL}  /* Sentinel */
};


#endif /* _TOKYO_PYTHON_H */
