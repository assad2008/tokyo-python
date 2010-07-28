/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_idb_error(TCIDB *idb, long long key)
{
    int ecode;

    ecode = tcidbecode(idb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return PyErr_Format(PyExc_KeyError, "%ld", (long)key);
    }
    return set_error(Error, tcidberrmsg(ecode));
}


/*******************************************************************************
* IDB iterator types
*******************************************************************************/

/* new_IDBIter */
static PyObject *
new_IDBIter(IDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tcidbiterinit(self->idb)) {
        Py_DECREF(iter);
        return set_idb_error(self->idb, 0);
    }
    self->changed = false;
    return iter;
}


/* IDBIterKeysType.tp_iternext */
static PyObject *
IDBIterKeys_tp_iternext(DBIter *self)
{
    IDB *idb = (IDB *)self->db;
    long long key;
    PyObject *pykey;

    if (idb->changed) {
        return set_error(Error, "IDB changed during iteration");
    }
    key = uint64_to_int64(tcidbiternext(idb->idb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcidbecode(idb->idb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_idb_error(idb->idb, 0);
    }
    pykey = PyLong_FromLongLong(key);
    return pykey;
}


/* IDBIterKeysType */
static PyTypeObject IDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.IDBIterKeys",             /*tp_name*/
    sizeof(DBIter),                           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)DBIter_tp_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                        /*tp_doc*/
    (traverseproc)DBIter_tp_traverse,         /*tp_traverse*/
    (inquiry)DBIter_tp_clear,                 /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    PyObject_SelfIter,                        /*tp_iter*/
    (iternextfunc)IDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* IDBIterValuesType.tp_iternext */
static PyObject *
IDBIterValues_tp_iternext(DBIter *self)
{
    IDB *idb = (IDB *)self->db;
    long long key;
    char *value;
    PyObject *pyvalue;

    if (idb->changed) {
        return set_error(Error, "IDB changed during iteration");
    }
    key = uint64_to_int64(tcidbiternext(idb->idb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcidbecode(idb->idb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_idb_error(idb->idb, 0);
    }
    value = tcidbget(idb->idb, key);
    pyvalue = PyUnicode_FromString(value);
    tcfree(value);
    return pyvalue;
}


/* IDBIterValuesType */
static PyTypeObject IDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.IDBIterValues",           /*tp_name*/
    sizeof(DBIter),                           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)DBIter_tp_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                        /*tp_doc*/
    (traverseproc)DBIter_tp_traverse,         /*tp_traverse*/
    (inquiry)DBIter_tp_clear,                 /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    PyObject_SelfIter,                        /*tp_iter*/
    (iternextfunc)IDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* IDBIterItemsType.tp_iternext */
static PyObject *
IDBIterItems_tp_iternext(DBIter *self)
{
    IDB *idb = (IDB *)self->db;
    long long key;
    char *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (idb->changed) {
        return set_error(Error, "IDB changed during iteration");
    }
    key = uint64_to_int64(tcidbiternext(idb->idb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcidbecode(idb->idb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_idb_error(idb->idb, 0);
    }
    value = tcidbget(idb->idb, key);
    pykey = PyLong_FromLongLong(key);
    pyvalue = PyUnicode_FromString(value);
    if (pykey && pyvalue) {
        pyresult = PyTuple_Pack(2, pykey, pyvalue);
    }
    Py_XDECREF(pykey);
    Py_XDECREF(pyvalue);
    tcfree(value);
    return pyresult;
}


/* IDBIterItemsType */
static PyTypeObject IDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.IDBIterItems",            /*tp_name*/
    sizeof(DBIter),                           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)DBIter_tp_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                        /*tp_doc*/
    (traverseproc)DBIter_tp_traverse,         /*tp_traverse*/
    (inquiry)DBIter_tp_clear,                 /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    PyObject_SelfIter,                        /*tp_iter*/
    (iternextfunc)IDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


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
IDB_Contains(IDB *self, PyObject *pykey)
{
    long long key;
    char *value;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return -1;
    }
    value = tcidbget(self->idb, key);
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
IDB_GetItem(IDB *self, PyObject *pykey)
{
    long long key;
    char *value;
    PyObject *pyvalue;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return NULL;
    }
    value = tcidbget(self->idb, key);
    if (!value) {
        return set_idb_error(self->idb, key);
    }
    pyvalue = PyUnicode_FromString(value);
    tcfree(value);
    return pyvalue;
}


/* IDB_tp_as_mapping.mp_ass_subscript */
static int
IDB_SetItem(IDB *self, PyObject *pykey, PyObject *pyvalue)
{
    long long key;
    char *value;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return -1;
    }
    if (pyvalue) {
        value = PyUnicode_AsString(pyvalue);
        if (!value) {
            return -1;
        }
        if (!tcidbput(self->idb, key, value)) {
            set_idb_error(self->idb, 0);
            return -1;
        }
    }
    else {
        if (!tcidbout(self->idb, key)) {
            set_idb_error(self->idb, key);
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


/* IDBType.tp_iter */
static PyObject *
IDB_tp_iter(IDB *self)
{
    return new_IDBIter(self, &IDBIterKeysType);
}


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


/* IDB.clear() */
PyDoc_STRVAR(IDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
IDB_clear(IDB *self)
{
    if (!tcidbvanish(self->idb)) {
        return set_idb_error(self->idb, 0);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* IDB.copy(path) */
PyDoc_STRVAR(IDB_copy_doc,
"copy(path)\n\
\n\
Copy the database.\n\
'path': path to the destination directory.");

static PyObject *
IDB_copy(IDB *self, PyObject *args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s:copy", &path)) {
        return NULL;
    }
    if (*path == '@') {
        /* disable this feature until I find out more */
        return set_error(PyExc_NotImplementedError,
                         "this feature is not supported");
    }
    if (!tcidbcopy(self->idb, path)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.get(key) */
PyDoc_STRVAR(IDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
IDB_get(IDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return IDB_GetItem(self, pykey);
}


/* IDB.remove(key) */
PyDoc_STRVAR(IDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
IDB_remove(IDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (IDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* IDB.put(key, value) */
PyDoc_STRVAR(IDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
IDB_put(IDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (IDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* IDB.sync() */
PyDoc_STRVAR(IDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database.");

static PyObject *
IDB_sync(IDB *self)
{
    if (!tcidbsync(self->idb)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.search(expr[, mode]) -> frozenset */
PyDoc_STRVAR(IDB_search_doc,
"search(expr[, mode]) -> frozenset\n\
\n\
.");

static PyObject *
IDB_search(IDB *self, PyObject *args, PyObject *kwargs)
{
    const char *expr;
    int mode = -1, result_size;
    uint64_t *result;
    PyObject *pyexpr, *pyresult;

    if (!PyArg_ParseTuple(args, "O|i:search", &pyexpr, &mode)) {
        return NULL;
    }
    expr = PyUnicode_AsString(pyexpr);
    if (!expr) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    if (mode < 0) {
        result = tcidbsearch2(self->idb, expr, &result_size);
    }
    else {
        result = tcidbsearch(self->idb, expr, mode, &result_size);
    }
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_idb_error(self->idb, 0);
    }
    pyresult = ids_to_frozenset(result, result_size);
    tcfree(result);
    return pyresult;
}


/* IDB.optimize() */
PyDoc_STRVAR(IDB_optimize_doc,
"optimize()\n\
\n\
Optimize a database.\n\
\n\
Note:\n\
Optimizing a read only database is an invalid operation.");

static PyObject *
IDB_optimize(IDB *self)
{
    if (!tcidboptimize(self->idb)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.tune(ernum, etnum, iusiz, opts) */
PyDoc_STRVAR(IDB_tune_doc,
"tune(ernum, etnum, iusiz, opts)\n\
\n\
Tune a database.\n\
'ernum': the expected number of records to be stored. If specified as 0 or as a\n\
         negative value, the default value (1000000) is used.\n\
'etnum': the expected number of tokens to be stored. If specified as 0 or as a\n\
         negative value, the default value (1000000) is used.\n\
'iusiz': the unit size of each index file. If specified as 0 or as a negative\n\
         value, the default value (536870912) is used.\n\
'opts': TODO (0).\n\
\n\
Note:\n\
Tuning an open database is an invalid operation.");

static PyObject *
IDB_tune(IDB *self, PyObject *args)
{
    long long ernum, etnum, iusiz;
    unsigned char opts;

    if (!PyArg_ParseTuple(args, "LLLb:tune", &ernum, &etnum, &iusiz, &opts)) {
        return NULL;
    }
    if (!tcidbtune(self->idb, ernum, etnum, iusiz, opts)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.setcache(icsiz, lcnum) */
PyDoc_STRVAR(IDB_setcache_doc,
"setcache(icsiz, lcnum)\n\
\n\
Set the cache size.\n\
'icsiz': the size of the token cache. If specified as 0 or as a negative value,\n\
         the default value (134217728) is used.\n\
'lcnum': the maximum number of cached leaf nodes. If specified as 0 or as a\n\
         negative value, the default value (64 for writer or 1024 for reader) is\n\
         used.\n\
\n\
Note:\n\
Setting the cache size on an open database is an invalid operation.");

static PyObject *
IDB_setcache(IDB *self, PyObject *args)
{
    long long icsiz;
    long lcnum;

    if (!PyArg_ParseTuple(args, "Ll:setcache", &icsiz, &lcnum)) {
        return NULL;
    }
    if (!tcidbsetcache(self->idb, icsiz, lcnum)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.setfwmmax(fwmmax) */
PyDoc_STRVAR(IDB_setfwmmax_doc,
"setfwmmax(fwmmax)\n\
\n\
Set the maximum number of forward matching expansion.\n\
'fwmmax': the maximum number of forward matching expansion.\n\
\n\
Note:\n\
Setting this on an open database is an invalid operation.");

static PyObject *
IDB_setfwmmax(IDB *self, PyObject *args)
{
    long fwmmax;

    if (!PyArg_ParseTuple(args, "l:setfwmmax", &fwmmax)) {
        return NULL;
    }
    if (!tcidbsetfwmmax(self->idb, fwmmax)) {
        return set_idb_error(self->idb, 0);
    }
    Py_RETURN_NONE;
}


/* IDB.iterkeys() */
PyDoc_STRVAR(IDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
IDB_iterkeys(IDB *self)
{
    return new_IDBIter(self, &IDBIterKeysType);
}


/* IDB.itervalues() */
PyDoc_STRVAR(IDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
IDB_itervalues(IDB *self)
{
    return new_IDBIter(self, &IDBIterValuesType);
}


/* IDB.iteritems() */
PyDoc_STRVAR(IDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
IDB_iteritems(IDB *self)
{
    return new_IDBIter(self, &IDBIterItemsType);
}


/* IDBType.tp_methods */
static PyMethodDef IDB_tp_methods[] = {
    {"open", (PyCFunction)IDB_open, METH_VARARGS, IDB_open_doc},
    {"close", (PyCFunction)IDB_close, METH_NOARGS, IDB_close_doc},
    {"clear", (PyCFunction)IDB_clear, METH_NOARGS, IDB_clear_doc},
    {"copy", (PyCFunction)IDB_copy, METH_VARARGS, IDB_copy_doc},
    {"get", (PyCFunction)IDB_get, METH_VARARGS, IDB_get_doc},
    {"remove", (PyCFunction)IDB_remove, METH_VARARGS, IDB_remove_doc},
    {"put", (PyCFunction)IDB_put, METH_VARARGS, IDB_put_doc},
    {"sync", (PyCFunction)IDB_sync, METH_NOARGS, IDB_sync_doc},
    {"search", (PyCFunction)IDB_search, METH_VARARGS, IDB_search_doc},
    {"optimize", (PyCFunction)IDB_optimize, METH_NOARGS, IDB_optimize_doc},
    {"tune", (PyCFunction)IDB_tune, METH_VARARGS, IDB_tune_doc},
    {"setcache", (PyCFunction)IDB_setcache, METH_VARARGS, IDB_setcache_doc},
    {"setfwmmax", (PyCFunction)IDB_setfwmmax, METH_VARARGS, IDB_setfwmmax_doc},
    {"iterkeys", (PyCFunction)IDB_iterkeys, METH_NOARGS, IDB_iterkeys_doc},
    {"itervalues", (PyCFunction)IDB_itervalues, METH_NOARGS, IDB_itervalues_doc},
    {"iteritems", (PyCFunction)IDB_iteritems, METH_NOARGS, IDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* IDB.path */
PyDoc_STRVAR(IDB_path_doc,
"The path to the database directory.");

static PyObject *
IDB_path_get(IDB *self, void *closure)
{
    const char *path;

    path = tcidbpath(self->idb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* IDB.size */
PyDoc_STRVAR(IDB_size_doc,
"The size in bytes of the database.");

static PyObject *
IDB_size_get(IDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcidbfsiz(self->idb));
}


/* IDBType.tp_getsets */
static PyGetSetDef IDB_tp_getsets[] = {
    {"path", (getter)IDB_path_get, NULL, IDB_path_doc, NULL},
    {"size", (getter)IDB_size_get, NULL, IDB_size_doc, NULL},
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
    (getiterfunc)IDB_tp_iter,                 /*tp_iter*/
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
