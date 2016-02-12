/**
 * \file    sbml2matlab.i
 * \brief   Language-independent SWIG directives for wrapping sbml2matlab
 * \author  Lucian Smith, based on libsbml code from Ben Bornstein and Ben Kovitz
 *
 */

%module(directors="1") sbml2matlab

#pragma SWIG nowarn=473,401,844

%pragma(java) moduleclassmodifiers="
/**
  * Wrapper class for global methods and constants defined by libSBML.
  * <p>
  * <em style='color: #555'>
  * This class of objects is defined by sbml2matlab only and has no direct
  * equivalent in terms of Sbml2matlab components.
  * </em>
  * <p>
  * In the C++ version of sbml2matlab, models are parsed and stored in a
  * global object, which can then be queried by subsequent calls to
  * sbml2matlab API functions.  However, all returned elements become the
  * property of the caller.
  */
public class"


%{
#include "sbml2matlab.h"
%}

/**
 *
 * Includes a language specific interface file.
 *
 */

%include local.i

/**
 * Disable warnings about const/non-const versions of functions.
 */
#pragma SWIG nowarn=516


/**
 * Ignore the 'freeMatlabString' function, as SWIG handles the new returned objects above,
 * and ignore all the functions that use argument inputs.
 */


%ignore freeMatlabString;
%ignore sbml2matlab;
%ignore getNthSbmlError;

/**
 * Rename getMatlab as 'sbml2matlab',
 */
%rename(sbml2matlab) getMatlab;


/**
 * Converting input arguments to output arguments
 */

%include "typemaps.i"

/**
 * Wrap these files.
 */

%include sbml2matlab.h
