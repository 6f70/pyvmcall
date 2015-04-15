from distutils.core import setup, Extension

module = Extension('vmcall',
            sources = ['pyvmcall.c'])

setup(name = 'pyvmcall',
        version = '1.0',
        description = 'This is a pyvmcall package',
        ext_modules = [module])
