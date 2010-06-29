/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_hdb_error(TCHDB *hdb, const char *key)
{
    int ecode;

    ecode = tchdbecode(hdb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return set_key_error(key);
    }
    return set_error(Error, tchdberrmsg(ecode));
}


/*******************************************************************************
* HDB iterator types
*******************************************************************************/

/* new_HDBIter */
static PyObject *
new_HDBIter(HDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tchdbiterinit(self->hdb)) {
        Py_DECREF(iter);
        return set_hdb_error(self->hdb, NULL);
    }
    self->changed = false;
    return iter;
}


/* HDBIterKeysType.tp_iternext */
static PyObject *
HDBIterKeys_tp_iternext(DBIter *self)
{
    HDB *hdb = (HDB *)self->db;
    const char *key;
    PyObject *pykey;

    if (hdb->changed) {
        return set_error(Error, "HDB changed during iteration");
    }
    key = tchdbiternext2(hdb->hdb);
    if (!key) {
        if (tchdbecode(hdb->hdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_hdb_error(hdb->hdb, NULL);
    }
    pykey = PyBytes_FromString(key);
    tcfree((void *)key);
    return pykey;
}


/* HDBIterKeysType */
static PyTypeObject HDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.HDBIterKeys",              /*tp_name*/
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
    (iternextfunc)HDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* HDBIterValuesType.tp_iternext */
static PyObject *
HDBIterValues_tp_iternext(DBIter *self)
{
    HDB *hdb = (HDB *)self->db;
    TCXSTR *key, *value;
    PyObject *pyvalue = NULL;

    if (hdb->changed) {
        return set_error(Error, "HDB changed during iteration");
    }
    key = tcxstrnew();
    value = tcxstrnew();
    if (!tchdbiternext3(hdb->hdb, key, value)) {
        if (tchdbecode(hdb->hdb) == TCENOREC) {
            set_stopiteration_error();
        }
        else {
            set_hdb_error(hdb->hdb, NULL);
        }
    }
    else {
        pyvalue = PyBytes_FromString((char *)tcxstrptr(value));
    }
    tcxstrdel(key);
    tcxstrdel(value);
    return pyvalue;
}


/* HDBIterValuesType */
static PyTypeObject HDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.HDBIterValues",            /*tp_name*/
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
    (iternextfunc)HDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* HDBIterItemsType.tp_iternext */
static PyObject *
HDBIterItems_tp_iternext(DBIter *self)
{
    HDB *hdb = (HDB *)self->db;
    TCXSTR *key, *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (hdb->changed) {
        return set_error(Error, "HDB changed during iteration");
    }
    key = tcxstrnew();
    value = tcxstrnew();
    if (!tchdbiternext3(hdb->hdb, key, value)) {
        if (tchdbecode(hdb->hdb) == TCENOREC) {
            set_stopiteration_error();
        }
        else {
            set_hdb_error(hdb->hdb, NULL);
        }
    }
    else {
        pykey = PyBytes_FromString((char *)tcxstrptr(key));
        pyvalue = PyBytes_FromString((char *)tcxstrptr(value));
        if (pykey && pyvalue) {
            pyresult = PyTuple_Pack(2, pykey, pyvalue);
        }
        Py_XDECREF(pykey);
        Py_XDECREF(pyvalue);
    }
    tcxstrdel(key);
    tcxstrdel(value);
    return pyresult;
}


/* HDBIterItemsType */
static PyTypeObject HDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.HDBIterItems",             /*tp_name*/
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
    (iternextfunc)HDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* HDBType
*******************************************************************************/

/* HDBType.tp_doc */
PyDoc_STRVAR(HDB_tp_doc,
"HDB()\n\
\n\
Hash Database.\n\
\n\
See also:\n\
Tokyo Cabinet Hash Database API at:\n\
http://1978th.net/tokyocabinet/spex-en.html#tchdbapi");


/* HDBType.tp_dealloc */
static void
HDB_tp_dealloc(HDB *self)
{
    if (self->hdb) {
        tchdbdel(self->hdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* HDBType.tp_new */
static PyObject *
HDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    HDB *self = (HDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->hdb */
    self->hdb = tchdbnew();
    if (!self->hdb) {
        set_error(Error, "could not create HDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    if (!tchdbsetmutex(self->hdb)) {
        set_hdb_error(self->hdb, NULL);
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* HDB_tp_as_sequence.sq_contains */
static int
HDB_Contains(HDB *self, PyObject *pykey)
{
    const char *key, *value;

    key = PyBytes_AsString(pykey);
    if (!key) {
        return -1;
    }
    value = tchdbget2(self->hdb, key);
    if (!value) {
        if (tchdbecode(self->hdb) == TCENOREC) {
            return 0;
        }
        set_hdb_error(self->hdb, NULL);
        return -1;
    }
    tcfree((void *)value);
    return 1;
}


/* HDBType.tp_as_sequence */
static PySequenceMethods HDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)HDB_Contains,                 /*sq_contains*/
};


/* HDB_tp_as_mapping.mp_length */
static Py_ssize_t
HDB_Length(HDB *self)
{
    return (Py_ssize_t)tchdbrnum(self->hdb);
}


/* HDB_tp_as_mapping.mp_subscript */
static PyObject *
HDB_GetItem(HDB *self, PyObject *pykey)
{
    const char *key, *value;
    PyObject *pyvalue;

    key = PyBytes_AsString(pykey);
    if (!key) {
        return NULL;
    }
    value = tchdbget2(self->hdb, key);
    if (!value) {
        return set_hdb_error(self->hdb, key);
    }
    pyvalue = PyBytes_FromString(value);
    tcfree((void *)value);
    return pyvalue;
}


/* HDB_tp_as_mapping.mp_ass_subscript */
static int
HDB_SetItem(HDB *self, PyObject *pykey, PyObject *pyvalue)
{
    const char *key, *value;

    key = PyBytes_AsString(pykey);
    if (!key) {
        return -1;
    }
    if (pyvalue) {
        value = PyBytes_AsString(pyvalue);
        if (!value) {
            return -1;
        }
        if (!tchdbput2(self->hdb, key, value)) {
            set_hdb_error(self->hdb, NULL);
            return -1;
        }
    }
    else {
        if (!tchdbout2(self->hdb, key)) {
            set_hdb_error(self->hdb, key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* HDBType.tp_as_mapping */
static PyMappingMethods HDB_tp_as_mapping = {
    (lenfunc)HDB_Length,                      /*mp_length*/
    (binaryfunc)HDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)HDB_SetItem                /*mp_ass_subscript*/
};


/* HDBType.tp_iter */
static PyObject *
HDB_tp_iter(HDB *self)
{
    return new_HDBIter(self, &HDBIterKeysType);
}


/* HDB.__length_hint__ */
PyDoc_STRVAR(HDB_length_hint_doc,
"Private method returning an estimate of len(list(hdb)).");

static PyObject *
HDB_length_hint(HDB *self)
{
    return PyLong_FromSsize_t(HDB_Length(self));
}


/* HDB.open(path, mode) */
PyDoc_STRVAR(HDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database file.\n\
'mode': connection mode.");

static PyObject *
HDB_open(HDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tchdbopen(self->hdb, path, mode)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.close() */
PyDoc_STRVAR(HDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
HDBs are closed when garbage-collected.");

static PyObject *
HDB_close(HDB *self)
{
    if (!tchdbclose(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.clear() */
PyDoc_STRVAR(HDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
HDB_clear(HDB *self)
{
    if (!tchdbvanish(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* HDB.copy(path) */
PyDoc_STRVAR(HDB_copy_doc,
"copy(path)\n\
\n\
Copy the database file.\n\
'path': path to the destination file.");

static PyObject *
HDB_copy(HDB *self, PyObject *args)
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
    if (!tchdbcopy(self->hdb, path)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.begin() */
PyDoc_STRVAR(HDB_begin_doc,
"begin()\n\
\n\
Begin a transaction.");

static PyObject *
HDB_begin(HDB *self)
{
    if (!tchdbtranbegin(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.commit() */
PyDoc_STRVAR(HDB_commit_doc,
"commit()\n\
\n\
Commit a transaction.");

static PyObject *
HDB_commit(HDB *self)
{
    if (!tchdbtrancommit(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.abort() */
PyDoc_STRVAR(HDB_abort_doc,
"abort()\n\
\n\
Abort a transaction.");

static PyObject *
HDB_abort(HDB *self)
{
    if (!tchdbtranabort(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.get(key) */
PyDoc_STRVAR(HDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
HDB_get(HDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return HDB_GetItem(self, pykey);
}


/* HDB.remove(key) */
PyDoc_STRVAR(HDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
HDB_remove(HDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (HDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* HDB.put(key, value) */
PyDoc_STRVAR(HDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
HDB_put(HDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (HDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* HDB.putkeep(key, value) */
PyDoc_STRVAR(HDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (hdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
HDB_putkeep(HDB *self, PyObject *args)
{
    const char *key, *value;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putkeep", &pykey, &pyvalue)) {
        return NULL;
    }
    key = PyBytes_AsString(pykey);
    value = PyBytes_AsString(pyvalue);
    if (!(key && value)) {
        return NULL;
    }
    if (!tchdbputkeep2(self->hdb, key, value)) {
        return set_hdb_error(self->hdb, key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* HDB.putcat(key, value) */
PyDoc_STRVAR(HDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
HDB_putcat(HDB *self, PyObject *args)
{
    const char *key, *value;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putcat", &pykey, &pyvalue)) {
        return NULL;
    }
    key = PyBytes_AsString(pykey);
    value = PyBytes_AsString(pyvalue);
    if (!(key && value)) {
        return NULL;
    }
    if (!tchdbputcat2(self->hdb, key, value)) {
        return set_hdb_error(self->hdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* HDB.putasync(key, value) */
PyDoc_STRVAR(HDB_putasync_doc,
"putasync(key, value)\n\
\n\
Store a record in the database in an asynchronous fashion.\n\
Records passed to this method are accumulated into an inner buffer and written\n\
into the file at a ?blast? (when?, relation to sync()?).");

static PyObject *
HDB_putasync(HDB *self, PyObject *args)
{
    const char *key, *value;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putasync", &pykey, &pyvalue)) {
        return NULL;
    }
    key = PyBytes_AsString(pykey);
    value = PyBytes_AsString(pyvalue);
    if (!(key && value)) {
        return NULL;
    }
    if (!tchdbputasync2(self->hdb, key, value)) {
        return set_hdb_error(self->hdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* HDB.sync() */
PyDoc_STRVAR(HDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database file?");

static PyObject *
HDB_sync(HDB *self)
{
    if (!tchdbsync(self->hdb)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(HDB_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
HDB_searchkeys(HDB *self, PyObject *args)
{
    const char *prefix;
    int max = -1;
    TCLIST *result;
    PyObject *pyprefix, *pyresult;

    if (!PyArg_ParseTuple(args, "O|i:searchkeys", &pyprefix, &max)) {
        return NULL;
    }
    prefix = PyBytes_AsString(pyprefix);
    if (!prefix) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tchdbfwmkeys2(self->hdb, prefix, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* HDB.optimize([bnum=0[, apow=-1[, fpow=-1[, opts=255]]]]) */
PyDoc_STRVAR(HDB_optimize_doc,
"optimize([bnum=0[, apow=-1[, fpow=-1[, opts=255]]]])\n\
\n\
Optimize a database.\n\
'bnum': the number of elements in a bucket array. If specified as 0 or as a\n\
        negative value, the default value (twice the number of records) is used.\n\
'apow': (?) TODO. If specified as a negative value, the current setting is kept.\n\
'fpow': (?) TODO. If specified as a negative value, the current setting is kept.\n\
'opts': TODO. If specified as 255 (UINT8_MAX), the current setting is kept.\n\
\n\
Note:\n\
Optimizing a read only database, or during a transaction, is an invalid\n\
operation.");

static PyObject *
HDB_optimize(HDB *self, PyObject *args, PyObject *kwargs)
{
    long long bnum = 0;
    int iapow = -1, ifpow = -1;
    char apow, fpow;
    unsigned char opts = UINT8_MAX;

    static char *kwlist[] = {"bnum", "apow", "fpow", "opts", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Liib:optimize", kwlist,
                                     &bnum, &iapow, &ifpow, &opts)) {
        return NULL;
    }
    apow = int_to_char(iapow);
    fpow = int_to_char(ifpow);
    if ((apow == -1 || fpow == -1) && PyErr_Occurred()) {
        return NULL;
    }
    if (!tchdboptimize(self->hdb, bnum, apow, fpow, opts)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.tune(bnum, apow, fpow, opts) */
PyDoc_STRVAR(HDB_tune_doc,
"tune(bnum, apow, fpow, opts)\n\
\n\
Tune a database.\n\
'bnum': the number of elements in a bucket array. If specified as 0 or as a\n\
        negative value, the default value (131071) is used.\n\
'apow': (?) TODO (-1).\n\
'fpow': (?) TODO (-1).\n\
'opts': TODO (0).\n\
\n\
Note:\n\
Tuning an open database is an invalid operation.");

static PyObject *
HDB_tune(HDB *self, PyObject *args)
{
    long long bnum;
    int iapow, ifpow;
    char apow, fpow;
    unsigned char opts;

    if (!PyArg_ParseTuple(args, "Liib:tune", &bnum, &iapow, &ifpow, &opts)) {
        return NULL;
    }
    apow = int_to_char(iapow);
    fpow = int_to_char(ifpow);
    if ((apow == -1 || fpow == -1) && PyErr_Occurred()) {
        return NULL;
    }
    if (!tchdbtune(self->hdb, bnum, apow, fpow, opts)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.setcache(rcnum) */
PyDoc_STRVAR(HDB_setcache_doc,
"setcache(rcnum)\n\
\n\
Set the cache size.\n\
'rcnum': the maximum number of records to be cached. If specified as 0 or as a\n\
         negative value, caching is disabled (default).\n\
\n\
Note:\n\
Setting the cache size on an open database is an invalid operation.");

static PyObject *
HDB_setcache(HDB *self, PyObject *args)
{
    long rcnum;

    if (!PyArg_ParseTuple(args, "l:setcache", &rcnum)) {
        return NULL;
    }
    if (!tchdbsetcache(self->hdb, rcnum)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.setxmsiz(xmsiz) */
PyDoc_STRVAR(HDB_setxmsiz_doc,
"setxmsiz(xmsiz)\n\
\n\
Set the extra mapped memory size.\n\
'xmsiz': the amount of extra mapped memory (in what unit?). If specified as 0\n\
         or as a negative value, the extra mapped memory is disabled.\n\
         Default is 67108864 (again unit?).\n\
\n\
Note:\n\
Setting the extra memory size on an open database is an invalid operation.");

static PyObject *
HDB_setxmsiz(HDB *self, PyObject *args)
{
    long long xmsiz;

    if (!PyArg_ParseTuple(args, "L:setxmsiz", &xmsiz)) {
        return NULL;
    }
    if (!tchdbsetxmsiz(self->hdb, xmsiz)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.setdfunit(dfunit) */
PyDoc_STRVAR(HDB_setdfunit_doc,
"setdfunit(dfunit)\n\
\n\
Set auto defragmentation's unit step number.\n\
'dfunit': the unit step number(?). If specified as 0 or as a negative value,\n\
          auto defragmentation is disabled (default).\n\
\n\
Note:\n\
Setting this on an open database is an invalid operation.");

static PyObject *
HDB_setdfunit(HDB *self, PyObject *args)
{
    long dfunit;

    if (!PyArg_ParseTuple(args, "l:setdfunit", &dfunit)) {
        return NULL;
    }
    if (!tchdbsetdfunit(self->hdb, dfunit)) {
        return set_hdb_error(self->hdb, NULL);
    }
    Py_RETURN_NONE;
}


/* HDB.addint(key, num) -> int */
PyDoc_STRVAR(HDB_addint_doc,
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
HDB_addint(HDB *self, PyObject *args)
{
    PyObject *pykey;
    char *key;
    Py_ssize_t key_size;
    int num, result;

    if (!PyArg_ParseTuple(args, "Oi:addint", &pykey, &num)) {
        return NULL;
    }
    if(PyBytes_AsStringAndSize(pykey, &key, &key_size)) {
        return NULL;
    }
    result = tchdbaddint(self->hdb, key, (int)key_size, num);
    if (result == INT_MIN && tchdbecode(self->hdb) != TCESUCCESS) {
        return set_hdb_error(self->hdb, key);
    }
    self->changed = true;
    return PyInt_FromLong((long)result);
}


/* HDB.adddouble(key, num) -> float */
PyDoc_STRVAR(HDB_adddouble_doc,
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
HDB_adddouble(HDB *self, PyObject *args)
{
    PyObject *pykey;
    char *key;
    Py_ssize_t key_size;
    double num, result;

    if (!PyArg_ParseTuple(args, "Od:adddouble", &pykey, &num)) {
        return NULL;
    }
    if(PyBytes_AsStringAndSize(pykey, &key, &key_size)) {
        return NULL;
    }
    result = tchdbadddouble(self->hdb, key, (int)key_size, num);
    if (Py_IS_NAN(result)) {
        return set_hdb_error(self->hdb, key);
    }
    self->changed = true;
    return PyFloat_FromDouble(result);
}


/* HDB.iterkeys() */
PyDoc_STRVAR(HDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database’s keys.");

static PyObject *
HDB_iterkeys(HDB *self)
{
    return new_HDBIter(self, &HDBIterKeysType);
}


/* HDB.itervalues() */
PyDoc_STRVAR(HDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database’s values.");

static PyObject *
HDB_itervalues(HDB *self)
{
    return new_HDBIter(self, &HDBIterValuesType);
}


/* HDB.iteritems() */
PyDoc_STRVAR(HDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database’s items.");

static PyObject *
HDB_iteritems(HDB *self)
{
    return new_HDBIter(self, &HDBIterItemsType);
}


/* HDBType.tp_methods */
static PyMethodDef HDB_tp_methods[] = {
    {"__length_hint__", (PyCFunction)HDB_length_hint, METH_NOARGS,
     HDB_length_hint_doc},
    {"open", (PyCFunction)HDB_open, METH_VARARGS, HDB_open_doc},
    {"close", (PyCFunction)HDB_close, METH_NOARGS, HDB_close_doc},
    {"clear", (PyCFunction)HDB_clear, METH_NOARGS, HDB_clear_doc},
    {"copy", (PyCFunction)HDB_copy, METH_VARARGS, HDB_copy_doc},
    {"begin", (PyCFunction)HDB_begin, METH_NOARGS, HDB_begin_doc},
    {"commit", (PyCFunction)HDB_commit, METH_NOARGS, HDB_commit_doc},
    {"abort", (PyCFunction)HDB_abort, METH_NOARGS, HDB_abort_doc},
    {"get", (PyCFunction)HDB_get, METH_VARARGS, HDB_get_doc},
    {"remove", (PyCFunction)HDB_remove, METH_VARARGS, HDB_remove_doc},
    {"put", (PyCFunction)HDB_put, METH_VARARGS, HDB_put_doc},
    {"putkeep", (PyCFunction)HDB_putkeep, METH_VARARGS, HDB_putkeep_doc},
    {"putcat", (PyCFunction)HDB_putcat, METH_VARARGS, HDB_putcat_doc},
    {"putasync", (PyCFunction)HDB_putasync, METH_VARARGS, HDB_putasync_doc},
    {"sync", (PyCFunction)HDB_sync, METH_NOARGS, HDB_sync_doc},
    {"searchkeys", (PyCFunction)HDB_searchkeys, METH_VARARGS,
     HDB_searchkeys_doc},
    {"optimize", (PyCFunction)HDB_optimize, METH_VARARGS | METH_KEYWORDS,
     HDB_optimize_doc},
    {"tune", (PyCFunction)HDB_tune, METH_VARARGS, HDB_tune_doc},
    {"setcache", (PyCFunction)HDB_setcache, METH_VARARGS, HDB_setcache_doc},
    {"setxmsiz", (PyCFunction)HDB_setxmsiz, METH_VARARGS, HDB_setxmsiz_doc},
    {"setdfunit", (PyCFunction)HDB_setdfunit, METH_VARARGS, HDB_setdfunit_doc},
    {"addint", (PyCFunction)HDB_addint, METH_VARARGS, HDB_addint_doc},
    {"adddouble", (PyCFunction)HDB_adddouble, METH_VARARGS, HDB_adddouble_doc},
    {"iterkeys", (PyCFunction)HDB_iterkeys, METH_NOARGS, HDB_iterkeys_doc},
    {"itervalues", (PyCFunction)HDB_itervalues, METH_NOARGS, HDB_itervalues_doc},
    {"iteritems", (PyCFunction)HDB_iteritems, METH_NOARGS, HDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* HDB.path */
PyDoc_STRVAR(HDB_path_doc,
"The path to the database file.");

static PyObject *
HDB_path_get(HDB *self, void *closure)
{
    const char *path;

    path = tchdbpath(self->hdb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* HDB.size */
PyDoc_STRVAR(HDB_size_doc,
"The size in bytes of the database file.");

static PyObject *
HDB_size_get(HDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tchdbfsiz(self->hdb));
}


/* HDBType.tp_getsets */
static PyGetSetDef HDB_tp_getsets[] = {
    {"path", (getter)HDB_path_get, NULL, HDB_path_doc, NULL},
    {"size", (getter)HDB_size_get, NULL, HDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* HDBType */
static PyTypeObject HDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.HDB",                      /*tp_name*/
    sizeof(HDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)HDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &HDB_tp_as_sequence,                      /*tp_as_sequence*/
    &HDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    HDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)HDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    HDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    HDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    HDB_tp_new,                               /*tp_new*/
};
