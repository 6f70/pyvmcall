# pyvmcall

Call VMCALL from python

## Installation

    python setup.py install

## Usage

    >>> import vmcall
    >>> vmcall.vmcall(0, 1, 2, 3, 4, 5)
    0
    >>> vmcall.vmcall(b'hello', rsi=100, rdi=200)
    0
    >>> vmcall.vmcall_getregs(0, b'dbgsh')
    (1, 139955703340600, 0, 0, 0, 0)
    >>>

## License

This projected is licensed under the terms of the MIT license.
