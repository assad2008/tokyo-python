/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_idb_error(TCIDB *idb, long long id)
{
    int ecode;

    ecode = tcidbecode(idb);
    if (id && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return PyErr_Format(PyExc_KeyError, "%lld", id);
    }
    return set_error(Error, tcidberrmsg(ecode));
}


/*******************************************************************************
* IDBType
*******************************************************************************/

/* IDBType.tp_doc */
PyDoc_STRVAR(IDB_tp_doc,
"IDB()\n\
\n\
Indexed Database.\n\
\n\
See also:\n\
Tokyo Dystopia Core API at:\n\
http://1978th.net/tokyodystopia/spex.html#dystopiaapi");


/* IDBType.tp_dealloc */
static void
IDB_tp_dealloc(IDB *self)
{
    if (self->idb) {
        tcidbdel(self->idb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* IDBType.tp_new */
static PyObject *
IDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    IDB *self = (IDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->idb */
    self->idb = tcidbnew();
    if (!self->idb) {
        set_error(Error, "could not create IDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* IDB_tp_as_sequence.sq_contains */
static int
IDB_Contains(IDB *self, PyObject *pyid)
{
    long long id;
    char *value;

    id = PyLong_AsLongLong(pyid);
    if (id == -1 && PyErr_Occurred()) {
        return -1;
    }
    value = tcidbget(self->idb, id);
    if (!value) {
        if (tcidbecode(self->idb) == TCENOREC) {
            return 0;
        }
        set_idb_error(self->idb, 0);
        return -1;
    }
    tcfree(value);
    return 1;
}


/* IDBType.tp_as_sequence */
static PySequenceMethods IDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)IDB_Contains,                 /*sq_contains*/
};


/* IDB_tp_as_mapping.mp_length */
static Py_ssize_t
IDB_Length(IDB *self)
{
    return DB_Length(tcidbrnum(self->idb));
}


/* IDB_tp_as_mapping.mp_subscript */
static PyObject *
IDB_GetItem(IDB *self, PyObject *pyid)
{
    long long id;
    char *value;
    PyObject *pyvalue;

    id = PyLong_AsLongLong(pyid);
    if (id == -1 && PyErr_Occurred()) {
        return NULL;
    }
    value = tcidbget(self->idb, id);
    if (!value) {
        return set_idb_error(self->idb, id);
    }
    pyvalue = PyString_FromString(value);
    tcfree(value);
    return pyvalue;
}


/* IDB_tp_as_mapping.mp_ass_subscript */
static int
IDB_SetItem(IDB *self, PyObject *pyid, PyObject *pyvalue)
{
    long long id;
    char *value;

    id = PyLong_AsLongLong(pyid);
    if (id == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (pyvalue) {
        value = PyString_AsString(pyvalue);
        if (!value) {
            return -1;
        }
        if (!tcidbput(self->idb, id, value)) {
            set_idb_error(self->idb, 0);
            return -1;
        }
    }
    else {
        if (!tcidbout(self->idb, id)) {
            set_idb_error(self->idb, id);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* IDBType.tp_as_mapping */
static PyMappingMethods IDB_tp_as_mapping = {
    (lenfunc)IDB_Length,                      /*mp_length*/
    (binaryfunc)IDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)IDB_SetItem                /*mp_ass_subscript*/
};


/* IDB.open(path, mode) */
PyDoc_STRVAR(IDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database directory.\n\
'mode': connection mode.");

static PyObject *
IDB_open(IDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tcidbopen(self->idb, path, mode)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.close() */
PyDoc_STRVAR(IDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
IDBs are closed when garbage-collected.");

static PyObject *
IDB_close(IDB *self)
{
    if (!tcidbclose(self->idb)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDBType.tp_methods */
static PyMethodDef IDB_tp_methods[] = {
    {"open", (PyCFunction)IDB_open, METH_VARARGS, IDB_open_doc},
    {"close", (PyCFunction)IDB_close, METH_NOARGS, IDB_close_doc},
    {NULL}  /* Sentinel */
};


/* IDBType.tp_getsets */
static PyGetSetDef IDB_tp_getsets[] = {
    {NULL}  /* Sentinel */
};


/* IDBType */
static PyTypeObject IDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.IDB",                     /*tp_name*/
    sizeof(IDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)IDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &IDB_tp_as_sequence,                      /*tp_as_sequence*/
    &IDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    IDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    IDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    IDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    IDB_tp_new,                               /*tp_new*/
};
