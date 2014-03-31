#!/usr/bin/env python

from distutils.core import setup

setup(name='sbml2matlab',
      version='1.2.2',
      description='An SBML to MATLAB translator',
      author='Stanley Gu, Lucian Smith',
      author_email='stanleygu@gmail.com, lucianoelsmitho@gmail.com',
      url='https://github.com/stanleygu/sbml2matlab',
      packages=['sbml2matlab'],
      package_data={
              # add dll, won't hurt unix, not there anyway
              "sbml2matlab": ["_libsbml2matlab.pyd", "*.dll", "*.txt",
                              "*.lib"]
      })
