#include <Python.h>
#include <signal.h>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#else
#include <intobject.h>
#include <bytesobject.h>
#endif

#define NUM_REGS    6

typedef struct {
    unsigned long rax, rbx, rcx, rdx, rsi, rdi;
} regs_t;

static jmp_buf vmcall_jmp;

void sighandler(int signo)
{
    longjmp(vmcall_jmp, signo);
}

static int
call_vmcall(regs_t *argregs, regs_t *retregs)
{
    void (*old_sigill)(int);
    old_sigill = signal (SIGILL, sighandler);
    if (old_sigill == SIG_ERR) {
        PyErr_SetString(PyExc_RuntimeError, "signal() failed");
        return 0;
    }

    int signo = setjmp(vmcall_jmp);
    if (signo) {
        PyErr_SetString(PyExc_RuntimeError, "SIGILL on vmcall");
        return 0;
    }

    asm volatile (
        "vmcall;"
        : "=a" (retregs->rax), "=b" (retregs->rbx),
          "=c" (retregs->rcx), "=d" (retregs->rdx),
          "=S" (retregs->rsi), "=D" (retregs->rdi)
        : "a" (argregs->rax), "b" (argregs->rbx),
          "c" (argregs->rcx), "d" (argregs->rdx),
          "S" (argregs->rsi), "D" (argregs->rdi)
        : "memory");

    signal (SIGILL, old_sigill);

    return 1;
}

static int
conv_pyobj_reg(PyObject *pyobj, unsigned long *reg)
{
#ifdef IS_PY3K
    if (PyLong_Check(pyobj)) {
#else
    if (PyLong_Check(pyobj) || PyInt_Check(pyobj)) {
#endif
        *reg = PyLong_AsUnsignedLong(pyobj);
    }
    else if (PyBool_Check(pyobj)) {
        *reg = (pyobj == Py_True) ? 1 : 0;
    }
    else if (PyBytes_Check(pyobj)) {
        *reg = (unsigned long)PyBytes_AsString(pyobj);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "argument should be unsigned long, bool or bytes");
        return 0;
    }

    if (PyErr_Occurred()) {
        return 0;
    }

    return 1;
}

static int
build_regs(PyObject *args[NUM_REGS], regs_t *regs)
{
    unsigned long *regarray[NUM_REGS] = {&regs->rax, &regs->rbx, &regs->rcx, &regs->rdx, &regs->rsi, &regs->rdi};

    memset(regs, 0, sizeof(*regs));

    int i;
    for (i = 0; i < NUM_REGS; i++) {
        if (args[i] == NULL) {
            continue;
        }
        else if (!conv_pyobj_reg(args[i], regarray[i])) {
            return 0;
        }
    }

    return 1;
}

static int
_vmcall_vmcall(PyObject *self, PyObject *args, PyObject *keywds, regs_t *retregs)
{
    static char *kwlist[] = {"rax", "rbx", "rcx", "rdx", "rdi", "rsi", NULL};

    PyObject *aobjs[NUM_REGS] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OOOOOO", kwlist,
        &aobjs[0], &aobjs[1], &aobjs[2], &aobjs[3], &aobjs[4], &aobjs[5])) {
        return 0;
    }

    regs_t argregs;
    if (!build_regs(aobjs, &argregs)) {
        return 0;
    }

    if (!call_vmcall(&argregs, retregs)) {
        return 0;
    }

    return 1;
}

static PyObject *
vmcall_vmcall(PyObject *self, PyObject *args, PyObject *keywds)
{
    regs_t retregs;
    if (!_vmcall_vmcall(self, args, keywds, &retregs)) {
        return 0;
    }

    return Py_BuildValue("k", retregs.rax);
}

static PyObject *
vmcall_vmcall_getregs(PyObject *self, PyObject *args, PyObject *keywds)
{
    regs_t retregs;
    if (!_vmcall_vmcall(self, args, keywds, &retregs)) {
        return 0;
    }

    return Py_BuildValue("(kkkkkk)", retregs.rax, retregs.rbx, retregs.rcx, retregs.rdx, retregs.rsi, retregs.rdi);
}

static PyMethodDef vmcall_methods[] = {
    {"vmcall", vmcall_vmcall, METH_VARARGS | METH_KEYWORDS,
    "call vmcall and return rax"},
    {"vmcall_getregs", vmcall_vmcall_getregs, METH_VARARGS | METH_KEYWORDS,
    "call vmcall and return tuple of rax, rbx, rcx, rdx, rsi and rdi"},
    {NULL, NULL, 0, NULL}
};

#ifdef IS_PY3K

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "vmcall",
    NULL,
    -1,
    vmcall_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyObject *
PyInit_vmcall(void)
{
    PyObject *module = PyModule_Create(&moduledef);
    return module;
}

#else

PyMODINIT_FUNC
initvmcall(void)
{
    (void) Py_InitModule("vmcall", vmcall_methods);
}

#endif
