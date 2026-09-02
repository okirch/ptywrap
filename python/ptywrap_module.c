/*
 * Framework for testing TTY based linux applications
 * Copyright (C) 2026 SUSE Linux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <ptywrap.h>

/* PTYSession object structure */
typedef struct {
    PyObject_HEAD
    ptywrap_session_t *session;
    int rows;
    int cols;
} PTYSessionObject;

/* PTYSession.__new__ */
static PyObject *
PTYSession_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PTYSessionObject *self;
    self = (PTYSessionObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->session = NULL;
        self->rows = 0;
        self->cols = 0;
    }
    return (PyObject *)self;
}

/* PTYSession.__init__ */
static int
PTYSession_init(PTYSessionObject *self, PyObject *args, PyObject *kwds)
{
    const char *container_id = NULL;
    PyObject *command_obj = NULL;
    int rows = 0, cols = 0;
    static char *kwlist[] = {"container_id", "rows", "cols", "command", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ziOi", kwlist,
                                     &container_id, &rows, &cols, &command_obj)) {
        return -1;
    }

    if (container_id && command_obj) {
        PyErr_SetString(PyExc_ValueError, "Cannot specify both container_id and command");
        return -1;
    }

    if (!container_id && !command_obj) {
        PyErr_SetString(PyExc_ValueError, "Must specify either container_id or command");
        return -1;
    }

    if (container_id) {
        /* Create PTY session attached to container */
        self->session = ptywrap_create(container_id, rows, cols);
    } else {
        /* Parse command sequence */
        if (!PySequence_Check(command_obj)) {
            PyErr_SetString(PyExc_TypeError, "command must be a sequence of strings");
            return -1;
        }

        Py_ssize_t size = PySequence_Size(command_obj);
        if (size <= 0) {
            PyErr_SetString(PyExc_ValueError, "command sequence cannot be empty");
            return -1;
        }

        /* Allocate argv array */
        char **argv = calloc(size + 1, sizeof(char *));
        if (!argv) {
            PyErr_NoMemory();
            return -1;
        }

        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *item = PySequence_GetItem(command_obj, i);
            if (!item) {
                for (Py_ssize_t j = 0; j < i; j++) free(argv[j]);
                free(argv);
                return -1;
            }

            PyObject *str_item = PyObject_Str(item);
            Py_DECREF(item);
            if (!str_item) {
                for (Py_ssize_t j = 0; j < i; j++) free(argv[j]);
                free(argv);
                return -1;
            }

            const char *item_utf8 = PyUnicode_AsUTF8(str_item);
            if (!item_utf8) {
                Py_DECREF(str_item);
                for (Py_ssize_t j = 0; j < i; j++) free(argv[j]);
                free(argv);
                return -1;
            }

            argv[i] = strdup(item_utf8);
            Py_DECREF(str_item);
            if (!argv[i]) {
                for (Py_ssize_t j = 0; j < i; j++) free(argv[j]);
                free(argv);
                PyErr_NoMemory();
                return -1;
            }
        }
        argv[size] = NULL;

        /* Create direct PTY session */
        self->session = ptywrap_create_direct(argv, rows, cols);

        /* Clean up argv */
        for (Py_ssize_t i = 0; i < size; i++) {
            free(argv[i]);
        }
        free(argv);
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create PTY session");
        return -1;
    }

    /* Get actual size */
    ptywrap_get_size(self->session, &self->rows, &self->cols);

    return 0;
}

/* PTYSession.__del__ */
static void
PTYSession_dealloc(PTYSessionObject *self)
{
    if (self->session != NULL) {
        ptywrap_destroy(self->session);
        self->session = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* PTYSession.send(data) */
static PyObject *
PTYSession_send(PTYSessionObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t len;

    if (!PyArg_ParseTuple(args, "s#", &data, &len)) {
        return NULL;
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    int written = ptywrap_send(self->session, data, len);
    if (written < 0) {
        PyErr_SetString(PyExc_IOError, "Failed to send data");
        return NULL;
    }

    return PyLong_FromLong(written);
}

/* PTYSession.send_line(line) */
static PyObject *
PTYSession_send_line(PTYSessionObject *self, PyObject *args)
{
    const char *line;

    if (!PyArg_ParseTuple(args, "s", &line)) {
        return NULL;
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    int written = ptywrap_send_line(self->session, line);
    if (written < 0) {
        PyErr_SetString(PyExc_IOError, "Failed to send line");
        return NULL;
    }

    return PyLong_FromLong(written);
}

/* PTYSession.get_cell(row, col) */
static PyObject *
PTYSession_get_cell(PTYSessionObject *self, PyObject *args)
{
    int row, col;
    ptywrap_cell_t cell;

    if (!PyArg_ParseTuple(args, "ii", &row, &col)) {
        return NULL;
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    int ret = ptywrap_get_cell(self->session, row, col, &cell);
    if (ret != PTYWRAP_OK) {
        PyErr_SetString(PyExc_ValueError, "Invalid row or column");
        return NULL;
    }

    /* Return dictionary with cell data */
    return Py_BuildValue("{s:c,s:i,s:i,s:i}",
                         "char", cell.ch,
                         "fg_color", cell.fg_color,
                         "bg_color", cell.bg_color,
                         "attrs", cell.attrs);
}

/* PTYSession.get_row_text(row) */
static PyObject *
PTYSession_get_row_text(PTYSessionObject *self, PyObject *args)
{
    int row;
    char *buffer;

    if (!PyArg_ParseTuple(args, "i", &row)) {
        return NULL;
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    /* Allocate buffer */
    buffer = malloc(self->cols + 1);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }

    int len = ptywrap_get_row_text(self->session, row, buffer, self->cols + 1);
    if (len < 0) {
        free(buffer);
        PyErr_SetString(PyExc_ValueError, "Invalid row");
        return NULL;
    }

    PyObject *result = PyUnicode_FromString(buffer);
    free(buffer);
    return result;
}

/* PTYSession.get_cursor() */
static PyObject *
PTYSession_get_cursor(PTYSessionObject *self, PyObject *Py_UNUSED(ignored))
{
    int row, col;

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    int ret = ptywrap_get_cursor(self->session, &row, &col);
    if (ret != PTYWRAP_OK) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get cursor position");
        return NULL;
    }

    return Py_BuildValue("(ii)", row, col);
}

/* PTYSession.screenshot(start_row, end_row) */
static PyObject *
PTYSession_screenshot(PTYSessionObject *self, PyObject *args, PyObject *kwds)
{
    int start_row = 0;
    int end_row = -1;
    static char *kwlist[] = {"start_row", "end_row", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ii", kwlist,
                                     &start_row, &end_row)) {
        return NULL;
    }

    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    char *markdown = ptywrap_screenshot_markdown(self->session, start_row, end_row);
    if (markdown == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to generate screenshot");
        return NULL;
    }

    PyObject *result = PyUnicode_FromString(markdown);
    free(markdown);
    return result;
}

/* PTYSession.is_alive() */
static PyObject *
PTYSession_is_alive(PTYSessionObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->session == NULL) {
        Py_RETURN_FALSE;
    }

    int alive = ptywrap_container_alive(self->session);
    if (alive > 0) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

/* PTYSession.get_pid() */
static PyObject *
PTYSession_get_pid(PTYSessionObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->session == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session is closed");
        return NULL;
    }

    pid_t pid = ptywrap_get_container_pid(self->session);
    return PyLong_FromLong(pid);
}

/* PTYSession.rows property getter */
static PyObject *
PTYSession_get_rows(PTYSessionObject *self, void *Py_UNUSED(closure))
{
    return PyLong_FromLong(self->rows);
}

/* PTYSession.cols property getter */
static PyObject *
PTYSession_get_cols(PTYSessionObject *self, void *Py_UNUSED(closure))
{
    return PyLong_FromLong(self->cols);
}

/* PTYSession methods */
static PyMethodDef PTYSession_methods[] = {
    {"send", (PyCFunction)PTYSession_send, METH_VARARGS,
     "send(data: bytes) -> int\n\n"
     "Send raw bytes to the container shell.\n\n"
     "Args:\n"
     "    data: Bytes to send\n\n"
     "Returns:\n"
     "    Number of bytes written\n\n"
     "Raises:\n"
     "    IOError: If send fails\n"
     "    RuntimeError: If session is closed"
    },
    {"send_line", (PyCFunction)PTYSession_send_line, METH_VARARGS,
     "send_line(line: str) -> int\n\n"
     "Send a line of text to the container shell (with newline appended).\n\n"
     "Args:\n"
     "    line: String to send\n\n"
     "Returns:\n"
     "    Number of bytes written\n\n"
     "Raises:\n"
     "    IOError: If send fails\n"
     "    RuntimeError: If session is closed"
    },
    {"get_cell", (PyCFunction)PTYSession_get_cell, METH_VARARGS,
     "get_cell(row: int, col: int) -> dict\n\n"
     "Get terminal cell at specific position.\n\n"
     "Args:\n"
     "    row: Row index (0-based)\n"
     "    col: Column index (0-based)\n\n"
     "Returns:\n"
     "    Dictionary with keys: 'char', 'fg_color', 'bg_color', 'attrs'\n\n"
     "Raises:\n"
     "    ValueError: If row or column is out of bounds\n"
     "    RuntimeError: If session is closed"
    },
    {"get_row_text", (PyCFunction)PTYSession_get_row_text, METH_VARARGS,
     "get_row_text(row: int) -> str\n\n"
     "Get text content of a terminal row (without attributes).\n\n"
     "Args:\n"
     "    row: Row index (0-based)\n\n"
     "Returns:\n"
     "    String containing row text\n\n"
     "Raises:\n"
     "    ValueError: If row is out of bounds\n"
     "    RuntimeError: If session is closed"
    },
    {"get_cursor", (PyCFunction)PTYSession_get_cursor, METH_NOARGS,
     "get_cursor() -> tuple[int, int]\n\n"
     "Get current cursor position.\n\n"
     "Returns:\n"
     "    Tuple of (row, col)\n\n"
     "Raises:\n"
     "    RuntimeError: If session is closed or operation fails"
    },
    {"screenshot", (PyCFunction)PTYSession_screenshot, METH_VARARGS | METH_KEYWORDS,
     "screenshot(start_row: int = 0, end_row: int = -1) -> str\n\n"
     "Generate markdown representation of terminal buffer.\n\n"
     "The output includes styling annotations for colors and text attributes.\n"
     "Styled text is shown as: [(bold)(fg:blue)text]\n\n"
     "Args:\n"
     "    start_row: First row to capture (default: 0)\n"
     "    end_row: Last row to capture (default: -1 for last row)\n\n"
     "Returns:\n"
     "    Markdown-formatted string with terminal content\n\n"
     "Raises:\n"
     "    RuntimeError: If session is closed or screenshot fails"
    },
    {"is_alive", (PyCFunction)PTYSession_is_alive, METH_NOARGS,
     "is_alive() -> bool\n\n"
     "Check if the exec process is still running.\n\n"
     "Returns:\n"
     "    True if running, False otherwise"
    },
    {"get_pid", (PyCFunction)PTYSession_get_pid, METH_NOARGS,
     "get_pid() -> int\n\n"
     "Get the PID of the podman exec process.\n\n"
     "Returns:\n"
     "    Process ID\n\n"
     "Raises:\n"
     "    RuntimeError: If session is closed"
    },
    {NULL}  /* Sentinel */
};

/* PTYSession properties */
static PyGetSetDef PTYSession_getsetters[] = {
    {"rows", (getter)PTYSession_get_rows, NULL,
     "Number of terminal rows", NULL},
    {"cols", (getter)PTYSession_get_cols, NULL,
     "Number of terminal columns", NULL},
    {NULL}  /* Sentinel */
};

/* PTYSession type definition */
static PyTypeObject PTYSessionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ptywrap.PTYSession",
    .tp_doc = "PTYSession(container_id: str = None, rows: int = 0, cols: int = 0, command: list[str] = None)\n\n"
              "PTY session attached to a running container or driving a local application directly.\n\n"
              "Creates a pseudo-terminal session that attaches to an existing\n"
              "running container via 'podman exec -it', or runs a command directly inside the current\n"
              "environment. The session provides full VT100/ANSI terminal emulation with color and\n"
              "attribute support.\n\n"
              "Args:\n"
              "    container_id: Container ID or name (must be running). Do not specify if using 'command'.\n"
              "    rows: Terminal height (default: 40)\n"
              "    cols: Terminal width (default: 150)\n"
              "    command: Sequence of strings representing executable and arguments. Do not specify if using 'container_id'.\n\n"
              "Example (container-based):\n"
              "    >>> session = PTYSession('mycontainer', rows=40, cols=150)\n"
              "    >>> session.send_line('ls -la')\n"
              "    >>> time.sleep(1)\n"
              "    >>> text = session.get_row_text(0)\n"
              "    >>> print(text)\n\n"
              "Example (direct application execution):\n"
              "    >>> session = PTYSession(command=['/bin/sh', '-i'], rows=40, cols=150)\n"
              "    >>> session.send_line('echo Hello')\n"
              "    >>> time.sleep(1)\n"
              "    >>> print(session.get_row_text(0))",
    .tp_basicsize = sizeof(PTYSessionObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PTYSession_new,
    .tp_init = (initproc)PTYSession_init,
    .tp_dealloc = (destructor)PTYSession_dealloc,
    .tp_methods = PTYSession_methods,
    .tp_getset = PTYSession_getsetters,
};

/* Module methods */
static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

/* Module definition */
static struct PyModuleDef ptywrapmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "ptywrap",
    .m_doc = "Python bindings for ptywrap - PTY container emulator library\n\n"
             "This module provides Python bindings for the ptywrap C library,\n"
             "which allows attaching to running containers via pseudo-terminals\n"
             "with full VT100/ANSI terminal emulation.\n\n"
             "Key Features:\n"
             "  - Attach to existing running containers via 'podman exec'\n"
             "  - Full VT100/ANSI terminal emulation\n"
             "  - Color and text attribute support\n"
             "  - Thread-safe buffer access\n"
             "  - Markdown screenshot export\n\n"
             "Classes:\n"
             "  PTYSession: Main session object for container interaction\n\n"
             "Constants:\n"
             "  ATTR_BOLD: Bold text attribute flag\n"
             "  ATTR_UNDERLINE: Underline text attribute flag\n"
             "  ATTR_REVERSE: Reverse video attribute flag\n"
             "  ATTR_BLINK: Blinking text attribute flag",
    .m_size = -1,
    .m_methods = module_methods,
};

/* Module initialization */
PyMODINIT_FUNC
PyInit_ptywrap(void)
{
    PyObject *m;

    if (PyType_Ready(&PTYSessionType) < 0)
        return NULL;

    m = PyModule_Create(&ptywrapmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PTYSessionType);
    if (PyModule_AddObject(m, "PTYSession", (PyObject *)&PTYSessionType) < 0) {
        Py_DECREF(&PTYSessionType);
        Py_DECREF(m);
        return NULL;
    }

    /* Add attribute constants */
    PyModule_AddIntConstant(m, "ATTR_BOLD", PTYWRAP_ATTR_BOLD);
    PyModule_AddIntConstant(m, "ATTR_UNDERLINE", PTYWRAP_ATTR_UNDERLINE);
    PyModule_AddIntConstant(m, "ATTR_REVERSE", PTYWRAP_ATTR_REVERSE);
    PyModule_AddIntConstant(m, "ATTR_BLINK", PTYWRAP_ATTR_BLINK);

    return m;
}
