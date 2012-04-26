///** 
//* Filename    : NOMLib.cpp
//* Description : NOM version of libSBML
//* Original by :
//* Author(s)   : Frank Bergmann <fbergman@kgi.edu> 
//* Author(s)   : Sri Paladugu <spaladug@kgi.edu> 
//* Author(s)   : Herbert Sauro <Herbert_Sauro@kgi.edu> 
//* This version: Herbert Sauro <hsauro@u.washington.edu> 
//*               Stanley Gu <stanleygu@gmail.com>
//*
//* Organization: University of Washington
//*
//* Copyright 2007-2011 SBW Team http://sys-bio.org/
//* 
//* 
//* Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
//* and associated documentation files (the "Software"), to deal in the Software without restriction, 
//* including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//* and/or sell copies of the Software, and to permit persons to whom the Software is furnished to 
//* do so, subject to the following conditions:
//* 
//* The above copyright notice and this permission notice shall be included in all copies or 
//* substantial portions of the Software.
//* 
//* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
//* NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
//* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
//* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*/

// November 16th 2011, Removed dependence on SBW


//#define WIN32_LEAN_AND_MEAN
#include "NOM.h"

//#include "sbml/SBMLReader.h"
//#include "sbml/SBMLWriter.h"
//#include <sbml/SBMLError.h>
//#include "sbml/FunctionDefinition.h"
//
//#include <sbml/math/FormulaFormatter.h>
//#include <sbml/math/FormulaParser.h>
//#include <sbml/math/MathML.h>
//#include <sbml/math/ASTNode.h>
//
//#include "sbml/util/util.h"
//#include "sbml/util/StringBuffer.h"
//
//
//#include <sbml/xml/XMLInputStream.h>
//#include <sbml/xml/XMLErrorLog.h> 
//
//#include <string>
//#include <sstream>
//
#include "sbml/conversion/ConversionProperties.h"
#include "sbml/conversion/SBMLRuleConverter.h"
#include "sbml/conversion/SBMLFunctionDefinitionConverter.h"


SBMLDocument* _oSBMLDocCPP = NULL;
Model* _oModelCPP = NULL;

int errorCode = 0;
char *extendedErrorMessage;

static char *errorMessages[] = {
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


extern "C" {

// -------------------------------------------------------------------------------------------
// Support functions used by the library
// -------------------------------------------------------------------------------------------

// Concatentate two strings together and return the new string
char *strconcat(char *s1, char *s2)
{
	size_t old_size;
	char *t;

	old_size = strlen(s1);

	/* cannot use realloc() on initial const char* */
	t = (char *) malloc(old_size + strlen(s2) + 1);
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
	SBMLDocument *oDoc = oReader.readSBMLFromString(sbml); //readSBMLFromString(sbml.c_str());
	if (oDoc->getNumErrors() > 0)
	{
		stringstream oStream; 
		oDoc->printErrors (oStream);
		errorCode = 2;
		extendedErrorMessage = (char *) oStream.str().c_str();		
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

DLL_EXPORT int convertSBML(char *inputModel, char **outputModel, int nLevel, int nVersion)
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
	  char ** outputModel;
	  char * inputModel;
	  strcpy(inputModel,sModel.c_str());
	  convertSBML(inputModel,outputModel, 2, 1);
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
DLL_EXPORT int loadSBML(char* sbmlStr)
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
			char *c;
			c = new char [sSBML.length() + 1];
			strcpy (c, sSBML.c_str());
			loadSBML(c);
			return 0;
		}
		ConversionProperties props;
		props.addOption("sortRules", true, "sort rules");
		_oSBMLDocCPP->convert(props);
		return validateInternal(arg);
	}

}


// Call this is if any method returns -1, it will return the error message string
DLL_EXPORT char *getError () 
{
	if (extendedErrorMessage != NULL) {
	   char *t = strconcat (errorMessages[errorCode], extendedErrorMessage);
	   extendedErrorMessage = NULL;  // Memory leak?
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


//XMLNode *SBMLSupportModule::convertStringToXMLNode(std::string sString)
//{
//	XMLInputStream stream(sString.c_str(), false);
//	XMLErrorLog log;
//	stream.setErrorLog(&log); 
//	XMLNode *oNode = new XMLNode(stream);
//	return oNode;
//}
//
//DataBlockWriter  SBMLSupportModule::getAnnotationImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getAnnotationImpl");
//	}
//	string sId; arguments >> sId;
//	string sResult;
//
//	if (_oModelCPP->getId() == sId || _oModelCPP->getName() == sId)
//	{
//		if (_oModelCPP->isSetAnnotation())
//			sResult = writeXMLNode(_oModelCPP->getAnnotation());
//		return DataBlockWriter() << sResult;
//
//	}
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		if (oSpecies->isSetAnnotation())
//			sResult = writeXMLNode(oSpecies->getAnnotation());
//		return DataBlockWriter() << sResult;
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		if (oParameter->isSetAnnotation())
//			sResult = writeXMLNode(oParameter->getAnnotation());
//		return DataBlockWriter() << sResult;
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		if (oCompartment->isSetAnnotation())
//			sResult = writeXMLNode(oCompartment->getAnnotation());
//		return DataBlockWriter() << sResult;
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		if (oReaction->isSetAnnotation())
//			sResult = writeXMLNode(oReaction->getAnnotation());
//		return DataBlockWriter() << sResult;
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		if (oRule->isSetAnnotation())
//			sResult = writeXMLNode(oRule->getAnnotation());
//		return DataBlockWriter() << sResult;
//	}	
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method getAnnotationImpl.");
//
//}
//
//DataBlockWriter  SBMLSupportModule::setAnnotationImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method setAnnotationImpl");
//	}
//
//	string sId; arguments >> sId; string sXML; arguments >> sXML;
//	XMLNode *oNode = convertStringToXMLNode(sXML);
//	string sResult;
//
//	if (_oModelCPP->getId() == sId || _oModelCPP->getName() == sId)
//	{
//		_oModelCPP->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter();
//	}
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		oSpecies->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		oParameter->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		oCompartment->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		oReaction->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		oRule->setAnnotation(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}	
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method setAnnotationImpl.");
//
//}
//
//DataBlockWriter  SBMLSupportModule::setNotesImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method setNotesImpl");
//	}
//
//	string sId; arguments >> sId; string sXML; arguments >> sXML;
//	XMLNode *oNode = convertStringToXMLNode(sXML);
//	string sResult;
//
//	if (_oModelCPP->getId() == sId || _oModelCPP->getName() == sId)
//	{
//		_oModelCPP->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter();
//
//	}
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		oSpecies->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		oParameter->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		oCompartment->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		oReaction->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		oRule->setNotes(oNode);
//		delete oNode;
//		return DataBlockWriter() << sResult;
//	}	
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method setNotesImpl.");
//
//
//}
//
//DataBlockWriter  SBMLSupportModule::getNotesImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNotesImpl");
//	}
//
//	string sId; arguments >> sId;
//	string sResult;
//
//	if (_oModelCPP->getId() == sId || _oModelCPP->getName() == sId)
//	{
//		if (_oModelCPP->isSetNotes())
//			sResult = writeXMLNode(_oModelCPP->getNotes());
//		return DataBlockWriter() << sResult;
//
//	}
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		if (oSpecies->isSetNotes())
//			sResult = writeXMLNode(oSpecies->getNotes());
//		return DataBlockWriter() << sResult;
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		if (oParameter->isSetNotes())
//			sResult = writeXMLNode(oParameter->getNotes());
//		return DataBlockWriter() << sResult;
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		if (oCompartment->isSetNotes())
//			sResult = writeXMLNode(oCompartment->getNotes());
//		return DataBlockWriter() << sResult;
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		if (oReaction->isSetNotes())
//			sResult = writeXMLNode(oReaction->getNotes());
//		return DataBlockWriter() << sResult;
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		if (oRule->isSetNotes())
//			sResult = writeXMLNode(oRule->getNotes());
//		return DataBlockWriter() << sResult;
//	}	
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method getNotesImpl.");
//
//}
//
//std::string SBMLSupportModule::writeXMLNode(XMLNode *node)
//{
//	string sResult;
//	if (node != NULL)
//	{
//		ostringstream   os; 
//		XMLOutputStream stream(os, "UTF-8", false); 
//		node->write(stream);
//		sResult = os.str();
//	}
//	return sResult;
//}
//
//DataBlockWriter SBMLSupportModule::getSBOTermImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getValueImpl");
//	}
//
//	string sId; arguments >> sId;
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		return DataBlockWriter() << oSpecies->getSBOTerm ();
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		return DataBlockWriter() << oParameter->getSBOTerm ();
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		return DataBlockWriter() << oCompartment->getSBOTerm ();
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		return DataBlockWriter() << oReaction->getSBOTerm ();
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		return DataBlockWriter() << oRule->getSBOTerm ();
//	}	
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method getSBOTermImpl.");
//}
//
//DataBlockWriter SBMLSupportModule::setSBOTermImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getValueImpl");
//	}
//
//	string sId; int nSBOTerm; arguments >> sId >> nSBOTerm;
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		oSpecies->setSBOTerm  (nSBOTerm);
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		oParameter->setSBOTerm  (nSBOTerm);
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		oCompartment->setSBOTerm  (nSBOTerm);
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		oReaction->setSBOTerm  (nSBOTerm);
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		oRule->setSBOTerm  (nSBOTerm);
//	}
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method setSBOTermImpl.");
//}
//
//DataBlockWriter SBMLSupportModule::hasSBOTermImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getValueImpl");
//	}
//
//	string sId; arguments >> sId;
//
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies!= NULL)
//	{
//		return DataBlockWriter() << (bool)oSpecies->isSetSBOTerm  ();
//	}
//
//	Parameter* oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		return DataBlockWriter() << (bool)oParameter->isSetSBOTerm  ();
//	}
//
//	Compartment* oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment!=NULL)
//	{
//		return DataBlockWriter() << (bool)oCompartment->isSetSBOTerm  ();
//	}
//
//	Reaction* oReaction = _oModelCPP->getReaction(sId);
//	if (oReaction != NULL)
//	{
//		return DataBlockWriter() << (bool)oReaction->isSetSBOTerm  ();
//	}
//
//	Rule* oRule = _oModelCPP->getRule(sId);
//	if (oRule!=NULL)
//	{
//		return DataBlockWriter() << (bool)oRule->isSetSBOTerm  ();
//	}
//
//	throw new SBWApplicationException("Invalid id. No element with the given id exists in the model.", "Exception raised in method hasSBOTermImpl.");
//}


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
		else
			*value = oSpecies->getInitialConcentration();
		return 0;
	}

	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
	if (oCompartment != NULL)
	{
		*value = oCompartment->getVolume();
		return 0;
	}

	Parameter *oParameter = _oModelCPP->getParameter(sId);
	if (oParameter != NULL)
	{
		*value = oParameter->getValue();
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

//DataBlockWriter SBMLSupportModule::existsImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method existsImpl");				
//	}
//
//	string sId; arguments >> sId;
//	Species *oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies != NULL)
//	{
//		return DataBlockWriter() << true;
//	}
//
//	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment != NULL)
//	{
//		return DataBlockWriter() << true;	
//	}
//
//	Parameter *oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		return DataBlockWriter() << true;
//	}
//
//	return DataBlockWriter() << false;
//
//}


DLL_EXPORT int validate(char *sbml)
{
	string sbmlStr = sbml;

	SBMLReader oReader;
	SBMLDocument *oDoc = oReader.readSBMLFromString(sbmlStr); 
	if (oDoc->getNumErrors() > 0)
	{
		stringstream oStream; 
		oDoc->printErrors (oStream);
		errorCode = 2;
		string str = oStream.str();
		extendedErrorMessage = (char *) malloc (str.length() + 1);
		strcpy(extendedErrorMessage, str.c_str());		

		//extendedErrorMessage = (char *) str.c_str();	
		delete oDoc;
		return -1;
	}
	return 0;
}

//DataBlockWriter SBMLSupportModule::validateWithConsistencyImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	string sSBML; arguments >> sSBML;
//	if (sSBML == "")
//	{
//		throw new SBWApplicationException("The input string cannot be blank", "Exception raised in method validateSBMLImpl");
//	}
//
//	SBMLDocument *oDoc = readSBMLFromString(sSBML.c_str());
//	if (oDoc->getNumErrors() + oDoc->checkConsistency()> 0)
//	{
//
//		stringstream oStream; 
//		oDoc->printErrors (oStream);		
//		delete oDoc;
//		throw new SBWApplicationException("Validation failed", oStream.str());		
//	}
//
//	delete oDoc;
//
//	return DataBlockWriter() << "Validation Successfull"; 
//}
//
//DataBlockWriter SBMLSupportModule::checkConsistencyImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oSBMLDocCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method checkConsistencyImpl");
//	}
//	return DataBlockWriter() << (int)_oSBMLDocCPP->checkConsistency();
//
//}

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


//DataBlockWriter SBMLSupportModule::getListOfErrorsImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oSBMLDocCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getListOfErrorsImpl");
//	}
//	unsigned int nErrors = _oSBMLDocCPP->getNumErrors();
//
//	DataBlockWriter oErrorList;
//	for (unsigned int i = 0; i < nErrors; i++)
//	{
//		const SBMLError* error = _oSBMLDocCPP->getError(i);
//
//
//		DataBlockWriter oResult;
//		switch (error->getSeverity())
//		{
//		default:
//		case LIBSBML_SEV_INFO:            oResult << "Advisory"; break;
//		case LIBSBML_SEV_WARNING:         oResult << "Warning";  break;
//		case LIBSBML_SEV_FATAL:           oResult << "Fatal";    break;
//		case LIBSBML_SEV_ERROR:           oResult << "Error";    break;
//		case LIBSBML_SEV_SCHEMA_ERROR:    oResult << "Error";    break;
//		case LIBSBML_SEV_GENERAL_WARNING: oResult << "Warning";  break;
//		}
//		oResult << (int)error->getLine() << (int)error->getColumn() << (int) error->getErrorId() << error->getMessage();
//		oErrorList << oResult;
//
//	}
//	return DataBlockWriter() << oErrorList;
//
//}

DLL_EXPORT int validateSBML(char *cSBML)
{
	string sSBML = cSBML;
	if (sSBML == "")
	{
		errorCode = 1;
		return -1;
	}

	return validateInternal(sSBML.c_str());
}

//DataBlockWriter SBMLSupportModule::getSBMLImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getSBMLImpl");
//	}
//	SBMLWriter oWriter;	
//	DataBlockWriter result;
//	char * sSBML = oWriter.writeToString(_oSBMLDocCPP);
//	result << sSBML;
//	free (sSBML);
//	return result;
//}

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
		return -1; 
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
		return -1;
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

	*fnId = (char *) fnDefn->getId().c_str();
	*numArgs		= fnDefn->getNumArguments( );

	*argList = (char **) malloc (*numArgs);
	for (n = 0; n < *numArgs; n++)
	{
		(*argList)[n] = (char *) fnDefn->getArgument(n)->getName();
	}

	(*body) = (char *) malloc (strlen (fnMath) + 1);
	strcpy ((*body), fnMath);

	//*body = fnMath;

	free(fnMath);

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

//DataBlockWriter SBMLSupportModule::getOutsideCompartmentImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if(_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthCompartmentIdImpl");
//
//	}
//
//	string sId;arguments >> sId; 
//
//	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment == NULL)
//	{
//		throw new SBWApplicationException("There is no compartment corresponding to the input argument.", "Exception raised in method getNthCompartmentIdImpl");
//	}
//	return DataBlockWriter() << oCompartment->getOutside();
//}
//
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

//DataBlockWriter SBMLSupportModule::getListOfFloatingSpeciesImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	DataBlockWriter floatingSpeciesList;
//
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getListOfFloatingSpeciesImpl");
//	}
//
//	for(unsigned i= 0; i < _oModelCPP->getNumSpecies(); i++)
//	{
//		Species *oSpecies = _oModelCPP->getSpecies (i);
//		if (!oSpecies->getBoundaryCondition())
//		{
//			DataBlockWriter oSpeciesValues;
//			oSpeciesValues << GET_ID_IF_POSSIBLE(oSpecies);
//			oSpeciesValues << (oSpecies->isSetInitialConcentration() ? oSpecies->getInitialConcentration() : oSpecies->getInitialAmount());
//			oSpeciesValues << oSpecies->isSetInitialConcentration();
//
//			floatingSpeciesList << oSpeciesValues;
//		}
//	}
//
//	return DataBlockWriter() << floatingSpeciesList;
//}

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

//DLL_EXPORT int getListOfBoundarySpecies(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getListOfBoundarySpeciesImpl");
//	}
//
//	for(unsigned i= 0; i < _oModelCPP->getNumSpecies(); i++)
//	{
//		Species *oSpecies = _oModelCPP->getSpecies (i);
//		if (oSpecies->getBoundaryCondition())
//		{
//			DataBlockWriter oSpeciesValues;
//			oSpeciesValues << GET_ID_IF_POSSIBLE(oSpecies);
//			oSpeciesValues << (oSpecies->isSetInitialConcentration() ? oSpecies->getInitialConcentration() : oSpecies->getInitialAmount());
//			oSpeciesValues << oSpecies->isSetInitialConcentration();
//
//			boundarySpeciesList << oSpeciesValues;
//		}
//	}
//
//	return DataBlockWriter() << boundarySpeciesList;
//
//}

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
}

//DataBlockWriter SBMLSupportModule::getNthFloatingSpeciesCompartmentNameImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthFloatingSpeciesCompartmentNameImpl");
//
//	}
//	int nIndex; arguments >> nIndex;
//	int nCount = 0;
//	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
//	{
//		Species *oSpecies = _oModelCPP->getSpecies(i);
//		if(!oSpecies->getBoundaryCondition())
//		{
//			if(nCount == nIndex)
//			{
//				return DataBlockWriter() << oSpecies->getCompartment();
//			}
//			else
//			{
//				nCount ++;
//			}
//		}
//	}
//	throw new SBWApplicationException("The model does not have a floating species corresponding to the index provided", "Exception raised in method getNthFloatingSpeciesCompartmentNameImpl");
//}
//DataBlockWriter SBMLSupportModule::getNthBoundarySpeciesCompartmentNameImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthBoundarySpeciesCompartmentNameImpl");
//
//	}
//	int nIndex; arguments >> nIndex;
//	int nCount = 0;
//	for(unsigned int i = 0; i< _oModelCPP->getNumSpecies(); i++)
//	{
//		Species *oSpecies = _oModelCPP->getSpecies(i);
//		if(oSpecies->getBoundaryCondition())
//		{
//			if(nCount == nIndex)
//			{
//				return DataBlockWriter() << oSpecies->getCompartment();
//			}
//			else
//			{
//				nCount ++;
//			}
//		}
//	}
//	throw new SBWApplicationException("The model does not have a boundary species corresponding to the index provided", "Exception raised in method getNthBoundarySpeciesCompartmentNameImpl");
//}

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


//DataBlockWriter SBMLSupportModule::getNthListOfReactants(int nIndex)
//{
//	DataBlockWriter reactantList;
//	if(nIndex >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthListOfReactantsImpl");
//	}
//
//	Reaction* r = _oModelCPP->getReaction(nIndex);
//	ListOfSpeciesReferences* reactantsList = r->getListOfReactants();
//
//	int numReactants = (int)r->getNumReactants();
//	for (int i = 0; i < numReactants; i++)
//	{
//		SpeciesReference* reactant = (SpeciesReference* ) reactantsList->get(i);
//
//		DataBlockWriter reactantTemp;
//
//		reactantTemp << (reactant->getSpecies()) << reactant->getStoichiometry();
//		const StoichiometryMath* oMath = reactant->getStoichiometryMath();
//		if (oMath == NULL || !oMath->isSetMath())
//		{
//			reactantTemp << "";
//		}
//		else
//		{
//			char* formula =  SBML_formulaToString (oMath->getMath());	
//			reactantTemp << formula; 
//			free(formula);
//		}
//		reactantList << reactantTemp;
//	}
//	return reactantList;
//}


//DataBlockWriter SBMLSupportModule::getNthListOfReactantsImpl(Module/*from*/, DataBlockReader arguments)
//{
//
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthListOfReactantsImpl");
//	}
//
//	int arg; arguments >> arg;
//	DataBlockWriter reactantList = getNthListOfReactants(arg);
//
//
//	return DataBlockWriter() << reactantList;
//}
//
//DataBlockWriter SBMLSupportModule::getNthListOfProducts(int nIndex)
//{
//	DataBlockWriter productList;
//
//	if(nIndex >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthListOfProductsImpl");
//	}
//
//	Reaction* r = _oModelCPP->getReaction(nIndex);
//	ListOfSpeciesReferences* productsList = r->getListOfProducts();
//
//	int numproducts = (int)r->getNumProducts();
//	for (int i = 0; i < numproducts; i++)
//	{
//		SpeciesReference* product = (SpeciesReference* ) productsList->get(i);
//
//		DataBlockWriter productTemp;
//
//		productTemp << (product->getSpecies()) << product->getStoichiometry();
//
//		const StoichiometryMath* oMath = product->getStoichiometryMath();
//		if (oMath == NULL || !oMath->isSetMath())
//		{
//			productTemp << "";
//		}
//		else
//		{
//			char* formula =  SBML_formulaToString (oMath->getMath());	
//			productTemp << formula; 
//			free(formula);
//		}
//
//		productList << productTemp;
//	}
//	return productList;
//}
//
//
//DataBlockWriter SBMLSupportModule::getNthListOfProductsImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthListOfProductsImpl");
//	}
//
//	int arg; arguments >> arg;
//
//	DataBlockWriter productList = getNthListOfProducts(arg);
//
//	return DataBlockWriter() << productList;
//}
//DataBlockWriter SBMLSupportModule::getNthListOfModifiersImpl(Module/*from*/, DataBlockReader arguments)
//{	
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthListOfModifiersImpl");
//
//	}
//
//	int arg; arguments >> arg;
//	DataBlockWriter modifierList = getNthListOfModifiers(arg);
//	return DataBlockWriter() << modifierList;
//}
//
//DataBlockWriter SBMLSupportModule::getNthListOfModifiers(int nIndex)
//{
//	DataBlockWriter modifierList;
//	if(nIndex >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthListOfModifiersImpl");
//
//	}
//
//	Reaction* r = _oModelCPP->getReaction(nIndex);
//	int numModifiers = r->getNumModifiers();
//	for (int i = 0; i < numModifiers; i++)
//	{
//		modifierList << r->getModifier(i)->getSpecies();
//	}
//	return modifierList;
//}


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
		*kineticLaw = "0";
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

//DataBlockWriter SBMLSupportModule::getNthReactantStoichiometryDoubleImpl(Module/*from*/, DataBlockReader arguments)
//{
//	int arg1;
//	int arg2;
//	double result;
//	ListOf* reactantsList;
//	SpeciesReference* reactant;
//	Reaction* r;
//
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthReactantStoichiometryImpl");
//	}
//
//	arguments >> arg1 >> arg2;
//	if(arg1 >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthReactantStoichiometryImpl");
//	}
//
//	r = _oModelCPP->getReaction(arg1);
//	reactantsList = r->getListOfReactants();
//
//	if(arg2 >= (int)ListOf_size(reactantsList))
//	{
//		throw new SBWApplicationException("Index exceeds the number of reactants in the list", "Exception raised in method getNthReactantStoichiometryImpl");
//	}
//
//	reactant = (SpeciesReference* ) reactantsList->get(arg2);
//	result =  reactant->getStoichiometry();
//	return DataBlockWriter() << result;
//}
//DataBlockWriter SBMLSupportModule::getNthProductStoichiometryDoubleImpl(Module/*from*/, DataBlockReader arguments)
//{
//	int arg1;
//	int arg2;
//	double result;
//	ListOf* productsList;
//	SpeciesReference* product;
//	Reaction* r;
//
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthProductStoichiometryImpl");
//	}
//
//	arguments >> arg1 >> arg2;
//
//	if(arg1 >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthProductStoichiometryImpl");
//	}
//
//	r = _oModelCPP->getReaction(arg1);
//	productsList =  r->getListOfProducts();
//
//	if(arg2 >= (int)productsList->size())
//	{
//		throw new SBWApplicationException("Index exceeds the number of products in the list", "Exception raised in method getNthProductStoichiometryImpl");
//	}
//
//	product = (SpeciesReference* ) productsList->get(arg2);
//	result =  product->getStoichiometry();
//	return DataBlockWriter() << result;
//}

//DLL_EXPORT int getListOfParametersImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	int					numOfGlobalParameters;
//	int					numOfLocalParameters;
//	int					numOfReactions;
//	int					level;
//	int					i, j;
//	string				paramStr;
//	double				paramValue;
//	ListOf*			localParametersList;
//	Parameter*		parameter;
//	Reaction*			r;
//	KineticLaw*		kl;
//	DataBlockWriter paramStrValueList;
//
//	if (_oModelCPP == NULL)
//	{
//		errorCode = 2;
//		return -1;
//	}
//
//	level =  _oSBMLDocCPP->getLevel();
//	numOfGlobalParameters =  _oModelCPP->getNumParameters();
//	for(i=0; i < numOfGlobalParameters; i++)
//	{
//		parameter = (Parameter *) _oModelCPP->getParameter(i);
//		paramStr = parameter->getId();
//		DataBlockWriter tempStrValueList;
//
//		tempStrValueList << paramStr;
//
//		if( (parameter->isSetValue()) )
//		{
//			paramValue	= parameter->getValue();
//		}
//		else
//		{
//			paramValue = 0.0;
//		}
//		tempStrValueList << paramValue;
//
//		paramStrValueList << tempStrValueList;
//	}
//
//	numOfReactions = _oModelCPP->getNumReactions();
//	for(i=0; i<numOfReactions; i++)
//	{
//		r = _oModelCPP->getReaction(i);
//		kl =  r->getKineticLaw();
//		if(kl == NULL)
//		{
//			numOfLocalParameters = 0;
//		}
//		else
//		{
//			numOfLocalParameters = kl->getNumParameters();
//		}
//		localParametersList = kl->getListOfParameters();
//		for(j=0; j<numOfLocalParameters; j++)
//		{
//			parameter	= (Parameter*) localParametersList->get(j);
//			paramStr = parameter->getId();
//			DataBlockWriter tempStrValueList;
//
//			tempStrValueList << paramStr;
//			if( parameter->isSetValue() )
//			{
//				paramValue	= parameter->getValue();
//			}
//			else
//			{
//				paramValue = 0.0;
//			}
//			tempStrValueList << paramValue; 
//			paramStrValueList << tempStrValueList;
//		}
//	}
//	return DataBlockWriter() << paramStrValueList ;
//}

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


DLL_EXPORT int getParamPromotedSBML (char *inSBML, char **outSBML)
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
		//free(resultSBML);
		return 0;
	}
}

//DataBlockWriter SBMLSupportModule::isConstantImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method isConstantImpl");
//	string sId; arguments >> sId;
//	Species* oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies == NULL)
//		throw new SBWApplicationException("No species '" + sId + "'in the model.", "Exception raised in method isConstantImpl");
//	return DataBlockWriter() << (bool)oSpecies->getConstant();
//}


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

//DLL_EXPORT int getNthLocalParameterHasValue(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//      errorCode = 1;
//      return -1;
//	}
//
//	int nReactionIndex; int nParameterIndex; arguments >> nReactionIndex >> nParameterIndex;
//
//	if (nReactionIndex < 0 || nReactionIndex >= (int)_oModelCPP->getNumReactions())
//	{
//		throw new SBWApplicationException("There is no reaction corresponding to the index you provided", "Exception raised in method getNthParameterHasValueImpl");
//	}
//
//	Reaction *oReaction = _oModelCPP->getReaction(nReactionIndex);
//	ListOfParameters *oList = oReaction->getKineticLaw()->getListOfParameters();
//
//	if (nParameterIndex < 0 || nParameterIndex >= (int)oList->size())
//	{
//		throw new SBWApplicationException("Index exceeds the number of parameters in the list", "Exception raised in method getNthParameterHasValueImpl");
//	}
//
//	return DataBlockWriter() << ((Parameter*)oList->get(nParameterIndex))->isSetValue();
//}

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

//DLL_EXPORT int getNthRuleType (Module/*from*/, DataBlockReader arguments)
//{
//	int arg;
//	string result;
//	SBMLTypeCode_t type;
//	Rule *rule;
//
//	if (_oModelCPP == NULL)
//	{
//		errorCode = 1;
//		return -1;
//	}
//
//	arguments >> arg;
//	rule = _oModelCPP->getRule(arg);
//
//	if(rule == NULL)
//	{
//		throw new SBWApplicationException("The model does not have a Rule corresponding to the index provided", "Exception raised in method getNthRuleImpl");
//	}
//
//	type = rule->getTypeCode();
//	if ( type == SBML_PARAMETER_RULE )
//	{
//		result = "Parameter_Rule";
//	}
//
//	if ( type == SBML_SPECIES_CONCENTRATION_RULE )
//	{
//		result = "Species_Concentration_Rule";
//	}
//
//	if ( type == SBML_COMPARTMENT_VOLUME_RULE )
//	{
//		result = "Compartment_Volume_Rule";
//	}
//
//	if ( type == SBML_ASSIGNMENT_RULE )
//	{
//		result = "Assignment_Rule";
//	}
//
//	if ( type == SBML_ALGEBRAIC_RULE )
//	{
//		result = "Algebraic_Rule";
//	}
//
//	if ( type == SBML_RATE_RULE )
//	{
//		result = "Rate_Rule";
//	}
//
//	DataBlockWriter resultWriter; resultWriter << result;	
//	return resultWriter;
//}


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
	EventAssignment*	ea;
	Event*			    event;
	const char* zero = "0";
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

//DataBlockWriter SBMLSupportModule::hasValueImpl(Module/*from*/, DataBlockReader arguments)
//{	
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method hasValueImpl");
//	}
//
//	string sArg; arguments >> sArg; 
//
//
//	Species *s = _oModelCPP->getSpecies(sArg);
//	if (s != NULL)
//	{ 
//		if (s->isSetInitialAmount() || s->isSetInitialConcentration())
//			return DataBlockWriter() << true;		
//		return DataBlockWriter() << false;
//	}
//
//	Compartment *c = _oModelCPP->getCompartment(sArg);
//	if (c != NULL)
//	{
//		if (c->isSetVolume())
//			return DataBlockWriter() << true;		
//		return DataBlockWriter() << false;
//	}
//
//	Parameter *p = _oModelCPP->getParameter(sArg);
//	if (p != NULL)
//	{
//		if (p->isSetValue())
//			return DataBlockWriter() << true;		
//		return DataBlockWriter() << false;
//	}
//
//	throw new SBWApplicationException("Invalid string name. The name does not exist in the model", "Exception raised in method hasValueImpl");
//}
//DataBlockWriter SBMLSupportModule::getBuiltinFunctionsImpl(Module/*from*/, DataBlockReader /*arguments*/)		
//{
//	vector<string> result;
//	unsigned int i;
//
//	for(i = 0; i < FUNCDATAROWS; i++)
//	{
//		result.push_back(_oPredefinedFunctions[i][0]);
//	}
//	return DataBlockWriter() << result;
//}
//
//DataBlockWriter SBMLSupportModule::getBuiltinFunctionInfoImpl(Module/*from*/, DataBlockReader arguments)
//{
//	string sName; arguments >> sName;
//	vector<string> result;
//	int funcFound = 0;
//	int i = 0;
//	int j = 0;
//
//	while(i < FUNCDATAROWS)
//	{
//		if(sName == _oPredefinedFunctions[i][0])
//		{
//			funcFound = 1;
//			break;
//		}
//		else
//		{
//			i++;
//		}
//	}
//
//	if(funcFound)
//	{
//		while(_oPredefinedFunctions[i][j] != NULL && j < 16)
//		{
//			result.push_back(_oPredefinedFunctions[i][j]);
//			j++;
//		}
//	}
//	else
//	{
//		throw new SBWApplicationException("Invalid string name. There is no inbuilt function with that name: " + sName, "Exception raised in method getBuiltinFunctionInfoImpl");
//	}
//	return DataBlockWriter() << result;		
//}
//DataBlockWriter SBMLSupportModule::convertPowImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	char*			arg;
//	char*			result;
//	ASTNode_t*		ast_Node;
//	Reaction_t*		r;
//	KineticLaw_t*	kl;
//	unsigned int i;
//	SBMLDocument_t* doc;
//	Model_t*		model;
//	const char*			strKineticFormula;
//
//	arguments >> arg;
//	if (strcmp(arg,"") == 0)
//	{
//		throw new SBWApplicationException("The input string cannot be blank", "Exception raised in method convertPowImpl");
//	}
//
//	doc = readSBMLFromString(arg);
//	model = SBMLDocument_getModel(doc);
//	if (model == NULL)
//	{
//		throw new SBWApplicationException("Error in sbml input. ", "Exception raised in method convertPow");
//	}
//
//	for(i = 0; i < Model_getNumReactions(model); i++)
//	{
//		r  = Model_getReaction(model, i);
//		kl = Reaction_getKineticLaw(r);
//
//		if (kl == NULL)
//		{
//			strKineticFormula = "";
//		}
//		else
//		{
//			strKineticFormula = KineticLaw_getFormula(kl);
//			if (strKineticFormula == NULL)
//			{
//				throw new SBWApplicationException("The kinetic law has errors", "Exception raised in method convertPow");
//			}
//		}
//		ast_Node	= (ASTNode_t *) SBML_parseFormula (strKineticFormula);
//		changePow(ast_Node);
//		KineticLaw_setMath (kl, ast_Node);
//	}
//
//	SBMLDocument_setLevelAndVersion(doc, 1,2);
//	result = writeSBMLToString(doc);
//	SBMLDocument_free(doc);
//	DataBlockWriter resultWriter; resultWriter << result;
//	free(result);
//	free(arg);
//	return resultWriter;
//}

DLL_EXPORT int convertMathMLToString (char *mathMLStr, char **infix)
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
}

DLL_EXPORT int convertStringToMathML (char* infixStr, char **mathMLStr)
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

//DataBlockWriter SBMLSupportModule::convertLevel1ToLevel2Impl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	char*			arg;
//	char*			result;
//	unsigned int		errors;
//	SBMLDocument_t		*oSBMLDoc;
//
//	arguments >> arg;
//	oSBMLDoc		=	readSBMLFromString(arg);
//	errors			=	SBMLDocument_getNumErrors(oSBMLDoc);
//
//	if (errors > 0)
//	{
//		throw new SBWApplicationException("The input SBML is invalid.", "Exception raised in method convertLevel1ToLevel2Impl");
//	}
//
//
//	SBMLDocument_setLevelAndVersion(oSBMLDoc, 2, 1);
//	errors = SBMLDocument_getNumErrors(oSBMLDoc);
//	if (errors > 0)
//	{
//		throw new SBWApplicationException("Conversion cannot be done.", "Exception raised in method convertLevel1ToLevel2Impl");
//	}
//
//	result = writeSBMLToString(oSBMLDoc);
//
//	SBMLDocument_free(oSBMLDoc);
//	DataBlockWriter resultWriter; resultWriter << result;
//	free(arg);
//	free(result);
//	return resultWriter;
//}
//DataBlockWriter SBMLSupportModule::convertSBMLImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	std::string sModel; int nLevel; int nVersion;
//	arguments >> sModel >> nLevel >> nVersion;
//
//	return DataBlockWriter() << convertSBML(sModel,nLevel, nVersion);
//
//}
//DataBlockWriter SBMLSupportModule::convertLevel2ToLevel1Impl(Module/*from*/, DataBlockReader arguments)
//{
//	SBWOSMutexLock lock(_oMutex);
//	string arg;
//	char*			result;
//	unsigned int		errors;
//	SBMLDocument_t		*oSBMLDoc;
//
//	arguments >> arg;
//	oSBMLDoc		=	readSBMLFromString(arg.c_str());
//	errors	=	SBMLDocument_getNumErrors(oSBMLDoc);
//
//	if (errors > 0)
//	{	
//		SBMLDocument_free(oSBMLDoc);
//		validateInternal(arg.c_str());
//		throw new SBWApplicationException("The input SBML is invalid.", "Exception raised in method convertLevel2ToLevel1Impl");
//	}
//
//
//	SBMLDocument_setLevelAndVersion(oSBMLDoc, 1, 2);
//	errors	=	SBMLDocument_getNumErrors(oSBMLDoc);
//	if (errors > 0)
//	{
//		SBMLDocument_free(oSBMLDoc);
//		throw new SBWApplicationException("Conversion cannot be done.", "Exception raised in method convertLevel2ToLevel1Impl");
//	}
//
//	result = writeSBMLToString(oSBMLDoc);
//
//	SBMLDocument_free(oSBMLDoc);
//	DataBlockWriter resultWriter; resultWriter << result;
//	free(result);
//	return resultWriter;
//}

DLL_EXPORT int addMissingModifiers (char *inSBML, char **outSBML)
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

		SBMLRuleConverter *converter = new SBMLRuleConverter();
		converter->setDocument(doc);
		converter->setProperties(&props);
		converter->convert();
		char * string = doc->toSBML();
		*sbml = string; 
		free(doc);
		free(converter);
		return 0;
	}
	catch (...)
	{
		errorCode = 25;
		return -1;
	}
}

//DataBlockWriter SBMLSupportModule::reorderRulesImpl(Module/*from*/, DataBlockReader arguments)
//{
//  string sbml; arguments >> sbml;
//  SBMLDocument *doc = readSBMLFromString(sbml.c_str());
//  ConversionProperties props;
//  props.addOption("sortRules", true);
//  
//  SBMLRuleConverter *converter = new SBMLRuleConverter();
//  converter->setDocument(doc);
//  converter->setProperties(&props);
//  converter->convert();
//  //doc->convert(props);
//  string result = doc->toSBML();
//  delete doc;
//  delete converter;
//  return DataBlockWriter() << result;
//}


//DataBlockWriter SBMLSupportModule::convertTimeImpl(Module/*from*/, DataBlockReader arguments)
//{
//	SBMLDocument *oSBMLDoc = NULL;
//	Model *oModel = NULL;
//
//	string sArg; string sTimeSymbol; arguments >> sArg >> sTimeSymbol; 
//
//
//	oSBMLDoc = readSBMLFromString(sArg.c_str());
//	//oSBMLDoc->setLevelAndVersion( 2, 1);
//	oModel = oSBMLDoc->getModel();
//
//	if (oModel == NULL)
//	{	
//		throw new SBWApplicationException("SBML Validation failed","Model could not be read.");
//	}
//	else
//	{
//		changeTimeSymbol(oModel, sTimeSymbol.c_str());
//
//		char *resultSBML = writeSBMLToString(oSBMLDoc);
//
//		delete oSBMLDoc;
//		oModel = NULL;
//		oSBMLDoc = NULL;
//		DataBlockWriter result; result << resultSBML;
//		free(resultSBML);
//		return  result;
//	}
//
//}

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


//DataBlockWriter SBMLSupportModule::getDerivedUnitDefinitionImpl(Module/*from*/, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getDerivedUnitDefinitionImpl");				
//	}
//
//	string sId; arguments >> sId; 
//
//	Species *oSpecies = _oModelCPP->getSpecies(sId);
//	if (oSpecies != NULL)
//	{
//		UnitDefinition* oUnitDef = oSpecies->getDerivedUnitDefinition();
//		DataBlockWriter oResult; addUnitDefinitionToResult(oResult, oUnitDef); return oResult;
//	}
//
//	Compartment *oCompartment = _oModelCPP->getCompartment(sId);
//	if (oCompartment != NULL)
//	{
//		UnitDefinition* oUnitDef = oCompartment->getDerivedUnitDefinition();
//		DataBlockWriter oResult; addUnitDefinitionToResult(oResult, oUnitDef); return oResult;
//	}
//
//	Parameter *oParameter = _oModelCPP->getParameter(sId);
//	if (oParameter != NULL)
//	{
//		UnitDefinition* oUnitDef = oParameter->getDerivedUnitDefinition();
//		DataBlockWriter oResult; addUnitDefinitionToResult(oResult, oUnitDef); return oResult;
//	}
//
//	return DataBlockWriter();
//
//
//}
//
//
//DataBlockWriter SBMLSupportModule::getNumConstraintsImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	return DataBlockWriter() << getNumConstraints();
//}
//DataBlockWriter SBMLSupportModule::getNumInitialAssignmentsImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	return DataBlockWriter() << getNumInitialAssignments();
//}
//DataBlockWriter SBMLSupportModule::getNthConstraintImpl(Module/*from*/, DataBlockReader arguments)
//{
//	int nIndex; arguments >> nIndex;
//	string message; string formula = getNthConstraint(nIndex, message);
//	DataBlockWriter list; list <<  formula << message;
//	return DataBlockWriter() << list;
//}
//DataBlockWriter SBMLSupportModule::getNthInitialAssignmentImpl(Module/*from*/, DataBlockReader arguments)
//{
//	int nIndex; arguments >> nIndex;
//	return DataBlockWriter() << getNthInitialAssignment(nIndex);
//}
//DataBlockWriter SBMLSupportModule::getListOfConstraintsImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	int numConstraints = getNumConstraints();
//	DataBlockWriter result; 
//	for (int i = 0; i < numConstraints; i++)
//	{
//		string message; string formula = getNthConstraint(i, message);
//		DataBlockWriter list; list << formula << message;
//		result << list;
//	}
//	return DataBlockWriter() << result;
//}
//DataBlockWriter SBMLSupportModule::getListOfInitialAssignmentsImpl(Module/*from*/, DataBlockReader /*arguments*/)
//{
//	int numAssignments = getNumInitialAssignments();
//	DataBlockWriter result;
//	for (int i = 0; i < numAssignments; i++)
//	{
//		result << getNthInitialAssignment(i);
//	}
//	return DataBlockWriter() << result;
//}
//
//
//
//int SBMLSupportModule::getNumConstraints()
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNumConstraints");				
//	}
//	return (int)_oModelCPP->getNumConstraints();
//}
//int SBMLSupportModule::getNumInitialAssignments()
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNumInitialAssignments");				
//	}
//	return (int)_oModelCPP->getNumInitialAssignments();
//
//}
//std::string SBMLSupportModule::getNthInitialAssignment(int nIndex)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthInitialAssignment");				
//	}
//
//	InitialAssignment* assignment = _oModelCPP->getInitialAssignment(nIndex);
//	if (assignment == NULL)
//		throw new SBWApplicationException("No initial assignment exists for the provided Index", "Exception raised in method getNthInitialAssignment");
//
//	if (!assignment->isSetSymbol() || !assignment->isSetMath())
//		throw new SBWApplicationException("Initial assignment is invalid symbol or math element", "Exception raised in method getNthInitialAssignment");
//	return assignment->getSymbol() + " = " + SBML_formulaToString ( assignment->getMath() );
//
//}
//std::string SBMLSupportModule::getNthConstraint(int nIndex, std::string &outMessage)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthConstraint");				
//	}
//
//	Constraint *constraint = _oModelCPP->getConstraint( nIndex );
//	if (constraint == NULL)
//		throw new SBWApplicationException("No constraint exists for the provided index", "Exception raised in method getNthConstraint");
//
//	if (! constraint->isSetMath() )
//		throw new SBWApplicationException("Invalid constraint, need math expression defined.", "Exception raised in method getNthConstraint");
//
//	if (! constraint->isSetMessage() )
//	{
//		stringstream sTemp; sTemp << "Constraint " << nIndex << " was violated."; outMessage = sTemp.str();
//	}
//	else
//	{
//		outMessage = XMLNode::convertXMLNodeToString( constraint->getMessage() );
//	}
//
//	return SBML_formulaToString ( constraint->getMath() );
//}
//
//DataBlockWriter SBMLSupportModule::getListOfReactionInfoImpl(Module from, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getListOfReactionInfoImpl");				
//	}
//
//	DataBlockWriter oResult;
//	for (int i = 0; i < (int)_oModelCPP->getNumReactions(); i++)
//	{
//		DataBlockWriter oReactionInfo = getNthReactionInfo(i);
//		oResult << oReactionInfo;
//	}
//	return DataBlockWriter() << oResult;		
//}
//
//DataBlockWriter SBMLSupportModule::getNthReactionInfoImpl(Module from, DataBlockReader arguments)
//{
//	if (_oModelCPP == NULL)
//	{
//		throw new SBWApplicationException("You need to load the model first", "Exception raised in method getNthReactionInfoImpl");				
//	}
//
//	int nIndex; arguments >> nIndex; 
//
//	DataBlockWriter oResult = getNthReactionInfo(nIndex);
//
//	return DataBlockWriter() << oResult;
//
//}
//
//DataBlockWriter SBMLSupportModule::shortenNamesImpl(Module/*from*/, DataBlockReader arguments)
//{
//	string sbml; int maxLength; 
//	arguments >> sbml >> maxLength;
//	SBMLReader reader;
//	SBMLDocument *doc = reader.readSBMLFromString(sbml);
//	Model* model = doc->getModel();
//
//	if (model == NULL)
//	{
//		delete doc;
//		validateInternal(sbml);
//		throw new SBWApplicationException("Invalid SBML");
//	}
//
//	vector<string> names; 
//
//	for (unsigned int i = 0; i < model->getNumCompartments(); i++)
//	{
//		Compartment *comp = model->getCompartment(i);
//		if (comp->isSetId())
//			names.push_back(comp->getId());
//	}
//		
//	for (unsigned int i = 0; i < model->getNumSpecies(); i++)
//	{
//		Species *species = model->getSpecies(i);
//		if (species->isSetId())
//			names.push_back(species->getId());
//	}
//
//	for (unsigned int i = 0; i < model->getNumParameters(); i++)
//	{
//		Parameter *param = model->getParameter(i);
//		if (param->isSetId())
//			names.push_back(param->getId());
//	}
//
//	for (unsigned int i = 0; i < model->getNumCompartments(); i++)
//	{
//		Compartment *comp = model->getCompartment(i);
//		string oldName = comp->getId();
//		string newName = RenameSBaseElement(comp, maxLength, model, names);
//
//		int index = GetIndex(oldName, names);
//		if (index >= 0 && newName != "")
//			names[index] = newName;
//	}
//
//	for (unsigned int i = 0; i < model->getNumSpecies(); i++)
//	{
//		Species *species = model->getSpecies(i);
//		string oldName = species->getId();
//		string newName = RenameSBaseElement(species, maxLength, model, names);
//
//		int index = GetIndex(oldName, names);
//		if (index >= 0 && newName != "")
//			names[index] = newName;
//	}
//
//	for (unsigned int i = 0; i < model->getNumParameters(); i++)
//	{
//		Parameter *parameter = model->getParameter(i);
//		string oldName = parameter->getId();
//		string newName = RenameSBaseElement(parameter, maxLength, model, names);
//
//		int index = GetIndex(oldName, names);
//		if (index >= 0 && newName != "")
//			names[index] = newName;
//	}
//
//
//	SBMLWriter writer;
//	sbml = writer.writeSBMLToString(doc);
//
//	delete doc;
//
//	return DataBlockWriter() << sbml;
//}
//int SBMLSupportModule::GetIndex(const std::string & name, const std::vector<std::string>& names)
//{
//	std::vector<std::string>::iterator it;
//	int index = 0;
//	for (vector<string>::const_iterator it = names.begin(); it!=names.end(); ++it)
//	{
//		
//		if (*it == name)
//			return index;
//		index++;
//	}
//	return -1;
//}
//std::string SBMLSupportModule::getNewName(const std::string &baseName, int maxLength,const  std::vector<std::string>& existingNames)
//{
//	string newName =  baseName.substr(0, maxLength);
//
//	int ncount= 0;
//	int index = GetIndex(newName, existingNames);
//	while (index != -1)
//	{
//		int length;
//		if (ncount < 10)
//		{
//		length = 1;
//		}
//		else 
//		if (ncount >= 10 && ncount <100)
//		{
//			length = 2;
//		}
//		else if (ncount >= 100 && ncount <1000)
//		{
//			length = 3;
//		}
//		else 
//		{
//			break;
//		}
//		stringstream stream; 
//		stream << baseName.substr(0, maxLength -length) << (ncount++);
//		newName = stream.str();			
//		index = GetIndex(newName, existingNames);
//	}
//	return newName;
//}
//string SBMLSupportModule::RenameSBaseElement(SBase* element, int maxLength, Model* model,const std::vector<std::string>& names)
//{
//	static string empty;
//	if (element->isSetId() && element->getId().length() > (unsigned int)maxLength)
//	{
//		string newName = getNewName(element->getId(), maxLength, names);		
//		RenameElement(element->getId(), newName, model);
//		element->setId(newName);
//		return newName;
//	}
//	return empty;
//}
//void SBMLSupportModule::RenameElement(const std::string& oldName, const std::string& newName, Model* model)
//{
//	cout << "renaming " << oldName << " to " << newName << endl;
//	for (unsigned int i = 0; i < model->getNumInitialAssignments(); i++)
//	{
//		InitialAssignment* init = model->getInitialAssignment(i);
//		if (init->isSetSymbol() && init->getSymbol() == oldName)
//			init->setSymbol(newName);
//		if (init->isSetMath())
//		{
//			ASTNode* node = const_cast<ASTNode *>(init->getMath());
//			RenameElementName(oldName, newName, node);
//			init->setMath(node);
//		}
//	}
//	for (unsigned int i = 0; i < model->getNumSpecies(); i++)
//	{
//		Species *species = model->getSpecies(i);
//		if (species->isSetCompartment() && species->getCompartment() == oldName)
//			species->setCompartment(newName);
//	}
//	for (unsigned int i = 0; i < model->getNumReactions(); i++)
//	{
//		Reaction* reaction = model->getReaction(i);
//		if (reaction->isSetKineticLaw() && reaction->getKineticLaw()->isSetMath())
//		{
//			ASTNode* node = const_cast<ASTNode*>(reaction->getKineticLaw()->getMath());
//			RenameElementName(oldName, newName, node);
//			reaction->getKineticLaw()->setMath(node);			
//		}
//
//		for (unsigned int j = 0; j < reaction->getNumProducts(); j++)
//		{
//			SpeciesReference* ref = reaction->getProduct(j);
//			if (ref->isSetSpecies() && ref->getSpecies() == oldName)
//				ref->setSpecies(newName);
//		}
//
//		for (unsigned int j = 0; j < reaction->getNumReactants(); j++)
//		{
//			SpeciesReference* ref = reaction->getReactant(j);
//			if (ref->isSetSpecies() && ref->getSpecies() == oldName)
//				ref->setSpecies(newName);
//		}
//
//		for (unsigned int j = 0; j < reaction->getNumModifiers(); j++)
//		{
//			ModifierSpeciesReference* ref = reaction->getModifier(j);
//			if (ref->isSetSpecies() && ref->getSpecies() == oldName)
//				ref->setSpecies(newName);
//		}
//
//	}
//
//	for (unsigned int i = 0; i < model->getNumRules(); i++)
//	{
//		Rule* rule = model->getRule(i);
//		if (rule->isSetMath())
//		{
//			ASTNode* node = const_cast<ASTNode*>(rule->getMath());
//			RenameElementName(oldName, newName, node);
//			rule->setMath(node);			
//		}
//		if (rule->isSetVariable() && rule->getVariable() == oldName)
//			rule->setVariable(newName);
//	}
//
//	for (unsigned int i = 0; i < model->getNumEvents(); i++)
//	{
//		Event *oEvent = model->getEvent(i);
//		if (oEvent->isSetTrigger() && oEvent->getTrigger()->isSetMath())
//		{
//			ASTNode* node = const_cast<ASTNode*>(oEvent->getTrigger()->getMath());
//			RenameElementName(oldName, newName, node);
//			oEvent->getTrigger()->setMath(node);
//		}
//
//		for (unsigned int j = 0; j < oEvent->getNumEventAssignments(); j++)
//		{
//			EventAssignment* assignment = oEvent->getEventAssignment(j);
//			if (assignment->isSetVariable() && assignment->getVariable()==oldName)
//				assignment->setVariable(newName);
//
//			if (assignment->isSetMath())
//			{
//				ASTNode* node = const_cast<ASTNode*>(assignment->getMath());
//				RenameElementName(oldName, newName, node);
//				assignment->setMath(node);
//			}
//		}
//	}
//
//}
//
//void SBMLSupportModule::RenameElementName(const std::string& oldName, const std::string& newName, ASTNode* node)
//{
//	if (node == NULL) return;
//	if (node->isName() && node->getName() == oldName)
//	{
//		node->setName(newName.c_str());
//	}
//
//	for (unsigned int i = 0; i < node->getNumChildren(); i++)
//	{
//		RenameElementName(oldName, newName, node->getChild(i));
//	}
//
//}
//
//DataBlockWriter SBMLSupportModule::getNthReactionInfo(int nIndex)
//{
//	DataBlockWriter oResult;
//
//
//	Reaction *oReaction = _oModelCPP->getReaction(nIndex);
//
//	if (oReaction == NULL)
//	{
//		throw new SBWApplicationException("No Reaction for given Index", "No reaction for the given index exists. Exception raised in method getNthReactionInfoImpl");
//	}
//
//
//
//	oResult << oReaction->getId();
//	oResult << oReaction->getName();
//	oResult << (bool)oReaction->getReversible();
//
//	KineticLaw *kl = oReaction->getKineticLaw();
//	if (kl == NULL)
//	{
//		oResult  << "0";
//	}
//	else
//	{
//		const string formula =  kl->getFormula();	
//		oResult  << formula; 
//	}	
//
//	DataBlockWriter oTempReactantWriter = getNthListOfReactants(nIndex);
//	oResult << oTempReactantWriter;
//
//	DataBlockWriter oTempProductWriter = getNthListOfProducts(nIndex);
//	oResult << oTempProductWriter;
//
//	DataBlockWriter oTempModifierWriter = getNthListOfModifiers(nIndex);	
//	oResult << oTempModifierWriter;
//
//	DataBlockWriter oTempParameters;
//	if (kl != NULL)
//	{
//		for (unsigned int i = 0; i < kl->getNumParameters(); i++)
//		{
//			DataBlockWriter oList;
//			Parameter *oParameter = kl->getParameter(i);
//			if (oParameter != NULL)
//			{
//				oList << oParameter->getId() << oParameter->getValue();
//			}
//			oTempParameters << oList;
//		}
//	}
//	oResult << oTempParameters;
//	return oResult;
//}
//
//
//DataBlockWriter SBMLSupportModule::addSourceSinkNodes(Module/*from*/, DataBlockReader arguments)
//{
//	string sbml; arguments >> sbml;	
//
//	SBMLReader reader; SBMLWriter writer;
//
//	SBMLDocument *oSBMLDoc = reader.readSBMLFromString(sbml);
//	Model* oModel = oSBMLDoc->getModel();
//	int errors = oSBMLDoc->getNumErrors();
//
//	if (errors > 0)
//	{
//		if (oSBMLDoc != NULL) delete oSBMLDoc;
//		throw new SBWApplicationException("The input SBML is invalid.", "Exception raised in method convertLevel1ToLevel2Impl");
//	}	
//
//
//	bool bNeedSource = NeedSource(oModel);
//	bool bNeedSink = NeedSink(oModel);
//
//	if (bNeedSource)
//	{
//		Species *source = oModel->getSpecies("source");
//		if (source == NULL)
//		{
//			source = oModel->createSpecies();
//			source->setId("source"); source->setName("Source");
//			source->setCompartment(oModel->getCompartment(0)->getId());
//			source->setBoundaryCondition(true);
//			source->setInitialAmount(1);
//		}
//
//		for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//		{
//			Reaction *r = oModel->getReaction(i);
//			if (r->getNumReactants() == 0)
//			{
//				SpeciesReference *ref = r->createReactant();
//				ref->setSpecies("source");
//			}
//		}
//	}
//
//	if (bNeedSink)
//	{
//		Species *sink = oModel->getSpecies("sink");
//		if (sink == NULL)
//		{
//			sink = oModel->createSpecies();
//			sink->setId("sink"); sink->setName("Sink");
//			sink->setCompartment(oModel->getCompartment(0)->getId());
//			sink->setBoundaryCondition(true);
//			sink->setInitialAmount(0);
//		}
//
//		for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//		{
//			Reaction *r = oModel->getReaction(i);
//			if (r->getNumProducts() == 0)
//			{
//				SpeciesReference *ref = r->createProduct();
//				ref->setSpecies("sink");
//			}
//		}
//	}
//
//	char *result = writer.writeToString(oSBMLDoc);
//	DataBlockWriter resultWriter = DataBlockWriter() << result;
//	delete oSBMLDoc; free(result);	
//	return resultWriter;
//}
//
//DataBlockWriter SBMLSupportModule::addEmptySetNode(Module/*from*/, DataBlockReader arguments)
//{
//	string sbml; arguments >> sbml;	
//
//	SBMLReader reader; SBMLWriter writer;
//
//	SBMLDocument *oSBMLDoc = reader.readSBMLFromString(sbml);
//	Model* oModel = oSBMLDoc->getModel();
//	int errors = oSBMLDoc->getNumErrors();
//
//	if (errors > 0)
//	{
//		if (oSBMLDoc != NULL) delete oSBMLDoc;
//		throw new SBWApplicationException("The input SBML is invalid.", "Exception raised in method addEmptySetNode");
//	}	
//
//
//	bool bNeedEmptySet = NeedEmptySet(oModel);
//
//
//	if (bNeedEmptySet)
//	{
//		Species *source = oModel->getSpecies("emptySet");
//		if (source == NULL)
//		{
//			source = oModel->createSpecies();
//			source->setId("emptySet"); source->setName("EmptySet");
//			source->setCompartment(oModel->getCompartment(0)->getId());
//			source->setBoundaryCondition(true);
//			source->setInitialAmount(0);
//		}
//
//		for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//		{
//			Reaction *r = oModel->getReaction(i);
//			if (r->getNumReactants() == 0)
//			{
//				SpeciesReference *ref = r->createReactant();
//				ref->setSpecies("emptySet");				
//			}
//			else if (r->getNumProducts() == 0)
//			{
//				SpeciesReference *ref = r->createProduct();
//				ref->setSpecies("emptySet");
//			}
//		}
//	}
//
//
//
//	char *result = writer.writeToString(oSBMLDoc);
//	DataBlockWriter resultWriter = DataBlockWriter() << result;
//	delete oSBMLDoc; free(result);	
//	return resultWriter;
//}
//
//void SBMLSupportModule::addUnitDefinitionToResult(DataBlockWriter &oWriter, const UnitDefinition* oDefinition)
//{
//	const ListOfUnits * oList = oDefinition->getListOfUnits();
//	for (int i = 0; i < (int)oList->size(); i++)
//	{
//		const Unit *oUnit = oDefinition->getUnit(i);
//		if (oUnit != NULL)
//		{
//			DataBlockWriter oTemp; 
//			oTemp << UnitKind_toString(oUnit->getKind()) << oUnit->getExponent() << oUnit->getMultiplier() << oUnit->getOffset() << oUnit->getScale();
//			oWriter << oTemp;
//		}
//	}
//}
//
//
//bool SBMLSupportModule::NeedSource(Model* oModel)
//{
//	if (oModel == NULL) return false;
//
//	for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//	{
//		const Reaction *r = oModel->getReaction(i);
//		if (r->getNumReactants()==0) return true;
//	}
//	return false;
//}
//
//bool SBMLSupportModule::NeedSink(Model* oModel)
//{
//	if (oModel == NULL) return false;
//
//	for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//	{
//		const Reaction *r = oModel->getReaction(i);
//		if (r->getNumProducts()==0) return true;
//	}
//	return false;
//}
//
//bool SBMLSupportModule::NeedEmptySet(Model* oModel)
//{
//	if (oModel == NULL) return false;
//
//	for (unsigned int i = 0; i < oModel->getNumReactions(); i++)
//	{
//		const Reaction *r = oModel->getReaction(i);
//		if (r->getNumReactants()== 0|| r->getNumProducts() == 0) return true;
//	}
//	return false;
//}
//

//// static instance of this class, access only via this method
//SBMLSupportModule* SBMLSupportModule::_oInstance = NULL;
//
//const char* SBMLSupportModule::_oPredefinedFunctions[FUNCDATAROWS][FUNCDATACOLS] = {
//	{ "massi1", "Irreversible Mass Action Kinetics for 1 substrate", "S", "k", "k * S" },
//	{ "massi2", "Irreversible Mass Action Kinetics for 2 substrates", "S1", "S2", "k", "k * S1 * S2" },
//	{ "massi3", "Irreversible Mass Action Kinetics for 3 substrates", "S1", "S2", "S3", "k", "k * S1 * S2 * S3" },
//	{ "massr11", "Reversible Mass Action Kinetics for 1 substrate and 1 product", "S", "P", "k_1", "k_2",
//	"k_1 * S - k_2 * P"},
//	{ "massr12", "Reversible Mass Action Kinetics for 1 substrate and 2 products", "S", "P1", "P2", "k_1", "k_2",
//	"k_1 * S - k_2 * P1 * P2"},
//	{ "massr13", "Reversible Mass Action Kinetics for 1 substrate and 3 products", "S", "P1", "P2", "P3", "k_1", "k_2",
//	"k_1 * S - k_2 * P1 * P2 * P3"},
//	{ "massr21", "Reversible Mass Action Kinetics for 2 substrates and 1 product", "S1", "S2", "P", "k_1", "k_2",
//	"k_1 * S1 * S2 - k_2 * P"},
//	{ "massr22", "Reversible Mass Action Kinetics for 2 substrates and 2 products", "S1", "S2", "P1", "P2", "k_1", "k_2",
//	"k_1 * S1 * S2 - k_2 * P1 * P2"},
//	{ "massr23", "Reversible Mass Action Kinetics for 2 substrates and 3 products", "S1", "S2", "P1", "P2", "P3", "k_1", "k_2",
//	"k_1 * S1 * S2 - k_2 * P1 * P2 * P3"},
//	{ "massr31", "Reversible Mass Action Kinetics for 3 substrates and 1 product", "S1", "S2", "S3", "P", "k_1", "k_2",
//	"k_1 * S1 * S2 * S3 - k_2 * P"},
//	{ "massr32", "Reversible Mass Action Kinetics for 3 substrates and 2 products", "S1", "S2", "S3", "P1", "P2", "k_1", "k_2",
//	"k_1 * S1 * S2 * S3 - k_2 * P1 * P2"},
//	{ "massr33", "Reversible Mass Action Kinetics for 3 substrates and 3 products", "S1", "S2", "S3", "P1", "P2", "P3", "k_1", "k_2",
//	"k_1 * S1 * S2 * S3 - k_2 * P1 * P2 * P3"},
//	{ "uui", "Irreversible Simple Michaelis-Menten ", "S", "V_m", "K_m", "(V_m * S)/(K_m + S)" },
//	{ "uur", "Uni-Uni Reversible Simple Michaelis-Menten", "S", "P", "V_f", "V_r", "K_ms", "K_mp",
//	"(V_f * S / K_ms - V_r * P / K_mp)/(1 + S / K_ms +  P / K_mp)"},
//	{ "uuhr", "Uni-Uni Reversible Simple Michaelis-Menten with Haldane adjustment", "S", "P", "V_f", "K_m1", "K_m2", "K_eq",
//	"( V_f / K_m1 * (S - P / K_eq ))/(1 + S / K_m1 + P / K_m2)"},
//	{ "isouur", "Iso Uni-Uni", "S", "P", "V_f", "K_ms", "K_mp", "K_ii", "K_eq",
//	"(V_f * (S - P / K_eq ))/(S * (1 + P / K_ii ) + K_ms * (1 + P / K_mp))"},
//	{ "hilli", "Hill Kinetics", "S", "V", "S_0_5", "h", "(V * pow(S,h))/(pow(S_0_5,h) + pow(S,h))"},
//	{ "hillr", "Reversible Hill Kinetics", "S", "P", "V_f", "S_0_5", "P_0_5", "h", "K_eq",
//	"(V_f * (S / S_0_5) * (1 - P / (S * K_eq) ) * pow(S / S_0_5 + P / P_0_5, h-1))/(1 + pow(S / S_0_5 + P / P_0_5, h))"},
//	{ "hillmr", "Reversible Hill Kinetics with One Modifier", "S", "M", "P", "V_f", "K_eq", "k", "h", "alpha",
//	"(V_f * (S / S_0_5) * (1 - P / (S * K_eq) ) * pow(S / S_0_5 + P / P_0_5, h-1))/( pow(S / S_0_5 + P / P_0_5, h) + (1 + pow(M / M_0_5, h))/(1 + alpha * pow(M/M_0_5,h)))"},
//	{ "hillmmr", "Reversible Hill Kinetics with Two Modifiers", "S", "P", "M", "V_f", "K_eq", "k", "h", "a", "b", "alpha_1", "alpha_2", "alpha_12",
//	"(V_f * (S / S_0_5) * (1 - P / (S * K_eq) ) * pow(S / S_0_5 + P / P_0_5, h-1)) / (pow(S / S_0_5 + P / P_0_5, h) + ((1 + pow(Ma/Ma_0_5,h) + pow(Mb/Mb_0_5,h))/( 1 + alpha_1 * pow(Ma/Ma_0_5,h) + alpha_2 * pow(Mb/Mb_0_5,h) + alpha_1 * alpha_2 * alpha_12 * pow(Ma/Ma_0_5,h) * pow(Mb/Mb_0_5,h))))"},
//	{ "usii", "Substrate Inhibition Kinetics (Irreversible)", "S", "V", "K_m", "K_i", "V*(S/K_m)/(1 + S/K_m + sqr(S)/K_i)"},
//	{ "usir", "Substrate Inhibition Kinetics (Reversible)", "S", "P", "V_f", "V_r", "K_ms", "K_mp", "K_i",
//	"(V_f*S/K_ms + V_r*P/K_mp)/(1 + S/K_ms + P/K_mp + sqr(S)/K_i)"},
//	{ "usai", "Substrate Activation", "S", "V", "K_sa", "K_sc", "V * sqr(S/K_sa)/(1 + S/K_sc + sqr(S/K_sa) + S/K_sa)"},
//	{ "ucii", "Competitive Inhibition (Irreversible)", "S", "V", "K_m", "K_i", "(V * S/K_m)/(1 + S/K_m + I/K_i)"},
//	{ "ucir", "Competitive Inhibition (Reversible)", "S", "P", "V_f", "V_r", "K_ms", "K_mp", "K_i",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + S/K_ms + P/K_mp + I/K_i)"},
//	{ "unii", "Noncompetitive Inhibition (Irreversible)", "S", "I", "V", "K_m", "K_i", "(V*S/K_m)/(1 + I/K_i + (S/K_m)*(1 + I/K_i))"},
//	{ "unir", "Noncompetitive Inhibition (Reversible)", "S", "P", "I", "V_f", "K_ms", "K_mp", "K_i",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + I/K_i + (S/K_ms + P/K_mp )*(1 + I/K_i))"},
//	{ "uuci", "Uncompetitive Inhibition (Irreversible)", "S", "I", "V", "K_m", "K_i", "(V*S/K_m)/(1 + (S/K_m)*(1 + I/K_i))"},
//	{ "uucr", "Uncompetitive Inhibition (Reversible)", "S", "P", "I", "V_f", "V_r", "K_ms", "K_mp", "K_i",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + ( S/K_ms + P/K_mp )*( 1 + I/K_i))"},
//	{ "umi", "Mixed Inhibition Kinetics (Irreversible)", "S", "I", "V", "K_m", "K_is", "K_ic",
//	"(V*S/K_m)/(1 + I/K_is + (S/K_m)*(1 + I/K_ic))"},
//	{ "umr", "Mixed Inhibition Kinetics (Reversible)", "S", "P", "I", "V_f", "V_r", "K_ms", "K_mp", "K_is", "K_ic",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + I/K_is + ( S/K_ms + P/K_mp )*( 1 + I/K_ic ))"},
//	{ "uai", "Specific Activation Kinetics - irreversible", "S", "A_c", "V", "K_m", "K_a", "(V*S/K_m)/(1 + S/K_m + K_a/A_c)"},
//	{ "uar", "Specific Activation Kinetics (Reversible)", "S", "P", "A_c", "V_f", "V_r", "K_ms", "K_mp", "K_a",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + S/K_ms + P/K_mp + K_a/A_c)"},
//	{ "ucti", "Catalytic Activation (Irreversible)", "S", "A_c", "V", "K_m", "K_a", "(V*S/K_m)/(1 + K_a/A_c + (S/K_m)*(1 + K_a/A_c))"},
//	{ "uctr", "Catalytic Activation (Reversible)", "S", "P", "A_c", "V_f", "V_r", "K_ms", "K_mp", "K_a",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + K_a/A_c + (S/K_ms + P/K_mp)*(1 + K_a/A_c))"},
//	{ "umai", "Mixed Activation Kinetics (Irreversible)", "S", "A_c", "V", "K_m", "Kas", "Kac",
//	"(V*S/K_m)/(1 + Kas/A_c + (S/K_m)*(1 + Kac/A_c))"},
//	{ "umar", "Mixed Activation Kinetics (Reversible)", "S", "P", "A_c", "V_f", "V_r", "K_ms", "K_mp", "K_as", "K_ac",
//	"(V_f*S/K_ms - V_r*P/K_mp)/(1 + K_as/A_c + (S/K_ms + P/K_mp)*(1 + K_ac/A_c))"},
//	{ "uhmi", "General Hyperbolic Modifier Kinetics (Irreversible)", "S", "M", "V", "K_m", "K_d", "a", "b",
//	"(V*(S/K_m)*(1 + b * M / (a*K_d)))/(1 + M/K_d + (S/K_m)*(1 + M/(a*K_d)))"},
//	{ "uhmr", "General Hyperbolic Modifier Kinetics (Reversible)", "S", "P", "M", "V_f", "V_r", "K_ms", "K_mp", "K_d", "a", "b",
//	"((V_f*S/K_ms - V_r*P/K_mp)*(1 + b*M/(a*K_d)))/(1 + M/K_d + (S/K_ms + P/K_mp)*(1 + M/(a*K_d)))"},
//	{ "ualii", "Allosteric inhibition (Irreversible)", "S", "I", "V", "K_s", "K_ii", "n", "L",
//	"(V*pow(1 + S/K_s, n-1))/(L*pow(1 + I/K_ii,n) + pow(1 + S/K_s,n))"},
//	{ "ordubr", "Ordered Uni Bi Kinetics", "A", "P", "Q", "V_f", "V_r", "K_ma", "K_mq", "K_mp", "K_ip", "K_eq",
//	"(V_f*( A - P*Q/K_eq))/(K_ma + A*(1 + P/K_ip) + (V_f/(V_r*K_eq))*(K_mq*P + K_mp*Q + P*Q))"},
//	{ "ordbur", "Ordered Bi Uni Kinetics", "A", "B", "P", "V_f", "V_r", "K_ma", "Kmb", "K_mp", "K_ia", "K_eq",
//	"(V_f*(A*B - P/K_eq))/(A*B + K_ma*B + Kmb*A + (V_f/(V_r*K_eq))*(K_mp + P*(1 + A/K_ia)))"},
//	{ "ordbbr", "Ordered Bi Bi Kinetics", "A", "B", "P", "Q", "V_f", "K_ma", "K_mb", "K_mp", "K_ia", "K_ib", "K_ip", "K_eq",
//	"(V_f*(A*B - P*Q/K_eq))/(A*B*(1 + P/K_ip) + K_mb*(A + K_ia) + K_ma*B + ((V_f / (V_r*K_eq)) * (K_mq*P*( 1 + A/K_ia) + Q*(K_mp*( 1 + (K_ma*B)/(K_ia*K_mb) + P*(1 + B/K_ib))))))"},
//	{ "ppbr", "Ping Pong Bi Bi Kinetics", "A", "B", "P", "Q", "V_f", "V_r", "K_ma", "K_mb", "K_mp", "K_mq", "K_ia", "K_iq", "K_eq",
//	"(V_f*(A*B - P*Q/K_eq))/(A*B + K_mb*A + K_ma*B*(1 + Q/K_iq) + ((V_f/(V_r*K_eq))*(K_mq*P*(1 + A/K_ia) + Q*(K_mp + P))))"}};
//

} // extern "C"