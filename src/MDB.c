/*******************************************************************************
* MDBType
*******************************************************************************/

/* MDBType.tp_doc */
PyDoc_STRVAR(MDB_tp_doc,
"MDB([bnum])\n\
\n\
On-memory Hash Database.\n\
'bnum': the number of elements in a bucket array. If ommited or specified as 0,\n\
        the default value (65536) is used.\n\
\n\
See also:\n\
Tokyo Cabinet On-memory Hash Database API at:\n\
http://1978th.net/tokyocabinet/spex-en.html#tcutilapi_mdbapi");


/* MDBType.tp_dealloc */
static void
MDB_tp_dealloc(MDB *self)
{
    if (self->mdb) {
        tcmdbdel(self->mdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* MDBType.tp_new */
static PyObject *
MDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    unsigned long bnum = 0;
    MDB *self;

    if (!PyArg_ParseTuple(args, "|k:__new__", &bnum)) {
        return NULL;
    }
    self = (MDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->mdb */
    if (bnum) {
        self->mdb = tcmdbnew2(bnum);
    }
    else {
        self->mdb = tcmdbnew();
    }
    if (!self->mdb) {
        set_error(Error, "could not create MDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* MDB_tp_as_sequence.sq_contains */
static
int MDB_Contains(MDB *self, PyObject *pykey)
{
    const char *key, *value;

    key = PyBytes_AsString(pykey);
    if (!key) {
        return -1;
    }
    value = tcmdbget2(self->mdb, key);
    if (!value) {
        return 0;
    }
    tcfree((void *)value);
    return 1;
}


/* MDBType.tp_as_sequence */
static PySequenceMethods MDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)MDB_Contains,                 /*sq_contains*/
};


/* MDB_tp_as_mapping.mp_length */
static Py_ssize_t
MDB_Length(MDB *self)
{
    return (Py_ssize_t)tcmdbrnum(self->mdb);
}


/* MDB_tp_as_mapping.mp_subscript */
static PyObject *
MDB_GetItem(MDB *self, PyObject *pykey)
{
    const char *key, *value;
    PyObject *pyvalue;

    key = PyBytes_AsString(pykey);
    if (!key) {
        return NULL;
    }
    value = tcmdbget2(self->mdb, key);
    if (!value) {
        return set_key_error(key);
    }
    pyvalue = PyBytes_FromString(value);
    tcfree((void *)value);
    return pyvalue;
}


/* MDB_tp_as_mapping.mp_ass_subscript */
static int
MDB_SetItem(MDB *self, PyObject *pykey, PyObject *pyvalue)
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
        tcmdbput2(self->mdb, key, value);
    }
    else {
        if (!tcmdbout2(self->mdb, key)) {
            set_key_error(key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* MDBType.tp_as_mapping */
static PyMappingMethods MDB_tp_as_mapping = {
    (lenfunc)MDB_Length,                      /*mp_length*/
    (binaryfunc)MDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)MDB_SetItem                /*mp_ass_subscript*/
};


/* MDBType.tp_iter */
static PyObject *
MDB_tp_iter(MDB *self)
{
    tcmdbiterinit(self->mdb);
    self->changed = false;
    Py_INCREF(self);
    return (PyObject *)self;
}


/* MDBType.tp_iternext */
static PyObject *
MDB_tp_iternext(MDB *self)
{
    const char *key;
    PyObject *pykey;

    if (self->changed) {
        return set_error(Error, "MDB changed during iteration");
    }
    key = tcmdbiternext2(self->mdb);
    if (!key) {
        return set_stopiteration_error();
    }
    pykey = PyBytes_FromString(key);
    tcfree((void *)key);
    return pykey;
}


/* MDB.__length_hint__ */
PyDoc_STRVAR(MDB_length_hint_doc,
"Private method returning an estimate of len(list(mdb)).");

static PyObject *
MDB_length_hint(MDB *self)
{
    return PyLong_FromSsize_t(MDB_Length(self));
}


/* MDB.clear() */
PyDoc_STRVAR(MDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
MDB_clear(MDB *self)
{
    tcmdbvanish(self->mdb);
    self->changed = true;
    Py_RETURN_NONE;
}


/* MDB.get(key) */
PyDoc_STRVAR(MDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
MDB_get(MDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return MDB_GetItem(self, pykey);
}


/* MDB.remove(key) */
PyDoc_STRVAR(MDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
MDB_remove(MDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (MDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* MDB.put(key, value) */
PyDoc_STRVAR(MDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
MDB_put(MDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (MDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* MDB.putkeep(key, value) */
PyDoc_STRVAR(MDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (mdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
MDB_putkeep(MDB *self, PyObject *args)
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
    if (!tcmdbputkeep2(self->mdb, key, value)) {
        return set_key_error(key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* MDB.putcat(key, value) */
PyDoc_STRVAR(MDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
MDB_putcat(MDB *self, PyObject *args)
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
    tcmdbputcat2(self->mdb, key, value);
    self->changed = true;
    Py_RETURN_NONE;
}


/* MDB.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(MDB_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
MDB_searchkeys(MDB *self, PyObject *args)
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
    result = tcmdbfwmkeys2(self->mdb, prefix, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* MDBType.tp_methods */
static PyMethodDef MDB_tp_methods[] = {
    {"__length_hint__", (PyCFunction)MDB_length_hint, METH_NOARGS,
     MDB_length_hint_doc},
    {"clear", (PyCFunction)MDB_clear, METH_NOARGS, MDB_clear_doc},
    {"get", (PyCFunction)MDB_get, METH_VARARGS, MDB_get_doc},
    {"remove", (PyCFunction)MDB_remove, METH_VARARGS, MDB_remove_doc},
    {"put", (PyCFunction)MDB_put, METH_VARARGS, MDB_put_doc},
    {"putkeep", (PyCFunction)MDB_putkeep, METH_VARARGS, MDB_putkeep_doc},
    {"putcat", (PyCFunction)MDB_putcat, METH_VARARGS, MDB_putcat_doc},
    {"searchkeys", (PyCFunction)MDB_searchkeys, METH_VARARGS,
     MDB_searchkeys_doc},
    {NULL}  /* Sentinel */
};


/* MDB.size */
PyDoc_STRVAR(MDB_size_doc,
"The size in bytes of the database.");

static PyObject *
MDB_size_get(MDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcmdbmsiz(self->mdb));
}


/* MDBType.tp_getsets */
static PyGetSetDef MDB_tp_getsets[] = {
    {"size", (getter)MDB_size_get, NULL, MDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* MDBType */
static PyTypeObject MDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.MDB",                      /*tp_name*/
    sizeof(MDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)MDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &MDB_tp_as_sequence,                      /*tp_as_sequence*/
    &MDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    MDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)MDB_tp_iter,                 /*tp_iter*/
    (iternextfunc)MDB_tp_iternext,            /*tp_iternext*/
    MDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    MDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    MDB_tp_new,                               /*tp_new*/
};
