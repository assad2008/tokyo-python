/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_tdb_error(TCTDB *tdb, const char *key)
{
    int ecode;

    ecode = tctdbecode(tdb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return set_key_error(key);
    }
    return set_error(Error, tctdberrmsg(ecode));
}


/* same as tctdbiternext3 without adding primary key */
TCMAP *pytctdbiternext3(TCTDB *tdb){
    assert(tdb);
    TCXSTR *kstr = tcxstrnew();
    TCXSTR *vstr = tcxstrnew();
    TCMAP *cols = NULL;
    if(tchdbiternext3(tdb->hdb, kstr, vstr)){
        cols = tcmapload(TCXSTRPTR(vstr), TCXSTRSIZE(vstr));
    }
    tcxstrdel(vstr);
    tcxstrdel(kstr);
    return cols;
}


/* same as above but keep the key */
TCMAP *pytctdbiternext4(TCTDB *tdb, TCXSTR *kstr){
    assert(tdb && kstr);
    TCXSTR *vstr = tcxstrnew();
    TCMAP *cols = NULL;
    if(tchdbiternext3(tdb->hdb, kstr, vstr)){
        cols = tcmapload(TCXSTRPTR(vstr), TCXSTRSIZE(vstr));
    }
    tcxstrdel(vstr);
    return cols;
}


/*******************************************************************************
* TDBQueryType
*******************************************************************************/

/* TDBQuery process callback */
static int
TDBQuery_process_cb(const void *key, int key_size, TCMAP *value, void *op)
{
    PyObject *pykey, *pyvalue = NULL, *pyresult = NULL, *callback = op;
    int result;
    TCMAP *new_value;
    TCMAP tmp_value;

    pykey = void_to_bytes(key, key_size);
    if (!pykey) {
        goto fail;
    }
    pyvalue = tcmap_to_dict(value);
    if (!pyvalue) {
        goto fail;
    }
    pyresult = PyObject_CallFunctionObjArgs(callback, pykey, pyvalue, NULL);
    if (!pyresult) {
        goto fail;
    }
#if PY_MAJOR_VERSION >= 3
    if (PyLong_Check(pyresult)) {
#else
    if (PyInt_Check(pyresult)) {
#endif
        result = (int)PyLong_AsLong(pyresult);
        if (result == -1 && PyErr_Occurred()) {
            goto fail;
        }
        if (result & TDBQPPUT) {
            new_value = dict_to_tcmap(pyvalue);
            if (!new_value) {
                goto fail;
            }
            /* musical chairs! \o/ */
            tmp_value = *value;
            *value = *new_value;
            *new_value = tmp_value;
            tcmapdel(new_value);
        }
    }
    else {
        result = 0;
    }
    Py_DECREF(pykey);
    Py_DECREF(pyvalue);
    Py_DECREF(pyresult);
    return result;

fail:
    Py_XDECREF(pykey);
    Py_XDECREF(pyvalue);
    Py_XDECREF(pyresult);
    return TDBQPSTOP;
}


/* new_TDBQuery */
TDBQuery *
new_TDBQuery(PyTypeObject *type, TDB *tdb)
{
    TDBQuery *self = (TDBQuery *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->qry */
    self->qry = tctdbqrynew(tdb->tdb);
    if (!self->qry) {
        set_error(Error, "could not create TDBQuery, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    /* self->tdb */
    Py_INCREF(tdb);
    self->tdb = tdb;
    return self;
}


/* TDBQueryType.tp_traverse */
static int
TDBQuery_tp_traverse(TDBQuery *self, visitproc visit, void *arg)
{
    Py_VISIT(self->tdb);
    return 0;
}


/* TDBQueryType.tp_clear */
static int
TDBQuery_tp_clear(TDBQuery *self)
{
    Py_CLEAR(self->tdb);
    return 0;
}


/* TDBQueryType.tp_dealloc */
static void
TDBQuery_tp_dealloc(TDBQuery *self)
{
    if (self->qry) {
        tctdbqrydel(self->qry);
    }
    TDBQuery_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* TDBQuery.search() */
PyDoc_STRVAR(TDBQuery_search_doc,
"search()\n\
\n\
Execute the query.");

static PyObject *
TDBQuery_search(TDBQuery *self)
{
    TCLIST *result;
    PyObject *pyresult;

    Py_BEGIN_ALLOW_THREADS
    result = tctdbqrysearch(self->qry);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_tuple(result);
    tclistdel(result);
    return pyresult;
}


/* TDBQuery.remove() */
PyDoc_STRVAR(TDBQuery_remove_doc,
"remove()\n\
\n\
Remove the records corresponding to the query from the database.");

static PyObject *
TDBQuery_remove(TDBQuery *self)
{
    bool result;

    Py_BEGIN_ALLOW_THREADS
    result = tctdbqrysearchout(self->qry);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_tdb_error(self->tdb->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDBQuery.process(callback) */
PyDoc_STRVAR(TDBQuery_process_doc,
"process(callback)\n\
\n\
");

static PyObject *
TDBQuery_process(TDBQuery *self, PyObject *args)
{
    PyObject *callback;
    bool result;

    if (!PyArg_ParseTuple(args, "O:process", &callback)) {
        return NULL;
    }
    if (!PyCallable_Check(callback)) {
        return set_error(PyExc_TypeError, "a callable is required");
    }
    result = tctdbqryproc(self->qry, TDBQuery_process_cb, (void *)callback);
    if (!result) {
        return set_tdb_error(self->tdb->tdb, NULL);
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* TDBQuery.sort(column, type) */
PyDoc_STRVAR(TDBQuery_sort_doc,
"sort(column, type)\n\
\n\
");

static PyObject *
TDBQuery_sort(TDBQuery *self, PyObject *args)
{
#if PY_MAJOR_VERSION >= 3
    const char *format = "yi:sort";
#else
    const char *format = "si:sort";
#endif
    const char *column;
    int type;

    if (!PyArg_ParseTuple(args, format, &column, &type)) {
        return NULL;
    }
    tctdbqrysetorder(self->qry, column, type);
    Py_RETURN_NONE;
}


/* TDBQuery.limit([max[, skip]]) */
PyDoc_STRVAR(TDBQuery_limit_doc,
"limit([max[, skip]])\n\
\n\
");

static PyObject *
TDBQuery_limit(TDBQuery *self, PyObject *args)
{
    int max=-1, skip=0;

    if (!PyArg_ParseTuple(args, "|ii:limit", &max, &skip)) {
        return NULL;
    }
    tctdbqrysetlimit(self->qry, max, skip);
    Py_RETURN_NONE;
}


/* TDBQuery.filter(column, condition, expr) */
PyDoc_STRVAR(TDBQuery_filter_doc,
"filter(column, condition, expr)\n\
\n\
");

static PyObject *
TDBQuery_filter(TDBQuery *self, PyObject *args)
{
#if PY_MAJOR_VERSION >= 3
    const char *format = "yiy:filter";
#else
    const char *format = "sis:filter";
#endif
    const char *column, *expr;
    int condition;

    if (!PyArg_ParseTuple(args, format, &column, &condition, &expr)) {
        return NULL;
    }
    tctdbqryaddcond(self->qry, column, condition, expr);
    Py_RETURN_NONE;
}


/* TDBQuery.count() */
PyDoc_STRVAR(TDBQuery_count_doc,
"Returns the length of the result set.");

static PyObject *
TDBQuery_count(TDBQuery *self)
{
    return PyInt_FromLong((long)tctdbqrycount(self->qry));
}


/* TDBQueryType.tp_methods */
static PyMethodDef TDBQuery_tp_methods[] = {
    {"search", (PyCFunction)TDBQuery_search, METH_NOARGS, TDBQuery_search_doc},
    {"remove", (PyCFunction)TDBQuery_remove, METH_NOARGS, TDBQuery_remove_doc},
    {"process", (PyCFunction)TDBQuery_process, METH_VARARGS,
     TDBQuery_process_doc},
    {"sort", (PyCFunction)TDBQuery_sort, METH_VARARGS, TDBQuery_sort_doc},
    {"limit", (PyCFunction)TDBQuery_limit, METH_VARARGS, TDBQuery_limit_doc},
    {"filter", (PyCFunction)TDBQuery_filter, METH_VARARGS, TDBQuery_filter_doc},
    {"count", (PyCFunction)TDBQuery_count, METH_NOARGS, TDBQuery_count_doc},
    {NULL}  /* Sentinel */
};


/* TDBQuery.hint */
PyDoc_STRVAR(TDBQuery_hint_doc,
"TODO.");

static PyObject *
TDBQuery_hint_get(TDBQuery *self, void *closure)
{
    return PyString_FromString(tctdbqryhint(self->qry));
}


/* TDBQueryType.tp_getsets */
static PyGetSetDef TDBQuery_tp_getsets[] = {
    {"hint", (getter)TDBQuery_hint_get, NULL, TDBQuery_hint_doc, NULL},
    {NULL}  /* Sentinel */
};


/* TDBQueryType */
static PyTypeObject TDBQueryType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBQuery",                 /*tp_name*/
    sizeof(TDBQuery),                         /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)TDBQuery_tp_dealloc,          /*tp_dealloc*/
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
    (traverseproc)TDBQuery_tp_traverse,       /*tp_traverse*/
    (inquiry)TDBQuery_tp_clear,               /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    TDBQuery_tp_methods,                      /*tp_methods*/
    0,                                        /*tp_members*/
    TDBQuery_tp_getsets,                      /*tp_getsets*/
};


/*******************************************************************************
* TDB iterator types
*******************************************************************************/

/* new_TDBIter */
static PyObject *
new_TDBIter(TDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tctdbiterinit(self->tdb)) {
        Py_DECREF(iter);
        return set_tdb_error(self->tdb, NULL);
    }
    self->changed = false;
    return iter;
}


/* TDBIterKeysType.tp_iternext */
static PyObject *
TDBIterKeys_tp_iternext(DBIter *self)
{
    TDB *tdb = (TDB *)self->db;
    void *key;
    int key_size;
    PyObject *pykey;

    if (tdb->changed) {
        return set_error(Error, "TDB changed during iteration");
    }
    key = tctdbiternext(tdb->tdb, &key_size);
    if (!key) {
        if (tctdbecode(tdb->tdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_tdb_error(tdb->tdb, NULL);
    }
    pykey = void_to_bytes(key, key_size);
    tcfree(key);
    return pykey;
}


/* TDBIterKeysType */
static PyTypeObject TDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBIterKeys",              /*tp_name*/
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
    (iternextfunc)TDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* TDBIterValuesType.tp_iternext */
static PyObject *
TDBIterValues_tp_iternext(DBIter *self)
{
    TDB *tdb = (TDB *)self->db;
    TCMAP *value;
    PyObject *pyvalue;

    if (tdb->changed) {
        return set_error(Error, "TDB changed during iteration");
    }
    value = pytctdbiternext3(tdb->tdb);
    if (!value) {
        if (tctdbecode(tdb->tdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_tdb_error(tdb->tdb, NULL);
    }
    pyvalue = tcmap_to_dict(value);
    tcmapdel(value);
    return pyvalue;
}


/* TDBIterValuesType */
static PyTypeObject TDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBIterValues",            /*tp_name*/
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
    (iternextfunc)TDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* TDBIterItemsType.tp_iternext */
static PyObject *
TDBIterItems_tp_iternext(DBIter *self)
{
    TDB *tdb = (TDB *)self->db;
    TCXSTR *key;
    TCMAP *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (tdb->changed) {
        return set_error(Error, "TDB changed during iteration");
    }
    key = tcxstrnew();
    value = pytctdbiternext4(tdb->tdb, key);
    if (!value) {
        if (tctdbecode(tdb->tdb) == TCENOREC) {
            set_stopiteration_error();
        }
        else {
            set_tdb_error(tdb->tdb, NULL);
        }
    }
    else {
        pykey = tcxstr_to_bytes(key);
        pyvalue = tcmap_to_dict(value);
        if (pykey && pyvalue) {
            pyresult = PyTuple_Pack(2, pykey, pyvalue);
        }
        tcmapdel(value);
        Py_XDECREF(pykey);
        Py_XDECREF(pyvalue);
    }
    tcxstrdel(key);
    return pyresult;
}


/* TDBIterItemsType */
static PyTypeObject TDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBIterItems",             /*tp_name*/
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
    (iternextfunc)TDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* TDBIterValuesKeysType.tp_iternext */
static PyObject *
TDBIterValuesKeys_tp_iternext(DBIter *self)
{
    TDB *tdb = (TDB *)self->db;
    TCMAP *value;
    TCLIST *valuekeys;
    PyObject *pyvaluekeys;

    if (tdb->changed) {
        return set_error(Error, "TDB changed during iteration");
    }
    value = pytctdbiternext3(tdb->tdb);
    if (!value) {
        if (tctdbecode(tdb->tdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_tdb_error(tdb->tdb, NULL);
    }
    valuekeys = tcmapkeys(value);
    pyvaluekeys = tclist_to_tuple(valuekeys);
    tcmapdel(value);
    tclistdel(valuekeys);
    return pyvaluekeys;
}


/* TDBIterValuesKeysType */
static PyTypeObject TDBIterValuesKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBIterValuesKeys",        /*tp_name*/
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
    (iternextfunc)TDBIterValuesKeys_tp_iternext, /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* TDBIterValuesValsType.tp_iternext */
static PyObject *
TDBIterValuesVals_tp_iternext(DBIter *self)
{
    TDB *tdb = (TDB *)self->db;
    TCMAP *value;
    TCLIST *valuevals;
    PyObject *pyvaluevals;

    if (tdb->changed) {
        return set_error(Error, "TDB changed during iteration");
    }
    value = pytctdbiternext3(tdb->tdb);
    if (!value) {
        if (tctdbecode(tdb->tdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_tdb_error(tdb->tdb, NULL);
    }
    valuevals = tcmapvals(value);
    pyvaluevals = tclist_to_tuple(valuevals);
    tcmapdel(value);
    tclistdel(valuevals);
    return pyvaluevals;
}


/* TDBIterValuesValsType */
static PyTypeObject TDBIterValuesValsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDBIterValuesVals",        /*tp_name*/
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
    (iternextfunc)TDBIterValuesVals_tp_iternext, /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* TDBType
*******************************************************************************/

/* TDBType.tp_doc */
PyDoc_STRVAR(TDB_tp_doc,
"TDB()\n\
\n\
Table Database.\n\
\n\
See also:\n\
Tokyo Cabinet Table Database API at:\n\
http://fallabs.com/tokyocabinet/spex-en.html#tctdbapi");


/* TDBType.tp_dealloc */
static void
TDB_tp_dealloc(TDB *self)
{
    if (self->tdb) {
        tctdbdel(self->tdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* TDBType.tp_new */
static PyObject *
TDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TDB *self = (TDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->tdb */
    self->tdb = tctdbnew();
    if (!self->tdb) {
        set_error(Error, "could not create TDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    if (!tctdbsetmutex(self->tdb)) {
        set_tdb_error(self->tdb, NULL);
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* TDB_tp_as_sequence.sq_contains */
static int
TDB_Contains(TDB *self, PyObject *pykey)
{
    void *key;
    int key_size;
    TCMAP *value;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    value = tctdbget(self->tdb, key, key_size);
    if (!value) {
        if (tctdbecode(self->tdb) == TCENOREC) {
            return 0;
        }
        set_tdb_error(self->tdb, NULL);
        return -1;
    }
    tcmapdel(value);
    return 1;
}


/* TDBType.tp_as_sequence */
static PySequenceMethods TDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)TDB_Contains,                 /*sq_contains*/
};


/* TDB_tp_as_mapping.mp_length */
static Py_ssize_t
TDB_Length(TDB *self)
{
    return DB_Length(tctdbrnum(self->tdb));
}


/* TDB_tp_as_mapping.mp_subscript */
static PyObject *
TDB_GetItem(TDB *self, PyObject *pykey)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pyvalue;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    value = tctdbget(self->tdb, key, key_size);
    if (!value) {
        return set_tdb_error(self->tdb, key);
    }
    pyvalue = tcmap_to_dict(value);
    tcmapdel(value);
    return pyvalue;
}


/* TDB_tp_as_mapping.mp_ass_subscript */
static int
TDB_SetItem(TDB *self, PyObject *pykey, PyObject *pyvalue)
{
    void *key;
    int key_size;
    TCMAP *value;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    if (pyvalue) {
        value = dict_to_tcmap(pyvalue);
        if (!value) {
            return -1;
        }
        if (!tctdbput(self->tdb, key, key_size, value)) {
            tcmapdel(value);
            set_tdb_error(self->tdb, NULL);
            return -1;
        }
        tcmapdel(value);
    }
    else {
        if (!tctdbout(self->tdb, key, key_size)) {
            set_tdb_error(self->tdb, key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* TDBType.tp_as_mapping */
static PyMappingMethods TDB_tp_as_mapping = {
    (lenfunc)TDB_Length,                      /*mp_length*/
    (binaryfunc)TDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)TDB_SetItem                /*mp_ass_subscript*/
};


/* TDBType.tp_iter */
static PyObject *
TDB_tp_iter(TDB *self)
{
    return new_TDBIter(self, &TDBIterKeysType);
}


/* TDB.open(path, mode) */
PyDoc_STRVAR(TDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database file.\n\
'mode': connection mode.");

static PyObject *
TDB_open(TDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tctdbopen(self->tdb, path, mode)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.close() */
PyDoc_STRVAR(TDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
TDBs are closed when garbage-collected.");

static PyObject *
TDB_close(TDB *self)
{
    if (!tctdbclose(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.clear() */
PyDoc_STRVAR(TDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
TDB_clear(TDB *self)
{
    if (!tctdbvanish(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* TDB.copy(path) */
PyDoc_STRVAR(TDB_copy_doc,
"copy(path)\n\
\n\
Copy the database file.\n\
'path': path to the destination file.");

static PyObject *
TDB_copy(TDB *self, PyObject *args)
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
    if (!tctdbcopy(self->tdb, path)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.begin() */
PyDoc_STRVAR(TDB_begin_doc,
"begin()\n\
\n\
Begin a transaction.");

static PyObject *
TDB_begin(TDB *self)
{
    if (!tctdbtranbegin(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.commit() */
PyDoc_STRVAR(TDB_commit_doc,
"commit()\n\
\n\
Commit a transaction.");

static PyObject *
TDB_commit(TDB *self)
{
    if (!tctdbtrancommit(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.abort() */
PyDoc_STRVAR(TDB_abort_doc,
"abort()\n\
\n\
Abort a transaction.");

static PyObject *
TDB_abort(TDB *self)
{
    if (!tctdbtranabort(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.get(key) */
PyDoc_STRVAR(TDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
TDB_get(TDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return TDB_GetItem(self, pykey);
}


/* TDB.remove(key) */
PyDoc_STRVAR(TDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
TDB_remove(TDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (TDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* TDB.put(key, value) */
PyDoc_STRVAR(TDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
TDB_put(TDB *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pykey, *pyvalue = NULL;

    if (!PyArg_ParseTuple(args, "O|O;put() takes at least 2 arguments",
                          &pykey, &pyvalue)) {
        return NULL;
    }
    pyvalue = merge_put_args("put", pyvalue, kwargs);
    if (!pyvalue) {
        return NULL;
    }
    if (TDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* TDB.putkeep(key, value) */
PyDoc_STRVAR(TDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (tdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
TDB_putkeep(TDB *self, PyObject *args, PyObject *kwargs)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pykey, *pyvalue = NULL;

    if (!PyArg_ParseTuple(args, "O|O;putkeep() takes at least 2 arguments",
                          &pykey, &pyvalue)) {
        return NULL;
    }
    pyvalue = merge_put_args("putkeep", pyvalue, kwargs);
    if (!pyvalue) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    value = dict_to_tcmap(pyvalue);
    if (!value) {
        return NULL;
    }
    if (!tctdbputkeep(self->tdb, key, key_size, value)) {
        tcmapdel(value);
        return set_tdb_error(self->tdb, key);
    }
    tcmapdel(value);
    self->changed = true;
    Py_RETURN_NONE;
}


/* TDB.putcat(key, value) */
PyDoc_STRVAR(TDB_putcat_doc,
"putcat(key, value)\n\
\n\
Merge a value with an existing one, does not override existing items in current\n\
value. If there is no corresponding record, a new record is stored.");

static PyObject *
TDB_putcat(TDB *self, PyObject *args, PyObject *kwargs)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pykey, *pyvalue = NULL;

    if (!PyArg_ParseTuple(args, "O|O;putcat() takes at least 2 arguments",
                          &pykey, &pyvalue)) {
        return NULL;
    }
    pyvalue = merge_put_args("putcat", pyvalue, kwargs);
    if (!pyvalue) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    value = dict_to_tcmap(pyvalue);
    if (!value) {
        return NULL;
    }
    if (!tctdbputcat(self->tdb, key, key_size, value)) {
        tcmapdel(value);
        return set_tdb_error(self->tdb, NULL);
    }
    tcmapdel(value);
    self->changed = true;
    Py_RETURN_NONE;
}


/* TDB.sync() */
PyDoc_STRVAR(TDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database file?");

static PyObject *
TDB_sync(TDB *self)
{
    if (!tctdbsync(self->tdb)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(TDB_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
TDB_searchkeys(TDB *self, PyObject *args)
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
    result = tctdbfwmkeys(self->tdb, prefix, prefix_size, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* TDB.optimize([bnum=0[, apow=-1[, fpow=-1[, opts=255]]]]) */
PyDoc_STRVAR(TDB_optimize_doc,
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
Optimizing a read only database, or during a transaction, is an invalid operation.");

static PyObject *
TDB_optimize(TDB *self, PyObject *args, PyObject *kwargs)
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
    if (!tctdboptimize(self->tdb, bnum, apow, fpow, opts)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.tune(bnum, apow, fpow, opts) */
PyDoc_STRVAR(TDB_tune_doc,
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
TDB_tune(TDB *self, PyObject *args)
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
    if (!tctdbtune(self->tdb, bnum, apow, fpow, opts)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.setcache(rcnum, lcnum, ncnum) */
PyDoc_STRVAR(TDB_setcache_doc,
"setcache(rcnum, lcnum, ncnum)\n\
\n\
Set the cache size.\n\
'rcnum': the maximum number of records to be cached. If specified as 0 or as a\n\
         negative value, caching is disabled (default).\n\
'lcnum': the maximum number of leaf nodes to be cached. If specified as 0 or as\n\
         a negative value, the default value (4096) is used.\n\
'ncnum': the maximum number of non-leaf nodes to be cached. If specified as 0 or\n\
         as a negative value, the default value (512) is used.\n\
\n\
Note:\n\
Setting the cache size on an open database is an invalid operation.");

static PyObject *
TDB_setcache(TDB *self, PyObject *args)
{
    long rcnum, lcnum, ncnum;

    if (!PyArg_ParseTuple(args, "lll:setcache", &rcnum, &lcnum, &ncnum)) {
        return NULL;
    }
    if (!tctdbsetcache(self->tdb, rcnum, lcnum, ncnum)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.setxmsiz(xmsiz) */
PyDoc_STRVAR(TDB_setxmsiz_doc,
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
TDB_setxmsiz(TDB *self, PyObject *args)
{
    long long xmsiz;

    if (!PyArg_ParseTuple(args, "L:setxmsiz", &xmsiz)) {
        return NULL;
    }
    if (!tctdbsetxmsiz(self->tdb, xmsiz)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.setdfunit(dfunit) */
PyDoc_STRVAR(TDB_setdfunit_doc,
"setdfunit(dfunit)\n\
\n\
Set auto defragmentation's unit step number.\n\
'dfunit': the unit step number(?). If specified as 0 or as a negative value,\n\
          auto defragmentation is disabled (default).\n\
\n\
Note:\n\
Setting this on an open database is an invalid operation.");

static PyObject *
TDB_setdfunit(TDB *self, PyObject *args)
{
    long dfunit;

    if (!PyArg_ParseTuple(args, "l:setdfunit", &dfunit)) {
        return NULL;
    }
    if (!tctdbsetdfunit(self->tdb, dfunit)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.setindex(column, type) */
PyDoc_STRVAR(TDB_setindex_doc,
"setindex(column, type)\n\
\n\
Add an index to a column.");

static PyObject *
TDB_setindex(TDB *self, PyObject *args)
{
#if PY_MAJOR_VERSION >= 3
    const char *format = "yi:setindex";
#else
    const char *format = "si:setindex";
#endif
    const char *column;
    int type;

    if (!PyArg_ParseTuple(args, format, &column, &type)) {
        return NULL;
    }
    if (!tctdbsetindex(self->tdb, column, type)) {
        return set_tdb_error(self->tdb, NULL);
    }
    Py_RETURN_NONE;
}


/* TDB.uid() */
PyDoc_STRVAR(TDB_uid_doc,
"uid()\n\
\n\
Return a new unique id.");

static PyObject *
TDB_uid(TDB *self)
{
    long long uid;

    uid = tctdbgenuid(self->tdb);
    if (uid == -1) {
        return set_tdb_error(self->tdb, NULL);
    }
    return PyLong_FromLongLong(uid);
}


/* TDB.query() */
PyDoc_STRVAR(TDB_query_doc,
"query()\n\
\n\
");

static PyObject *
TDB_query(TDB *self)
{
    return (PyObject *)new_TDBQuery(&TDBQueryType, self);
}


/* TDB.metasearch(queries, type) */
PyDoc_STRVAR(TDB_metasearch_doc,
"metasearch(queries, type)\n\
\n\
Combine queries.");

static PyObject *
TDB_metasearch(TDB *notused, PyObject *args)
{
    PyObject *pyqueries, *pyseq, *pyquery, *pyresult;
    int type;
    const char *msg = "a sequence of TDBQuery's is required";
    Py_ssize_t len, i;
    TDBQRY **queries;
    TCLIST *result;

    if (!PyArg_ParseTuple(args, "Oi:metasearch", &pyqueries, &type)) {
        return NULL;
    }
    pyseq = PySequence_Fast(pyqueries, msg);
    if (!pyseq) {
        return NULL;
    }
    len = PySequence_Fast_GET_SIZE(pyseq);
    if (check_py_ssize_t_len(len, pyseq)) {
        Py_DECREF(pyseq);
        return NULL;
    }
    queries = (TDBQRY **)tcmalloc((size_t)len * sizeof(TDBQRY *));
    if (!queries) {
        Py_DECREF(pyseq);
        return PyErr_NoMemory();
    }
    for (i = 0; i < len; i++) {
        pyquery = PySequence_Fast_GET_ITEM(pyseq, i);
        if (!PyObject_TypeCheck(pyquery, &TDBQueryType)) {
            Py_DECREF(pyseq);
            tcfree(queries);
            return set_error(PyExc_TypeError, msg);
        }
        queries[i] = ((TDBQuery *)pyquery)->qry;
    }
    Py_DECREF(pyseq);
    Py_BEGIN_ALLOW_THREADS
    result = tctdbmetasearch(queries, (int)len, type);
    tcfree(queries);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_tuple(result);
    tclistdel(result);
    return pyresult;
}


/* TDB.iterkeys() */
PyDoc_STRVAR(TDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
TDB_iterkeys(TDB *self)
{
    return new_TDBIter(self, &TDBIterKeysType);
}


/* TDB.itervalues() */
PyDoc_STRVAR(TDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
TDB_itervalues(TDB *self)
{
    return new_TDBIter(self, &TDBIterValuesType);
}


/* TDB.iteritems() */
PyDoc_STRVAR(TDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
TDB_iteritems(TDB *self)
{
    return new_TDBIter(self, &TDBIterItemsType);
}


/* TDB.itervalueskeys() */
PyDoc_STRVAR(TDB_itervalueskeys_doc,
"itervalueskeys()\n\
\n\
Return an iterator over the database's values' keys.");

static PyObject *
TDB_itervalueskeys(TDB *self)
{
    return new_TDBIter(self, &TDBIterValuesKeysType);
}


/* TDB.itervaluesvals() */
PyDoc_STRVAR(TDB_itervaluesvals_doc,
"itervaluesvals()\n\
\n\
Return an iterator over the database's values' values.");

static PyObject *
TDB_itervaluesvals(TDB *self)
{
    return new_TDBIter(self, &TDBIterValuesValsType);
}


/* TDBType.tp_methods */
static PyMethodDef TDB_tp_methods[] = {
    {"open", (PyCFunction)TDB_open, METH_VARARGS, TDB_open_doc},
    {"close", (PyCFunction)TDB_close, METH_NOARGS, TDB_close_doc},
    {"clear", (PyCFunction)TDB_clear, METH_NOARGS, TDB_clear_doc},
    {"copy", (PyCFunction)TDB_copy, METH_VARARGS, TDB_copy_doc},
    {"begin", (PyCFunction)TDB_begin, METH_NOARGS, TDB_begin_doc},
    {"commit", (PyCFunction)TDB_commit, METH_NOARGS, TDB_commit_doc},
    {"abort", (PyCFunction)TDB_abort, METH_NOARGS, TDB_abort_doc},
    {"get", (PyCFunction)TDB_get, METH_VARARGS, TDB_get_doc},
    {"remove", (PyCFunction)TDB_remove, METH_VARARGS, TDB_remove_doc},
    {"put", (PyCFunction)TDB_put, METH_VARARGS | METH_KEYWORDS, TDB_put_doc},
    {"putkeep", (PyCFunction)TDB_putkeep, METH_VARARGS | METH_KEYWORDS,
     TDB_putkeep_doc},
    {"putcat", (PyCFunction)TDB_putcat, METH_VARARGS | METH_KEYWORDS,
     TDB_putcat_doc},
    {"sync", (PyCFunction)TDB_sync, METH_NOARGS, TDB_sync_doc},
    {"searchkeys", (PyCFunction)TDB_searchkeys, METH_VARARGS,
     TDB_searchkeys_doc},
    {"optimize", (PyCFunction)TDB_optimize, METH_VARARGS | METH_KEYWORDS,
     TDB_optimize_doc},
    {"tune", (PyCFunction)TDB_tune, METH_VARARGS, TDB_tune_doc},
    {"setcache", (PyCFunction)TDB_setcache, METH_VARARGS, TDB_setcache_doc},
    {"setxmsiz", (PyCFunction)TDB_setxmsiz, METH_VARARGS, TDB_setxmsiz_doc},
    {"setdfunit", (PyCFunction)TDB_setdfunit, METH_VARARGS, TDB_setdfunit_doc},
    {"setindex", (PyCFunction)TDB_setindex, METH_VARARGS, TDB_setindex_doc},
    {"uid", (PyCFunction)TDB_uid, METH_NOARGS, TDB_uid_doc},
    {"query", (PyCFunction)TDB_query, METH_NOARGS, TDB_query_doc},
    {"metasearch", (PyCFunction)TDB_metasearch, METH_VARARGS | METH_STATIC,
     TDB_metasearch_doc},
    {"iterkeys", (PyCFunction)TDB_iterkeys, METH_NOARGS, TDB_iterkeys_doc},
    {"itervalues", (PyCFunction)TDB_itervalues, METH_NOARGS, TDB_itervalues_doc},
    {"iteritems", (PyCFunction)TDB_iteritems, METH_NOARGS, TDB_iteritems_doc},
    {"itervalueskeys", (PyCFunction)TDB_itervalueskeys, METH_NOARGS,
     TDB_itervalueskeys_doc},
    {"itervaluesvals", (PyCFunction)TDB_itervaluesvals, METH_NOARGS,
     TDB_itervaluesvals_doc},
    {NULL}  /* Sentinel */
};


/* TDB.path */
PyDoc_STRVAR(TDB_path_doc,
"The path to the database file.");

static PyObject *
TDB_path_get(TDB *self, void *closure)
{
    const char *path;

    path = tctdbpath(self->tdb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* TDB.size */
PyDoc_STRVAR(TDB_size_doc,
"The size in bytes of the database file.");

static PyObject *
TDB_size_get(TDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tctdbfsiz(self->tdb));
}


/* TDBType.tp_getsets */
static PyGetSetDef TDB_tp_getsets[] = {
    {"path", (getter)TDB_path_get, NULL, TDB_path_doc, NULL},
    {"size", (getter)TDB_size_get, NULL, TDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* TDBType */
static PyTypeObject TDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.TDB",                      /*tp_name*/
    sizeof(TDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)TDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &TDB_tp_as_sequence,                      /*tp_as_sequence*/
    &TDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    TDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)TDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    TDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    TDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    TDB_tp_new,                               /*tp_new*/
};
