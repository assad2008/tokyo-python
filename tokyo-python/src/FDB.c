/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_fdb_error(TCFDB *fdb, long long key)
{
    int ecode;

    ecode = tcfdbecode(fdb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return PyErr_Format(PyExc_KeyError, "%ld", (long)key);
    }
    return set_error(Error, tcfdberrmsg(ecode));
}


/*******************************************************************************
* FDB iterator types
*******************************************************************************/

/* new_FDBIter */
static PyObject *
new_FDBIter(FDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tcfdbiterinit(self->fdb)) {
        Py_DECREF(iter);
        return set_fdb_error(self->fdb, 0);
    }
    self->changed = false;
    return iter;
}


/* FDBIterKeysType.tp_iternext */
static PyObject *
FDBIterKeys_tp_iternext(DBIter *self)
{
    FDB *fdb = (FDB *)self->db;
    long long key;
    PyObject *pykey;

    if (fdb->changed) {
        return set_error(Error, "FDB changed during iteration");
    }
    key = uint64_to_int64(tcfdbiternext(fdb->fdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcfdbecode(fdb->fdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_fdb_error(fdb->fdb, 0);
    }
    pykey = PyLong_FromLongLong(key);
    return pykey;
}


/* FDBIterKeysType */
static PyTypeObject FDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.FDBIterKeys",              /*tp_name*/
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
    (iternextfunc)FDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* FDBIterValuesType.tp_iternext */
static PyObject *
FDBIterValues_tp_iternext(DBIter *self)
{
    FDB *fdb = (FDB *)self->db;
    long long key;
    void *value;
    int value_size;
    PyObject *pyvalue;

    if (fdb->changed) {
        return set_error(Error, "FDB changed during iteration");
    }
    key = uint64_to_int64(tcfdbiternext(fdb->fdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcfdbecode(fdb->fdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_fdb_error(fdb->fdb, 0);
    }
    value = tcfdbget(fdb->fdb, key, &value_size);
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    return pyvalue;
}


/* FDBIterValuesType */
static PyTypeObject FDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.FDBIterValues",            /*tp_name*/
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
    (iternextfunc)FDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* FDBIterItemsType.tp_iternext */
static PyObject *
FDBIterItems_tp_iternext(DBIter *self)
{
    FDB *fdb = (FDB *)self->db;
    long long key;
    void *value;
    int value_size;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (fdb->changed) {
        return set_error(Error, "FDB changed during iteration");
    }
    key = uint64_to_int64(tcfdbiternext(fdb->fdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcfdbecode(fdb->fdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_fdb_error(fdb->fdb, 0);
    }
    value = tcfdbget(fdb->fdb, key, &value_size);
    pykey = PyLong_FromLongLong(key);
    pyvalue = void_to_bytes(value, value_size);
    if (pykey && pyvalue) {
        pyresult = PyTuple_Pack(2, pykey, pyvalue);
    }
    Py_XDECREF(pykey);
    Py_XDECREF(pyvalue);
    tcfree(value);
    return pyresult;
}


/* FDBIterItemsType */
static PyTypeObject FDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.FDBIterItems",             /*tp_name*/
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
    (iternextfunc)FDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* FDBType
*******************************************************************************/

/* FDBType.tp_doc */
PyDoc_STRVAR(FDB_tp_doc,
"FDB()\n\
\n\
Fixed-length Database.\n\
\n\
See also:\n\
Tokyo Cabinet Fixed-length Database API at:\n\
http://1978th.net/tokyocabinet/spex-en.html#tcfdbapi");


/* FDBType.tp_dealloc */
static void
FDB_tp_dealloc(FDB *self)
{
    if (self->fdb) {
        tcfdbdel(self->fdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* FDBType.tp_new */
static PyObject *
FDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FDB *self = (FDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->fdb */
    self->fdb = tcfdbnew();
    if (!self->fdb) {
        set_error(Error, "could not create FDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    if (!tcfdbsetmutex(self->fdb)) {
        set_fdb_error(self->fdb, 0);
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* FDB_tp_as_sequence.sq_contains */
static int
FDB_Contains(FDB *self, PyObject *pykey)
{
    long long key;
    int value_size;
    void *value;

    key = PyLong_AsLongLong(pykey);
    if (key == -1 && PyErr_Occurred()) {
        return -1;
    }
    value = tcfdbget(self->fdb, key, &value_size);
    if (!value) {
        if (tcfdbecode(self->fdb) == TCENOREC) {
            return 0;
        }
        set_fdb_error(self->fdb, 0);
        return -1;
    }
    tcfree(value);
    return 1;
}


/* FDBType.tp_as_sequence */
static PySequenceMethods FDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)FDB_Contains,                 /*sq_contains*/
};


/* FDB_tp_as_mapping.mp_length */
static Py_ssize_t
FDB_Length(FDB *self)
{
    return DB_Length(tcfdbrnum(self->fdb));
}


/* FDB_tp_as_mapping.mp_subscript */
static PyObject *
FDB_GetItem(FDB *self, PyObject *pykey)
{
    long long key;
    void *value;
    int value_size;
    PyObject *pyvalue;

    key = PyLong_AsLongLong(pykey);
    if (key == -1 && PyErr_Occurred()) {
        return NULL;
    }
    value = tcfdbget(self->fdb, key, &value_size);
    if (!value) {
        return set_fdb_error(self->fdb, key);
    }
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    return pyvalue;
}


/* FDB_tp_as_mapping.mp_ass_subscript */
static int
FDB_SetItem(FDB *self, PyObject *pykey, PyObject *pyvalue)
{
    long long key;
    void *value;
    int value_size;

    key = PyLong_AsLongLong(pykey);
    if (key == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (pyvalue) {
        if (bytes_to_void(pyvalue, &value, &value_size)) {
            return -1;
        }
        if (!tcfdbput(self->fdb, key, value, value_size)) {
            set_fdb_error(self->fdb, 0);
            return -1;
        }
    }
    else {
        if (!tcfdbout(self->fdb, key)) {
            set_fdb_error(self->fdb, key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* FDBType.tp_as_mapping */
static PyMappingMethods FDB_tp_as_mapping = {
    (lenfunc)FDB_Length,                      /*mp_length*/
    (binaryfunc)FDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)FDB_SetItem                /*mp_ass_subscript*/
};


/* FDBType.tp_iter */
static PyObject *
FDB_tp_iter(FDB *self)
{
    return new_FDBIter(self, &FDBIterKeysType);
}


/* FDB.open(path, mode) */
PyDoc_STRVAR(FDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database file.\n\
'mode': connection mode.");

static PyObject *
FDB_open(FDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tcfdbopen(self->fdb, path, mode)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.close() */
PyDoc_STRVAR(FDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
FDBs are closed when garbage-collected.");

static PyObject *
FDB_close(FDB *self)
{
    if (!tcfdbclose(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.clear() */
PyDoc_STRVAR(FDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
FDB_clear(FDB *self)
{
    if (!tcfdbvanish(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* FDB.copy(path) */
PyDoc_STRVAR(FDB_copy_doc,
"copy(path)\n\
\n\
Copy the database file.\n\
'path': path to the destination file.");

static PyObject *
FDB_copy(FDB *self, PyObject *args)
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
    if (!tcfdbcopy(self->fdb, path)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.begin() */
PyDoc_STRVAR(FDB_begin_doc,
"begin()\n\
\n\
Begin a transaction.");

static PyObject *
FDB_begin(FDB *self)
{
    if (!tcfdbtranbegin(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.commit() */
PyDoc_STRVAR(FDB_commit_doc,
"commit()\n\
\n\
Commit a transaction.");

static PyObject *
FDB_commit(FDB *self)
{
    if (!tcfdbtrancommit(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.abort() */
PyDoc_STRVAR(FDB_abort_doc,
"abort()\n\
\n\
Abort a transaction.");

static PyObject *
FDB_abort(FDB *self)
{
    if (!tcfdbtranabort(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.get(key) */
PyDoc_STRVAR(FDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
FDB_get(FDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return FDB_GetItem(self, pykey);
}


/* FDB.remove(key) */
PyDoc_STRVAR(FDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
FDB_remove(FDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (FDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* FDB.put(key, value) */
PyDoc_STRVAR(FDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
FDB_put(FDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (FDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* FDB.putkeep(key, value) */
PyDoc_STRVAR(FDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (fdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
FDB_putkeep(FDB *self, PyObject *args)
{
    long long key;
    void *value;
    int value_size;
    PyObject *pyvalue;

    if (!PyArg_ParseTuple(args, "LO:putkeep", &key, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcfdbputkeep(self->fdb, key, value, value_size)) {
        return set_fdb_error(self->fdb, key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* FDB.putcat(key, value) */
PyDoc_STRVAR(FDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
FDB_putcat(FDB *self, PyObject *args)
{
    long long key;
    void *value;
    int value_size;
    PyObject *pyvalue;

    if (!PyArg_ParseTuple(args, "LO:putcat", &key, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcfdbputcat(self->fdb, key, value, value_size)) {
        return set_fdb_error(self->fdb, 0);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* FDB.sync() */
PyDoc_STRVAR(FDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database file?");

static PyObject *
FDB_sync(FDB *self)
{
    if (!tcfdbsync(self->fdb)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.range([lower=FDBIDMIN[, upper=FDBIDMAX[, max=-1]]]) -> frozenset */
PyDoc_STRVAR(FDB_range_doc,
"range([lower=FDBIDMIN[, upper=FDBIDMAX[, max=-1]]]) -> frozenset\n\
\n\
.");

static PyObject *
FDB_range(FDB *self, PyObject *args, PyObject *kwargs)
{
    long long lower = FDBIDMIN, upper = FDBIDMAX;
    int max = -1, result_size;
    uint64_t *result;
    PyObject *pyresult;

    static char *kwlist[] = {"lower", "upper", "max", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|LLi:range", kwlist,
                                     &lower, &upper, &max)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcfdbrange(self->fdb, lower, upper, max, &result_size);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_fdb_error(self->fdb, 0);
    }
    pyresult = ids_to_frozenset(result, result_size);
    tcfree(result);
    return pyresult;
}


/* FDB.optimize([width=0[, size=0]]) */
PyDoc_STRVAR(FDB_optimize_doc,
"optimize([width=0[, size=0]])\n\
\n\
Optimize a database.\n\
'width': the max lenght (in bytes) of the value of each record. If specified as\n\
         0 or as a negative value, the current setting is kept.\n\
'size': the max size (in bytes) of the database file. If specified as 0 or as\n\
        a negative value, the current setting is kept.\n\
\n\
Note:\n\
Optimizing a read only database, or during a transaction, is an invalid operation.");

static PyObject *
FDB_optimize(FDB *self, PyObject *args, PyObject *kwargs)
{
    long width = 0;
    long long size = 0;

    static char *kwlist[] = {"width", "size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|lL:optimize", kwlist,
                                     &width, &size)) {
        return NULL;
    }
    if (!tcfdboptimize(self->fdb, width, size)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.tune(width, size) */
PyDoc_STRVAR(FDB_tune_doc,
"tune(width, size)\n\
\n\
Tune a database.\n\
'width': the max lenght (in bytes) of the value of each record. If specified as\n\
         0 or as a negative value, the default value (255) is used.\n\
'size': the max size (in bytes) of the database file. If specified as 0 or as a\n\
        negative value, the default value (268435456) is used.\n\
\n\
Note:\n\
Tuning an open database is an invalid operation.");

static PyObject *
FDB_tune(FDB *self, PyObject *args)
{
    long width;
    long long size;

    if (!PyArg_ParseTuple(args, "lL:tune", &width, &size)) {
        return NULL;
    }
    if (!tcfdbtune(self->fdb, width, size)) {
        return set_fdb_error(self->fdb, 0);
    }
    Py_RETURN_NONE;
}


/* FDB.addint(key, num) -> int */
PyDoc_STRVAR(FDB_addint_doc,
"addint(key, num) -> int\n\
\n\
Stores an int in the database.\n\
If key is not in the database, this method stores num in the database and\n\
returns it. If key is already in the database, then it will add num to its\n\
current value and return the result.\n\
\n\
Note:\n\
If key exists but its value cannot be treated as an int this method raises\n\
KeyError.");

static PyObject *
FDB_addint(FDB *self, PyObject *args)
{
    long long key;
    int num, result, ecode;

    if (!PyArg_ParseTuple(args, "Li:addint", &key, &num)) {
        return NULL;
    }
    result = tcfdbaddint(self->fdb, key, num);
    if (result == INT_MIN) {
        ecode = tcfdbecode(self->fdb);
        if (ecode != TCESUCCESS && ecode != TCENOREC) {
            return set_fdb_error(self->fdb, key);
        }
    }
    if (num) {
        self->changed = true;
    }
    return PyInt_FromLong((long)result);
}


/* FDB.adddouble(key, num) -> float */
PyDoc_STRVAR(FDB_adddouble_doc,
"adddouble(key, num) -> float\n\
\n\
Stores a float in the database.\n\
If key is not in the database, this method stores num in the database and\n\
returns it. If key is already in the database, then it will add num to its\n\
current value and return the result.\n\
\n\
Note:\n\
If key exists but its value cannot be treated as a float this method raises\n\
KeyError.");

static PyObject *
FDB_adddouble(FDB *self, PyObject *args)
{
    long long key;
    double num, result;

    if (!PyArg_ParseTuple(args, "Ld:adddouble", &key, &num)) {
        return NULL;
    }
    result = tcfdbadddouble(self->fdb, key, num);
    if (Py_IS_NAN(result)) {
        return set_fdb_error(self->fdb, key);
    }
    if (num) {
        self->changed = true;
    }
    return PyFloat_FromDouble(result);
}


/* FDB.iterkeys() */
PyDoc_STRVAR(FDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
FDB_iterkeys(FDB *self)
{
    return new_FDBIter(self, &FDBIterKeysType);
}


/* FDB.itervalues() */
PyDoc_STRVAR(FDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
FDB_itervalues(FDB *self)
{
    return new_FDBIter(self, &FDBIterValuesType);
}


/* FDB.iteritems() */
PyDoc_STRVAR(FDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
FDB_iteritems(FDB *self)
{
    return new_FDBIter(self, &FDBIterItemsType);
}


/* FDBType.tp_methods */
static PyMethodDef FDB_tp_methods[] = {
    {"open", (PyCFunction)FDB_open, METH_VARARGS, FDB_open_doc},
    {"close", (PyCFunction)FDB_close, METH_NOARGS, FDB_close_doc},
    {"clear", (PyCFunction)FDB_clear, METH_NOARGS, FDB_clear_doc},
    {"copy", (PyCFunction)FDB_copy, METH_VARARGS, FDB_copy_doc},
    {"begin", (PyCFunction)FDB_begin, METH_NOARGS, FDB_begin_doc},
    {"commit", (PyCFunction)FDB_commit, METH_NOARGS, FDB_commit_doc},
    {"abort", (PyCFunction)FDB_abort, METH_NOARGS, FDB_abort_doc},
    {"get", (PyCFunction)FDB_get, METH_VARARGS, FDB_get_doc},
    {"remove", (PyCFunction)FDB_remove, METH_VARARGS, FDB_remove_doc},
    {"put", (PyCFunction)FDB_put, METH_VARARGS, FDB_put_doc},
    {"putkeep", (PyCFunction)FDB_putkeep, METH_VARARGS, FDB_putkeep_doc},
    {"putcat", (PyCFunction)FDB_putcat, METH_VARARGS, FDB_putcat_doc},
    {"sync", (PyCFunction)FDB_sync, METH_NOARGS, FDB_sync_doc},
    {"range", (PyCFunction)FDB_range, METH_VARARGS | METH_KEYWORDS,
     FDB_range_doc},
    {"optimize", (PyCFunction)FDB_optimize, METH_VARARGS | METH_KEYWORDS,
     FDB_optimize_doc},
    {"tune", (PyCFunction)FDB_tune, METH_VARARGS, FDB_tune_doc},
    {"addint", (PyCFunction)FDB_addint, METH_VARARGS, FDB_addint_doc},
    {"adddouble", (PyCFunction)FDB_adddouble, METH_VARARGS, FDB_adddouble_doc},
    {"iterkeys", (PyCFunction)FDB_iterkeys, METH_NOARGS, FDB_iterkeys_doc},
    {"itervalues", (PyCFunction)FDB_itervalues, METH_NOARGS, FDB_itervalues_doc},
    {"iteritems", (PyCFunction)FDB_iteritems, METH_NOARGS, FDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* FDB.path */
PyDoc_STRVAR(FDB_path_doc,
"The path to the database file.");

static PyObject *
FDB_path_get(FDB *self, void *closure)
{
    const char *path;

    path = tcfdbpath(self->fdb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* FDB.size */
PyDoc_STRVAR(FDB_size_doc,
"The size in bytes of the database file.");

static PyObject *
FDB_size_get(FDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcfdbfsiz(self->fdb));
}


/* FDBType.tp_getsets */
static PyGetSetDef FDB_tp_getsets[] = {
    {"path", (getter)FDB_path_get, NULL, FDB_path_doc, NULL},
    {"size", (getter)FDB_size_get, NULL, FDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* FDBType */
static PyTypeObject FDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.FDB",                      /*tp_name*/
    sizeof(FDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)FDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &FDB_tp_as_sequence,                      /*tp_as_sequence*/
    &FDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    FDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)FDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    FDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    FDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    FDB_tp_new,                               /*tp_new*/
};
