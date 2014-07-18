/** 
 * Filename    : NOMLib.cpp
 * Description : NOM version of libSBML
 * Original by :
 * Author(s)   : Frank Bergmann <fbergman@kgi.edu> 
 * Author(s)   : Sri Paladugu <spaladug@kgi.edu> 
 * Author(s)   : Herbert Sauro <Herbert_Sauro@kgi.edu> 
 * This version: Herbert Sauro <hsauro@u.washington.edu> 
 *               Stanley Gu <stanleygu@gmail.com>
 *
 * Organization: University of Washington
 *
 * Copyright 2007-2011 SBW Team http://sys-bio.org/
 * 
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

// November 16th 2011, Removed dependence on SBW


//#define WIN32_LEAN_AND_MEAN
#include "NOM.h"

#include "sbml/conversion/ConversionProperties.h"
#include "sbml/conversion/SBMLRuleConverter.h"
#include "sbml/conversion/SBMLFunctionDefinitionConverter.h"

#ifdef WIN32
#ifndef CYGWIN
#define strdup _strdup
#endif
#endif


SBMLDocument* _oSBMLDocCPP = NULL;
Model* _oModelCPP = NULL;

int errorCode = 0;
char *extendedErrorMessage = NULL;

static const char *errorMessages[] = {
	  "No Error", 
	  "The input string cannot be blank in loadSBML", // 1
	  "Validation failed: ", // 2
	  "Invalid input - Argument should be >= 0 and should be less than total number of Function Definitions in the model", // 3
	  "Invalid input - Argument should be >= 0 and should be less than total number of compartments in the model",
	  "The model does not have a floating species corresponding to the index provided", // 5
	  "The model does not have a floating species corresponding to the index provided", // 6
	  "The model does not have a boundary species corresponding to the index provided", // 7
	  "The model does not have a boundary species corresponding to the index provided",
	  "Index exceeds the number of reactants in the list", // 9
	  "There is no reaction corresponding to the index you provided", // 10
	  "Index exceeds the number of products in the list", // 11
	  "There is no parameter corresponding to the index you provided", // 12
	  "Model symbol does not exist", // 13
	  "Invalid string name. The name is not a valid id/name of a floating / boundary species.", // 14
	  "Invalid string name. The id does not exist in the model", // 15
	  "There is no reaction corresponding to the index you provided", // 16
	  "Index exceeds the number of parameters in the list", // 17
	  "The model does not have a species corresponding to the Id provided", // 18
	  "The model does not have a Rule corresponding to the index provided", // 19
	  "The model does not have a Event corresponding to the index provided", // 20
	  "Invalid string name. The name is not a valid id/name of a floating / boundary species.", // 21
	  "No Error for this index.", // 22
	  "The input MathML is invalid", // 23
	  "The input infix is invalid", // 24
	  "SBML rule re-ordering failed" // 25
	 ,"SBML conversion failed" // 26
}; 

static const char* zero = "0";

extern "C" {

// -------------------------------------------------------------------------------------------
// Support functions used by the library
// -------------------------------------------------------------------------------------------

// Concatentate two strings together and return the new string
char *strconcat(const char *s1, const char *s2)
{
	size_t old_size;
	char *t;

	old_size = strlen(s1);

	/* cannot use realloc() on initial const char* */
	t = (char *) malloc(sizeof(char)*(old_size + strlen(s2) + 1));
	strcpy(t, s1);
	strcpy(t + old_size, s2);
	return t;
}


char* strCopySubstr(const char *str, int index, int count)
{
	char *result;
	int i;
	if (str == NULL || index < 0 || count <= 0) return NULL;

	result = (char* ) malloc(sizeof(char)*(count+1) );
	memset(result, 0, sizeof(char)*(count+1));

	for (i = 0; i < count; i++)
		result[i] = str[index + i];
	result[i] = '\0';
	return result;
}


int validateInternal (const std::string &sbml)
{
	SBMLReader oReader;
	SBMLDocument *oDoc = oReader.readSBMLFromString(sbml);
    if (oDoc->getErrorLog()->getNumFailsWithSeverity(LIBSBML_SEV_ERROR) > 0)
	{
		stringstream oStream; 
		oDoc->printErrors (oStream);
		errorCode = 2;
        free(extendedErrorMessage);
		extendedErrorMessage = strdup(oStream.str().c_str());		
		return -1;
	}
	delete oDoc;
	return 0;
}

unsigned int getNumBoundarySpeciesInternal()
{
	unsigned int nNumSpecies = _oModelCPP->getNumSpecies();
	unsigned int nResult = 0;
	for (unsigned int i = 0; i < nNumSpecies; i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies(i);
		if (oSpecies->getBoundaryCondition())
		{
			nResult++;
		}
	}
	return nResult;
}

void checkForMissingNames(const ASTNode * node, vector< string> &results, vector < string > symbols)
{
	for (unsigned int i = 0; i < node->getNumChildren(); i ++)
	{
		checkForMissingNames(node->getChild(i), results, symbols);
	}
	if (node->isName())
	{
		string sName = node->getName();
		if (sName.empty() || sName == "") return;
		bool bFound = false;

		for (unsigned int i = 0; i < results.size(); i++)
		{
			if (results[i] == sName)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				if (symbols[i] == sName)
				{
					bFound = true;
					break;
				}
			}
		}
		if (!bFound)
			results.push_back(sName);
	}
}

DLL_EXPORT int convertSBML(const char *inputModel, char **outputModel, int nLevel, int nVersion)
{
	SBMLDocument *oSBMLDoc = readSBMLFromString(inputModel);
	Model *oModel = oSBMLDoc->getModel();

	if (oModel == NULL )
	{
		delete oSBMLDoc;
		oModel = NULL;
		oSBMLDoc = NULL;

		validateInternal(inputModel);
	}

	oSBMLDoc->getErrorLog()->clearLog();
	oSBMLDoc->setLevelAndVersion(nLevel, nVersion, false);

	if (oSBMLDoc->getNumErrors() > 0)
	{

		stringstream oStream; 
		oSBMLDoc->printErrors (oStream);
		delete oSBMLDoc;
		oSBMLDoc = NULL;
		oModel = NULL;
		errorCode = 26;
		return -1;
		throw ("Conversion failed", oStream.str());		
	}

	*outputModel = writeSBMLToString(oSBMLDoc);
	delete oSBMLDoc;
	oSBMLDoc = NULL;
	oModel = NULL;
	return 0;
}

void freeModel()
{
	try
	{
		delete _oSBMLDocCPP;
	}
	catch(...)
	{
	}
	_oModelCPP = NULL;
	_oSBMLDocCPP = NULL;

}


char* addMissingModifiersInternal(const string& sModel)
{
	try
	{
		SBMLDocument* d = readSBMLFromString(sModel.c_str());  

	if (d->getLevel() == 1)
	{
	  delete d;
	  //string newModel = convertSBML(sModel, 2, 1);
	  char * outputModel[1];
	  convertSBML(sModel.c_str(),outputModel, 2, 1);
	  string newModel = *outputModel;
	  return addMissingModifiersInternal(newModel);
	}
		try
		{
			const Model * oModel = d->getModel();
			if (oModel != NULL)
			{									
				vector < string > _species;
				for (unsigned int i = 0; i < oModel->getNumSpecies(); i++)
				{
					_species.push_back(GET_ID_IF_POSSIBLE(oModel->getSpecies(i)));
				}
				unsigned int nReactions = oModel->getNumReactions();
				bool bReplaced = false;
				for (unsigned int i = 0; i < nReactions; i++)
				{
					Reaction *oReaction = const_cast<Reaction *>(oModel->getReaction(i));
					KineticLaw *oLaw = oReaction->getKineticLaw();
					if (oLaw == NULL) continue;
					vector< string > symbols;
					for (unsigned int j = 0; j < oReaction->getNumModifiers(); j++)
					{
						symbols.push_back(oReaction->getModifier(j)->getSpecies());
					}
					for (unsigned int j = 0; j < oModel->getNumParameters(); j++)
					{
						symbols.push_back(GET_ID_IF_POSSIBLE(oModel->getParameter(j)));
					}
					for (unsigned int j = 0; j < oModel->getNumCompartments(); j++)
					{
						symbols.push_back( GET_ID_IF_POSSIBLE(oModel->getCompartment(j)));
					}
					for (unsigned int j = 0; j < oModel->getNumFunctionDefinitions(); j++)
					{
						symbols.push_back(GET_ID_IF_POSSIBLE(oModel->getFunctionDefinition(j)));
					}


					for (unsigned int j = 0; j < oReaction->getNumReactants(); j++)
					{
						symbols.push_back(oReaction->getReactant(j)->getSpecies());
					}
					for (unsigned int j = 0; j < oReaction->getNumProducts(); j++)
					{
						symbols.push_back(oReaction->getProduct(j)->getSpecies());
					}						
					for (unsigned int j = 0; j < oLaw->getNumParameters(); j++)
					{
						symbols.push_back(GET_ID_IF_POSSIBLE(oLaw->getParameter(j)));
					}

					const ASTNode *oRoot = oLaw->getMath();
					vector< string > oMissingNames;
					// here the fancy function that discoveres themissing names and solves all problems 
					// magically ... 
					checkForMissingNames(oRoot, oMissingNames, symbols);
					string sMissingName;
					if (oMissingNames.size() > 0)
					{
						bReplaced = true;
						//cout << "Found missing names for reaction: " << i << endl;
						for (unsigned int j = 0; j < oMissingNames.size(); j++)
						{
							sMissingName = oMissingNames[j];
							if (sMissingName != "" && !sMissingName.empty())
							{
								bool bFound = false;
								for (unsigned int k =0; k < _species.size(); k++)
								{
									if (sMissingName == _species[k])
									{
										bFound = true;
										break;
									}
								}
								if (!bFound)
								{
									//cout << "found missing symbol: " << sMissingName << " but it seems to be no species ... ";
								}
								else
								{
									//cout << "name: " << j << " is: " << sMissingName << endl;
									ModifierSpeciesReference* reference = oReaction->createModifier();
									reference->setSpecies(sMissingName);
									oReaction->addModifier(reference);
								}
							}								
						}
					}
				}
				if (!bReplaced) 
				{
					return strdup(sModel.c_str());
				}
			}
		}
		catch(...)
		{
			throw ("Exception occured while trying to modify the SBML file");
		}

		SBMLWriter oWriter;
		char *cResult = oWriter.writeToString(d);			
		delete d;			
		return  cResult;
	}
	catch(...)
	{
		throw ("Model could not be modified");
	}
}


// -------------------------------------------------------------------------------------------
// START OF EXPORTED FUNCTIONS
// -------------------------------------------------------------------------------------------


// Load SBML file into the NOM
DLL_EXPORT int loadSBML(const char* sbmlStr)
{	
	string arg = sbmlStr;

	if (sbmlStr == "")
	{
		errorCode = 1;
		return -1;
	}

	if (_oSBMLDocCPP != NULL || _oModelCPP != NULL)
	{
		freeModel();
	}

	SBMLReader oReader;
	_oSBMLDocCPP = oReader.readSBMLFromString(arg);
	_oModelCPP = _oSBMLDocCPP->getModel();

	if (_oModelCPP == NULL)
	{
		if (arg.find("<?xml") == arg.npos)
		{
			string sSBML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + arg;
			return loadSBML(sSBML.c_str());
		}
		ConversionProperties props;
		props.addOption("sortRules", true, "sort rules");
		_oSBMLDocCPP->convert(props);
		return validateInternal(arg);
	}
    //Otherwise, this worked.
    return 0;
}


// Call this is if any method returns -1, it will return the error message string
DLL_EXPORT const char *getError () 
{
	if (extendedErrorMessage != NULL) {
	   char *t = strconcat (errorMessages[errorCode], extendedErrorMessage);
	   return t;
	} else
	   return errorMessages[errorCode];
}

DLL_EXPORT int hasInitialAmount (char *sId, bool *isInitialAmount)
{
	string ssId = sId;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;				
	}

	Species *oSpecies = _oModelCPP->getSpecies(ssId);
	if (oSpecies != NULL) {
		*isInitialAmount = oSpecies->isSetInitialAmount();
		return 0;
	}

	errorCode = 14;
	return -1;
	throw ("Invalid string name. The name is not a valid id/name of a floating / boundary species.");
}



DLL_EXPORT int hasInitialConcentration (char *cId, int *hasInitial)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	string sId = cId;

	Species *oSpecies = _oModelCPP->getSpecies(sId);
	if (oSpecies != NULL) {
		*hasInitial = (int) oSpecies->isSetInitialConcentration();
		return 0;
	}

	errorCode = 21;
	return -1;
}


DLL_EXPORT int getValue (char *sId, double *value)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	Species *oSpecies = _oModelCPP->getSpecies(sId);
	if (oSpecies != NULL)
	{
		if (oSpecies->isSetInitialAmount())
			*value = oSpecies->getInitialAmount();
		else if (oSpecies->isSetInitialConcentration())
			*value = oSpecies->getInitialConcentration();
		else
			*value = 0.0;
		return 0;
	}

	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
	if (oCompartment != NULL)
	{
      if (oCompartment->isSetVolume())
		*value = oCompartment->getVolume();
      else
        *value = 0.0;
      return 0;
	}

	Parameter *oParameter = _oModelCPP->getParameter(sId);
	if (oParameter != NULL)
	{
      if (oParameter->isSetValue())
		*value = oParameter->getValue();
      else 
        *value = 0.0;
      return 0;
	}
	errorCode = 15;
	return -1;
}

DLL_EXPORT int setValue (char *sId, double dValue)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	Species *oSpecies = _oModelCPP->getSpecies(sId);
	if (oSpecies != NULL)
	{
		if (oSpecies->isSetInitialAmount())
			oSpecies->setInitialAmount(dValue);
		else
			oSpecies->setInitialConcentration(dValue);
		return 0;
	}

	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
	if (oCompartment != NULL)
	{
		oCompartment->setVolume(dValue);
		return 0;
	}

	Parameter *oParameter = _oModelCPP->getParameter(sId);
	if (oParameter != NULL)
	{
		oParameter->setValue(dValue);
		return 0;
	}

	errorCode = 13;
	return -1;
}

DLL_EXPORT int validate(const char *sbml)
{
	string sbmlStr = sbml;

	SBMLReader oReader;
	SBMLDocument *oDoc = oReader.readSBMLFromString(sbmlStr); 
    if (oDoc->getErrorLog()->getNumFailsWithSeverity(LIBSBML_SEV_ERROR) > 0)
	{
		stringstream oStream; 
		oDoc->printErrors (oStream);
		errorCode = 2;
		string str = oStream.str();
        free(extendedErrorMessage);
		extendedErrorMessage = (char *) malloc (sizeof(char)*(str.length() + 1));
		strcpy(extendedErrorMessage, str.c_str());		

		//extendedErrorMessage = (char *) str.c_str();	
		delete oDoc;
		return -1;
	}
	return 0;
}

DLL_EXPORT int getNumErrors()
{
	if (_oSBMLDocCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	return (int)_oSBMLDocCPP->getNumErrors();
}

DLL_EXPORT int getNthError (int index, int *line, int *column, int *errorId, char **errorType, char **errorMsg)
{
	if (_oSBMLDocCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	unsigned int nErrors = _oSBMLDocCPP->getNumErrors();
	if (index >= (int)nErrors) {
		errorCode = 22;
		return -1;
	}

	const SBMLError* error = _oSBMLDocCPP->getError(index);


	string oResult;
	switch (error->getSeverity())
	{
	default:
	case LIBSBML_SEV_INFO:            oResult = "Advisory"; break;
	case LIBSBML_SEV_WARNING:         oResult = "Warning";  break;
	case LIBSBML_SEV_FATAL:           oResult = "Fatal";    break;
	case LIBSBML_SEV_ERROR:           oResult = "Error";    break;
	case LIBSBML_SEV_SCHEMA_ERROR:    oResult = "Error";    break;
	case LIBSBML_SEV_GENERAL_WARNING: oResult = "Warning";  break;
	}
	*line = (int)error->getLine();
	*column = (int)error->getColumn();
	*errorId = (int) error->getErrorId();
	*errorType = (char *) oResult.c_str();
	*errorMsg = (char *) error->getMessage().c_str();
	return 0;
}


DLL_EXPORT int validateSBML(const char *cSBML)
{
	string sSBML(cSBML);
	if (sSBML == "")
	{
		errorCode = 1;
		return -1;
	}

	return validateInternal(sSBML.c_str());
}

DLL_EXPORT int getModelName (char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	*name = (char *) GET_NAME_IF_POSSIBLE(_oModelCPP).c_str();
	return 0;
}


DLL_EXPORT int getModelId (char **Id)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	*Id = (char *) GET_ID_IF_POSSIBLE(_oModelCPP).c_str();
	return 0;
}

DLL_EXPORT int setModelId (char *cId)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	string sId = cId;
	_oModelCPP->setId(sId);
	return 0;
}

DLL_EXPORT int getNumFunctionDefinitions()
{	
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}	
	return (int)_oModelCPP->getNumFunctionDefinitions();
}

DLL_EXPORT int getNumCompartments()
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	return (int)_oModelCPP->getNumCompartments();
}

DLL_EXPORT int getNumReactions()
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	return (int)_oModelCPP->getNumReactions();
}

DLL_EXPORT int getNumFloatingSpecies()
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return 0; 
	}
	int numSpecies = _oModelCPP->getNumSpecies();
	int numBoundarySpecies = getNumBoundarySpeciesInternal();
	int result = numSpecies - numBoundarySpecies;
	return result;
}

DLL_EXPORT int getNumBoundarySpecies()
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return 0;
	}	
	return (int)getNumBoundarySpeciesInternal();
}


DLL_EXPORT int getNumGlobalParameters()
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	return (int)_oModelCPP->getNumParameters();
}


DLL_EXPORT int getNthFunctionDefinition (int index, char** fnId, int *numArgs, char*** argList, char** body)
{
	int n;
	FunctionDefinition* fnDefn;
	string fnStrId;
	char* fnMath;

	fprintf (stderr, "Stage 1\n");

	if(_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(index < 0 || index >= (int)_oModelCPP->getNumFunctionDefinitions())
	{
		errorCode = 3;
		return -1;
	}

	fnDefn		= _oModelCPP->getFunctionDefinition(index);

	fnMath		= SBML_formulaToString ( fnDefn->getBody() );

	*fnId = strdup(fnDefn->getId().c_str());
	*numArgs		= fnDefn->getNumArguments( );

	*argList = (char **) malloc (*numArgs * sizeof(char**));
	for (n = 0; n < *numArgs; n++)
	{
		(*argList)[n] = strdup(fnDefn->getArgument(n)->getName());
	}

	(*body) = (char *) malloc (strlen (fnMath) + 1);
	strcpy ((*body), fnMath);

	//*body = fnMath;

	return 0;
}

DLL_EXPORT int getNthCompartmentName (int nIndex, char **name)
{
	if(_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(nIndex < 0 || nIndex >= (int)_oModelCPP->getNumCompartments())
	{
		errorCode = 4;
		return -1;
	}
	Compartment* oCompartment = _oModelCPP->getCompartment(nIndex);
	*name = (char *) GET_NAME_IF_POSSIBLE(oCompartment).c_str();
	return 0;
}

DLL_EXPORT int getNthCompartmentId (int nIndex, char **Id)
{
	if(_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(nIndex < 0 || nIndex >= (int)_oModelCPP->getNumCompartments())
	{
		errorCode = 4;
		return -1;
	}
	Compartment* oCompartment = _oModelCPP->getCompartment(nIndex);
	*Id = (char *) GET_ID_IF_POSSIBLE(oCompartment).c_str();
	return 0;
}

DLL_EXPORT int getListOfFloatingSpeciesIds (char*** IdList, int *numFloat)
{
	if(_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	int count = 0;
	char *tmp;
	*numFloat = getNumFloatingSpecies ();
	*IdList = (char **) malloc (*numFloat);

	for(unsigned i= 0; i < _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies (i);
		if (!oSpecies->getBoundaryCondition())
		{
			tmp =  (char *) GET_ID_IF_POSSIBLE(oSpecies).c_str();
			(*IdList)[count] = tmp;
			count++;
		}
	}
	return 0;
}

DLL_EXPORT int getNthFloatingSpeciesName (int nIndex, char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	int nCount = 0;
	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies(i);
		if(!oSpecies->getBoundaryCondition())
		{
			if(nCount == nIndex)
			{
				*name = (char *) GET_NAME_IF_POSSIBLE(oSpecies).c_str();
				return 0;
			}
			else
			{
				nCount ++;
			}
		}
	}
	errorCode = 5;
	return -1;
}

DLL_EXPORT int getNthFloatingSpeciesId (int nIndex, char** name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	int nCount = 0;
	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies(i);
		if(!oSpecies->getBoundaryCondition())
		{
			if(nCount == nIndex)
			{
				*name = (char *) GET_ID_IF_POSSIBLE(oSpecies).c_str();
				return 0;
			}
			else
			{
				nCount ++;
			}
		}
	}
	errorCode = 6;
	return -1;
}

DLL_EXPORT int getListOfBoundarySpeciesIds (char*** IdList, int *numBoundary)
{

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	int count = 0;
	*numBoundary = getNumBoundarySpecies();
	*IdList = (char **) malloc (*numBoundary);

	for(unsigned i= 0; i < _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies (i);
		if (oSpecies->getBoundaryCondition())
		{
			(*IdList)[count] =  (char *) GET_ID_IF_POSSIBLE(oSpecies).c_str();
			count++;
		}
	}
	return 0;
}

DLL_EXPORT int getNthBoundarySpeciesName (int nIndex, char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	int nCount = 0;
	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies(i);
		if(oSpecies->getBoundaryCondition())
		{
			if(nCount == nIndex)
			{
				*name = (char *) GET_NAME_IF_POSSIBLE(oSpecies).c_str();
				return 0;
			}
			else
			{
				nCount ++;
			}
		}
	}
	errorCode = 7;
	return -1;
}

DLL_EXPORT int getNthBoundarySpeciesId (int nIndex, char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	int nCount = 0;
	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
	{
		Species *oSpecies = _oModelCPP->getSpecies(i);
		if(oSpecies->getBoundaryCondition())
		{
			if(nCount == nIndex)
			{
				*name = (char *) GET_ID_IF_POSSIBLE(oSpecies).c_str();
				return 0;
			}
			else
			{
				nCount ++;
			}
		}
	}
	errorCode = 8;
	return -1;

}

DLL_EXPORT int getCompartmentIdBySpeciesId (char *cId, char **compId)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	string sId = cId; 

	Species* oSpecies = _oModelCPP->getSpecies(sId);
	if (oSpecies == NULL)
	{
		errorCode = 17;
		return -1;
	}
	*compId = (char *) oSpecies->getCompartment().c_str();
    return 0;
}

DLL_EXPORT int isReactionReversible(int arg, int *isReversible)
{
  if (_oModelCPP == NULL)
  {
	 errorCode = 1;
	 return -1;
  }
  
  if(arg >= (int) _oModelCPP->getNumReactions())
  {
	 errorCode = 10;
	 return -1;
  }
  *isReversible = _oModelCPP->getReaction(arg)->getReversible();
  return 0;
}


DLL_EXPORT int getNthReactionName (int nIndex, char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if (nIndex >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 11;
		return -1;
	}

	Reaction *oReaction = _oModelCPP->getReaction((unsigned int) nIndex);
	if (oReaction == NULL)
	{
		errorCode = 11;
		return -1;
	}
	*name = (char *) GET_NAME_IF_POSSIBLE(oReaction).c_str();
	return 0;
}

DLL_EXPORT int getNthReactionId (int nIndex, char **Id)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(nIndex >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 11;
		return -1;
	}

	Reaction *oReaction = _oModelCPP->getReaction((unsigned int) nIndex);
	if (oReaction == NULL)
	{
		errorCode = 11;
		return -1;
	}
	*Id = (char *) GET_ID_IF_POSSIBLE(oReaction).c_str();
	return 0;
}

DLL_EXPORT int getNumReactants (int arg)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	return (int)_oModelCPP->getReaction(arg)->getNumReactants();;
}

DLL_EXPORT int getNumProducts (int arg)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	return (int)_oModelCPP->getReaction(arg)->getNumProducts();;
}

DLL_EXPORT int getNthReactantName (int arg1, int arg2, char **name)
{
	ListOfSpeciesReferences* reactantsList;
	SpeciesReference* reactant;
	Reaction* r;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(arg1 >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 10;
		return -1;
	}

	r = _oModelCPP->getReaction(arg1);
	reactantsList = r->getListOfReactants();

	if(arg2 >= (int)reactantsList->size())
	{
		errorCode = 9;
		return -1;
	}

	reactant = r->getReactant(arg2);
	if (reactant == NULL) {
		errorCode = 9;
		return -1;
	}
	*name = (char *) reactant->getSpecies().c_str();
	return 0;
}

DLL_EXPORT int getNthProductName (int arg1, int arg2, char **name)
{
	SpeciesReference* product;
	Reaction* r;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if (arg1 >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 10;
		return -1;
	}

	r = _oModelCPP->getReaction(arg1);

	if (arg2 >= (int)(r->getNumProducts()))
	{
		errorCode = 11;
		return -1;
	}

	product = r->getProduct(arg2);
	if (product == NULL) {
		errorCode = 11;
		return -1;
	}
	*name = (char *) product->getSpecies().c_str();
	return 0;
}

DLL_EXPORT int getKineticLaw (int index, char **kineticLaw)
{
	Reaction* r;
	KineticLaw* kl;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(index >= (int)_oModelCPP->getNumReactions() || index < 0)
	{
		errorCode = 10;
		return -1;
	}

	r = _oModelCPP->getReaction(index);
	kl = r->getKineticLaw();
	if (kl == NULL)
	{
		*kineticLaw = (char *) zero;
	}
	else
	{
		*kineticLaw =  (char *) kl->getFormula().c_str();	
	}
	return 0;
}

DLL_EXPORT double getNthReactantStoichiometry (int arg1, int arg2)
{
	double result;
	ListOf* reactantsList;
	SpeciesReference* reactant;
	Reaction* r;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(arg1 >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 10;
		return -1;
	}

	r = _oModelCPP->getReaction(arg1);
	reactantsList = r->getListOfReactants();

	if(arg2 >= (int)ListOf_size(reactantsList))
	{
		errorCode = 9;
		return -1;
	}

	reactant = (SpeciesReference* ) reactantsList->get(arg2);
	result =  (double)reactant->getStoichiometry();
	return result;
}

DLL_EXPORT double getNthProductStoichiometry (int arg1, int arg2)
{
	double result;
	ListOf* productsList;
	SpeciesReference* product;
	Reaction* r;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if (arg1 >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 10;
		return -1;
	}

	r = _oModelCPP->getReaction(arg1);
	productsList =  r->getListOfProducts();

	if(arg2 >= (int)productsList->size())
	{
		errorCode = 11;
	}

	product = (SpeciesReference* ) productsList->get(arg2);
	result =  (double)product->getStoichiometry();
	return result;
}

void changeTimeSymbol (ASTNode *node, const char* time) 
{ 
	unsigned int c;
	if ( node->getType() == AST_NAME_TIME && strcmp(node->getName(), time) != 0) 
		node->setName(time);
	for (c = 0; c < node->getNumChildren() ; c++) 
		changeTimeSymbol( node->getChild (c), time); 
} 


void changeTimeSymbolModel (Model* oModel, const char* sNewSymbol)
{
	for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
	{
		Reaction* oReaction = oModel->getReaction(i);
		KineticLaw* oKinetics = oReaction->getKineticLaw();
		if (oKinetics != NULL && oKinetics->isSetMath())
		{
			ASTNode* node = const_cast<ASTNode*>(oKinetics->getMath());
			changeTimeSymbol(node, sNewSymbol) ;
			//oKinetics->setMath( );			
		}
	}
	for (unsigned int i = 0; i < oModel->getNumFunctionDefinitions(); i++)
	{
		FunctionDefinition* oDefn = oModel->getFunctionDefinition(i);
		if (oDefn->isSetMath())
		{
			ASTNode* node = const_cast<ASTNode*>(oDefn->getMath());
			changeTimeSymbol(node, sNewSymbol);
			//oDefn->setMath();

		}
	}
	for (unsigned int i = 0; i < oModel->getNumConstraints(); i++)
	{
		Constraint* oConstraint = oModel->getConstraint(i);
		if (oConstraint->isSetMath())
		{
			ASTNode* node = const_cast<ASTNode*>(oConstraint->getMath());
			changeTimeSymbol(node, sNewSymbol);
			//oConstraint->setMath(changeTimeSymbol(node, sNewSymbol));			
		}
	}
	for (unsigned int i = 0; i < oModel->getNumInitialAssignments(); i++)
	{
		InitialAssignment* oAssignment = oModel->getInitialAssignment(i);
		if (oAssignment->isSetMath())
		{
			ASTNode* node = const_cast<ASTNode*>(oAssignment->getMath());
			changeTimeSymbol(node, sNewSymbol);
			//oAssignment->setMath(changeTimeSymbol(node, sNewSymbol));
		}
	}
	for (unsigned int i = 0; i < oModel->getNumRules(); i++)
	{
		Rule* oRule = oModel->getRule(i);
		if (oRule->isSetMath())
		{
			ASTNode* node = const_cast<ASTNode*>(oRule->getMath());
			changeTimeSymbol(node, sNewSymbol);
			//oRule->setMath(changeTimeSymbol(node, sNewSymbol));
		}
	}
	for (unsigned int i = 0; i < oModel->getNumEvents(); i++)
	{
		Event* oEvent = oModel->getEvent(i);
		if (oEvent->isSetTrigger())
		{
			Trigger* oTrigger = const_cast<Trigger*>( oEvent->getTrigger());
			if (oTrigger->isSetMath())
			{
				ASTNode* node = const_cast<ASTNode*>(oTrigger->getMath());
				changeTimeSymbol(node, sNewSymbol);
				//oTrigger->setMath();
				//oEvent->setTrigger(oTrigger);
			}
		}
		for (unsigned int j = 0; j < oEvent->getNumEventAssignments(); j++)
		{
			EventAssignment* oAssignment = oEvent->getEventAssignment(j);
			if (oAssignment != NULL && oAssignment->isSetMath())
			{
				ASTNode* node = const_cast<ASTNode*>(oAssignment->getMath());
				changeTimeSymbol(node, sNewSymbol);			
			}
		}
	}
}

void modifyKineticLaws(SBMLDocument * /*oDoc*/, Model * oModelCPP)
{
	char*				spApndParamName;
	int					memorySize;
	char*				newKineticFormula;
	char*				subStr;
	int					startIndex;
	int					index;
	int					isParameter;
	char*				newParamName;
	int					tokenStartIndex;
	int					numOfLocalParameters;
	int					 j;
	ListOfParameters*	localParametersList;
	Parameter*		parameter;

	unsigned int numOfReactions = oModelCPP->getNumReactions();
	for(unsigned int i=0; i<numOfReactions; i++)
	{
		Reaction *oReaction = oModelCPP->getReaction(i);
		string sId = GET_ID_IF_POSSIBLE(oReaction);
		KineticLaw *oLaw = oReaction->getKineticLaw();
		if (oLaw == NULL) 
		{
			numOfLocalParameters = 0;
			continue;
		}
		else
		{
			numOfLocalParameters = oLaw->getNumParameters();		
		}
		std::string kineticFormula = oLaw->getFormula();

		localParametersList = oLaw->getListOfParameters();

		memorySize = (int)kineticFormula.length() + (int)sId.length()*numOfLocalParameters + 1;
		newKineticFormula = (char* ) malloc(sizeof(char)*memorySize);
		memset(newKineticFormula, 0, sizeof(char)*memorySize);

		startIndex=0; 
		while (startIndex < (int)kineticFormula.length())
		{
			index = startIndex;

			tokenStartIndex = index;

			while(index < (int)kineticFormula.length())
			{
				if (kineticFormula[index] == '+' || 
					kineticFormula[index] == '-' || 
					kineticFormula[index] == '(' || 
					kineticFormula[index] == ')' || 
					kineticFormula[index] == '*' || 
					kineticFormula[index] == '/' ||
					kineticFormula[index] == '^' ||
					kineticFormula[index] == ' ' ||
					kineticFormula[index] == '.' ||
					kineticFormula[index] == ',' )
					++index;
				else
					break;
			}

			if(index > tokenStartIndex)
			{
				subStr		= strCopySubstr(kineticFormula.c_str(), tokenStartIndex, index-tokenStartIndex);
				if((int)strlen(newKineticFormula)+(int)strlen(subStr)+1 > memorySize)
				{
					memorySize = memorySize*2;
					newKineticFormula = (char* ) realloc(newKineticFormula, sizeof(char)*memorySize);
				}
				strcat(newKineticFormula, subStr);
				free(subStr);
			}
			startIndex = index;

			while(index<(int)kineticFormula.length())
			{
				if (kineticFormula[index] == '+' || 
					kineticFormula[index] == '-' || 
					kineticFormula[index] == '(' || 
					kineticFormula[index] == ')' || 
					kineticFormula[index] == '*' || 
					kineticFormula[index] == '/' ||
					kineticFormula[index] == '^' ||
					kineticFormula[index] == ' ' ||
					kineticFormula[index] == '.' ||
					kineticFormula[index] == ',' )
					break;
				else
					++index;
			}
			subStr		= strCopySubstr(kineticFormula.c_str(), startIndex, index-startIndex);

			isParameter = 0;
			for(j=0; j<numOfLocalParameters; j++)
			{
				parameter		=  (Parameter*)localParametersList->get(j);
				string paramName=  GET_ID_IF_POSSIBLE(parameter);
				spApndParamName = (char* ) malloc(sizeof(char)*(paramName.length() +2));
				memset(spApndParamName, 0, sizeof(char)*(paramName.length()+2)); 
				strcpy(spApndParamName, paramName.c_str());
				strcat(spApndParamName, " ");
				if((paramName ==  subStr) || strcmp(spApndParamName, subStr) == 0)
				{
					isParameter = 1;
					break;
				}
			}

			if(isParameter == 1)
			{
				newParamName = (char* ) malloc(sizeof(char)*(sId.length() + strlen(subStr) + 1) );
				memset(newParamName, 0, sizeof(char)*(sId.length() + strlen(subStr) + 1));
				strcpy(newParamName, sId.c_str());
				strcat(newParamName, subStr);
				if((int)strlen(newKineticFormula)+(int)strlen(newParamName)+1 > memorySize)
				{
					memorySize = memorySize*2;
					newKineticFormula = (char* ) realloc(newKineticFormula, sizeof(char)*memorySize);
				}
				strcat(newKineticFormula, newParamName);
				free(newParamName);
			}
			else
			{
				if((int)strlen(newKineticFormula)+(int)strlen(subStr)+1 > memorySize)
				{
					memorySize = memorySize*2;
					newKineticFormula = (char* ) realloc(newKineticFormula, sizeof(char)*memorySize);
				}
				strcat(newKineticFormula, subStr);
			}
			free(subStr);

			tokenStartIndex = index;

			while(index < (int)kineticFormula.length())
			{
				if (kineticFormula[index] == '+' || 
					kineticFormula[index] == '-' || 
					kineticFormula[index] == '(' || 
					kineticFormula[index] == ')' || 
					kineticFormula[index] == '*' || 
					kineticFormula[index] == '/' ||
					kineticFormula[index] == '^' ||
					kineticFormula[index] == ' ' ||
					kineticFormula[index] == '.' ||
					kineticFormula[index] == ',' )
					++index;
				else
					break;
			}

			if(index > tokenStartIndex)
			{
				subStr		= strCopySubstr(kineticFormula.c_str(), tokenStartIndex, index-tokenStartIndex);
				if((int)strlen(newKineticFormula)+(int)strlen(subStr)+1 > memorySize)
				{
					memorySize = memorySize*2;
					newKineticFormula = (char* ) realloc(newKineticFormula, sizeof(char)*memorySize);
				}
				strcat(newKineticFormula, subStr);
				free(subStr);
			}
			startIndex = index;
		}
		oLaw->setFormula(newKineticFormula);		
		free(newKineticFormula);
	}
}

void promoteLocalParamToGlobal(SBMLDocument *oDoc, Model *oModelCPP)
{
	std::string			reactionStr;
	int					numOfLocalParameters;
	int					numOfReactions;
	int					i, j;
	std::string			oldParamStr;
	ListOfParameters*			localParametersList;
	Parameter*		parameter;
	Reaction*			r;
	KineticLaw*		kl;

	numOfReactions = oModelCPP->getNumReactions();
	for(i=0; i<numOfReactions; i++)
	{
		r = oModelCPP->getReaction(i);
		reactionStr = GET_ID_IF_POSSIBLE(r);
		kl = r->getKineticLaw();
		if(kl == NULL)
		{
			numOfLocalParameters = 0;
		}
		else
		{
			numOfLocalParameters = kl->getNumParameters();
		}

		localParametersList = kl->getListOfParameters();

		for(j=numOfLocalParameters-1; j>=0; j--)
		{
			parameter	=  (Parameter*) localParametersList->remove(j);
			if (!parameter) continue;
			oldParamStr = GET_ID_IF_POSSIBLE(parameter);
			string newParamStr = reactionStr + oldParamStr;

			if (oDoc->getLevel() == 1)
				parameter->setName(newParamStr);
			parameter->setId(newParamStr);				

			oModelCPP->addParameter(parameter);
		}
	}
}


DLL_EXPORT int getParamPromotedSBML (const char *inSBML, char **outSBML)
{
	SBMLDocument *oSBMLDoc = NULL;
	Model *oModel = NULL;

	oSBMLDoc = readSBMLFromString(inSBML);
	if (oSBMLDoc->getLevel() == 1)
		oSBMLDoc->setLevelAndVersion( 2, 1, false);
	oModel = oSBMLDoc->getModel();

	if (oModel == NULL)
	{	
		errorCode = 2;
		return -1;
	}
	else
	{
		modifyKineticLaws(oSBMLDoc, oModel);

		promoteLocalParamToGlobal(oSBMLDoc, oModel);

		changeTimeSymbolModel(oModel, "time");

		char *resultSBML = writeSBMLToString(oSBMLDoc);

		delete oSBMLDoc;
		oModel = NULL;
		oSBMLDoc = NULL;
		*outSBML = resultSBML;
		return 0;
	}
}

DLL_EXPORT int getNumLocalParameters (int reactionIndex)
{
	int result;
	Reaction* r;
	KineticLaw* kl;

	if (_oModelCPP == NULL)
	{
		errorCode = 2;
		return -1;
	}

	if(reactionIndex >= (int)_oModelCPP->getNumReactions() || reactionIndex < 0)
	{
		errorCode = 15;
		return -1;
	}

	r = _oModelCPP->getReaction(reactionIndex);
	kl = r->getKineticLaw();

	if(kl == NULL)
	{
		result = 0;
	}
	else
	{
		result = kl->getNumParameters();
	}
	return result;
}

DLL_EXPORT int getNthLocalParameterName (int reactionIndex, int parameterIndex, char **sId)
{
	Reaction* r;
	KineticLaw* kl;
	ListOf* parametersList;
	Parameter* parameter;

	if (_oModelCPP == NULL)
	{
		errorCode = 2;
		return -1;
	}

	if (reactionIndex >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 15;
		return -1;
	}

	r = _oModelCPP->getReaction(reactionIndex);
	kl = r->getKineticLaw();
	parametersList = kl->getListOfParameters();

	if(parameterIndex >= (int)parametersList->size())
	{
		errorCode = 17;
		return -1;
	}

	parameter = (Parameter *) parametersList->get(parameterIndex);	
	*sId = (char *) parameter->getName().c_str();
	return 0;
}

DLL_EXPORT int getNthLocalParameterId (int reactionIndex, int parameterIndex, char **sId)
{
	string result;	
	Reaction* r;
	KineticLaw* kl;
	ListOf* parametersList;
	Parameter* parameter;

	if (_oModelCPP == NULL)
	{
		errorCode = 2;
		return -1;
	}

	if (reactionIndex >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 15;
		return -1;
	}

	r = _oModelCPP->getReaction(reactionIndex);
	kl = r->getKineticLaw();
	parametersList = kl->getListOfParameters();

	if (parameterIndex >= (int)parametersList->size())
	{
		errorCode = 17;
		return -1;
	}

	parameter = (Parameter *) parametersList->get(parameterIndex);

	*sId = (char *) parameter->getId().c_str();
	return 0;
}


DLL_EXPORT int getNthLocalParameterValue (int reactionIndex, int parameterIndex, double *value)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 2;
		return -1;
	}

	if(reactionIndex < 0 || reactionIndex >= (int)_oModelCPP->getNumReactions())
	{
		errorCode = 15;
		return -1;
	}

	Reaction *oReaction = _oModelCPP->getReaction(reactionIndex);
	ListOfParameters *oList = oReaction->getKineticLaw()->getListOfParameters();

	if(parameterIndex < 0 || parameterIndex >= (int)oList->size())
	{
		errorCode = 17;
		return -1;
	}

	*value = ((Parameter*)oList->get(parameterIndex))->getValue();
	return 0;
}

DLL_EXPORT int getNthGlobalParameterName (int nIndex, char **name)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(nIndex >= (int)_oModelCPP->getNumParameters())
	{
		errorCode = 12;
		return -1;
	}

	Parameter *oParameter = _oModelCPP->getParameter((unsigned int) nIndex);
	if (oParameter == NULL)
	{
		errorCode = 12;
		return -1;
	}
	*name = (char *) GET_NAME_IF_POSSIBLE(oParameter).c_str();
	return 0;
}

DLL_EXPORT int getNthGlobalParameterId (int nIndex, char **Id)
{
	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	if(nIndex >= (int)_oModelCPP->getNumParameters())
	{
		errorCode = 12;
		return -1;
	}

	Parameter *oParameter = _oModelCPP->getParameter((unsigned int) nIndex);
	if (oParameter == NULL)
	{
		errorCode = 12;
		return -1;
	}
	*Id = (char *) GET_ID_IF_POSSIBLE(oParameter).c_str();
	return 0;
}

DLL_EXPORT int getNumRules()
{
	int		result;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}
	result	= _oModelCPP->getNumRules();

	return result;
}

DLL_EXPORT int getNthRule (int nIndex, char **rule, int *ruleType)
{

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	Rule *oRule = _oModelCPP->getRule(nIndex);
	if (oRule == NULL)
	{
		errorCode = 19;
	}

	SBMLTypeCode_t type = (SBMLTypeCode_t) oRule->getTypeCode();
	*ruleType =  type;

	switch(type)
	{
	case SBML_PARAMETER_RULE:
	case SBML_SPECIES_CONCENTRATION_RULE:
	case SBML_COMPARTMENT_VOLUME_RULE:
	case SBML_ASSIGNMENT_RULE:
	case SBML_RATE_RULE:
		{
			string lValue = oRule->getVariable();
			string rValue = oRule->getFormula();

			string str = lValue + " = " + rValue;
			*rule = (char *) malloc (str.length() + 1);
			strcpy(*rule, str.c_str());		
			return 0;
			break;
		}
	case SBML_ALGEBRAIC_RULE: 
		{
			string rValue = oRule->getFormula();
			
			string str = rValue + " = 0";
			*rule = (char *) malloc (str.length() + 1);
			strcpy(*rule, str.c_str());		
			return 0;
			break;
		}


	default:
		break;
	}

	
	*rule = NULL;
	return 0;
}


DLL_EXPORT int getNumEvents()
{
	int		result;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return 0;
	}
	result	= _oModelCPP->getNumEvents();

	return result;
}


DLL_EXPORT int getNthEvent (int arg, char **trigger, char **delay, char **lValue, char **rValue)
{
	Event*			    event;
	int					numEventAssignments;

	if (_oModelCPP == NULL)
	{
		errorCode = 1;
		return -1;
	}

	event = _oModelCPP->getEvent (arg);
	if(event == NULL)
	{
		errorCode = 20;
		return -1;
	}

	*trigger = SBML_formulaToString (event->getTrigger()->getMath());

	bool bFreeDelay = false;
	if (!event->isSetDelay ())
	{
		*delay = const_cast<char*>(zero);
	}
	else
	{
		const Delay * oDelay = event->getDelay();
		if (oDelay->isSetMath())
		{
			*delay = SBML_formulaToString (oDelay->getMath());
		}
		else
		{
			*delay = const_cast<char*>(zero);
		}
	}

	numEventAssignments = event->getNumEventAssignments();

	//for (i=0; i<numEventAssignments; i++)
	//{
	//	ea = event->getEventAssignment (i);
	//	*lValue = ea->getVariable();
	//	*rValue	= SBML_formulaToString (ea->getMath());
	//}

	return 0;
}

DLL_EXPORT int convertMathMLToString (const char *mathMLStr, char **infix)
{
	char*				result;
	const ASTNode_t*	ast_Node;
	StringBuffer_t		*sb;
	
	/**
	* Prepend an XML header if not already present.
	*/
	if (mathMLStr[0] == '<' && mathMLStr[1] != '?')
	{
		sb = StringBuffer_create(1024);

		StringBuffer_append(sb, "<?xml version='1.0' encoding='ascii'?>\n");
		StringBuffer_append(sb, mathMLStr);

		mathMLStr = StringBuffer_getBuffer(sb);

		free(sb);
	}

	ast_Node	= readMathMLFromString (mathMLStr);
	if (ast_Node == NULL)
	{
		errorCode = 23;
		return -1;
	}
	result = SBML_formulaToString (ast_Node);

	if (result == NULL)
	{
		errorCode = 23;
		return -1;
	}
	*infix = result;
    return 0;
}

DLL_EXPORT int convertStringToMathML (const char* infixStr, char **mathMLStr)
{
	char*			result;
	ASTNode_t		*mtree_root;

	mtree_root	= (ASTNode_t *) SBML_parseFormula (infixStr);
	if (mtree_root == NULL) {
		errorCode = 24;
		return -1;
	}

	changeTimeSymbol(mtree_root, "time");
	result		= writeMathMLToString (mtree_root);

	*mathMLStr = result;
	return 0;
}

DLL_EXPORT int addMissingModifiers (const char *inSBML, char **outSBML)
{
	string sModel = inSBML;

	try
	{
		*outSBML = addMissingModifiersInternal(sModel);
		return 0;
	}
	catch(...)
	{
		errorCode = 15;
		return -1;
	}

}

DLL_EXPORT int reorderRules (char **sbml)
{
	try
	{
		SBMLDocument *doc = readSBMLFromString(*sbml);
		ConversionProperties props;
		props.addOption("sortRules", true);

		SBMLRuleConverter converter;
		converter.setDocument(doc);
		converter.setProperties(&props);
		int ret = converter.convert();
		char * string = doc->toSBML();
		*sbml = string; 
		delete doc;
		return ret;
	}
	catch (...)
	{
		errorCode = 25;
		return -1;
	}
}

void changePow (ASTNode* node)
{
	unsigned int c;

	if (ASTNode_getType(node) == AST_FUNCTION_POWER)
	{
		ASTNode_setType(node, AST_POWER);
	}

	for (c = 0; c < ASTNode_getNumChildren(node); c++)
	{
		changePow( ASTNode_getChild(node, c) );
	}
}

} // extern "C"
