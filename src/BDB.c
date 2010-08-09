/*******************************************************************************
* utilities
*******************************************************************************/

PyObject *
set_bdb_error(TCBDB *bdb, const char *key)
{
    int ecode;

    ecode = tcbdbecode(bdb);
    if (key && ((ecode == TCENOREC) || (ecode == TCEKEEP))) {
        return set_key_error(key);
    }
    return set_error(Error, tcbdberrmsg(ecode));
}


PyObject *
set_bdbcursor_move_error(TCBDB *bdb)
{
    if (tcbdbecode(bdb) == TCENOREC) {
        return set_stopiteration_error();
    }
    return set_bdb_error(bdb, NULL);
}


/* convert a Python sequence to a TCLIST */
TCLIST *
seq_to_tclist(PyObject *pyvalues)
{
    const char *msg = "a sequence is required";
    PyObject *pyseq;
    Py_ssize_t len, i;
    TCLIST *values;
    void *value;
    int value_size;

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
        if (bytes_to_void(PySequence_Fast_GET_ITEM(pyseq, i), &value,
                          &value_size)) {
            Py_DECREF(pyseq);
            tclistdel(values);
            return NULL;
        }
        tclistpush(values, value, value_size);
    }
    Py_DECREF(pyseq);
    return values;
}


/*******************************************************************************
* BDBCursorType
*******************************************************************************/

/* new_BDBCursor */
BDBCursor *
new_BDBCursor(PyTypeObject *type, BDB *bdb)
{
    BDBCursor *self = (BDBCursor *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->cur */
    self->cur = tcbdbcurnew(bdb->bdb);
    if (!self->cur) {
        set_error(Error, "could not create BDBCursor, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    /* self->bdb */
    Py_INCREF(bdb);
    self->bdb = bdb;
    return self;
}


/* BDBCursorType.tp_traverse */
static int
BDBCursor_tp_traverse(BDBCursor *self, visitproc visit, void *arg)
{
    Py_VISIT(self->bdb);
    return 0;
}


/* BDBCursorType.tp_clear */
static int
BDBCursor_tp_clear(BDBCursor *self)
{
    Py_CLEAR(self->bdb);
    return 0;
}


/* BDBCursorType.tp_dealloc */
static void
BDBCursor_tp_dealloc(BDBCursor *self)
{
    if (self->cur) {
        tcbdbcurdel(self->cur);
    }
    BDBCursor_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* BDBCursor.first() */
PyDoc_STRVAR(BDBCursor_first_doc,
"first()\n\
\n\
Move to the first record.");

static PyObject *
BDBCursor_first(BDBCursor *self)
{
    if (!tcbdbcurfirst(self->cur)) {
        return set_bdbcursor_move_error(self->bdb->bdb);
    }
    Py_RETURN_NONE;
}


/* BDBCursor.last() */
PyDoc_STRVAR(BDBCursor_last_doc,
"last()\n\
\n\
Move to the last record.");

static PyObject *
BDBCursor_last(BDBCursor *self)
{
    if (!tcbdbcurlast(self->cur)) {
        return set_bdbcursor_move_error(self->bdb->bdb);
    }
    Py_RETURN_NONE;
}


/* BDBCursor.jump(key) */
PyDoc_STRVAR(BDBCursor_jump_doc,
"jump(key)\n\
\n\
Move to the first occurrence of key. If key is not in the database, the cursor\n\
is positioned to the next available record.");

static PyObject *
BDBCursor_jump(BDBCursor *self, PyObject *args)
{
    void *key;
    int key_size;
    PyObject *pykey;

    if (!PyArg_ParseTuple(args, "O:jump", &pykey)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    if (!tcbdbcurjump(self->cur, key, key_size)) {
        return set_bdbcursor_move_error(self->bdb->bdb);
    }
    Py_RETURN_NONE;
}


/* BDBCursor.prev() */
PyDoc_STRVAR(BDBCursor_prev_doc,
"prev()\n\
\n\
Move to the previous record.");

static PyObject *
BDBCursor_prev(BDBCursor *self)
{
    if (!tcbdbcurprev(self->cur)) {
        return set_bdbcursor_move_error(self->bdb->bdb);
    }
    Py_RETURN_NONE;
}


/* BDBCursor.next() */
PyDoc_STRVAR(BDBCursor_next_doc,
"next()\n\
\n\
Move to the next record.");

static PyObject *
BDBCursor_next(BDBCursor *self)
{
    if (!tcbdbcurnext(self->cur)) {
        return set_bdbcursor_move_error(self->bdb->bdb);
    }
    Py_RETURN_NONE;
}


/* BDBCursor.put(value[, mode=BDBCPCURRENT]) */
PyDoc_STRVAR(BDBCursor_put_doc,
"put(value[, mode=BDBCPCURRENT])\n\
\n\
Store a value at/around the cursor's current position.\n\
'mode': can be one of BDBCPCURRENT, BDBCPBEFORE or BDBCPAFTER. If specified as\n\
        BDBCPCURRENT the current value is overwritten, else a new item is stored\n\
        before or after the current one and the cursor is moved to the newly\n\
        inserted record.");

static PyObject *
BDBCursor_put(BDBCursor *self, PyObject *args)
{
    void *value;
    int value_size;
    PyObject *pyvalue;
    int mode = BDBCPCURRENT;

    if (!PyArg_ParseTuple(args, "O|i:put", &pyvalue, &mode)) {
        return NULL;
    }
    if (bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcbdbcurput(self->cur, value, value_size, mode)) {
        return set_bdb_error(self->bdb->bdb, NULL);
    }
    self->bdb->changed = true;
    Py_RETURN_NONE;
}


/* BDBCursor.remove() */
PyDoc_STRVAR(BDBCursor_remove_doc,
"remove()\n\
\n\
Remove the current record. After deletion the cursor is moved to the next\n\
available record (if at all possible).");

static PyObject *
BDBCursor_remove(BDBCursor *self)
{
    if (!tcbdbcurout(self->cur)) {
        return set_bdb_error(self->bdb->bdb, NULL);
    }
    self->bdb->changed = true;
    Py_RETURN_NONE;
}


/* BDBCursor.key() -> string/bytes */
PyDoc_STRVAR(BDBCursor_key_doc,
"key() -> string/bytes\n\
\n\
Get current key.");

static PyObject *
BDBCursor_key(BDBCursor *self)
{
    void *key;
    int key_size;
    PyObject *pykey;

    key = tcbdbcurkey(self->cur, &key_size);
    if (!key) {
        return set_bdb_error(self->bdb->bdb, NULL);
    }
    pykey = void_to_bytes(key, key_size);
    tcfree(key);
    return pykey;
}


/* BDBCursor.value() -> string/bytes */
PyDoc_STRVAR(BDBCursor_value_doc,
"value() -> string/bytes\n\
\n\
Get current value.");

static PyObject *
BDBCursor_value(BDBCursor *self)
{
    void *value;
    int value_size;
    PyObject *pyvalue;

    value = tcbdbcurval(self->cur, &value_size);
    if (!value) {
        return set_bdb_error(self->bdb->bdb, NULL);
    }
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    return pyvalue;
}


/* BDBCursor.item() -> tuple */
PyDoc_STRVAR(BDBCursor_item_doc,
"item() -> tuple\n\
\n\
Get current item (key, value).");

static PyObject *
BDBCursor_item(BDBCursor *self)
{
    TCXSTR *key, *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    key = tcxstrnew();
    value = tcxstrnew();
    if (!tcbdbcurrec(self->cur, key, value)) {
        set_bdb_error(self->bdb->bdb, NULL);
    }
    else {
        pykey = tcxstr_to_bytes(key);
        pyvalue = tcxstr_to_bytes(value);
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


/* BDBCursorType.tp_methods */
static PyMethodDef BDBCursor_tp_methods[] = {
    {"first", (PyCFunction)BDBCursor_first, METH_NOARGS, BDBCursor_first_doc},
    {"last", (PyCFunction)BDBCursor_last, METH_NOARGS, BDBCursor_last_doc},
    {"jump", (PyCFunction)BDBCursor_jump, METH_VARARGS, BDBCursor_jump_doc},
    {"prev", (PyCFunction)BDBCursor_prev, METH_NOARGS, BDBCursor_prev_doc},
    {"next", (PyCFunction)BDBCursor_next, METH_NOARGS, BDBCursor_next_doc},
    {"put", (PyCFunction)BDBCursor_put, METH_VARARGS, BDBCursor_put_doc},
    {"remove", (PyCFunction)BDBCursor_remove, METH_NOARGS, BDBCursor_remove_doc},
    {"key", (PyCFunction)BDBCursor_key, METH_NOARGS, BDBCursor_key_doc},
    {"value", (PyCFunction)BDBCursor_value, METH_NOARGS, BDBCursor_value_doc},
    {"item", (PyCFunction)BDBCursor_item, METH_NOARGS, BDBCursor_item_doc},
    {NULL}  /* Sentinel */
};


/* BDBCursorType */
static PyTypeObject BDBCursorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.BDBCursor",                /*tp_name*/
    sizeof(BDBCursor),                        /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)BDBCursor_tp_dealloc,         /*tp_dealloc*/
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
    (traverseproc)BDBCursor_tp_traverse,      /*tp_traverse*/
    (inquiry)BDBCursor_tp_clear,              /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    BDBCursor_tp_methods,                     /*tp_methods*/
};


/*******************************************************************************
* BDB iterator types
*******************************************************************************/

/* new_BDBIter */
static PyObject *
new_BDBIter(BDB *self, PyTypeObject *type)
{
    PyObject *iter = DBIter_tp_new(type, (PyObject *)self);
    if (!iter) {
        return NULL;
    }
    if (!tcbdbcurfirst(self->cur)) {
        if (tcbdbecode(self->bdb) != TCENOREC) {
            Py_DECREF(iter);
            return set_bdb_error(self->bdb, NULL);
        }
    }
    self->changed = false;
    return iter;
}


/* BDBIterKeysType.tp_iternext */
static PyObject *
BDBIterKeys_tp_iternext(DBIter *self)
{
    BDB *bdb = (BDB *)self->db;
    void *key;
    int key_size;
    PyObject *pykey;

    if (bdb->changed) {
        return set_error(Error, "BDB changed during iteration");
    }
    key = tcbdbcurkey(bdb->cur, &key_size);
    if (!key) {
        if (tcbdbecode(bdb->bdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_bdb_error(bdb->bdb, NULL);
    }
    pykey = void_to_bytes(key, key_size);
    tcfree(key);
    if (!pykey) {
        return NULL;
    }
    tcbdbcurnext(bdb->cur);
    return pykey;
}


/* BDBIterKeysType */
static PyTypeObject BDBIterKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.BDBIterKeys",              /*tp_name*/
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
    (iternextfunc)BDBIterKeys_tp_iternext,    /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* BDBIterValuesType.tp_iternext */
static PyObject *
BDBIterValues_tp_iternext(DBIter *self)
{
    BDB *bdb = (BDB *)self->db;
    void *value;
    int value_size;
    PyObject *pyvalue;

    if (bdb->changed) {
        return set_error(Error, "BDB changed during iteration");
    }
    value = tcbdbcurval(bdb->cur, &value_size);
    if (!value) {
        if (tcbdbecode(bdb->bdb) == TCENOREC) {
            return set_stopiteration_error();
        }
        return set_bdb_error(bdb->bdb, NULL);
    }
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    if (!pyvalue) {
        return NULL;
    }
    tcbdbcurnext(bdb->cur);
    return pyvalue;
}


/* BDBIterValuesType */
static PyTypeObject BDBIterValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.BDBIterValues",            /*tp_name*/
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
    (iternextfunc)BDBIterValues_tp_iternext,  /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/* BDBIterItemsType.tp_iternext */
static PyObject *
BDBIterItems_tp_iternext(DBIter *self)
{
    BDB *bdb = (BDB *)self->db;
    TCXSTR *key, *value;
    PyObject *pykey, *pyvalue, *pyresult = NULL;

    if (bdb->changed) {
        return set_error(Error, "BDB changed during iteration");
    }
    key = tcxstrnew();
    value = tcxstrnew();
    if (!tcbdbcurrec(bdb->cur, key, value)) {
        if (tcbdbecode(bdb->bdb) == TCENOREC) {
            set_stopiteration_error();
        }
        else {
            set_bdb_error(bdb->bdb, NULL);
        }
    }
    else {
        pykey = tcxstr_to_bytes(key);
        pyvalue = tcxstr_to_bytes(value);
        if (pykey && pyvalue) {
            pyresult = PyTuple_Pack(2, pykey, pyvalue);
        }
        Py_XDECREF(pykey);
        Py_XDECREF(pyvalue);
    }
    tcxstrdel(key);
    tcxstrdel(value);
    if (!pyresult) {
        return NULL;
    }
    tcbdbcurnext(bdb->cur);
    return pyresult;
}


/* BDBIterItemsType */
static PyTypeObject BDBIterItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.BDBIterItems",             /*tp_name*/
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
    (iternextfunc)BDBIterItems_tp_iternext,   /*tp_iternext*/
    DBIter_tp_methods,                        /*tp_methods*/
};


/*******************************************************************************
* BDBType
*******************************************************************************/

/* BDB compare callback */
static int
BDB_cmp_cb(const char *a, int a_size, const char *b, int b_size, void *op)
{
    PyObject *pyresult = NULL, *pya, *pyb, *callback = op;
    int result;

    pya = PyBytes_FromStringAndSize(a, (Py_ssize_t)a_size);
    pyb = PyBytes_FromStringAndSize(b, (Py_ssize_t)b_size);
    if (!(pya && pyb)) {
        goto fail;
    }
    pyresult = PyObject_CallFunctionObjArgs(callback, pya, pyb, NULL);
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
        goto finish;
    }
    set_error(PyExc_TypeError, "compare callback must return an int");

fail:
    result = 0;
    PyErr_WriteUnraisable(callback);

finish:
    Py_XDECREF(pya);
    Py_XDECREF(pyb);
    Py_XDECREF(pyresult);
    return result;
}


/* BDBType.tp_doc */
PyDoc_STRVAR(BDB_tp_doc,
"BDB()\n\
\n\
B+ Tree Database.\n\
\n\
See also:\n\
Tokyo Cabinet B+ Tree Database API at:\n\
http://fallabs.com/tokyocabinet/spex-en.html#tcbdbapi");


/* BDBType.tp_dealloc */
static void
BDB_tp_dealloc(BDB *self)
{
    if (self->cur) {
        tcbdbcurdel(self->cur);
    }
    if (self->bdb) {
        tcbdbdel(self->bdb);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* BDBType.tp_new */
static PyObject *
BDB_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    BDB *self = (BDB *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    /* self->bdb */
    self->bdb = tcbdbnew();
    if (!self->bdb) {
        set_error(Error, "could not create BDB, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    if (!tcbdbsetmutex(self->bdb)) {
        set_bdb_error(self->bdb, NULL);
        Py_DECREF(self);
        return NULL;
    }
    /* self->cur */
    self->cur = tcbdbcurnew(self->bdb);
    if (!self->cur) {
        set_error(Error, "could not create cursor, memory issue?");
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}


/* BDB_tp_as_sequence.sq_contains */
static int
BDB_Contains(BDB *self, PyObject *pykey)
{
    void *key, *value;
    int key_size, value_size;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    value = tcbdbget(self->bdb, key, key_size, &value_size);
    if (!value) {
        if (tcbdbecode(self->bdb) == TCENOREC) {
            return 0;
        }
        set_bdb_error(self->bdb, NULL);
        return -1;
    }
    tcfree(value);
    return 1;
}


/* BDBType.tp_as_sequence */
static PySequenceMethods BDB_tp_as_sequence = {
    0,                                        /*sq_length*/
    0,                                        /*sq_concat*/
    0,                                        /*sq_repeat*/
    0,                                        /*sq_item*/
    0,                                        /*was_sq_slice*/
    0,                                        /*sq_ass_item*/
    0,                                        /*was_sq_ass_slice*/
    (objobjproc)BDB_Contains,                 /*sq_contains*/
};


/* BDB_tp_as_mapping.mp_length */
static Py_ssize_t
BDB_Length(BDB *self)
{
    return DB_Length(tcbdbrnum(self->bdb));
}


/* BDB_tp_as_mapping.mp_subscript */
static PyObject *
BDB_GetItem(BDB *self, PyObject *pykey)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pyvalue;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    value = tcbdbget(self->bdb, key, key_size, &value_size);
    if (!value) {
        return set_bdb_error(self->bdb, key);
    }
    pyvalue = void_to_bytes(value, value_size);
    tcfree(value);
    return pyvalue;
}


/* BDB_tp_as_mapping.mp_ass_subscript */
static int
BDB_SetItem(BDB *self, PyObject *pykey, PyObject *pyvalue)
{
    void *key, *value;
    int key_size, value_size;

    if (bytes_to_void(pykey, &key, &key_size)) {
        return -1;
    }
    if (pyvalue) {
        if (bytes_to_void(pyvalue, &value, &value_size)) {
            return -1;
        }
        if (!tcbdbput(self->bdb, key, key_size, value, value_size)) {
            set_bdb_error(self->bdb, NULL);
            return -1;
        }
    }
    else {
        if (!tcbdbout(self->bdb, key, key_size)) {
            set_bdb_error(self->bdb, key);
            return -1;
        }
    }
    self->changed = true;
    return 0;
}


/* BDBType.tp_as_mapping */
static PyMappingMethods BDB_tp_as_mapping = {
    (lenfunc)BDB_Length,                      /*mp_length*/
    (binaryfunc)BDB_GetItem,                  /*mp_subscript*/
    (objobjargproc)BDB_SetItem                /*mp_ass_subscript*/
};


/* BDBType.tp_iter */
static PyObject *
BDB_tp_iter(BDB *self)
{
    return new_BDBIter(self, &BDBIterKeysType);
}


/* BDB.open(path, mode) */
PyDoc_STRVAR(BDB_open_doc,
"open(path, mode)\n\
\n\
Open a database.\n\
'path': path to the database file.\n\
'mode': connection mode.");

static PyObject *
BDB_open(BDB *self, PyObject *args)
{
    const char *path;
    int mode;

    if (!PyArg_ParseTuple(args, "si:open", &path, &mode)) {
        return NULL;
    }
    if (!tcbdbopen(self->bdb, path, mode)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.close() */
PyDoc_STRVAR(BDB_close_doc,
"close()\n\
\n\
Close the database.\n\
\n\
Note:\n\
BDBs are closed when garbage-collected.");

static PyObject *
BDB_close(BDB *self)
{
    if (!tcbdbclose(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.clear() */
PyDoc_STRVAR(BDB_clear_doc,
"clear()\n\
\n\
Remove all records from the database.");

static PyObject *
BDB_clear(BDB *self)
{
    if (!tcbdbvanish(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.copy(path) */
PyDoc_STRVAR(BDB_copy_doc,
"copy(path)\n\
\n\
Copy the database file.\n\
'path': path to the destination file.");

static PyObject *
BDB_copy(BDB *self, PyObject *args)
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
    if (!tcbdbcopy(self->bdb, path)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.begin() */
PyDoc_STRVAR(BDB_begin_doc,
"begin()\n\
\n\
Begin a transaction.");

static PyObject *
BDB_begin(BDB *self)
{
    if (!tcbdbtranbegin(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.commit() */
PyDoc_STRVAR(BDB_commit_doc,
"commit()\n\
\n\
Commit a transaction.");

static PyObject *
BDB_commit(BDB *self)
{
    if (!tcbdbtrancommit(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.abort() */
PyDoc_STRVAR(BDB_abort_doc,
"abort()\n\
\n\
Abort a transaction.");

static PyObject *
BDB_abort(BDB *self)
{
    if (!tcbdbtranabort(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.get(key[, duplicate=False]) */
PyDoc_STRVAR(BDB_get_doc,
"get(key[, duplicate=False])\n\
\n\
Retrieve records.");

static PyObject *
BDB_get(BDB *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pykey, *pyresult, *duplicate = Py_False;
    void *key;
    int key_size;
    TCLIST *result;

    static char *kwlist[] = {"key", "duplicate", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", kwlist,
                                     &pykey, &duplicate)) {
        return NULL;
    }
    if (!PyBool_Check(duplicate)) {
        return set_error(PyExc_TypeError, "a boolean is required");
    }
    if (duplicate == Py_False) {
        return BDB_GetItem(self, pykey);
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcbdbget4(self->bdb, key, key_size);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_bdb_error(self->bdb, key);
    }
    pyresult = tclist_to_tuple(result);
    tclistdel(result);
    return pyresult;
}


/* BDB.remove(key[, duplicate=False]) */
PyDoc_STRVAR(BDB_remove_doc,
"remove(key[, duplicate=False])\n\
\n\
Remove records.");

static PyObject *
BDB_remove(BDB *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pykey, *duplicate = Py_False;
    void *key;
    int key_size;
    bool result;

    static char *kwlist[] = {"key", "duplicate", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:remove", kwlist,
                                     &pykey, &duplicate)) {
        return NULL;
    }
    if (!PyBool_Check(duplicate)) {
        return set_error(PyExc_TypeError, "a boolean is required");
    }
    if (duplicate == Py_False) {
        if (BDB_SetItem(self, pykey, NULL)) {
            return NULL;
        }
        Py_RETURN_NONE;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcbdbout3(self->bdb, key, key_size);
    Py_END_ALLOW_THREADS
    if (!result) {
        return set_bdb_error(self->bdb, key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.put(key, value[, duplicate=False]) */
PyDoc_STRVAR(BDB_put_doc,
"put(key, value[, duplicate=False])\n\
\n\
Store records.");

static PyObject *
BDB_put(BDB *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pykey, *pyvalue, *duplicate = Py_False;
    void *key, *value;
    int key_size, value_size;

    static char *kwlist[] = {"key", "value", "duplicate", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O:put", kwlist,
                                     &pykey, &pyvalue, &duplicate)) {
        return NULL;
    }
    if (!PyBool_Check(duplicate)) {
        return set_error(PyExc_TypeError, "a boolean is required");
    }
    if (duplicate == Py_False) {
        if (BDB_SetItem(self, pykey, pyvalue)) {
            return NULL;
        }
        Py_RETURN_NONE;
    }
    if (bytes_to_void(pykey, &key, &key_size) ||
        bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcbdbputdup(self->bdb, key, key_size, value, value_size)) {
        return set_bdb_error(self->bdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.putkeep(key, value) */
PyDoc_STRVAR(BDB_putkeep_doc,
"putkeep(key, value)\n\
\n\
Store a record in the database, unlike the standard forms (bdb[key] = value or\n\
put), this method raises KeyError if key is already in the database.");

static PyObject *
BDB_putkeep(BDB *self, PyObject *args)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putkeep", &pykey, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size) ||
        bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcbdbputkeep(self->bdb, key, key_size, value, value_size)) {
        return set_bdb_error(self->bdb, key);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.putcat(key, value) */
PyDoc_STRVAR(BDB_putcat_doc,
"putcat(key, value)\n\
\n\
Concatenate a value at the end of an existing one.\n\
If there is no corresponding record, a new record is stored.");

static PyObject *
BDB_putcat(BDB *self, PyObject *args)
{
    void *key, *value;
    int key_size, value_size;
    PyObject *pykey, *pyvalue;

    if (!PyArg_ParseTuple(args, "OO:putcat", &pykey, &pyvalue)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size) ||
        bytes_to_void(pyvalue, &value, &value_size)) {
        return NULL;
    }
    if (!tcbdbputcat(self->bdb, key, key_size, value, value_size)) {
        return set_bdb_error(self->bdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.putdup(key, values) */
PyDoc_STRVAR(BDB_putdup_doc,
"putdup(key, values)\n\
\n\
.");

static PyObject *
BDB_putdup(BDB *self, PyObject *args)
{
    void *key;
    int key_size;
    TCLIST *values;
    PyObject *pykey, *pyvalues;
    bool result;

    if (!PyArg_ParseTuple(args, "OO:putdup", &pykey, &pyvalues)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    values = seq_to_tclist(pyvalues);
    if (!values) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcbdbputdup3(self->bdb, key, key_size, values);
    Py_END_ALLOW_THREADS
    tclistdel(values);
    if (!result) {
        return set_bdb_error(self->bdb, NULL);
    }
    self->changed = true;
    Py_RETURN_NONE;
}


/* BDB.sync() */
PyDoc_STRVAR(BDB_sync_doc,
"sync()\n\
\n\
Flush modifications to the database file?");

static PyObject *
BDB_sync(BDB *self)
{
    if (!tcbdbsync(self->bdb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.searchkeys(prefix[, max]) -> frozenset */
PyDoc_STRVAR(BDB_searchkeys_doc,
"searchkeys(prefix[, max]) -> frozenset\n\
\n\
Return a frozenset of keys starting with prefix. If given, max is the maximum\n\
number of keys to fetch, if omitted or specified as a negative value no limit\n\
is applied.");

static PyObject *
BDB_searchkeys(BDB *self, PyObject *args)
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
    result = tcbdbfwmkeys(self->bdb, prefix, prefix_size, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* BDB.range([begin=None[, end=None[, max=-1]]]) -> frozenset */
PyDoc_STRVAR(BDB_range_doc,
"range([begin=None[, end=None[, max=-1]]]) -> frozenset\n\
\n\
.");

static PyObject *
BDB_range(BDB *self, PyObject *args, PyObject *kwargs)
{
    void *begin = NULL, *end = NULL;
    int begin_size, end_size, max = -1;
    TCLIST *result;
    PyObject *pybegin = Py_None, *pyend = Py_None, *pyresult;

    static char *kwlist[] = {"begin", "end", "max", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOi:range", kwlist,
                                     &pybegin, &pyend, &max)) {
        return NULL;
    }
    if (pybegin != Py_None) {
        if (bytes_to_void(pybegin, &begin, &begin_size)) {
            return NULL;
        }
    }
    if (pyend != Py_None) {
        if (bytes_to_void(pyend, &end, &end_size)) {
            return NULL;
        }
    }
    Py_BEGIN_ALLOW_THREADS
    result = tcbdbrange(self->bdb, begin, begin_size, true,
                        end, end_size, true, max);
    Py_END_ALLOW_THREADS
    pyresult = tclist_to_frozenset(result);
    tclistdel(result);
    return pyresult;
}


/* BDB.cursor() */
PyDoc_STRVAR(BDB_cursor_doc,
"cursor()\n\
\n\
");

static PyObject *
BDB_cursor(BDB *self)
{
    return (PyObject *)new_BDBCursor(&BDBCursorType, self);
}


/* BDB.optimize([lmemb=0[, nmemb=0[, bnum=0[, apow=-1[, fpow=-1[, opts=255]]]]]]) */
PyDoc_STRVAR(BDB_optimize_doc,
"optimize([lmemb=0[, nmemb=0[, bnum=0[, apow=-1[, fpow=-1[, opts=255]]]]]])\n\
\n\
Optimize a database.\n\
'lmemb': the number of members in each leaf page. If specified as 0 or as a\n\
         negative value, the current setting is kept.\n\
'nmemb': the number of members in each non-leaf page. If specified as 0 or as a\n\
         negative value, the current setting is kept.\n\
'bnum': the number of elements in a bucket array. If specified as 0 or as a\n\
        negative value, the default value (twice the number of pages) is used.\n\
'apow': (?) TODO. If specified as a negative value, the current setting is kept.\n\
'fpow': (?) TODO. If specified as a negative value, the current setting is kept.\n\
'opts': TODO. If specified as 255 (UINT8_MAX), the current setting is kept.\n\
\n\
Note:\n\
Optimizing a read only database, or during a transaction, is an invalid operation.");

static PyObject *
BDB_optimize(BDB *self, PyObject *args, PyObject *kwargs)
{
    long lmemb = 0, nmemb = 0;
    long long bnum = 0;
    int iapow = -1, ifpow = -1;
    char apow, fpow;
    unsigned char opts = UINT8_MAX;

    static char *kwlist[] = {"lmemb", "nmemb", "bnum", "apow", "fpow", "opts",
                             NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|llLiib:optimize", kwlist,
                                     &lmemb, &nmemb, &bnum, &iapow, &ifpow,
                                     &opts)) {
        return NULL;
    }
    apow = int_to_char(iapow);
    fpow = int_to_char(ifpow);
    if ((apow == -1 || fpow == -1) && PyErr_Occurred()) {
        return NULL;
    }
    if (!tcbdboptimize(self->bdb, lmemb, nmemb, bnum, apow, fpow, opts)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.tune(lmemb, nmemb, bnum, apow, fpow, opts) */
PyDoc_STRVAR(BDB_tune_doc,
"tune(lmemb, nmemb, bnum, apow, fpow, opts)\n\
\n\
Tune a database.\n\
'lmemb': the number of members in each leaf page. If specified as 0 or as a\n\
         negative value, the default value (128) is used.\n\
'nmemb': the number of members in each non-leaf page. If specified as 0 or as a\n\
         negative value, the default value (256) is used.\n\
'bnum': the number of elements in a bucket array. If specified as 0 or as a\n\
        negative value, the default value (32749) is used.\n\
'apow': (?) TODO (-1).\n\
'fpow': (?) TODO (-1).\n\
'opts': TODO (0).\n\
\n\
Note:\n\
Tuning an open database is an invalid operation.");

static PyObject *
BDB_tune(BDB *self, PyObject *args)
{
    long lmemb, nmemb;
    long long bnum;
    int iapow, ifpow;
    char apow, fpow;
    unsigned char opts;

    if (!PyArg_ParseTuple(args, "llLiib:tune", &lmemb, &nmemb, &bnum, &iapow,
                          &ifpow, &opts)) {
        return NULL;
    }
    apow = int_to_char(iapow);
    fpow = int_to_char(ifpow);
    if ((apow == -1 || fpow == -1) && PyErr_Occurred()) {
        return NULL;
    }
    if (!tcbdbtune(self->bdb, lmemb, nmemb, bnum, apow, fpow, opts)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.setcache(lcnum, ncnum) */
PyDoc_STRVAR(BDB_setcache_doc,
"setcache(lcnum, ncnum)\n\
\n\
Set the cache parameters.\n\
'lcnum': the maximum number of leaf nodes to be cached. If specified as 0 or as\n\
         a negative value, the default value (1024) is used.\n\
'ncnum': the maximum number of non-leaf nodes to be cached. If specified as 0 or\n\
         as a negative value, the default value (512) is used.\n\
\n\
Note:\n\
Setting the cache parameters on an open database is an invalid operation.");

static PyObject *
BDB_setcache(BDB *self, PyObject *args)
{
    long lcnum, ncnum;

    if (!PyArg_ParseTuple(args, "ll:setcache", &lcnum, &ncnum)) {
        return NULL;
    }
    if (!tcbdbsetcache(self->bdb, lcnum, ncnum)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.setxmsiz(xmsiz) */
PyDoc_STRVAR(BDB_setxmsiz_doc,
"setxmsiz(xmsiz)\n\
\n\
Set the extra mapped memory size.\n\
'xmsiz': the amount of extra mapped memory (in what unit?). If specified as 0\n\
         or as a negative value, the extra mapped memory is disabled (default).\n\
\n\
Note:\n\
Setting the extra memory size on an open database is an invalid operation.");

static PyObject *
BDB_setxmsiz(BDB *self, PyObject *args)
{
    long long xmsiz;

    if (!PyArg_ParseTuple(args, "L:setxmsiz", &xmsiz)) {
        return NULL;
    }
    if (!tcbdbsetxmsiz(self->bdb, xmsiz)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.setdfunit(dfunit) */
PyDoc_STRVAR(BDB_setdfunit_doc,
"setdfunit(dfunit)\n\
\n\
Set auto defragmentation's unit step number.\n\
'dfunit': the unit step number(?). If specified as 0 or as a negative value,\n\
          auto defragmentation is disabled (default).\n\
\n\
Note:\n\
Setting this on an open database is an invalid operation.");

static PyObject *
BDB_setdfunit(BDB *self, PyObject *args)
{
    long dfunit;

    if (!PyArg_ParseTuple(args, "l:setdfunit", &dfunit)) {
        return NULL;
    }
    if (!tcbdbsetdfunit(self->bdb, dfunit)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.setcmpfunc(callback) */
PyDoc_STRVAR(BDB_setcmpfunc_doc,
"setcmpfunc(callback)\n\
\n\
");

static PyObject *
BDB_setcmpfunc(BDB *self, PyObject *args)
{
    PyObject *pycb = NULL;
    TCCMP cb;
    int cmp;

    if (!PyArg_ParseTuple(args, "O:setcmpfunc", &pycb)) {
        return NULL;
    }
#if PY_MAJOR_VERSION >= 3
    if (PyLong_Check(pycb)) {
#else
    if (PyInt_Check(pycb)) {
#endif
        cmp = (int)PyLong_AsLong(pycb);
        if (cmp == -1 && PyErr_Occurred()) {
            return NULL;
        }
        switch (cmp) {
            case BDBCMPLEXICAL:
               cb = tccmplexical;
               break;
            case BDBCMPDECIMAL:
               cb = tccmpdecimal;
               break;
            case BDBCMPINT32:
               cb = tccmpint32;
               break;
            case BDBCMPINT64:
               cb = tccmpint64;
               break;
            default:
               return set_error(PyExc_ValueError,
                                "unknown compare callback constant");
        }
    }
    else if (PyCallable_Check(pycb)) {
        cb = BDB_cmp_cb;
    }
    else {
        return set_error(PyExc_TypeError, "a callable or an int is required");
    }
    if (!tcbdbsetcmpfunc(self->bdb, cb, (void *)pycb)) {
        return set_bdb_error(self->bdb, NULL);
    }
    Py_RETURN_NONE;
}


/* BDB.addint(key, num) -> int */
PyDoc_STRVAR(BDB_addint_doc,
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
BDB_addint(BDB *self, PyObject *args)
{
    PyObject *pykey;
    void *key;
    int key_size, num, result, ecode;

    if (!PyArg_ParseTuple(args, "Oi:addint", &pykey, &num)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    result = tcbdbaddint(self->bdb, key, key_size, num);
    if (result == INT_MIN) {
        ecode = tcbdbecode(self->bdb);
        if (ecode != TCESUCCESS && ecode != TCENOREC) {
            return set_bdb_error(self->bdb, key);
        }
    }
    if (num) {
        self->changed = true;
    }
    return PyInt_FromLong((long)result);
}


/* BDB.adddouble(key, num) -> float */
PyDoc_STRVAR(BDB_adddouble_doc,
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
BDB_adddouble(BDB *self, PyObject *args)
{
    PyObject *pykey;
    void *key;
    int key_size;
    double num, result;

    if (!PyArg_ParseTuple(args, "Od:adddouble", &pykey, &num)) {
        return NULL;
    }
    if (bytes_to_void(pykey, &key, &key_size)) {
        return NULL;
    }
    result = tcbdbadddouble(self->bdb, key, key_size, num);
    if (Py_IS_NAN(result)) {
        return set_bdb_error(self->bdb, key);
    }
    if (num) {
        self->changed = true;
    }
    return PyFloat_FromDouble(result);
}


/* BDB.iterkeys() */
PyDoc_STRVAR(BDB_iterkeys_doc,
"iterkeys()\n\
\n\
Return an iterator over the database's keys.");

static PyObject *
BDB_iterkeys(BDB *self)
{
    return new_BDBIter(self, &BDBIterKeysType);
}


/* BDB.itervalues() */
PyDoc_STRVAR(BDB_itervalues_doc,
"itervalues()\n\
\n\
Return an iterator over the database's values.");

static PyObject *
BDB_itervalues(BDB *self)
{
    return new_BDBIter(self, &BDBIterValuesType);
}


/* BDB.iteritems() */
PyDoc_STRVAR(BDB_iteritems_doc,
"iteritems()\n\
\n\
Return an iterator over the database's items.");

static PyObject *
BDB_iteritems(BDB *self)
{
    return new_BDBIter(self, &BDBIterItemsType);
}


/* BDBType.tp_methods */
static PyMethodDef BDB_tp_methods[] = {
    {"open", (PyCFunction)BDB_open, METH_VARARGS, BDB_open_doc},
    {"close", (PyCFunction)BDB_close, METH_NOARGS, BDB_close_doc},
    {"clear", (PyCFunction)BDB_clear, METH_NOARGS, BDB_clear_doc},
    {"copy", (PyCFunction)BDB_copy, METH_VARARGS, BDB_copy_doc},
    {"begin", (PyCFunction)BDB_begin, METH_NOARGS, BDB_begin_doc},
    {"commit", (PyCFunction)BDB_commit, METH_NOARGS, BDB_commit_doc},
    {"abort", (PyCFunction)BDB_abort, METH_NOARGS, BDB_abort_doc},
    {"get", (PyCFunction)BDB_get, METH_VARARGS | METH_KEYWORDS, BDB_get_doc},
    {"remove", (PyCFunction)BDB_remove, METH_VARARGS | METH_KEYWORDS,
     BDB_remove_doc},
    {"put", (PyCFunction)BDB_put, METH_VARARGS | METH_KEYWORDS, BDB_put_doc},
    {"putkeep", (PyCFunction)BDB_putkeep, METH_VARARGS, BDB_putkeep_doc},
    {"putcat", (PyCFunction)BDB_putcat, METH_VARARGS, BDB_putcat_doc},
    {"putdup", (PyCFunction)BDB_putdup, METH_VARARGS, BDB_putdup_doc},
    {"sync", (PyCFunction)BDB_sync, METH_NOARGS, BDB_sync_doc},
    {"searchkeys", (PyCFunction)BDB_searchkeys, METH_VARARGS,
     BDB_searchkeys_doc},
    {"range", (PyCFunction)BDB_range, METH_VARARGS | METH_KEYWORDS,
     BDB_range_doc},
    {"cursor", (PyCFunction)BDB_cursor, METH_NOARGS, BDB_cursor_doc},
    {"optimize", (PyCFunction)BDB_optimize, METH_VARARGS | METH_KEYWORDS,
     BDB_optimize_doc},
    {"tune", (PyCFunction)BDB_tune, METH_VARARGS, BDB_tune_doc},
    {"setcache", (PyCFunction)BDB_setcache, METH_VARARGS, BDB_setcache_doc},
    {"setxmsiz", (PyCFunction)BDB_setxmsiz, METH_VARARGS, BDB_setxmsiz_doc},
    {"setdfunit", (PyCFunction)BDB_setdfunit, METH_VARARGS, BDB_setdfunit_doc},
    {"setcmpfunc", (PyCFunction)BDB_setcmpfunc, METH_VARARGS, BDB_setcmpfunc_doc},
    {"addint", (PyCFunction)BDB_addint, METH_VARARGS, BDB_addint_doc},
    {"adddouble", (PyCFunction)BDB_adddouble, METH_VARARGS, BDB_adddouble_doc},
    {"iterkeys", (PyCFunction)BDB_iterkeys, METH_NOARGS, BDB_iterkeys_doc},
    {"itervalues", (PyCFunction)BDB_itervalues, METH_NOARGS, BDB_itervalues_doc},
    {"iteritems", (PyCFunction)BDB_iteritems, METH_NOARGS, BDB_iteritems_doc},
    {NULL}  /* Sentinel */
};


/* BDB.path */
PyDoc_STRVAR(BDB_path_doc,
"The path to the database file.");

static PyObject *
BDB_path_get(BDB *self, void *closure)
{
    const char *path;

    path = tcbdbpath(self->bdb);
    if (path) {
        return PyString_FromString(path);
    }
    Py_RETURN_NONE;
}


/* BDB.size */
PyDoc_STRVAR(BDB_size_doc,
"The size in bytes of the database file.");

static PyObject *
BDB_size_get(BDB *self, void *closure)
{
    return PyLong_FromUnsignedLongLong(tcbdbfsiz(self->bdb));
}


/* BDBType.tp_getsets */
static PyGetSetDef BDB_tp_getsets[] = {
    {"path", (getter)BDB_path_get, NULL, BDB_path_doc, NULL},
    {"size", (getter)BDB_size_get, NULL, BDB_size_doc, NULL},
    {NULL}  /* Sentinel */
};


/* BDBType */
static PyTypeObject BDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "tokyo.cabinet.BDB",                      /*tp_name*/
    sizeof(BDB),                              /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)BDB_tp_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &BDB_tp_as_sequence,                      /*tp_as_sequence*/
    &BDB_tp_as_mapping,                       /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    BDB_tp_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    (getiterfunc)BDB_tp_iter,                 /*tp_iter*/
    0,                                        /*tp_iternext*/
    BDB_tp_methods,                           /*tp_methods*/
    0,                                        /*tp_members*/
    BDB_tp_getsets,                           /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    BDB_tp_new,                               /*tp_new*/
};
