/**
* @file NOMLib.h
* @author  Frank Bergmann <fbergman@kgi.edu>, Sri Paladugu <spaladug@kgi.edu>, Herbert Sauro <hsauro@u.washington.edu>, Stanley Gu <stanleygu@gmail.com>
* @date February 22, 2012
* @brief NOM version of libSBML
*
*/

/* C API to libSBML that implements the original NOM API
*
* SBW defined a simplified SBML API. This project provides the same API in the form
* of a DLL than can be used from a variety of programming languages. 
*
* Copyright 2007-2011 SBW Team http://sys-bio.org/
*
* Organization: University of Washington
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
* and associated documentation files (the "Software"), to deal in the Software without restriction, 
* including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom the Software is furnished to 
* do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or 
* substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
* NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef SBML_SUPPORT_H
#define SBML_SUPPORT_H

#define DLL_EXPORT __declspec(dllexport)

#define WIN32_LEAN_AND_MEAN
#undef SEVERITY_ERROR

#include "sbml/SBMLTypes.h"
#include "sbml/math/ASTNode.h"

#define BUFFER_SIZE 1024
#define FUNCDATAROWS 44
#define FUNCDATACOLS 16

#define GET_ID_IF_POSSIBLE(x) (x->isSetId() ?  x->getId() :  x->getName() )

#define GET_NAME_IF_POSSIBLE(x) (x->isSetName() ?  x->getName() :  x->getId() )

#include <string>
#include <vector>


extern "C" {

	/** @brief Returns the error message given the last error code generated
	*
	* @return char* to the error message
	*/
	DLL_EXPORT char *getError ();


	/** @brief Load SBML into the NOM. 
	*
	* @param[in] sbmlStr sbmlStr is a char pointer to the SBML model
	* @return -1 if there has been an error, otherwise returns 0
	*/
	DLL_EXPORT int loadSBML(char* sbmlStr);


	/** @brief Returns number of errors in SBML model
	*
	* @return -1 if there has been an error, otherwise returns number of errors in SBML model
	*/
	DLL_EXPORT int getNumErrors();


	/** @brief Returns details on the index^th error
	*
	* @param[in] index The index^th error in the list
	* @param[out] line The line number in the SBML file that corresponds to the error
	* @param[out] column The column number in the SBML file that corresponds to the error
	* @param[out] errorId The SBML errorId (see libSBML for details);
	* @param[out] errorType The error type includes "Advisory", "Warning", "Fatal", "Error", and "Warning"
	* @param[out] errorMsg The error message associated with the error
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthError (int index, int *line, int *column, int *errorId, char **errorType, char **errorMsg);


	/** @brief Validates the given SBML model
	*
	* @return -1 if the SBML model is invalid, else returns 0
	*/
	DLL_EXPORT int validateSBML(char *cSBML);


	/** @brief Returns 0 (false) in the argument list if the species given by sId does not have
	*  an amount associated with it, otherwise returns 1 (true)
	*
	* @param[in] sId is the Id of the species
	* @param[out] isInitialAmount is 0 if false, 1 if true
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int hasInitialAmount (char *sId, bool *isInitialAmount);


	/** @brief Returns 0 (false) in the argument list if the species given by sId does not have
	*  a concentration associated with it, otherwise returns 1 (true)
	*
	* @param[in] cId is the Id of the species
	* @param[out] hasInitial is 0 if false, 1 if true
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int hasInitialConcentration (char *cId, int *hasInitial);


	/** @brief Get the value for a given symbol in the SBML
	*
	* @param[in] sId This is the name of the symbol to request the value for
	* @param[out] value The value of the symbol is returned in this argument
	* @return -1 if there has been an error, otherwise returns 0
	*/
	DLL_EXPORT int getValue (char *sId, double *value);


	/** @brief Set the value for a given symbol in the SBML
	*
	* @param[in] sId is the name of the symbol to set the value to
	* @param[in] dValue The value which wish to use
	* @return -1 if there has been an error, otherwise returns 0
	*/
	DLL_EXPORT int setValue (char *sId, double dValue);


	/** @brief Retrusn 0 (false) the supplied SBML string is invalid, else returns 1 (true)
	*
	* @param[in] sbmlStr is the SBML string to validate
	* @return -1 if there has been an error, otherwise returns 0
	*/
	DLL_EXPORT int validate(char *sbmlStr);


	/** @brief Return the model name in the current model
	*
	* @param[out] name of the model
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getModelName (char **name);


	/** @brief Return the model Id for the current model
	*
	* @param[out] sId the current model
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getModelId (char **sId);


	/** @brief Set the model Id for the current model
	*
	* @param[out] cId the Id to set in the current model
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int setModelId (char *cId);


	/** @brief Return the number of function definitions in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of function definitions
	*/
	DLL_EXPORT int getNumFunctionDefinitions();


	/** @brief Return the number of compartment in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of compartments
	*/
	DLL_EXPORT int getNumCompartments();


	/** @brief Return the number of reactions in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of reactions
	*/
	DLL_EXPORT int getNumReactions();


	/** @brief Return the number of floating species in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of floating species
	*/
	DLL_EXPORT int getNumFloatingSpecies();


	/** @brief Return the number of boundary species in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of boundary species
	*/
	DLL_EXPORT int getNumBoundarySpecies();


	/** @brief Return the number of global parameters in the current model
	*
	* @return -1 if there has been an error, otherwise returns # of global parameters
	*/
	DLL_EXPORT int getNumGlobalParameters();


	/** @brief Returns the nIndex^th global parameter name
	*
	* @param[in] nIndex is the nIndex^th global parameter name
	* @param[out] name is the name of the nIndex^th global parameter name
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthGlobalParameterName (int nIndex, char **name);


	/** @brief Returns the nIndex^th global parameter Id
	*
	* @param[in] nIndex is the nIndex^th global parameter Id
	* @param[out] sId is the Id of the nIndex^th global parameter Id
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthGlobalParameterId (int nIndex, char **sId);


	/** @brief Collects information on the index^th function definition
	*
	* @param[in] index is the index^th function definition to consider
	* @param[out] fnId is the Id for this function definition
	* @param[out] numArgs is the number of arguments for this function definition
	* @param[out] argList is the list of arguments (names) to the function definition
	* @param[out] body is the main body of the function definition in infix notation
	* @return -1 if there has been an error, otherwise returns 0
	*/
	DLL_EXPORT int getNthFunctionDefinition (int index, char** fnId, int *numArgs, char*** argList, char** body);



	/** @brief Returns the nIndex^th compartment name
	*
	* @param[in] nIndex is the nIndex^th compartment name
	* @param[out] name is the name of the nIndex^th compartment
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthCompartmentName (int nIndex, char **name);


	/** @brief Returns the nIndex^th compartment Id
	*
	* @param[in] nIndex is the nIndex^th compartment Id
	* @param[out] sId is the Id of the nIndex^th floating species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthCompartmentId (int nIndex, char **sId);


	/** @brief Returns the compartment Id associated with a particular species Id
	*
	* @param[in] cId species Id
	* @param[out] sId is the Id of the accociated species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getCompartmentIdBySpeciesId (char *cId, char **compId);

	/** @brief Returns a list of the Ids of the floating species
	*
	* @param[out] IdList is a array of char* containing the names of the floating species
	* @param[out] numFloat is the number of boundary species in the list
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getListOfFloatingSpeciesIds (char*** IdList, int *numFloat);


	/** @brief Returns the nIndex^th floating species name
	*
	* @param[in] nIndex is the nIndex^th floating species name
	* @param[out] name is the name of the nIndex^th floating species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthFloatingSpeciesName (int nIndex, char **name);


	/** @brief Returns the nIndex^th floating species Id
	*
	* @param[in] nIndex is the nIndex^th floating species Id
	* @param[out] sId is the Id of the nIndex^th floating species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthFloatingSpeciesId (int nIndex, char**sId);

	/** @brief Returns a list of the Ids of the boundary species
	*
	* @param[out] IdList is a array of char* containing the names of the boundary species
	* @param[out] numBoundary is the number of boundary species in the list
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getListOfBoundarySpeciesIds (char*** IdList, int *numBoundary);

	/** @brief Returns the nIndex^th boundary species name
	*
	* @param[in] nIndex is the nIndex^th boundary spedcies name
	* @param[out] name is the name of the nIndex^th boundary species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthBoundarySpeciesName (int nIndex, char **name);


	/** @brief Returns the nIndex^th boundary species Id
	*
	* @param[in] nIndex is the nIndex^th boundary spedcies Id
	* @param[out] sId is the Id of the nIndex^th boundary species
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthBoundarySpeciesId (int nIndex, char **sId);


	/** @brief Returns the number of rules in the SBML model
	*
	* @return -1 if there has been an error or the number of rules
	*/
	DLL_EXPORT int getNumRules();


	/** @brief Returns the nIndex^th rule from the current model
	*
	* @param[in] nIndex is the nIndex^th rule
	* @param[out] rule Pointer to a char* that will return the rule itself
	* @param[out] ruleType Pointer to a char* that will return the type of the rule (i.e. algebraic, assignment, etc)
	* @return -1 if there has been an error or the number of rules
	*/
	DLL_EXPORT int getNthRule (int nIndex, char **rule, int *ruleType);


	/** @brief Returns the number of events in the SBML model
	*
	* @return -1 if there has been an error or the number of events
	*/
	DLL_EXPORT int getNumEvents();


	/** @brief Returns the nIndex^th reaction name
	*
	* @param[in] nIndex is the nIndex^th reaction
	* @param[out] name is the name of the nIndex^th reaction
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthReactionName (int nIndex, char **name);


	/** @brief Returns if reaction is reversible
	*
	* @param[in] arg is the reaction number index
	* @param[out] isReversible is 1 if the reaction is reversible and 0 if not.
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int isReactionReversible(int arg, int *isReversible);


	/** @brief Returns the nIndex^th reaction Id
	*
	* @param[in] nIndex is the nIndex^th reaction Id
	* @param[out] sId is the Id that is returned to the caller
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthReactionId (int nIndex, char **sId);


	/** @brief Return the number of reactants for the arg^th reaction
	*
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNumReactants (int arg);


	/** @brief Return the number of reactants for the arg^th reaction
	*
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNumProducts (int arg);

	/** @brief Return the name of the arg2^th reactant from the arg1^th reaction
	*
	* @param[in] arg1 is the ith reaction 
	* @param[in] arg2 is the ith reactant
	* @param[out] name is the reactant name that is returned
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthReactantName (int arg1, int arg2, char **name);


	/** @brief Return the name of the arg2^th reactant from the arg1^th reaction
	*
	* @param[in] arg1 is the ith reaction 
	* @param[in] arg2 is the ith product
	* @param[out] name is the product name that is returned
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthProductName (int arg1, int arg2, char **name);


	/** @brief Return the kinetic law of the index^th reaction
	*
	* @param[in] index is the ith reaction  to obtain the kinetic law from
	* @param[out] kineticLaw is the string returned by the call
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getKineticLaw (int index, char **kineticLaw);


	/** @brief Returns the arg2^th reactant stoichiometry from the arg1^th reaction
	*
	* @param[in] arg1 is the ith reaction 
	* @param[in] arg2 is the ith reactant
	* @return -1 if there has been an error or the value of the stoichiometric amount
	*/
	DLL_EXPORT int getNthReactantStoichiometry (int arg1, int arg2);

	/** @brief Returns the arg2^th product stoichiometry from the arg1^th reaction
	*
	* @param[in] arg1 is the ith reaction 
	* @param[in] arg2 is the ith product
	* @return -1 if there has been an error or the value of the stoichiometric amount
	*/
	DLL_EXPORT int getNthProductStoichiometry (int arg1, int arg2);


	/** @brief Returns the number of local parameters
	*
	* @param[in] reactionIndex is the ith reaction 
	* @return -1 if there has been an error or the number of local parameters
	*/
	DLL_EXPORT int getNumLocalParameters (int reactionIndex);


	/** @brief Returns the name of nth local parameter is a given reaction
	*
	* @param[in] reactionIndex is the ith reaction 
	* @param[in] parameterIndex is the ith product
	* @param[out] sId Pointer to the name of the local parameter
	* @return -1 if there has been an error 
	*/
	DLL_EXPORT int getNthLocalParameterName (int reactionIndex, int parameterIndex, char **sId);


	/** @brief Returns the Id of nth local parameter is a given reaction
	*
	* @param[in] reactionIndex is the ith reaction 
	* @param[in] parameterIndex is the ith product
	* @param[out] sId Pointer to the Id of the local parameter
	* @return -1 if there has been an error 
	*/
	DLL_EXPORT int getNthLocalParameterId (int reactionIndex, int parameterIndex, char **sId);


	/** @brief Returns the value of the specificed local parameter
	*
	* @param[in] reactionIndex is the ith reaction 
	* @param[in] parameterIndex is the ith product
	* @param[out] value Pointer to the value of the local parameter
	* @return -1 if there has been an error 
	*/
	DLL_EXPORT int getNthLocalParameterValue (int reactionIndex, int parameterIndex, double *value);


	/** @brief Any local parameters in an SBML model are promoted to global status by this call. 
	*
	* @param[in] in SBML is the input sbml string
	* @param[out] ou tSBML is output sbml string with local parameters promoted to global parameters
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getParamPromotedSBML (char *inSBML, char **outSBML);


	/** @brief Fills in any missing modifiers to the SBML file
	*
	* @param[in] SBML is the input sbml string
	* @param[out] SBML is output sbml string with modifiers added
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int addMissingModifiers (char *inSBML, char **outSBML);


	/** @brief Converts a MathML string into infix notation
	*
	* @param[in] MathML is the input string
	* @param[out] infix notation is the output string
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int convertMathMLToString (char *mathMLStr, char **infix);

	/** @brief Converts an infix string into MathML Notation
	*
	* @param[in] infix is the input string
	* @param[out] MathML notation is the output string
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int convertStringToMathML (char* infixStr, char **mathMLStr);

	/** @brief reorders rules in SBML
	*
	* @param[in] sbml is the input sbml string to be modified by rule reordering
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int reorderRules(char **sbml);

	/** @brief converts input SBML to another level and version
	*
	* @param[in] inputModel is the input SBML to be converted to another version
	* @param[out] outputModel is the pointer to the output SBML 
	* @param[in] nLevel is the level of output SBML
	* @param[in] nVersion is the version of output SBML
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int convertSBML(char *inputModel, char **outputModel, int nLevel, int nVersion);

}

#endif

