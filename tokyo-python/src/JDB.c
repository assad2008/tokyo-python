/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_jdb_error(TCJDB *jdb, long long key)
{
    int ecode;

    ecode = tcjdbecode(jdb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return PyErr_Format(PyExc_KeyError, "%ld", (long)key);
    }
    return set_error(Error, tcjdberrmsg(ecode));
}


/* convert a Python sequence to a TCLIST */
TCLIST *
utf8_seq_to_tclist(PyObject *pyvalues)
{
    const char *msg = "a sequence is required";
    PyObject *pyseq;
    Py_ssize_t len, i;
    TCLIST *values;
    char *value;

    if (PyBytes_Check(pyvalues) || PyUnicode_Check(pyvalues)) {
        set_error(PyExc_TypeError, msg);
        return NULL;
    }
    pyseq = PySequence_Fast(pyvalues, msg);
    if (!pyseq) {
        return NULL;
    }
    len = PySequence_Fast_GET_SIZE(pyseq);
    if (check_py_ssize_t_len(len, pyseq)) {
        Py_DECREF(pyseq);
        return NULL;
    }
    values = tclistnew2((int)len);
    if (!values) {
        set_error(Error, "could not create TCLIST, memory issue?");
        Py_DECREF(pyseq);
        return NULL;
    }
    for (i = 0; i < len; i++) {
        value = PyUnicode_AsString(PySequence_Fast_GET_ITEM(pyseq, i));
        if (!value) {
            Py_DECREF(pyseq);
            tclistdel(values);
            return NULL;
        }
        tclistpush2(values, value);
    }
    Py_DECREF(pyseq);
    return values;
}


/* convert a TCLIST to a tuple */
PyObject *
utf8_tclist_to_tuple(TCLIST *result)
{
    const char *value;
    int len, i;
    PyObject *pyresult, *pyvalue;

    len = tclistnum(result);
    pyresult = PyTuple_New((Py_ssize_t)len);
    if (!pyresult) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        value = tclistval2(result, i);
        pyvalue = PyUnicode_FromString(value);
        if (!pyvalue) {
            Py_DECREF(pyresult);
            return NULL;
        }
        PyTuple_SET_ITEM(pyresult, (Py_ssize_t)i, pyvalue);
    }
    return pyresult;
}


/*******************************************************************************
* JDB iterator types
*******************************************************************************/

/* new_JDBIter */
static PyObject *
new_JDBIter(JDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tcjdbiterinit(self->jdb)) {
        Py_DECREF(iter);
        return set_jdb_error(self->jdb, 0);
    }
    self->changed = false;
    return iter;
}


/* JDBIterKeysType.tp_iternext */
static PyObject *
JDBIterKeys_tp_iternext(DBIter *self)
{
    JDB *jdb = (JDB *)self->db;
    long long key;
    PyObject *pykey;

    if (jdb->changed) {
        return set_error(Error, "JDB changed during iteration");
    }
    key = uint64_to_int64(tcjdbiternext(jdb->jdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcjdbecode(jdb->jdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_jdb_error(jdb->jdb, 0);
    }
    pykey = PyLong_FromLongLong(key);
    return pykey;
}


/* JDBIterKeysType */
static PyTypeObject JDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.JDBIterKeys",             /*tp_name*/
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
    (iternextfunc)JDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* JDBIterValuesType.tp_iternext */
static PyObject *
JDBIterValues_tp_iternext(DBIter *self)
{
    JDB *jdb = (JDB *)self->db;
    long long key;
    TCLIST *value;
    PyObject *pyvalue;

    if (jdb->changed) {
        return set_error(Error, "JDB changed during iteration");
    }
    key = uint64_to_int64(tcjdbiternext(jdb->jdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcjdbecode(jdb->jdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_jdb_error(jdb->jdb, 0);
    }
    value = tcjdbget(jdb->jdb, key);
    pyvalue = utf8_tclist_to_tuple(value);
    tclistdel(value);
    return pyvalue;
}


/* JDBIterValuesType */
static PyTypeObject JDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.JDBIterValues",           /*tp_name*/
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
    (iternextfunc)JDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* JDBIterItemsType.tp_iternext */
static PyObject *
JDBIterItems_tp_iternext(DBIter *self)
{
    JDB *jdb = (JDB *)self->db;
    long long key;
    TCLIST *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (jdb->changed) {
        return set_error(Error, "JDB changed during iteration");
    }
    key = uint64_to_int64(tcjdbiternext(jdb->jdb));
    if (key == -1) {
        return NULL;
    }
    else if (!key) {
        if (tcjdbecode(jdb->jdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_jdb_error(jdb->jdb, 0);
    }
    value = tcjdbget(jdb->jdb, key);
    pykey = PyLong_FromLongLong(key);
    pyvalue = utf8_tclist_to_tuple(value);
    if (pykey && pyvalue) {
        pyresult = PyTuple_Pack(2, pykey, pyvalue);
    }
    Py_XDECREF(pykey);
    Py_XDECREF(pyvalue);
    tclistdel(value);
    return pyresult;
}


/* JDBIterItemsType */
static PyTypeObject JDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.JDBIterItems",            /*tp_name*/
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
    (iternextfunc)JDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* JDBType
*******************************************************************************/

/* JDBType.tp_doc */
PyDoc_STRVAR(JDB_tp_doc,
"JDB()\n\
\n\
Tagged Database.\n\
\n\
See also:\n\
Tokyo Dystopia Simple API at:\n\
http://1978th.net/tokyodystopia/spex.html#laputaapi");


/* JDBType.tp_dealloc */
static void
JDB_tp_dealloc(JDB *self)
{
    if (self->jdb) {
        tcjdbdel(self->jdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* JDBType.tp_new */
static PyObject *
JDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    JDB *self = (JDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->jdb */
    self->jdb = tcjdbnew();
    if (!self->jdb) {
        set_error(Error, "could not create JDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* JDB_tp_as_sequence.sq_contains */
static int
JDB_Contains(JDB *self, PyObject *pykey)
{
    long long key;
    TCLIST *value;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return -1;
    }
    value = tcjdbget(self->jdb, key);
    if (!value) {
        if (tcjdbecode(self->jdb) == TCENOREC) {
            return 0;
        }
        set_jdb_error(self->jdb, 0);
        return -1;
    }
    tclistdel(value);
    return 1;
}


/* JDBType.tp_as_sequence */
static PySequenceMethods JDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)JDB_Contains,                 /*sq_contains*/
};


/* JDB_tp_as_mapping.mp_length */
static Py_ssize_t
JDB_Length(JDB *self)
{
    return DB_Length(tcjdbrnum(self->jdb));
}


/* JDB_tp_as_mapping.mp_subscript */
static PyObject *
JDB_GetItem(JDB *self, PyObject *pykey)
{
    long long key;
    TCLIST *value;
    PyObject *pyvalue;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return NULL;
    }
    value = tcjdbget(self->jdb, key);
    if (!value) {
        return set_jdb_error(self->jdb, key);
    }
    pyvalue = utf8_tclist_to_tuple(value);
    tclistdel(value);
    return pyvalue;
}


/* JDB_tp_as_mapping.mp_ass_subscript */
static int
JDB_SetItem(JDB *self, PyObject *pykey, PyObject *pyvalue)
{
    long long key;
    TCLIST *value;

    key = pylong_to_int64(pykey);
    if (key < 1) {
        return -1;
    }
    if (pyvalue) {
        value = utf8_seq_to_tclist(pyvalue);
        if (!value) {
            return -1;
        }
        if (!tcjdbput(self->jdb, key, value)) {
            tclistdel(value);
            set_jdb_error(self->jdb, 0);
            return -1;
        }
        tclistdel(value);
    }
    else {
        if (!tcjdbout(self->jdb, key)) {
            set_jdb_error(self->jdb, key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* JDBType.tp_as_mapping */
static PyMappingMethods JDB_tp_as_mapping = {
    (lenfunc)JDB_Length,                      /*mp_length*/
    (binaryfunc)JDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)JDB_SetItem                /*mp_ass_subscript*/
};


/* JDBType.tp_iter */
static PyObject *
JDB_tp_iter(JDB *self)
{
    return new_JDBIter(self, &JDBIterKeysType);
}


/* JDB.open(path, mode) */
PyDoc_STRVAR(JDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database directory.\n\
'mode': connection mode.");

static PyObject *
JDB_open(JDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tcjdbopen(self->jdb, path, mode)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.close() */
PyDoc_STRVAR(JDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
JDBs are closed when garbage-collected.");

static PyObject *
JDB_close(JDB *self)
{
    if (!tcjdbclose(self->jdb)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.clear() */
PyDoc_STRVAR(JDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
JDB_clear(JDB *self)
{
    if (!tcjdbvanish(self->jdb)) {
        return set_jdb_error(self->jdb, 0);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* JDB.copy(path) */
PyDoc_STRVAR(JDB_copy_doc,
"copy(path)\n\
\n\
Copy the database.\n\
'path': path to the destination directory.");

static PyObject *
JDB_copy(JDB *self, PyObject *args)
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
    if (!tcjdbcopy(self->jdb, path)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.get(key) */
PyDoc_STRVAR(JDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
JDB_get(JDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return JDB_GetItem(self, pykey);
}


/* JDB.remove(key) */
PyDoc_STRVAR(JDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
JDB_remove(JDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (JDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* JDB.put(key, value) */
PyDoc_STRVAR(JDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
JDB_put(JDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (JDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* JDB.sync() */
PyDoc_STRVAR(JDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database.");

static PyObject *
JDB_sync(JDB *self)
{
    if (!tcjdbsync(self->jdb)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.search(expr[, mode]) -> frozenset */
PyDoc_STRVAR(JDB_search_doc,
"search(expr[, mode]) -> frozenset\n\
\n\
.");

static PyObject *
JDB_search(JDB *self, PyObject *args, PyObject *kwargs)
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
        result = tcjdbsearch2(self->jdb, expr, &result_size);
    }
    else {
        result = tcjdbsearch(self->jdb, expr, mode, &result_size);
    }
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_jdb_error(self->jdb, 0);
    }
    pyresult = ids_to_frozenset(result, result_size);
    tcfree(result);
    return pyresult;
}


/* JDB.optimize() */
PyDoc_STRVAR(JDB_optimize_doc,
"optimize()\n\
\n\
Optimize a database.\n\
\n\
Note:\n\
Optimizing a read only database is an invalid operation.");

static PyObject *
JDB_optimize(JDB *self)
{
    if (!tcjdboptimize(self->jdb)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.tune(ernum, etnum, iusiz, opts) */
PyDoc_STRVAR(JDB_tune_doc,
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
JDB_tune(JDB *self, PyObject *args)
{
    long long ernum, etnum, iusiz;
    unsigned char opts;

    if (!PyArg_ParseTuple(args, "LLLb:tune", &ernum, &etnum, &iusiz, &opts)) {
        return NULL;
    }
    if (!tcjdbtune(self->jdb, ernum, etnum, iusiz, opts)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.setcache(icsiz, lcnum) */
PyDoc_STRVAR(JDB_setcache_doc,
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
JDB_setcache(JDB *self, PyObject *args)
{
    long long icsiz;
    long lcnum;

    if (!PyArg_ParseTuple(args, "Ll:setcache", &icsiz, &lcnum)) {
        return NULL;
    }
    if (!tcjdbsetcache(self->jdb, icsiz, lcnum)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.setfwmmax(fwmmax) */
PyDoc_STRVAR(JDB_setfwmmax_doc,
"setfwmmax(fwmmax)\n\
\n\
Set the maximum number of forward matching expansion.\n\
'fwmmax': the maximum number of forward matching expansion.\n\
\n\
Note:\n\
Setting this on an open database is an invalid operation.");

static PyObject *
JDB_setfwmmax(JDB *self, PyObject *args)
{
    long fwmmax;

    if (!PyArg_ParseTuple(args, "l:setfwmmax", &fwmmax)) {
        return NULL;
    }
    if (!tcjdbsetfwmmax(self->jdb, fwmmax)) {
        return set_jdb_error(self->jdb, 0);
    }
    Py_RETURN_NONE;
}


/* JDB.iterkeys() */
PyDoc_STRVAR(JDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
JDB_iterkeys(JDB *self)
{
    return new_JDBIter(self, &JDBIterKeysType);
}


/* JDB.itervalues() */
PyDoc_STRVAR(JDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
JDB_itervalues(JDB *self)
{
    return new_JDBIter(self, &JDBIterValuesType);
}


/* JDB.iteritems() */
PyDoc_STRVAR(JDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
JDB_iteritems(JDB *self)
{
    return new_JDBIter(self, &JDBIterItemsType);
}


/* JDBType.tp_methods */
static PyMethodDef JDB_tp_methods[] = {
    {"open", (PyCFunction)JDB_open, METH_VARARGS, JDB_open_doc},
    {"close", (PyCFunction)JDB_close, METH_NOARGS, JDB_close_doc},
    {"clear", (PyCFunction)JDB_clear, METH_NOARGS, JDB_clear_doc},
    {"copy", (PyCFunction)JDB_copy, METH_VARARGS, JDB_copy_doc},
    {"get", (PyCFunction)JDB_get, METH_VARARGS, JDB_get_doc},
    {"remove", (PyCFunction)JDB_remove, METH_VARARGS, JDB_remove_doc},
    {"put", (PyCFunction)JDB_put, METH_VARARGS, JDB_put_doc},
    {"sync", (PyCFunction)JDB_sync, METH_NOARGS, JDB_sync_doc},
    {"search", (PyCFunction)JDB_search, METH_VARARGS, JDB_search_doc},
    {"optimize", (PyCFunction)JDB_optimize, METH_NOARGS, JDB_optimize_doc},
    {"tune", (PyCFunction)JDB_tune, METH_VARARGS, JDB_tune_doc},
    {"setcache", (PyCFunction)JDB_setcache, METH_VARARGS, JDB_setcache_doc},
    {"setfwmmax", (PyCFunction)JDB_setfwmmax, METH_VARARGS, JDB_setfwmmax_doc},
    {"iterkeys", (PyCFunction)JDB_iterkeys, METH_NOARGS, JDB_iterkeys_doc},
    {"itervalues", (PyCFunction)JDB_itervalues, METH_NOARGS, JDB_itervalues_doc},
    {"iteritems", (PyCFunction)JDB_iteritems, METH_NOARGS, JDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* JDB.path */
PyDoc_STRVAR(JDB_path_doc,
"The path to the database directory.");

static PyObject *
JDB_path_get(JDB *self, void *closure)
{
    const char *path;

    path = tcjdbpath(self->jdb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* JDB.size */
PyDoc_STRVAR(JDB_size_doc,
"The size in bytes of the database.");

static PyObject *
JDB_size_get(JDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcjdbfsiz(self->jdb));
}


/* JDBType.tp_getsets */
static PyGetSetDef JDB_tp_getsets[] = {
    {"path", (getter)JDB_path_get, NULL, JDB_path_doc, NULL},
    {"size", (getter)JDB_size_get, NULL, JDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* JDBType */
static PyTypeObject JDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.dystopia.JDB",                     /*tp_name*/
    sizeof(JDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)JDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &JDB_tp_as_sequence,                      /*tp_as_sequence*/
    &JDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    JDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)JDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    JDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    JDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    JDB_tp_new,                               /*tp_new*/
};
