/*******************************************************************************
* RDBType
*******************************************************************************/

/* RDBType.tp_doc */
PyDoc_STRVAR(RDB_tp_doc,
"RDB()\n\
\n\
Remote Database.\n\
\n\
See also:\n\
Tokyo Tyrant Remote Database API at:\n\
http://1978th.net/tokyotyrant/spex.html#tcrdbapi");


/* RDB_tp_as_sequence.sq_contains */
static int
RDB_Contains(RDB *self, PyObject *pykey)
{
    void *key, *value;
    int key_size, value_size;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size, true)) {
        return -1;
    }
    Py_BEGIN_ALLOW_THREADS
    value = tcrdbget(rdbbase->rdb, key, key_size, &value_size);
    Py_END_ALLOW_THREADS
    if (!value) {
        if (tcrdbecode(rdbbase->rdb) == TTENOREC) {
            return 0;
        }
        set_rdb_error(rdbbase->rdb, NULL);
        return -1;
    }
    tcfree(value);
    return 1;
}


/* RDBType.tp_as_sequence */
static PySequenceMethods RDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)RDB_Contains,                 /*sq_contains*/
};


/* RDB_tp_as_mapping.mp_subscript */
static PyObject *
RDB_GetItem(RDB *self, PyObject *pykey)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pyvalue;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size, true)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    value = tcrdbget(rdbbase->rdb, key, key_size, &value_size);
    Py_END_ALLOW_THREADS
    if (!value) {
        return set_rdb_error(rdbbase->rdb, key);
    }
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    return pyvalue;
}


/* RDB_tp_as_mapping.mp_ass_subscript */
static int
RDB_SetItem(RDB *self, PyObject *pykey, PyObject *pyvalue)
{
    void *key, *value;
    int key_size, value_size;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (bytes_to_void(pykey, &key, &key_size, true)) {
        return -1;
    }
    if (pyvalue) {
        if (bytes_to_void(pyvalue, &value, &value_size, false)) {
            return -1;
        }
        Py_BEGIN_ALLOW_THREADS
        result = tcrdbput(rdbbase->rdb, key, key_size, value, value_size);
        Py_END_ALLOW_THREADS
        if (!result) {
            set_rdb_error(rdbbase->rdb, NULL);
            return -1;
        }
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        result = tcrdbout(rdbbase->rdb, key, key_size);
        Py_END_ALLOW_THREADS
        if (!result) {
            set_rdb_error(rdbbase->rdb, key);
            return -1;
        }
    }
    rdbbase->changed = true;
    return 0;
}


/* RDBType.tp_as_mapping */
static PyMappingMethods RDB_tp_as_mapping = {
    (lenfunc)RDBBase_Length,                  /*mp_length*/
    (binaryfunc)RDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)RDB_SetItem                /*mp_ass_subscript*/
};


/* RDB.open(host, port) */
PyDoc_STRVAR(RDB_open_doc,
"open(host, port)\n\
\n\
Open a database.\n\
'host': hostname/address.\n\
'port': port number.");

static PyObject *
RDB_open(RDB *self, PyObject *args, PyObject *kwargs)
{
    const char *dbtype;

    dbtype = RDBBase_open((RDBBase *)self, args, kwargs);
    if (!dbtype) {
        return NULL;
    }
    if (!strcmp(dbtype, "table")) {
        return set_error(Error, "wrong db type, use tokyo.tyrant.RTDB");
    }
    Py_RETURN_NONE;
}


/* RDB.get(key) */
PyDoc_STRVAR(RDB_get_doc,
"get(key)\n\
\n\
Retrieve a record from the database.");

static PyObject *
RDB_get(RDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:get", &pykey)) {
        return NULL;
    }
    return RDB_GetItem(self, pykey);
}


/* RDB.remove(key) */
PyDoc_STRVAR(RDB_remove_doc,
"remove(key)\n\
\n\
Remove a record from the database.");

static PyObject *
RDB_remove(RDB *self, PyObject *args)
{
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:remove", &pykey)) {
        return NULL;
    }
    if (RDB_SetItem(self, pykey, NULL)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* RDB.put(key, value) */
PyDoc_STRVAR(RDB_put_doc,
"put(key, value)\n\
\n\
Store a record in the database.");

static PyObject *
RDB_put(RDB *self, PyObject *args)
{
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:put", &pykey, &pyvalue)) {
        return NULL;
    }
    if (RDB_SetItem(self, pykey, pyvalue)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* RDB.putkeep(key, value) */
PyDoc_STRVAR(RDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (rdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
RDB_putkeep(RDB *self, PyObject *args)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, "OO:putkeep", &pykey, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size, true) ||
        bytes_to_void(pyvalue, &value, &value_size, false)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbputkeep(rdbbase->rdb, key, key_size, value, value_size);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(rdbbase->rdb, key);
    }
    rdbbase->changed = true;
    Py_RETURN_NONE;
}


/* RDB.putcat(key, value) */
PyDoc_STRVAR(RDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
RDB_putcat(RDB *self, PyObject *args)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue;
    bool result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, "OO:putcat", &pykey, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size, true) ||
        bytes_to_void(pyvalue, &value, &value_size, false)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbputcat(rdbbase->rdb, key, key_size, value, value_size);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    rdbbase->changed = true;
    Py_RETURN_NONE;
}


/* RDB.putnb(key, value) */
PyDoc_STRVAR(RDB_putnb_doc,
"putnb(key, value)\n\
\n\
Store a record in the database. This method doesn't wait for the server to \n\
respond (non-blocking).");

static PyObject *
RDB_putnb(RDB *self, PyObject *args)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, "OO:putnb", &pykey, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size, true) ||
        bytes_to_void(pyvalue, &value, &value_size, false)) {
        return NULL;
    }
    if (!tcrdbputnr(rdbbase->rdb, key, key_size, value, value_size)) {
        return set_rdb_error(rdbbase->rdb, NULL);
    }
    rdbbase->changed = true;
    Py_RETURN_NONE;
}


/* RDB.addint(key, num) -> int */
PyDoc_STRVAR(RDB_addint_doc,
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
RDB_addint(RDB *self, PyObject *args)
{
    PyObject *pykey;
    void *key, *value;
    int key_size, value_size, num, result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, "Oi:addint", &pykey, &num)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size, true)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbaddint(rdbbase->rdb, key, key_size, num);
    Py_END_ALLOW_THREADS
    //if (result == INT_MIN) {
    //    ecode = tcrdbecode(self->rdb);
    //    if (ecode != TTESUCCESS && ecode != TTENOREC) {
    //        return set_rdb_error(self->rdb, key);
    //    }
    //}
    // XXX: there is a bug/incoherence in Tokyo Tyrant, it sets TTEKEEP instead
    //      of TTENOREC when INT_MIN is an acceptable value.
    //      Hackish solution below :-(.
    //      TODO: report upstream
    if (result == INT_MIN && tcrdbecode(rdbbase->rdb) != TTESUCCESS) {
        Py_BEGIN_ALLOW_THREADS
        value = tcrdbget(rdbbase->rdb, key, key_size, &value_size);
        Py_END_ALLOW_THREADS
        if (strcmp((char *)value, "")) {
            return set_rdb_error(rdbbase->rdb, key);
        }
    }
    if (num) {
        rdbbase->changed = true;
    }
    return PyInt_FromLong((long)result);
}


/* RDB.adddouble(key, num) -> float */
PyDoc_STRVAR(RDB_adddouble_doc,
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
RDB_adddouble(RDB *self, PyObject *args)
{
    PyObject *pykey;
    void *key;
    int key_size;
    double num, result;
    RDBBase *rdbbase = (RDBBase *)self;

    if (!PyArg_ParseTuple(args, "Od:adddouble", &pykey, &num)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size, true)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcrdbadddouble(rdbbase->rdb, key, key_size, num);
    Py_END_ALLOW_THREADS
    if (Py_IS_NAN(result)) {
        return set_rdb_error(rdbbase->rdb, key);
    }
    if (num) {
        rdbbase->changed = true;
    }
    return PyFloat_FromDouble(result);
}


/* RDBType.tp_methods */
static PyMethodDef RDB_tp_methods[] = {
    {"open", (PyCFunction)RDB_open, METH_VARARGS | METH_KEYWORDS, RDB_open_doc},
    {"get", (PyCFunction)RDB_get, METH_VARARGS, RDB_get_doc},
    {"remove", (PyCFunction)RDB_remove, METH_VARARGS, RDB_remove_doc},
    {"put", (PyCFunction)RDB_put, METH_VARARGS, RDB_put_doc},
    {"putkeep", (PyCFunction)RDB_putkeep, METH_VARARGS, RDB_putkeep_doc},
    {"putcat", (PyCFunction)RDB_putcat, METH_VARARGS, RDB_putcat_doc},
    {"putnb", (PyCFunction)RDB_putnb, METH_VARARGS, RDB_putnb_doc},
    {"addint", (PyCFunction)RDB_addint, METH_VARARGS, RDB_addint_doc},
    {"adddouble", (PyCFunction)RDB_adddouble, METH_VARARGS, RDB_adddouble_doc},
    {NULL}  /* Sentinel */
};


/* RDBType */
static PyTypeObject RDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.tyrant.RDB",                       /*tp_name*/
    sizeof(RDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    0,                                        /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &RDB_tp_as_sequence,                      /*tp_as_sequence*/
    &RDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    RDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    RDB_tp_methods,                           /*tp_methods*/
};
