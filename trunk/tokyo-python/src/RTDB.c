/*******************************************************************************
* RTDBQueryType
*******************************************************************************/

/* new_RTDBQuery */
RTDBQuery *
new_RTDBQuery(PyTypeObject *type, RTDB *rtdb)
{
    RTDBQuery *self = (RTDBQuery *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->rqry */
    self->rqry = tcrdbqrynew(((RDBBase *)rtdb)->rdb);
    if (!self->rqry) {
        set_error(Error, "could not create RTDBQuery, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    /* self->rtdb */
    Py_INCREF(rtdb);
    self->rtdb = rtdb;
    return self;
}


/* RTDBQueryType.tp_traverse */
static int
RTDBQuery_tp_traverse(RTDBQuery *self, visitproc visit, void *arg)
{
    Py_VISIT(self->rtdb);
    return 0;
}


/* RTDBQueryType.tp_clear */
static int
RTDBQuery_tp_clear(RTDBQuery *self)
{
    Py_CLEAR(self->rtdb);
    return 0;
}


/* RTDBQueryType.tp_dealloc */
static void
RTDBQuery_tp_dealloc(RTDBQuery *self)
{
    if (self->rqry) {
        tcrdbqrydel(self->rqry);
    }
    RTDBQuery_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* RTDBQuery.search() */
PyDoc_STRVAR(RTDBQuery_search_doc,
"search()\n\
\n\
Execute the query.");

static PyObject *
RTDBQuery_search(RTDBQuery *self)
{
    TCLIST *result;
    PyObject *pyresult;

    Py_BEGIN_ALLOW_THREADS
    result = tcrdbqrysearch(self->rqry);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_tuple(result);
    tclistdel(result);
    return pyresult;
}


/* RTDBQuery.remove() */
PyDoc_STRVAR(RTDBQuery_remove_doc,
"remove()\n\
\n\
Remove the records corresponding to the query from the database.");

static PyObject *
RTDBQuery_remove(RTDBQuery *self)
{
    bool result;

    Py_BEGIN_ALLOW_THREADS
    result = tcrdbqrysearchout(self->rqry);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(((RDBBase *)self->rtdb)->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RTDBQuery.sort(column, type) */
PyDoc_STRVAR(RTDBQuery_sort_doc,
"sort(column, type)\n\
\n\
");

static PyObject *
RTDBQuery_sort(RTDBQuery *self, PyObject *args)
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
    Py_BEGIN_ALLOW_THREADS
    tcrdbqrysetorder(self->rqry, column, type);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


/* RTDBQuery.limit([max[, skip]]) */
PyDoc_STRVAR(RTDBQuery_limit_doc,
"limit([max[, skip]])\n\
\n\
");

static PyObject *
RTDBQuery_limit(RTDBQuery *self, PyObject *args)
{
    int max=-1, skip=0;

    if (!PyArg_ParseTuple(args, "|ii:limit", &max, &skip)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    tcrdbqrysetlimit(self->rqry, max, skip);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


/* RTDBQuery.filter(column, condition, expr) */
PyDoc_STRVAR(RTDBQuery_filter_doc,
"filter(column, condition, expr)\n\
\n\
");

static PyObject *
RTDBQuery_filter(RTDBQuery *self, PyObject *args)
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
    Py_BEGIN_ALLOW_THREADS
    tcrdbqryaddcond(self->rqry, column, condition, expr);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


/* RTDBQuery.count() */
PyDoc_STRVAR(RTDBQuery_count_doc,
"Returns the length of the result set.");

static PyObject *
RTDBQuery_count(RTDBQuery *self)
{
    int count;

    Py_BEGIN_ALLOW_THREADS
    count = tcrdbqrysearchcount(self->rqry);
    Py_END_ALLOW_THREADS
    return PyInt_FromLong((long)count);
}


/* RTDBQueryType.tp_methods */
static PyMethodDef RTDBQuery_tp_methods[] = {
    {"search", (PyCFunction)RTDBQuery_search, METH_NOARGS, RTDBQuery_search_doc},
    {"remove", (PyCFunction)RTDBQuery_remove, METH_NOARGS, RTDBQuery_remove_doc},
    {"sort", (PyCFunction)RTDBQuery_sort, METH_VARARGS, RTDBQuery_sort_doc},
    {"limit", (PyCFunction)RTDBQuery_limit, METH_VARARGS, RTDBQuery_limit_doc},
    {"filter", (PyCFunction)RTDBQuery_filter, METH_VARARGS, RTDBQuery_filter_doc},
    {"count", (PyCFunction)RTDBQuery_count, METH_NOARGS, RTDBQuery_count_doc},
    {NULL}  /* Sentinel */
};


/* RTDBQuery.hint */
PyDoc_STRVAR(RTDBQuery_hint_doc,
"TODO.");

static PyObject *
RTDBQuery_hint_get(RTDBQuery *self, void *closure)
{
    return PyString_FromString(tcrdbqryhint(self->rqry));
}


/* RTDBQueryType.tp_getsets */
static PyGetSetDef RTDBQuery_tp_getsets[] = {
    {"hint", (getter)RTDBQuery_hint_get, NULL, RTDBQuery_hint_doc, NULL},
    {NULL}  /* Sentinel */
};


/* RTDBQueryType */
static PyTypeObject RTDBQueryType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.tyrant.RTDBQuery",                 /*tp_name*/
    sizeof(RTDBQuery),                        /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)RTDBQuery_tp_dealloc,         /*tp_dealloc*/
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
    (traverseproc)RTDBQuery_tp_traverse,      /*tp_traverse*/
    (inquiry)RTDBQuery_tp_clear,              /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    RTDBQuery_tp_methods,                     /*tp_methods*/
    0,                                        /*tp_members*/
    RTDBQuery_tp_getsets,                     /*tp_getsets*/
};


/*******************************************************************************
* RTDBType
*******************************************************************************/

/* RTDBType.tp_doc */
PyDoc_STRVAR(RTDB_tp_doc,
"RTDB()\n\
\n\
Remote Table Database.\n\
\n\
See also:\n\
Tokyo Tyrant Remote Table Database API at:\n\
http://1978th.net/tokyotyrant/spex.html#tcrdbapi_apitbl");


/* RTDB_tp_as_sequence.sq_contains */
static
int RTDB_Contains(RTDB *self, PyObject *pykey)
{
    void *key;
    int key_size;
    TCMAP *value;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    Py_BEGIN_ALLOW_THREADS
    value = tcrdbtblget(rdbbase->rdb, key, key_size);
    Py_END_ALLOW_THREADS
    if (!value) {
        if (tcrdbecode(rdbbase->rdb) == TTENOREC) {
            return 0;
        }
        set_rdb_error(rdbbase->rdb, NULL);
        return -1;
    }
    tcmapdel(value);
    return 1;
}


/* RTDBType.tp_as_sequence */
static PySequenceMethods RTDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)RTDB_Contains,                /*sq_contains*/
};


/* RTDB_tp_as_mapping.mp_subscript */
static PyObject *
RTDB_GetItem(RTDB *self, PyObject *pykey)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pyvalue;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    value = tcrdbtblget(rdbbase->rdb, key, key_size);
    Py_END_ALLOW_THREADS
    if (!value) {
        return set_rdb_error(rdbbase->rdb, key);
    }
    pyvalue = tcmap_to_dict(value);
    tcmapdel(value);
    return pyvalue;
}


/* RTDB_tp_as_mapping.mp_ass_subscript */
static int
RTDB_SetItem(RTDB *self, PyObject *pykey, PyObject *pyvalue)
{
    void *key;
    int key_size;
    TCMAP *value;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    if (pyvalue) {
        value = dict_to_tcmap(pyvalue);
        if (!value) {
            return -1;
        }
        Py_BEGIN_ALLOW_THREADS
        result = tcrdbtblput(rdbbase->rdb, key, key_size, value);
        Py_END_ALLOW_THREADS
        if (!result) {
            tcmapdel(value);
            set_rdb_error(rdbbase->rdb, NULL);
            return -1;
        }
        tcmapdel(value);
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        result = tcrdbtblout(rdbbase->rdb, key, key_size);
        Py_END_ALLOW_THREADS
        if (!result) {
            set_rdb_error(rdbbase->rdb, key);
            return -1;
        }
    }
    rdbbase->changed = true;
    return 0;
}


/* RTDBType.tp_as_mapping */
static PyMappingMethods RTDB_tp_as_mapping = {
    (lenfunc)RDBBase_Length,                  /*mp_length*/
    (binaryfunc)RTDB_GetItem,                 /*mp_subscript*/
    (objobjargproc)RTDB_SetItem               /*mp_ass_subscript*/
};


/* RTDB.open(host, port) */
PyDoc_STRVAR(RTDB_open_doc,
"open(host, port)\n\
\n\
Open a database.\n\
'host': hostname/address.\n\
'port': port number.");

static PyObject *
RTDB_open(RTDB *self, PyObject *args, PyObject *kwargs)
{
    const char *dbtype;

    dbtype = RDBBase_open((RDBBase *)self, args, kwargs);
    if (!dbtype) {
        return NULL;
    }
    if (strcmp(dbtype, "table")) {
        return set_error(Error, "wrong db type, use tokyo.tyrant.RDB");
    }
    Py_RETURN_NONE;
}


/* RTDB.get(key) */
PyDoc_STRVAR(RTDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
RTDB_get(RTDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return RTDB_GetItem(self, pykey);
}


/* RTDB.remove(key) */
PyDoc_STRVAR(RTDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
RTDB_remove(RTDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (RTDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* RTDB.put(key, value) */
PyDoc_STRVAR(RTDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
RTDB_put(RTDB *self, PyObject *args, PyObject *kwargs)
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
    if (RTDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* RTDB.putkeep(key, value) */
PyDoc_STRVAR(RTDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (tdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
RTDB_putkeep(RTDB *self, PyObject *args, PyObject *kwargs)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pykey, *pyvalue = NULL;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

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
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbtblputkeep(rdbbase->rdb, key, key_size, value);
    tcmapdel(value);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(rdbbase->rdb, key);
    }
    rdbbase->changed = true;
    Py_RETURN_NONE;
}


/* RTDB.putcat(key, value) */
PyDoc_STRVAR(RTDB_putcat_doc,
"putcat(key, value)\n\
\n\
Merge a value with an existing one, does not override existing items in current\n\
value. If there is no corresponding record, a new record is stored.");

static PyObject *
RTDB_putcat(RTDB *self, PyObject *args, PyObject *kwargs)
{
    void *key;
    int key_size;
    TCMAP *value;
    PyObject *pykey, *pyvalue = NULL;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

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
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbtblputcat(rdbbase->rdb, key, key_size, value);
    tcmapdel(value);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    rdbbase->changed = true;
    Py_RETURN_NONE;
}


/* RTDB.setindex(column, type) */
PyDoc_STRVAR(RTDB_setindex_doc,
"setindex(column, type)\n\
\n\
Add an index to a column.");

static PyObject *
RTDB_setindex(RTDB *self, PyObject *args)
{
#if PY_MAJOR_VERSION >= 3
    const char *format = "yi:setindex";
#else
    const char *format = "si:setindex";
#endif
    const char *column;
    int type;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, format, &column, &type)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbtblsetindex(rdbbase->rdb, column, type);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    Py_RETURN_NONE;
}


/* RTDB.uid() */
PyDoc_STRVAR(RTDB_uid_doc,
"uid()\n\
\n\
Return a new unique id.");

static PyObject *
RTDB_uid(RTDB *self)
{
    long long uid;
    RDBBase *rdbbase = (RDBBase *)self;

    Py_BEGIN_ALLOW_THREADS
    uid = tcrdbtblgenuid(rdbbase->rdb);
    Py_END_ALLOW_THREADS
    if (uid == -1) {
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    return PyLong_FromLongLong(uid);
}


/* RTDB.query() */
PyDoc_STRVAR(RTDB_query_doc,
"query()\n\
\n\
");

static PyObject *
RTDB_query(RTDB *self)
{
    return (PyObject *)new_RTDBQuery(&RTDBQueryType, self);
}


/* RTDB.metasearch(queries, type) */
PyDoc_STRVAR(RTDB_metasearch_doc,
"metasearch(queries, type)\n\
\n\
Combine queries.");

static PyObject *
RTDB_metasearch(RTDB *notused, PyObject *args)
{
    PyObject *pyqueries, *pyseq, *pyquery, *pyresult;
    int type;
    const char *msg = "a sequence of RTDBQuery's is required";
    Py_ssize_t len, i;
    RDBQRY **queries;
    TCLIST *result;

    if (!PyArg_ParseTuple(args, "Oi:metasearch", &pyqueries, &type)) {
        return NULL;
    }
    pyseq = PySequence_Fast(pyqueries, msg);
    if (!pyseq) {
        return NULL;
    }
    len = PySequence_Fast_GET_SIZE(pyseq);
#ifdef TK_PY_SIZE_T_BIGGER_THAN_INT
    if (len > TK_PY_MAX_LEN) {
        set_error(PyExc_OverflowError, "sequence is too large");
        Py_DECREF(pyseq);
        return NULL;
    }
#endif
    queries = (RDBQRY **)tcmalloc((size_t)len * sizeof(RDBQRY *));
    if (!queries) {
        Py_DECREF(pyseq);
        return PyErr_NoMemory();
    }
    for (i = 0; i < len; i++) {
        pyquery = PySequence_Fast_GET_ITEM(pyseq, i);
        if (!PyObject_TypeCheck(pyquery, &RTDBQueryType)) {
            Py_DECREF(pyseq);
            tcfree(queries);
            return set_error(PyExc_TypeError, msg);
        }
        queries[i] = ((RTDBQuery *)pyquery)->rqry;
    }
    Py_DECREF(pyseq);
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbmetasearch(queries, (int)len, type);
    tcfree(queries);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_tuple(result);
    tclistdel(result);
    return pyresult;
}


/* RTDBType.tp_methods */
static PyMethodDef RTDB_tp_methods[] = {
    {"open", (PyCFunction)RTDB_open, METH_VARARGS | METH_KEYWORDS, RTDB_open_doc},
    {"get", (PyCFunction)RTDB_get, METH_VARARGS, RTDB_get_doc},
    {"remove", (PyCFunction)RTDB_remove, METH_VARARGS, RTDB_remove_doc},
    {"put", (PyCFunction)RTDB_put, METH_VARARGS | METH_KEYWORDS, RTDB_put_doc},
    {"putkeep", (PyCFunction)RTDB_putkeep, METH_VARARGS | METH_KEYWORDS,
     RTDB_putkeep_doc},
    {"putcat", (PyCFunction)RTDB_putcat, METH_VARARGS | METH_KEYWORDS,
     RTDB_putcat_doc},
    {"setindex", (PyCFunction)RTDB_setindex, METH_VARARGS, RTDB_setindex_doc},
    {"uid", (PyCFunction)RTDB_uid, METH_NOARGS, RTDB_uid_doc},
    {"query", (PyCFunction)RTDB_query, METH_NOARGS, RTDB_query_doc},
    {"metasearch", (PyCFunction)RTDB_metasearch, METH_VARARGS | METH_STATIC,
     RTDB_metasearch_doc},
    {NULL}  /* Sentinel */
};


/* RTDBType */
static PyTypeObject RTDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.tyrant.RTDB",                      /*tp_name*/
    sizeof(RTDB),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    0,                                        /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &RTDB_tp_as_sequence,                     /*tp_as_sequence*/
    &RTDB_tp_as_mapping,                      /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    RTDB_tp_doc,                              /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    RTDB_tp_methods,                          /*tp_methods*/
};
