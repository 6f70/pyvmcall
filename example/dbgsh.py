#!/usr/bin/env python3
# dbgsh for BitVisor written in Python 3
import vmcall
import sys
import os
import ctypes
import termios

MINUS_ONE   = ctypes.c_ulong(-1).value

def vmcall_dbgsh(ch):
    func = vmcall.vmcall(0, b'dbgsh')
    return vmcall.vmcall(func, ch)

def dbgsh():
    vmcall_dbgsh(MINUS_ONE)
    s = MINUS_ONE
    while True:
        r = ctypes.c_long(vmcall_dbgsh(s)).value
        if r == (0x100 | ord('\n')):
            vmcall_dbgsh(0)
            break

        s = MINUS_ONE
        if r == 0:
            ch = sys.stdin.read(1)
            s = 0x100 if len(ch) == 0 else ord(ch)
        elif r > 0:
            r &= 0xff
            sys.stdout.write(chr(r))
            sys.stdout.flush()
            s = 0

def main():
    fd = sys.stdin.fileno()
    if os.isatty(fd):
        old = termios.tcgetattr(fd)
        new = termios.tcgetattr(fd)
        new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
        try:
            termios.tcsetattr(fd, termios.TCSADRAIN, new)
            dbgsh()
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)
    else:
        dbgsh()

if __name__ == '__main__':
    main()
