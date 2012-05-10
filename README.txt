# Introduction

sbml2matlab is an executable for translating SBML files to their MATLAB model equivalents.

# Usage
sbml2matlab may be called in four different ways:

1. sbml2matlab.exe
SBML input is from stdin, MATLAB output is to stdout

2. sbml2matlab.exe -input [inputFile.sbml]
SBML input is from inputFile.sbml, Matlab output is to stdout

3. sbml2matlab.exe -output [outputFile.m]
SBML input is from stdin, MATLAB output is to outputFile.m

4. sbml2matlab.exe -input [inputFile.sbml] -output [outputFile.m]
SBML input is from inputFile.sbml, MATLAB is to outputFile.m

Replace the square brackets with the paths of the input and output files, respectively.
