#!/usr/bin/env python

from setuptools import setup, Distribution

class BinaryDistribution(Distribution):
    """Distribution which always forces a binary package with platform name"""
    def has_ext_modules(foo):
        return True


setup(name='sbml2matlab',
      version='1.2.3',
      description='An SBML to MATLAB translator',
      author='Stanley Gu, Lucian Smith',
      author_email='stanleygu@gmail.com, lpsmith@uw.edu',
      url='https://github.com/stanleygu/sbml2matlab',
      packages=['sbml2matlab'],
      package_data={
          # add dll, won't hurt unix, not there anyway
          "sbml2matlab": ["_sbml2matlab.pyd", "*.dll", "*.txt",
                          "*.lib", "*.so"]},
      distclass=BinaryDistribution,
      )
