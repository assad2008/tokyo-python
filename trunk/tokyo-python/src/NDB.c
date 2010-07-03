/*******************************************************************************
* NDB iterator types
*******************************************************************************/

/* new_NDBIter */
static PyObject *
new_NDBIter(NDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    tcndbiterinit(self->ndb);
    self->changed = false;
    return iter;
}


/* NDBIterKeysType.tp_iternext */
static PyObject *
NDBIterKeys_tp_iternext(DBIter *self)
{
    NDB *ndb = (NDB *)self->db;
    void *key;
    int key_size;
    PyObject *pykey;

    if (ndb->changed) {
        return set_error(Error, "NDB changed during iteration");
    }
    key = tcndbiternext(ndb->ndb, &key_size);
    if (!key) {
        return set_stopiteration_error();
    }
    pykey = PyBytes_FromStringAndSize((char *)key, (Py_ssize_t)key_size);
    tcfree(key);
    return pykey;
}


/* NDBIterKeysType */
static PyTypeObject NDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.NDBIterKeys",              /*tp_name*/
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
    (iternextfunc)NDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* NDBIterValuesType.tp_iternext */
static PyObject *
NDBIterValues_tp_iternext(DBIter *self)
{
    NDB *ndb = (NDB *)self->db;
    void *key, *value;
    int key_size, value_size;
    PyObject *pyvalue;

    if (ndb->changed) {
        return set_error(Error, "NDB changed during iteration");
    }
    key = tcndbiternext(ndb->ndb, &key_size);
    if (!key) {
        return set_stopiteration_error();
    }
    value = tcndbget(ndb->ndb, key, key_size, &value_size);
    pyvalue = PyBytes_FromStringAndSize((char *)value, (Py_ssize_t)value_size);
    tcfree(key);
    tcfree(value);
    return pyvalue;
}


/* NDBIterValuesType */
static PyTypeObject NDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.NDBIterValues",            /*tp_name*/
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
    (iternextfunc)NDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* NDBIterItemsType.tp_iternext */
static PyObject *
NDBIterItems_tp_iternext(DBIter *self)
{
    NDB *ndb = (NDB *)self->db;
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (ndb->changed) {
        return set_error(Error, "NDB changed during iteration");
    }
    key = tcndbiternext(ndb->ndb, &key_size);
    if (!key) {
        return set_stopiteration_error();
    }
    value = tcndbget(ndb->ndb, key, key_size, &value_size);
    pykey = PyBytes_FromStringAndSize((char *)key, (Py_ssize_t)key_size);
    pyvalue = PyBytes_FromStringAndSize((char *)value, (Py_ssize_t)value_size);
    if (pykey && pyvalue) {
        pyresult = PyTuple_Pack(2, pykey, pyvalue);
    }
    Py_XDECREF(pykey);
    Py_XDECREF(pyvalue);
    tcfree(key);
    tcfree(value);
    return pyresult;
}


/* NDBIterItemsType */
static PyTypeObject NDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.NDBIterItems",             /*tp_name*/
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
    (iternextfunc)NDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* NDBType
*******************************************************************************/

/* NDBType.tp_doc */
PyDoc_STRVAR(NDB_tp_doc,
"NDB()\n\
\n\
On-memory Tree Database.\n\
\n\
See also:\n\
Tokyo Cabinet On-memory Tree Database API at:\n\
http://1978th.net/tokyocabinet/spex-en.html#tcutilapi_ndbapi");


/* NDBType.tp_dealloc */
static void
NDB_tp_dealloc(NDB *self)
{
    if (self->ndb) {
        tcndbdel(self->ndb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* NDBType.tp_new */
static PyObject *
NDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    NDB *self = (NDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->ndb */
    self->ndb = tcndbnew();
    if (!self->ndb) {
        set_error(Error, "could not create NDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* NDB_tp_as_sequence.sq_contains */
static
int NDB_Contains(NDB *self, PyObject *pykey)
{
    char *key;
    Py_ssize_t key_size;
    void *value;
    int value_size;

    if (PyBytes_AsStringAndSize(pykey, &key, &key_size)) {
        return -1;
    }
    value = tcndbget(self->ndb, (void *)key, (int)key_size, &value_size);
    if (!value) {
        return 0;
    }
    tcfree(value);
    return 1;
}


/* NDBType.tp_as_sequence */
static PySequenceMethods NDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)NDB_Contains,                 /*sq_contains*/
};


/* NDB_tp_as_mapping.mp_length */
static Py_ssize_t
NDB_Length(NDB *self)
{
    return (Py_ssize_t)tcndbrnum(self->ndb);
}


/* NDB_tp_as_mapping.mp_subscript */
static PyObject *
NDB_GetItem(NDB *self, PyObject *pykey)
{
    char *key;
    Py_ssize_t key_size;
    void *value;
    int value_size;
    PyObject *pyvalue;

    if (PyBytes_AsStringAndSize(pykey, &key, &key_size)) {
        return NULL;
    }
    value = tcndbget(self->ndb, (void *)key, (int)key_size, &value_size);
    if (!value) {
        return set_key_error(key);
    }
    pyvalue = PyBytes_FromStringAndSize((char *)value, (Py_ssize_t)value_size);
    tcfree(value);
    return pyvalue;
}


/* NDB_tp_as_mapping.mp_ass_subscript */
static int
NDB_SetItem(NDB *self, PyObject *pykey, PyObject *pyvalue)
{
    char *key, *value;
    Py_ssize_t key_size, value_size;

    if (PyBytes_AsStringAndSize(pykey, &key, &key_size)) {
        return -1;
    }
    if (pyvalue) {
        if (PyBytes_AsStringAndSize(pyvalue, &value, &value_size)) {
            return -1;
        }
        tcndbput(self->ndb, (void *)key, (int)key_size,
                 (void *)value, (int)value_size);
    }
    else {
        if (!tcndbout(self->ndb, (void *)key, (int)key_size)) {
            set_key_error(key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* NDBType.tp_as_mapping */
static PyMappingMethods NDB_tp_as_mapping = {
    (lenfunc)NDB_Length,                      /*mp_length*/
    (binaryfunc)NDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)NDB_SetItem                /*mp_ass_subscript*/
};


/* NDBType.tp_iter */
static PyObject *
NDB_tp_iter(NDB *self)
{
    return new_NDBIter(self, &NDBIterKeysType);
}


/* NDB.clear() */
PyDoc_STRVAR(NDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
NDB_clear(NDB *self)
{
    tcndbvanish(self->ndb);
    self->changed = true;
    Py_RETURN_NONE;
}


/* NDB.get(key) */
PyDoc_STRVAR(NDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
NDB_get(NDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return NDB_GetItem(self, pykey);
}


/* NDB.remove(key) */
PyDoc_STRVAR(NDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
NDB_remove(NDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (NDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* NDB.put(key, value) */
PyDoc_STRVAR(NDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
NDB_put(NDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (NDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* NDB.putkeep(key, value) */
PyDoc_STRVAR(NDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (ndb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
NDB_putkeep(NDB *self, PyObject *args)
{
    char *key, *value;
    Py_ssize_t key_size, value_size;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putkeep", &pykey, &pyvalue)) {
        return NULL;
    }
    if (PyBytes_AsStringAndSize(pykey, &key, &key_size) ||
        PyBytes_AsStringAndSize(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcndbputkeep(self->ndb, (void *)key, (int)key_size,
                      (void *)value, (int)value_size)) {
        return set_key_error(key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* NDB.putcat(key, value) */
PyDoc_STRVAR(NDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
NDB_putcat(NDB *self, PyObject *args)
{
    char *key, *value;
    Py_ssize_t key_size, value_size;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putcat", &pykey, &pyvalue)) {
        return NULL;
    }
    if (PyBytes_AsStringAndSize(pykey, &key, &key_size) ||
        PyBytes_AsStringAndSize(pyvalue, &value, &value_size)) {
        return NULL;
    }
    tcndbputcat(self->ndb, (void *)key, (int)key_size,
                (void *)value, (int)value_size);
    self->changed = true;
    Py_RETURN_NONE;
}


/* NDB.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(NDB_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
NDB_searchkeys(NDB *self, PyObject *args)
{
    char *prefix;
    Py_ssize_t prefix_size;
    int max = -1;
    TCLIST *result;
    PyObject *pyprefix, *pyresult;

    if (!PyArg_ParseTuple(args, "O|i:searchkeys", &pyprefix, &max)) {
        return NULL;
    }
    if (PyBytes_AsStringAndSize(pyprefix, &prefix, &prefix_size)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcndbfwmkeys(self->ndb, (void *)prefix, (int)prefix_size, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* NDB.iterkeys() */
PyDoc_STRVAR(NDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
NDB_iterkeys(NDB *self)
{
    return new_NDBIter(self, &NDBIterKeysType);
}


/* NDB.itervalues() */
PyDoc_STRVAR(NDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
NDB_itervalues(NDB *self)
{
    return new_NDBIter(self, &NDBIterValuesType);
}


/* NDB.iteritems() */
PyDoc_STRVAR(NDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
NDB_iteritems(NDB *self)
{
    return new_NDBIter(self, &NDBIterItemsType);
}


/* NDBType.tp_methods */
static PyMethodDef NDB_tp_methods[] = {
    {"clear", (PyCFunction)NDB_clear, METH_NOARGS, NDB_clear_doc},
    {"get", (PyCFunction)NDB_get, METH_VARARGS, NDB_get_doc},
    {"remove", (PyCFunction)NDB_remove, METH_VARARGS, NDB_remove_doc},
    {"put", (PyCFunction)NDB_put, METH_VARARGS, NDB_put_doc},
    {"putkeep", (PyCFunction)NDB_putkeep, METH_VARARGS, NDB_putkeep_doc},
    {"putcat", (PyCFunction)NDB_putcat, METH_VARARGS, NDB_putcat_doc},
    {"searchkeys", (PyCFunction)NDB_searchkeys, METH_VARARGS,
     NDB_searchkeys_doc},
    {"iterkeys", (PyCFunction)NDB_iterkeys, METH_NOARGS, NDB_iterkeys_doc},
    {"itervalues", (PyCFunction)NDB_itervalues, METH_NOARGS, NDB_itervalues_doc},
    {"iteritems", (PyCFunction)NDB_iteritems, METH_NOARGS, NDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* NDB.size */
PyDoc_STRVAR(NDB_size_doc,
"The size in bytes of the database.");

static PyObject *
NDB_size_get(NDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcndbmsiz(self->ndb));
}


/* NDBType.tp_getsets */
static PyGetSetDef NDB_tp_getsets[] = {
    {"size", (getter)NDB_size_get, NULL, NDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* NDBType */
static PyTypeObject NDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.NDB",                      /*tp_name*/
    sizeof(NDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &NDB_tp_as_sequence,                      /*tp_as_sequence*/
    &NDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    NDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)NDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    NDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    NDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    NDB_tp_new,                               /*tp_new*/
};
