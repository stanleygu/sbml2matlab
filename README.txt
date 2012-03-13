#summary Using the sbml2matlab

= Introduction =

sbml2matlab is an executable that can be called from the command line to translate SBML files to their MATLAB model equivalents.

= Usage =
To use sbml2matlab in command line:
{{{
sbml2matlab.exe -input [inputFile.sbml] -output [outputFile.m]
}}}
Replace the square brackets with the paths of the input and output files, respectively.

= Limitations =

Generally, sbml2matlab currently does not support the use of events, algebraic rules, or compartment rate rules.

Validation test and current analysis of sbml2matlab can be found [http://code.google.com/p/sbwtools/wiki/CurrentValidation here].

The updated sbml2matlab wiki may be found [http://code.google.com/p/sbml2matlab/w/list here].