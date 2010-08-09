/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_rdb_error(TCRDB *rdb, const char *key)
{
    int ecode;

    ecode = tcrdbecode(rdb);
    if (key && ((ecode == TTENOREC) || (ecode == TTEKEEP))) {
        return set_key_error(key);
    }
    return set_error(Error, tcrdberrmsg(ecode));
}


/* convert a dict to a string
   {"key1": "value1", "key2": "value2"} -> "key1=value1#key2=value2" */
char *
dict_to_str(PyObject *kwargs)
{
    PyObject *pykey, *pyvalue;
    Py_ssize_t len, pos = 0;
    const char *key, *value;
    TCLIST *item, *items;
    char *result;

    len = PyDict_Size(kwargs);
    /* not really needed, but hey... */
    if (check_py_ssize_t_len(len, kwargs)) {
        return NULL;
    }
    items = tclistnew2((int)len);
    if (!items) {
        set_error(Error, "could not create TCLIST, memory issue?");
        return NULL;
    }
    while (PyDict_Next(kwargs, &pos, &pykey, &pyvalue)) {
        key = PyString_AsString(pykey);
        value = PyString_AsString(pyvalue);
        if (!(key && value)) {
            tclistdel(items);
            return NULL;
        }
        item = tclistnew3(key, value, NULL);
        if (!item) {
            tclistdel(items);
            set_error(Error, "could not create TCLIST, memory issue?");
            return NULL;
        }
        tclistpush2(items, tcstrjoin(item, '='));
        tclistdel(item);
    }
    result = tcstrjoin(items, '#');
    tclistdel(items);
    return result;
}


/* convert a TSV formatted string to a dict
   "key1\tvalue1\nkey2\tvalue2\n" -> {"key1": "value1", "key2": "value2"} */
int
tsv_to_dict(const char *status, PyObject *pystatus)
{
    TCLIST *items, *item;
    int len, i;
    PyObject *pyvalue;

    items = tcstrsplit(status, "\n");
    len = tclistnum(items);
    for (i = 0; i < len; i++) {
        item = tcstrsplit(tclistval2(items, i), "\t");
        if (tclistnum(item) == 2) {
            pyvalue = PyString_FromString(tclistval2(item, 1));
            if (!pyvalue) {
                tclistdel(item);
                tclistdel(items);
                return -1;
            }
            if (PyDict_SetItemString(pystatus, tclistval2(item, 0), pyvalue)) {
                Py_DECREF(pyvalue);
                tclistdel(item);
                tclistdel(items);
                return -1;
            }
            Py_DECREF(pyvalue);
        }
        tclistdel(item);
    }
    tclistdel(items);
    return 0;
}


const char *
rdb_get_status(TCRDB *rdb)
{
    const char *status;

    Py_BEGIN_ALLOW_THREADS
    status = tcrdbstat(rdb);
    Py_END_ALLOW_THREADS
    return status;
}


/*******************************************************************************
* RDBBase iterator types
*******************************************************************************/

/* new_RDBBaseIter */
static PyObject *
new_RDBBaseIter(RDBBase *self, PyTypeObject *type)
{
    PyObject *iter;
    bool result;

    iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbiterinit(self->rdb);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    self->changed = false;
    return iter;
}


/* RDBBaseIterKeysType.tp_iternext */
static PyObject *
RDBBaseIterKeys_tp_iternext(DBIter *self)
{
    RDBBase *rdbbase = (RDBBase *)self->db;
    void *key;
    int key_size;
    PyObject *pykey;

    if (rdbbase->changed) {
        return set_error(Error, "DB changed during iteration");
    }
    Py_BEGIN_ALLOW_THREADS
    key = tcrdbiternext(rdbbase->rdb, &key_size);
    Py_END_ALLOW_THREADS
    if (!key) {
        if (tcrdbecode(rdbbase->rdb) == TTENOREC) {
            return set_stopiteration_error();
        }
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    pykey = void_to_bytes(key, key_size);
    tcfree(key);
    return pykey;
}


/* RDBBaseIterKeysType */
static PyTypeObject RDBBaseIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.tyrant.RDBBaseIterKeys",           /*tp_name*/
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
    (iternextfunc)RDBBaseIterKeys_tp_iternext, /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* RDBBaseType
*******************************************************************************/

/* RDBBase_update_status */
int
RDBBase_update_pystatus(RDBBase *self, const char *status)
{
    if (tsv_to_dict(status, self->pystatus)) {
        tcfree((void *)status);
        return -1;
    }
    tcfree((void *)status);
    return 0;
}


/* RDBBase_open */
const char *
RDBBase_open(RDBBase *self, PyObject *args, PyObject *kwargs)
{
    const char *host = "localhost", *status, *dbtype;
    int port = 1978;
    bool result;
    PyObject *pydbtype;

    static char *kwlist[] = {"host", "port", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|si:open", kwlist,
                                     &host, &port)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbopen(self->rdb, host, port);
    Py_END_ALLOW_THREADS
    if (!result) {
        set_rdb_error(self->rdb, NULL);
        return NULL;
    }
    status = rdb_get_status(self->rdb);
    if (!status) {
        set_error(Error, "could not get db status");
        return NULL;
    }
    if (RDBBase_update_pystatus(self, status)) {
        return NULL;
    }
    pydbtype = PyDict_GetItemString(self->pystatus, "type");
    if (!pydbtype) {
        set_error(Error, "could not get db type");
        return NULL;
    }
    dbtype = PyString_AsString(pydbtype);
    if (!dbtype) {
        return NULL;
    }
    if (!strcmp(dbtype, "fixed-length")) {
        set_error(PyExc_NotImplementedError,
                  "fixed-length databases are not supported");
        return NULL;
    }
    return dbtype;
}


/* RDB/RTDB_tp_as_mapping.mp_length */
static Py_ssize_t
RDBBase_Length(RDBBase *self)
{
    unsigned long long len;

    Py_BEGIN_ALLOW_THREADS
    len = tcrdbrnum(self->rdb);
    Py_END_ALLOW_THREADS
    return DB_Length(len);
}


/* RDBBaseType.tp_traverse */
static int
RDBBase_tp_traverse(RDBBase *self, visitproc visit, void *arg)
{
    Py_VISIT(self->pystatus);
    return 0;
}


/* RDBBaseType.tp_clear */
static int
RDBBase_tp_clear(RDBBase *self)
{
    Py_CLEAR(self->pystatus);
    return 0;
}


/* RDBBaseType.tp_dealloc */
static void
RDBBase_tp_dealloc(RDBBase *self)
{
    RDBBase_tp_clear(self);
    if (self->rdb) {
        tcrdbdel(self->rdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* RDBBaseType.tp_new */
static PyObject *
RDBBase_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    RDBBase *self = (RDBBase *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->rdb */
    self->rdb = tcrdbnew();
    if (!self->rdb) {
        set_error(Error, "could not create DB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    /* self->pystatus */
    self->pystatus = PyDict_New();
    if (!self->pystatus) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}

/* RDBBaseType.tp_iter */
static PyObject *
RDBBase_tp_iter(RDBBase *self)
{
    return new_RDBBaseIter(self, &RDBBaseIterKeysType);
}


/* RDBBase.close() */
PyDoc_STRVAR(RDBBase_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
DBs are closed when garbage-collected.");

static PyObject *
RDBBase_close(RDBBase *self)
{
    bool result;

    Py_BEGIN_ALLOW_THREADS
    result = tcrdbclose(self->rdb);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.clear() */
PyDoc_STRVAR(RDBBase_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
RDBBase_clear(RDBBase *self)
{
    bool result;

    Py_BEGIN_ALLOW_THREADS
    result = tcrdbvanish(self->rdb);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* RDBBase.copy(path) */
PyDoc_STRVAR(RDBBase_copy_doc,
"copy(path)\n\
\n\
Copy the database file.\n\
'path': path to the destination file.");

static PyObject *
RDBBase_copy(RDBBase *self, PyObject *args)
{
    const char *path;
    bool result;

    if (!PyArg_ParseTuple(args, "s:copy", &path)) {
        return NULL;
    }
    if (*path == '@') {
        /* disable this feature until I find out more */
        return set_error(PyExc_NotImplementedError,
                         "this feature is not supported");
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbcopy(self->rdb, path);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.sync() */
PyDoc_STRVAR(RDBBase_sync_doc,
"sync()\n\
\n\
Flush modifications to the database file?");

static PyObject *
RDBBase_sync(RDBBase *self)
{
    bool result;

    Py_BEGIN_ALLOW_THREADS
    result = tcrdbsync(self->rdb);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(RDBBase_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
RDBBase_searchkeys(RDBBase *self, PyObject *args)
{
    void *prefix;
    int prefix_size, max = -1;
    TCLIST *result;
    PyObject *pyprefix, *pyresult;

    if (!PyArg_ParseTuple(args, "O|i:searchkeys", &pyprefix, &max)) {
        return NULL;
    }
    if (bytes_to_void(pyprefix, &prefix, &prefix_size)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbfwmkeys(self->rdb, prefix, prefix_size, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* RDBBase.optimize(**kwargs) */
PyDoc_STRVAR(RDBBase_optimize_doc,
"optimize(**kwargs)\n\
\n\
Optimize a database.");

static PyObject *
RDBBase_optimize(RDBBase *self, PyObject *args, PyObject *kwargs)
{
    const char *params = NULL;
    bool result;

    if (!PyArg_ParseTuple(args, ";optimize() takes only keyword arguments")) {
        return NULL;
    }
    if (kwargs) {
        params = dict_to_str(kwargs);
        if (!params) {
            return NULL;
        }
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdboptimize(self->rdb, params);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.tune(timeout, opts) */
PyDoc_STRVAR(RDBBase_tune_doc,
"tune(timeout, opts)\n\
\n\
Tune a database.\n\
'timeout': timeout in seconds. If specified as 0 or as a negative value, no\n\
           timeout is applied.\n\
'opts': TODO (0).\n\
\n\
Note:\n\
Tuning an open database is an invalid operation.");

static PyObject *
RDBBase_tune(RDBBase *self, PyObject *args)
{
    double timeout;
    int opts;

    if (!PyArg_ParseTuple(args, "di:tune", &timeout, &opts)) {
        return NULL;
    }
    if (!tcrdbtune(self->rdb, timeout, opts)) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.restore(path, timestamp, opts) */
PyDoc_STRVAR(RDBBase_restore_doc,
"restore(path, timestamp, opts)\n\
\n\
Restore the database from update log.\n\
'path': path to the update log directory.\n\
'timestamp': starting timestamp in microseconds.\n\
'opts': TODO.");

static PyObject *
RDBBase_restore(RDBBase *self, PyObject *args)
{
    const char *path;
    unsigned long long ts;
    int opts;
    bool result;

    if (!PyArg_ParseTuple(args, "sKi:restore", &path, &ts, &opts)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbrestore(self->rdb, path, ts, opts);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.setmaster(host, port, timestamp, opts) */
PyDoc_STRVAR(RDBBase_setmaster_doc,
"setmaster(host, port, timestamp, opts)\n\
\n\
Set the replication master of a database.\n\
'host': hostname/address.\n\
'port': port number.\n\
'timestamp': starting timestamp in microseconds.\n\
'opts': TODO.");

static PyObject *
RDBBase_setmaster(RDBBase *self, PyObject *args)
{
    const char *host;
    int port, opts;
    unsigned long long ts;
    bool result;

    if (!PyArg_ParseTuple(args, "siKi:setmaster", &host, &port, &ts, &opts)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbsetmst(self->rdb, host, port, ts, opts);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(self->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RDBBase.iterkeys() */
PyDoc_STRVAR(RDBBase_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
RDBBase_iterkeys(RDBBase *self)
{
    return new_RDBBaseIter(self, &RDBBaseIterKeysType);
}


/* RDBBaseType.tp_methods */
static PyMethodDef RDBBase_tp_methods[] = {
    {"close", (PyCFunction)RDBBase_close, METH_NOARGS, RDBBase_close_doc},
    {"clear", (PyCFunction)RDBBase_clear, METH_NOARGS, RDBBase_clear_doc},
    {"copy", (PyCFunction)RDBBase_copy, METH_VARARGS, RDBBase_copy_doc},
    {"sync", (PyCFunction)RDBBase_sync, METH_NOARGS, RDBBase_sync_doc},
    {"searchkeys", (PyCFunction)RDBBase_searchkeys, METH_VARARGS,
     RDBBase_searchkeys_doc},
    {"optimize", (PyCFunction)RDBBase_optimize, METH_VARARGS | METH_KEYWORDS,
     RDBBase_optimize_doc},
    {"tune", (PyCFunction)RDBBase_tune, METH_VARARGS, RDBBase_tune_doc},
    {"restore", (PyCFunction)RDBBase_restore, METH_VARARGS, RDBBase_restore_doc},
    {"setmaster", (PyCFunction)RDBBase_setmaster, METH_VARARGS,
     RDBBase_setmaster_doc},
    {"iterkeys", (PyCFunction)RDBBase_iterkeys, METH_NOARGS,
     RDBBase_iterkeys_doc},
    {NULL}  /* Sentinel */
};


/* RDBBase.size */
PyDoc_STRVAR(RDBBase_size_doc,
"The size in bytes of the database file.");

static PyObject *
RDBBase_size_get(RDBBase *self, void *closure)
{
    unsigned long long size;

    Py_BEGIN_ALLOW_THREADS
    size = tcrdbsize(self->rdb);
    Py_END_ALLOW_THREADS
    return PyLong_FromUnsignedLongLong(size);
}


/* RDBBase.status */
PyDoc_STRVAR(RDBBase_status_doc,
"status.");

static PyObject *
RDBBase_status_get(RDBBase *self, void *closure)
{
    const char *status;

    status = rdb_get_status(self->rdb);
    if (!status) {
        Py_RETURN_NONE;
    }
    if (RDBBase_update_pystatus(self, status)) {
        return NULL;
    }
    Py_INCREF(self->pystatus);
    return self->pystatus;
}


/* RDBBaseType.tp_getsets */
static PyGetSetDef RDBBase_tp_getsets[] = {
    {"size", (getter)RDBBase_size_get, NULL, RDBBase_size_doc, NULL},
    {"status", (getter)RDBBase_status_get, NULL, RDBBase_status_doc, NULL},
    {NULL}  /* Sentinel */
};


/* RDBBaseType */
static PyTypeObject RDBBaseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.tyrant.RDBBase",                   /*tp_name*/
    sizeof(RDBBase),                          /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)RDBBase_tp_dealloc,           /*tp_dealloc*/
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
    (traverseproc)RDBBase_tp_traverse,        /*tp_traverse*/
    (inquiry)RDBBase_tp_clear,                /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)RDBBase_tp_iter,             /*tp_iter*/
    0,                                        /*tp_iternext*/
    RDBBase_tp_methods,                       /*tp_methods*/
    0,                                        /*tp_members*/
    RDBBase_tp_getsets,                       /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    RDBBase_tp_new,                           /*tp_new*/
};
