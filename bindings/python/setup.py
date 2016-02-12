#!/usr/bin/env python

from distutils.core import setup

sbml2matlab_version = '1.2.3'


# get the os string
def _getOSString():

    import platform

    s = platform.system().lower()
    unix = False

    if s.startswith('darwin'):
        s = 'macosx'
        unix = True
    elif s.startswith('linux'):
        s = 'linux'
        unix = True
    elif s.startswith('win'):
        s = 'win'
    else:
        raise SystemExit("I don't know how to install on {}, " +
                         "only know OSX(x86_64), Linux(i386,x86_64)" +
                         " and Win(32)"
                         .format(s))

    arch = platform.architecture()

    if unix and arch[0].startswith('64'):
        arch = 'x86_64'
    elif unix and arch[0].startswith('32'):
        arch = 'i386'
    else:
        arch = '32'  # win 32

    return s + "_" + arch

version = sbml2matlab_version + "-" + _getOSString()

setup(name='sbml2matlab',
      version=version,
      description='An SBML to MATLAB translator',
      author='Stanley Gu, Lucian Smith',
      author_email='stanleygu@gmail.com, lucianoelsmitho@gmail.com',
      url='https://github.com/stanleygu/sbml2matlab',
      packages=['sbml2matlab'],
      package_data={
          # add dll, won't hurt unix, not there anyway
          "sbml2matlab": ["_sbml2matlab.pyd", "*.dll", "*.txt",
                          "*.lib", "*.so"]
      })
