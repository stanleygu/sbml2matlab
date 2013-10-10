/* Filename    : sbml2matlab.cpp
Copyright (c) 2006-2012, Ravi Rao, Frank Bergmann, Herbert Sauro and Stanley Gu
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the University of Washington nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL STANLEY GU BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "sbml2matlab.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <clocale>
#include <exception>

#ifdef WIN32
#ifndef CYGWIN
#define strdup _strdup
#endif
#endif

#define CONVERT_ANY(source,target)\
		{\
		std::stringstream oStream;\
		oStream << source;\
		oStream >> target;\
		}


#include "uScanner.h"
#include "NOM.h"

using namespace uScanner;
using namespace std;
// const string TRANSLATOR_NAME			= "matlabTranslator";
// const string TRANSLATOR_VERSION			= "3.00";
// const string TRANSLATOR_AUTHOR			= "Frank Bergmann, Sri Paladugu, Ravishankar Rao Vallabhajosyula and Herbert Sauro";
// const string TRANSLATOR_DESCRIPTION		= "Translates SBML to Matlab Function.";
// const string TRANSLATOR_DISPLAYNAME		= "Matlab Translator";
// const string TRANSLATOR_COPYRIGHT		= "BSD";
// const string TRANSLATOR_URL				= "www.sys-bio.org";

typedef struct {
	string name;
	double value;
} TNameValue;

typedef struct {
	char *fnId;
	int numArgs;
	char **argList;
	char *body;
} TUserFuncInfo;


// Define a data structure to hold all information required
// for each species in the network
class spAttributes 
{
public:
	string	name;
	string	id;
	bool	boundary;
	bool	is_amount;
	double	init_conc;
	double	init_amount;
	string	compartment;
	double	compartment_vol;		
};

class NameValue
{
public:
	string name;
	double value;
};

class IdNameValue
{
public: 
	string id;
	string name;
	double value;
};


class MatlabError
	: public std::exception
{
public:
	MatlabError (const string& msg) : msg_(msg)
	{ }

	virtual ~MatlabError() throw()
	{ }

	string getMessage( ) const {return(msg_);}
private:
	string msg_;
};


class TReactionInfo
{
public: 

	string id;
	string name;
	bool isReversible;
	string rateLaw;
	int iIsReve;

	vector<TNameValue> reactants;
	vector<TNameValue> products;
	vector<TNameValue> parameters;

	TReactionInfo (int reactionIndex)
	{
		char *cId;

		getNthReactionId (reactionIndex, &cId);
		id = cId; 
		getNthReactionName (reactionIndex, &cId);
		name = cId;
		isReactionReversible (reactionIndex, &iIsReve);
		isReversible = (bool) iIsReve;
		getKineticLaw (reactionIndex, &cId);
		rateLaw = cId;
		int numOfReactants; int numOfProducts; 
		int numParameters;  double value;

		numOfReactants = getNumReactants (reactionIndex);
		for (int i=0; i<numOfReactants; i++) {
			TNameValue reactant;
			getNthReactantName (reactionIndex, i, &cId);
			reactant.name = cId;
			reactant.value = getNthReactantStoichiometry (reactionIndex, i);
			reactants.push_back(reactant);
		}

		numOfProducts = getNumProducts (reactionIndex);
		for (int i=0; i<numOfProducts; i++) {
			TNameValue reactant;
			getNthProductName (reactionIndex, i, &cId);
			reactant.name = cId;
			reactant.value = getNthProductStoichiometry (reactionIndex, i);
			products.push_back(reactant);
		}

		numParameters = getNumLocalParameters (reactionIndex);
		for (int i=0; i<numParameters; i++) {
			TNameValue parameter;
			getNthLocalParameterId (reactionIndex, i, &cId);
			parameter.name = cId;
			getNthLocalParameterValue (reactionIndex, i, &value);
			parameter.value = value;
			parameters.push_back(parameter);
		}
	}
};


class SBMLInfo
{
public: 

	SBMLInfo(const string& sbmlString)
	{
		std::string str;
		char *cstr;
		char *cstr_sbml;

		cstr_sbml = (char *) sbmlString.c_str(); 
		loadSBML(cstr_sbml);

		getModelId(&cstr);
		modelName = cstr;
		if (modelName == "")
			modelName = "ExportedModel";

		numFloatingSpecies = getNumFloatingSpecies();
		numBoundarySpecies = getNumBoundarySpecies();

		ReadCompartments();
		ReadGlobalParameters();
		ReadUserDefinedFunctions();
		ReadRules();
		ReadReactions();		
		ReadSpecies();
	}

	~SBMLInfo()
	{
		delete[] sp_list;
	}



	void ReadUserDefinedFunctions()
	{
		numUserDefinedFunctions = getNumFunctionDefinitions();
		char* fnId; int numArgs; char** argList; char* body;

		for (int i = 0; i < numUserDefinedFunctions; i++)
		{
			getNthFunctionDefinition(i, &fnId, &numArgs, &argList, &body);
			TUserFuncInfo *userStruct = (TUserFuncInfo *) malloc (sizeof (TUserFuncInfo));

			userStruct->fnId = fnId;
			userStruct->numArgs = numArgs;
			userStruct->argList = argList;
			userStruct->body = body;
			userDefinedFunctions.push_back (userStruct);
		}
	}

	void ReadRules()
	{
		numRules = getNumRules();

		char *cstr;
		int ruleType;

		for (int i = 0; i < numRules; i++)
		{
			getNthRule (i, &cstr, &ruleType);
			ruleTypes.push_back (ruleType);
			rules.push_back (cstr);
		}
	}

	void ReadReactions()
	{
		numReactions = getNumReactions();
		for (int i = 0; i < numReactions; i++)
		{
			reactions.push_back(TReactionInfo(i));
		}
	}

	void ReadCompartments()
	{
		char *cstr;

		numCompartments = getNumCompartments();
		for (int i = 0; i < numCompartments; i++)
		{
			IdNameValue compartment;

			getNthCompartmentId(i, &cstr);
			compartment.id = cstr;
			getNthCompartmentName(i, &cstr);
			compartment.name = cstr;
			cstr = (char*) compartment.id.c_str();
			getValue(cstr, &compartment.value);

			compartments.push_back(compartment);
			compartmentsList[compartment.id] = compartment.value;
		}
	}

	void ReadGlobalParameters()
	{
		char * cstr;

		numGlobalParameters = getNumGlobalParameters();
		for (int i = 0; i < numGlobalParameters; i++)
		{
			NameValue parameter;
			getNthGlobalParameterId(i, &cstr);
			parameter.name = cstr;

			cstr = (char*) parameter.name.c_str();
			getValue(cstr, &parameter.value);
			globalParameters.push_back(parameter);

			globalParametersList[parameter.name] = parameter.value;
			globalParamIndexList[parameter.name] = (i+1);
		}
	}
	// Method that takes the sbml string, and creates one spAttributes object
	// for each species encapsulating all available information. This includes
	// the following
	// - species name          string
	// - species id            string
	// - boundaryCondition     bool
	// - initialConcentration  double
	// - initialAmount         double
	// - compartment           string
	// - compartmentVolume     double
	void ReadSpecies() 
	{
		char *cstr;

		int numTotalSpecies = numFloatingSpecies + numBoundarySpecies;
		sp_list = new spAttributes[numTotalSpecies];

		for (int i=0; i<numFloatingSpecies; i++) 
		{
			getNthFloatingSpeciesId (i, &cstr);
			sp_list[i].id = cstr;

			double value; 
			getValue (cstr, &value);
			bool isConcentration;
			bool isAmount;

			getNthFloatingSpeciesName(i, &cstr);
			sp_list[i].name = cstr;
			getCompartmentIdBySpeciesId((char *) sp_list[i].id.c_str(), &cstr);  
			sp_list[i].compartment = cstr;
			sp_list[i].compartment_vol = compartmentsList[sp_list[i].compartment];
			sp_list[i].boundary = false;
			hasInitialAmount((char *) sp_list[i].name.c_str(), &isAmount);
			isConcentration = !isAmount;


			if (isConcentration)
			{
				sp_list[i].is_amount = false;
				sp_list[i].init_conc = value;
				sp_list[i].init_amount = value*sp_list[i].compartment_vol;
			}
			else 
			{
				sp_list[i].is_amount = true;
				sp_list[i].init_conc = value/sp_list[i].compartment_vol;
				sp_list[i].init_amount = value;
			}
		}

		for (int i=0; i<numBoundarySpecies; i++) 
		{

			int index = i + numFloatingSpecies;

			getNthBoundarySpeciesId (i, &cstr);
			sp_list[index].id = cstr;

			double value; 
			getValue (cstr, &value);	
			bool isConcentration; 
			bool isAmount; 
			hasInitialAmount(cstr, &isAmount);
			isConcentration = !isAmount;

			getNthBoundarySpeciesName(i, &cstr);
			sp_list[index].name = cstr;
			getCompartmentIdBySpeciesId((char *) sp_list[index].id.c_str(), &cstr);
			sp_list[index].compartment = cstr;
			sp_list[index].compartment_vol = compartmentsList[sp_list[index].compartment];
			sp_list[index].boundary = true;

			if (!hasInitialAmount((char *) sp_list[i].name.c_str(), &isConcentration))
			{
				sp_list[index].is_amount = false;
				sp_list[index].init_conc = value;
				sp_list[index].init_amount = value*sp_list[index].compartment_vol;
			}
			else 
			{
				sp_list[index].is_amount = true;
				sp_list[index].init_conc = value/sp_list[index].compartment_vol;
				sp_list[index].init_amount = value;
			}
		}
	}


	string modelName;

	int									numFloatingSpecies;
	int									numReactions;
	int									numBoundarySpecies;
	int									numParameters;
	int									numGlobalParameters;
	int									numCompartments;
	int									numRules;
	int									numUserDefinedFunctions;

	map<string, double>                 compartmentsList;
	map<string, double>                 localParameterList;
	map<string, double>                 allLocalParametersList;
	map<string, double>                 globalParametersList;
	map<string, int>                    globalParamIndexList;
	map<map<string, double>, int>       nthReactionParameters;
	map<string, string>					parameterMapList;
	map<string, double>::const_iterator iterator;


	spAttributes*						sp_list;

	vector<string>						rules;
	vector<int>							ruleTypes;
	vector<TUserFuncInfo*>				userDefinedFunctions;
	vector<TReactionInfo>				reactions;  
	vector<IdNameValue>					compartments;
	vector<NameValue>					globalParameters;

};

/*
* MatlabTranslator
* this class provides an implementation of the translator service
*/
class MatlabTranslator
{
private:	

	string								sbml, eqn, stoich, pname;
	double								pvalue;
	SBMLInfo*							_currentModel;

	const static string					NL;
	bool                                _bInlineMode;


	// deal with all strings, which could be: 
	// - global parameter (under which we also list boundary species)
	// - floating species
	// - compartment volumes
	// - local parameters
	// - function names
	// TODO: add flux names!!!
	string ReplaceStringToken(const string& currentToken, const string &reactionId, bool divideVolumes = true)
	{
		stringstream replaceStream;

		string innerString(stringInside(currentToken));		
		string localParameterId = reactionId + "_" + innerString;

		if ( _currentModel->globalParamIndexList.find ( innerString ) != _currentModel->globalParamIndexList.end() )
		{
			bool isBoundarySpecies = false;

			for (int ib=0; ib< _currentModel->numBoundarySpecies; ib++)
			{
				if (innerString == _currentModel->sp_list[ib + _currentModel->numFloatingSpecies].id) 
				{
					if (divideVolumes)
						replaceStream << "(";

					if (_bInlineMode)
					{
						replaceStream << _currentModel->globalParametersList[innerString];
					}
					else
					{						
						replaceStream << "rInfo.g_p" << _currentModel->globalParamIndexList[innerString];
					}

					if (divideVolumes)
					{
						if (_currentModel->compartmentsList[_currentModel->sp_list[ib + _currentModel->numFloatingSpecies].compartment] != 1.0)
							replaceStream << "/vol__" << _currentModel->sp_list[ib + _currentModel->numFloatingSpecies].compartment;

						replaceStream << ")";
					}

					isBoundarySpecies = true;
					break;
				}
			}

			if (isBoundarySpecies == false) {

				if (_bInlineMode)
				{					
					replaceStream << _currentModel->globalParametersList[innerString];
				}
				else
				{
					replaceStream << "rInfo.g_p" << _currentModel->globalParamIndexList[innerString];
				}

			}
		}
		else if ( _currentModel->parameterMapList.find ( localParameterId ) != _currentModel->parameterMapList.end() )
		{
			replaceStream << _currentModel->parameterMapList[localParameterId];
		}
		else if ( _currentModel->compartmentsList.find ( innerString ) != _currentModel->compartmentsList.end() )
		{
			replaceStream << "vol__" << innerString;
		}
		else
		{			
			bool match = false;
			for (int isp=0; isp<_currentModel->numFloatingSpecies; isp++)
			{

				string speciesId = _currentModel->sp_list[isp].id;
				if(speciesId == innerString)
				{

					string compartment = "vol__" + _currentModel->sp_list[isp].compartment;
					bool isUnitVolume = _currentModel->compartmentsList[_currentModel->sp_list[isp].compartment] == 1.0;
					if (divideVolumes)
						replaceStream << "(";

					replaceStream << "x(" << (isp+1) << ")";

					if (divideVolumes)
					{
						if (!isUnitVolume)
							replaceStream << "/" << compartment ;

						replaceStream << ")";
					}
					match = true;
					break;
				}

			}

			if (!match )
			{
				if (innerString == "exponentiale")
					replaceStream << "exp(1)";
				else if (innerString == "INF")
					replaceStream << "Inf";
				else if (innerString == "arcsin")
					replaceStream << "asin";
				else if (innerString == "arccos")
					replaceStream << "acos";
				else if (innerString == "arctan")
					replaceStream << "atan";
				else if (innerString == "arcsec")
					replaceStream << "asec";
				else if (innerString == "arccsc")
					replaceStream << "acsc";
				else if (innerString == "arccot")
					replaceStream << "acot";
				else if (innerString == "arcsinh")
					replaceStream << "asinh";
				else if (innerString == "arccosh")
					replaceStream << "acosh";
				else if (innerString == "arctanh")
					replaceStream << "atanh";
				else if (innerString == "arcsech")
					replaceStream << "asech";
				else if (innerString == "arccsch")
					replaceStream << "acsch";
				else if (innerString == "arccoth")
					replaceStream << "acoth";

				else 
					replaceStream << innerString;
			}
		}

		return replaceStream.str();
	}

	// replace a variable column, for example produces x(:,4) instead of x(4), called by subConstantsCol
	string ReplaceStringTokenCol(const string& currentToken, const string &reactionId, bool divideVolumes = true)
	{
		stringstream replaceStream;

		string innerString(stringInside(currentToken));		
		string localParameterId = reactionId + "_" + innerString;

		if ( _currentModel->globalParamIndexList.find ( innerString ) != _currentModel->globalParamIndexList.end() )
		{
			bool isBoundarySpecies = false;

			for (int ib=0; ib< _currentModel->numBoundarySpecies; ib++)
			{
				if (innerString == _currentModel->sp_list[ib + _currentModel->numFloatingSpecies].id) 
				{
					if (divideVolumes)
						replaceStream << "(";

					if (_bInlineMode)
					{
						replaceStream << _currentModel->globalParametersList[innerString];
					}
					else
					{						
						replaceStream << "rInfo.g_p" << _currentModel->globalParamIndexList[innerString];
					}

					if (divideVolumes)
					{
						if (_currentModel->compartmentsList[_currentModel->sp_list[ib + _currentModel->numFloatingSpecies].compartment] != 1.0)
							replaceStream << "/vol__" << _currentModel->sp_list[ib + _currentModel->numFloatingSpecies].compartment;

						replaceStream << ")";
					}

					isBoundarySpecies = true;
					break;
				}
			}

			if (isBoundarySpecies == false) {

				if (_bInlineMode)
				{					
					replaceStream << _currentModel->globalParametersList[innerString];
				}
				else
				{
					replaceStream << "rInfo.g_p" << _currentModel->globalParamIndexList[innerString];
				}

			}
		}
		else if ( _currentModel->parameterMapList.find ( localParameterId ) != _currentModel->parameterMapList.end() )
		{
			replaceStream << _currentModel->parameterMapList[localParameterId];
		}
		else if ( _currentModel->compartmentsList.find ( innerString ) != _currentModel->compartmentsList.end() )
		{
			replaceStream << "vol__" << innerString;
		}
		else
		{			
			bool match = false;
			for (int isp=0; isp<_currentModel->numFloatingSpecies; isp++)
			{

				string speciesId = _currentModel->sp_list[isp].id;
				if(speciesId == innerString)
				{

					string compartment = "vol__" + _currentModel->sp_list[isp].compartment;
					bool isUnitVolume = _currentModel->compartmentsList[_currentModel->sp_list[isp].compartment] == 1.0;
					if (divideVolumes)
						replaceStream << "(";

					replaceStream << "x(:," << (isp+1) << ")";

					if (divideVolumes)
					{
						if (!isUnitVolume)
							replaceStream << "/" << compartment ;

						replaceStream << ")";
					}
					match = true;
					break;
				}

			}

			if (!match )
			{
				if (innerString == "exponentiale")
					replaceStream << "exp(1)";
				else if (innerString == "INF")
					replaceStream << "Inf";
				else if (innerString == "arcsin")
					replaceStream << "asin";
				else if (innerString == "arccos")
					replaceStream << "acos";
				else if (innerString == "arctan")
					replaceStream << "atan";
				else if (innerString == "arcsec")
					replaceStream << "asec";
				else if (innerString == "arccsc")
					replaceStream << "acsc";
				else if (innerString == "arccot")
					replaceStream << "acot";
				else if (innerString == "arcsinh")
					replaceStream << "asinh";
				else if (innerString == "arccosh")
					replaceStream << "acosh";
				else if (innerString == "arctanh")
					replaceStream << "atanh";
				else if (innerString == "arcsech")
					replaceStream << "asech";
				else if (innerString == "arccsch")
					replaceStream << "acsch";
				else if (innerString == "arccoth")
					replaceStream << "acoth";

				else 
					replaceStream << innerString;
			}
		}

		return replaceStream.str();
	}

	string subConstants(const string &equation, const string &reactionId, bool divideVolumes = true)
	{
		// set up the scanner
		stringstream ss(equation);

		TScanner scanner;
		scanner.setStream(&ss);
		scanner.startScanner();
		scanner.nextToken();

		stringstream result;

		try
		{
			while(scanner.getToken()!= tEndOfStreamToken)
			{

				switch ( scanner.getToken() )
				{
				case tWordToken :	
					{
						string currentToken = scanner.tokenToString ( scanner.getToken() );						
						result << ReplaceStringToken(currentToken,  reactionId, divideVolumes);
					}
					break;
				case tDoubleToken:
					result << scanner.tokenDouble;
					break;
				case tIntToken	 :  
					result << scanner.tokenInteger;
					break;
				case tPlusToken	 :	result << "+";
					break;
				case tMinusToken :  result << "-";
					break;
				case tDivToken	 :  result << "/";
					break;
				case tMultToken	 :	result << "*";
					break;
				case tPowerToken :	result << "^";
					break;
				case tLParenToken:	result << "(";
					break;
				case tRParenToken:	result << ")";
					break;
				case tCommaToken :	result << ",";
					break;
				default			 :  MatlabError* ae =
										new MatlabError("Unknown token in subConstants (matlabTranslator): " + scanner.tokenToString( scanner.getToken() ));
					throw ae;
				}
				scanner.nextToken();
			}
			result << ";";
		}
		catch (MatlabError ae)
		{
			throw ae;
		}
		return result.str();
	}

	// subConstants function for matlab columns, for example, produces x(:,4) instead of x(4)
	string subConstantsCol(const string &equation, const string &reactionId, bool divideVolumes = true)
	{
		// set up the scanner
		stringstream ss(equation);

		TScanner scanner;
		scanner.setStream(&ss);
		scanner.startScanner();
		scanner.nextToken();

		stringstream result;

		try
		{
			while(scanner.getToken()!= tEndOfStreamToken)
			{

				switch ( scanner.getToken() )
				{
				case tWordToken :	
					{
						string currentToken = scanner.tokenToString ( scanner.getToken() );						
						result << ReplaceStringTokenCol(currentToken,  reactionId, divideVolumes);
					}
					break;
				case tDoubleToken:
					result << scanner.tokenDouble;
					break;
				case tIntToken	 :  
					result << scanner.tokenInteger;
					break;
				case tPlusToken	 :	result << "+";
					break;
				case tMinusToken :  result << "-";
					break;
				case tDivToken	 :  result << "/";
					break;
				case tMultToken	 :	result << "*";
					break;
				case tPowerToken :	result << "^";
					break;
				case tLParenToken:	result << "(";
					break;
				case tRParenToken:	result << ")";
					break;
				case tCommaToken :	result << ",";
					break;
				default			 :  MatlabError* ae =
										new MatlabError("Unknown token in subConstants (matlabTranslator): " + scanner.tokenToString( scanner.getToken() ));
					throw ae;
				}
				scanner.nextToken();
			}
			result << ";";
		}
		catch (MatlabError ae)
		{
			throw ae;
		}
		return result.str();
	}

	// if expression is parenthesized, drop parentheses
	string stringInside(const string &str)
	{
		if (str == "" || str.length() < 2) return str;
		return str.substr(1, str.length()-2);
	}

	/**************************************/
public:
	///
	///MatlabTranslator Constructor
	MatlabTranslator(bool bInline = false) : _bInlineMode(bInline)
	{
	}

	// prints out the wrapper function for doing assignment and algebraic rules and solving the ode
	string PrintWrapper()
	{
		stringstream result;
		result << "function [t x rInfo] = " << _currentModel->modelName << "(tspan,solver,options)" << endl;
		result << "    % initial conditions" << endl;
		result << "    [x rInfo] = model();" << endl;

		result << endl << "    % initial assignments" << endl;

		result << endl << "    % assignment rules" << endl;
		if (_currentModel->numRules > 0)
		{
			for (int i = 0; i < _currentModel->numRules; i++)
			{
				string rule = _currentModel->rules[i];
				int ruleType = _currentModel->ruleTypes[i];
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					if (isFloatingSpecies(variable.substr(0, variable.length()-1)))
					{
						variable = subConstants(variable, iCouldCareLess, false);
						string equation = rule.substr(index + 1);
						equation = subConstants(equation, iCouldCareLess);
						if (ruleType == SBML_ASSIGNMENT_RULE)
						{
							result << "    " <<  variable.substr(0, variable.length()-1) << " = " << equation << endl;
						}
					}
				}
			}
		}

		result << endl << "    % run simulation" << endl;
		result << "    [t x] = feval(solver,@model,tspan,x,options);" << endl;

		result << endl << "    % assignment rules" << endl;

		if (_currentModel->numRules > 0)
		{
			for (int i = 0; i < _currentModel->numRules; i++)
			{
				string rule = _currentModel->rules[i];
				int ruleType = _currentModel->ruleTypes[i];
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					if (isFloatingSpecies(variable.substr(0, variable.length()-1)))
					{
						variable = subConstantsCol(variable, iCouldCareLess, false);
						string equation = rule.substr(index + 1);
						equation = subConstantsCol(equation, iCouldCareLess);
						if (ruleType == SBML_ASSIGNMENT_RULE)
						{
							result << "    " <<  variable.substr(0, variable.length()-1) << " = " << equation << endl;
						}
					}
				}
			}
		}

		result << endl << "function [xdot rInfo] = model(time,x)" << endl;


		return result.str();
	}

	/// prints the header information on how to use the matlab file
	string PrintHeader()
	{
		stringstream result; 
		result <<  "%  How to use:" << endl;
		result <<  "%" << endl;
		result <<  "%  " << _currentModel->modelName << " takes 3 inputs and returns 3 outputs." << endl;
		result <<  "%" << endl;
		result <<  "%  [t x rInfo] = " << _currentModel->modelName << "(tspan,solver,options)" << endl;
		result <<  "%  INPUTS: " << endl;
		result <<  "%  tspan - the time vector for the simulation. It can contain every time point, " << endl;
		result <<  "%  or just the start and end (e.g. [0 1 2 3] or [0 100])." << endl;
		result <<  "%  solver - the function handle for the odeN solver you wish to use (e.g. @ode23s)." << endl;
		result <<  "%  options - this is the options structure returned from the MATLAB odeset" << endl;
		result <<  "%  function used for setting tolerances and other parameters for the solver." << endl;
		result <<  "%  " << endl;
		result <<  "%  OUTPUTS: " << endl;
		result <<  "%  t - the time vector that corresponds with the solution. If tspan only contains" << endl;
		result <<  "%  the start and end times, t will contain points spaced out by the solver." << endl;
		result <<  "%  x - the simulation results." << endl;
		result <<  "%  rInfo - a structure containing information about the model. The fields" << endl; 
		result <<  "%  within rInfo are: " << endl;
		result <<  "%     stoich - the stoichiometry matrix of the model " << endl;
		result <<  "%     floatingSpecies - a cell array containing floating species name, initial" << endl;
		result <<  "%     value, and indicator of the units being inconcentration or amount" << endl;
		result <<  "%     compartments - a cell array containing compartment names and volumes" << endl;
		result <<  "%     params - a cell array containing parameter names and values" << endl;
		result <<  "%     boundarySpecies - a cell array containing boundary species name, initial" << endl;
		result <<  "%     value, and indicator of the units being inconcentration or amount" << endl;
		result <<  "%     rateRules - a cell array containing the names of variables used in a rate rule" << endl;
		result <<  "%" << endl;
		result <<  "%  Sample function call:" << endl;
		result <<  "%     options = odeset('RelTol',1e-12,'AbsTol',1e-9);" << endl;
		result <<  "%     [t x rInfo] = " << _currentModel->modelName << "(linspace(0,100,100),@ode23s,options);" << endl;
		result <<  "%" << endl;

		return result.str();
	}

	/// prints out the compartment information
	string PrintOutCompartments()
	{
		stringstream result;

		result << endl << "% List of Compartments " << endl;		

		for(int i = 0; i < _currentModel->numCompartments; i++)
		{
			result << "vol__" << _currentModel->compartments[i].id 
				<< " = " << _currentModel->compartments[i].value 
				<< ";\t\t%"  << _currentModel->compartments[i].name << endl;
		}

		return result.str();
	}


	// prints out the list of global parameters
	string PrintOutGlobalParameters()
	{
		stringstream result;		

		if (_currentModel->numGlobalParameters > 0) {
			result << endl << "% Global Parameters " << endl;
		}
		for(int i = 0; i < _currentModel->numGlobalParameters; i++)
		{			

			result <<  "rInfo.g_p" << (i+1) << " = " 
				<< _currentModel->globalParameters[i].value << ";\t\t% " 
				<< _currentModel->globalParameters[i].name << endl;


		}

		return result.str();
	}

	// prints out the boundary species
	string PrintOutBoundarySpecies()
	{
		stringstream result;

		if (_currentModel->numBoundarySpecies > 0) 
		{
			result << endl << "% Boundary Conditions " << endl;
		}

		for(int i = 0; i < _currentModel->numBoundarySpecies; i++)
		{
			int index = i+_currentModel->numFloatingSpecies;
			bool isAmount = _currentModel->sp_list[index].is_amount;
			string speciesId = _currentModel->sp_list[index].id;

			result <<  "rInfo.g_p" << (_currentModel->numGlobalParameters + i+1) << " = "; 			

			double value;
			if (isAmount == true)
			{
				value =  _currentModel->sp_list[index].init_amount;				
			}
			else
			{
				value = _currentModel->sp_list[index].init_conc;				
			}

			result << value << ";\t\t% " << speciesId <<  " = " << _currentModel->sp_list[index].name 
				<< (isAmount ? " [Amount]" : "[Concentration]")  << endl;

			_currentModel->globalParametersList[speciesId] = value;
			_currentModel->globalParamIndexList[speciesId] = (_currentModel->numGlobalParameters + i + 1);
		}

		return result.str();
	}


	// prints out local parameters
	string PrintLocalParameters()
	{
		stringstream result;
		char buffer[100];
		string strPvalue;
		if (_currentModel->numReactions > 0) 
		{
			result << endl << "% Local Parameters" << endl;
		}
		string r_name;
		string p_modname;
		for(int i = 0; i < _currentModel->numReactions; i++)
		{
			_currentModel->localParameterList.clear();
			string low_pname;
			// ############## TO DO
			// r_name =  _currentModel->reactions[i].id;
			// _currentModel->numParameters = _currentModel->reactions[i].parameters.size();


			for(int j = 0; j < _currentModel->numParameters; j++)
			{
				// ########## TO DO
				//pname  = _currentModel->reactions[i].parameters[j].name;
				//pvalue = _currentModel->reactions[i].parameters[j].value;

				sprintf(buffer, "%g", pvalue);
				strPvalue = buffer;

				p_modname = r_name + "_" + pname;
				string str_I_index;
				string str_J_index;
				string str_P_index;

				sprintf(buffer, "%d", (i+1));
				str_I_index = buffer;


				if (!(_currentModel->globalParametersList.find ( pname ) != _currentModel->globalParametersList.end()))
				{
					sprintf(buffer, "%d", (j+1));
					str_J_index = buffer;
					str_P_index = "par_r" + str_I_index + "_p" + str_J_index;

					result <<   str_P_index << " = " << strPvalue 
						<< ";\t\t% " << "[" << r_name << ", " << pname +"]" << endl;

					_currentModel->parameterMapList[p_modname] = str_P_index;
					_currentModel->localParameterList[pname] = pvalue;
					_currentModel->allLocalParametersList[p_modname] = pvalue;
				}
			}
			_currentModel->nthReactionParameters[_currentModel->localParameterList] = i;
		}

		return result.str();
	}

	// prints an overview of floating species
	string PrintSpeciesOverview()
	{
		stringstream result;
		string floatingSpeciesName;
		for(int i = 0; i < _currentModel->numFloatingSpecies; i++)
		{			
			result <<  "%  x(" << (i+1) <<  ")        " << _currentModel->sp_list[i].id << endl;
		}

		//determining the number of rate rules to add to the initialization
		int numRateRules = 0;
		for (int i = 0; i < _currentModel->numRules; i++)
		{
			int ruleType = _currentModel->ruleTypes[i];
			if (ruleType == SBML_RATE_RULE)
			{
				numRateRules++;
			}
		}
		//result << endl << "xdot = zeros(" << _currentModel->numFloatingSpecies + numRateRules << ", 1);" << endl;

		return result.str();
	}

	// prints out the initial conditions and reaction info
	string PrintInitialConditions()
	{
		stringstream result;
		char buffer[100];
		// Print out Initial Conditions
		string strPvalue;
		result << endl <<  "if (nargin == 0)" << endl << endl;
		result << "    % set initial conditions" << endl;

		double value;
		string strValue;
		string strFloatingSpeciesIndex;
		string floatingSpeciesName;
		string low_floatingSpName;
		string low_fspname;
		string boundarySpeciesName;
		int initCondIndex = 1; //index of current initial condition
		for (int i = 0; i < _currentModel->numFloatingSpecies; i++)
		{

			floatingSpeciesName = _currentModel->sp_list[i].id;
			string bnd_data;
			if (_currentModel->sp_list[i].is_amount == true)
			{
				value = _currentModel->sp_list[i].init_amount;
				bnd_data = " [Amount]";

				sprintf( buffer, "%g", value );
				strValue = buffer;
			}
			else
			{
				value = _currentModel->sp_list[i].init_conc;
				bnd_data = " [Concentration]";

				sprintf( buffer, "%g", value );
				strValue = buffer;

				strValue = strValue + "*vol__" + _currentModel->sp_list[i].compartment;
			}

			sprintf( buffer, "%d", i+1 );
			strFloatingSpeciesIndex = buffer;

			result <<  "   xdot(" << strFloatingSpeciesIndex << ") = " 
				<<  strValue  <<  ";\t\t% " << floatingSpeciesName 
				<< " = " <<  _currentModel->sp_list[i].name << bnd_data << endl;
			initCondIndex++;
		}

		// adding initial conditions for rate rules
		int numRateRules = 0;
		for (int i = 0; i < _currentModel->numRules; i++) //adding initial condition for rate rule on a non-species
		{
			string rule = _currentModel->rules[i];
			int ruleType = _currentModel->ruleTypes[i];
			if (ruleType == SBML_RATE_RULE) // if rate rule, then promote parameter into ode (X)
			{
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					if (!isFloatingSpecies(variable.substr(0, variable.length()-1)))
					{
						variable = subConstants(variable, iCouldCareLess, false);
						string equation = rule.substr(index + 1);
						equation = subConstants(equation, iCouldCareLess);
						result << "   " << "xdot(" << initCondIndex << ")" << " = " << variable.substr(0, variable.length()-1) << ";" << endl;
						initCondIndex ++;
					}
				}
			}
		}



		// boundary conditions initial conditions - not used?
		for (int i = 0; i < _currentModel->numBoundarySpecies; i++)
		{
			int ip = i + _currentModel->numFloatingSpecies;

			boundarySpeciesName = _currentModel->sp_list[ip].name;
			if (_currentModel->sp_list[i].is_amount == true)
				value = _currentModel->sp_list[i].init_amount;
			else
				value = _currentModel->sp_list[i].init_conc;

			sprintf( buffer, "%g", value );
			strValue = buffer;
		}

		result << PrintOutModel();



		result << endl <<  "else" << endl;
		return result.str();
	}

	string PrintOutModel()
	{
		stringstream result;
		char buffer[100];
		// Printing out stoichiometry matrix
		string strIndex;
		result << endl << "   % reaction info structure";
		result << endl << "   rInfo.stoich = [" << endl;

		for (int i = 0; i < _currentModel->numFloatingSpecies; i++)
		{
			eqn = "     ";

			string floatingSpeciesName;
			floatingSpeciesName = _currentModel->sp_list[i].id;

			for (int j = 0; j < _currentModel->numReactions; j++)
			{
				int				numProducts = _currentModel->reactions[j].products.size();				
				string			productName;
				double			productStoichiometry = 0;
				double			reactantStoichiometry = 0;

				for(int k1 = 0; k1 < numProducts; k1++)
				{
					productName.clear();
					productName = _currentModel->reactions[j].products[k1].name;

					if (floatingSpeciesName == productName)
					{
						productStoichiometry = productStoichiometry + _currentModel->reactions[j].products[k1].value;

					}
				}

				int				numReactants;
				numReactants = _currentModel->reactions[j].reactants.size();
				string			reactantName;
				for(int k1 = 0; k1 < numReactants; k1++)
				{
					reactantName.clear();
					reactantName = _currentModel->reactions[j].reactants[k1].name;
					if (floatingSpeciesName == reactantName)
					{
						reactantStoichiometry = reactantStoichiometry + _currentModel->reactions[j].reactants[k1].value;

					}
				}

				sprintf(buffer, "%g", productStoichiometry - reactantStoichiometry);
				eqn      = eqn + " " + buffer;
			}
			result << eqn << endl;
		}

		result <<  "   ];" << endl;


		// Printing out species names
		result << endl << "   rInfo.floatingSpecies = {" << "\t\t% Each row: [Species Name, Initial Value, isAmount (1 for amount, 0 for concentration)]" <<endl;

		for (int i = 0; i < _currentModel->numFloatingSpecies; i++)
		{

			bool isAmount = _currentModel->sp_list[i].is_amount;
			string speciesId = _currentModel->sp_list[i].id;

			result <<  "      '" << speciesId << "' , ";

			double value;
			int valAmount;
			if (isAmount == true)
			{
				value =  _currentModel->sp_list[i].init_amount;
				valAmount = 1;
			}
			else
			{
				value = _currentModel->sp_list[i].init_conc;
				valAmount = 0;
			}

			result << value << ", " << valAmount << endl;

		}
		result << "   };" << endl;

		// Printing out compartment names and volume
		result << endl << "   rInfo.compartments = {" << "\t\t% Each row: [Compartment Name, Value]" << endl;

		for(int i = 0; i < _currentModel->numCompartments; i++)
		{
			result << "      '" << _currentModel->compartments[i].id 
				<< "' , " << _currentModel->compartments[i].value
				<< endl;

		}
		result << "   };" << endl;

		// Printing out parameter names and value

		result << endl << "   rInfo.params = {" << "\t\t% Each row: [Parameter Name, Value]" << endl;

		for(int i = 0; i < _currentModel->numGlobalParameters; i++)
		{			

			result <<  "      '" << _currentModel->globalParameters[i].name << "' , ";
			result << _currentModel->globalParameters[i].value << endl;

		}
		result << "   };" << endl;


		// printing out boundary species

		result << endl << "   rInfo.boundarySpecies = {" << "\t\t% Each row: [Species Name, Initial Value, isAmount (1 for amount, 0 for concentration)]" << endl;
		for(int i = 0; i < _currentModel->numBoundarySpecies; i++)
		{
			int index = i+_currentModel->numFloatingSpecies;
			bool isAmount = _currentModel->sp_list[index].is_amount;
			string speciesId = _currentModel->sp_list[index].id;

			result <<  "      '" << speciesId << "' , ";

			double value;
			int valAmount;
			if (isAmount == true)
			{
				value =  _currentModel->sp_list[index].init_amount;
				valAmount = 1;
			}
			else
			{
				value = _currentModel->sp_list[index].init_conc;
				valAmount = 0;
			}

			result << value << ", " << valAmount << endl;
		}
		result << "   };" << endl;

		// Print out rate rule information
		result << endl << "   rInfo.rateRules = { \t\t % List of variables involved in a rate rule " << endl;
		for (int i = 0; i < _currentModel->numRules; i++) //adding initial condition for rate rule on a non-species
		{
			string rule = _currentModel->rules[i];
			int ruleType = _currentModel->ruleTypes[i];
			if (ruleType == SBML_RATE_RULE) // if rate rule, then promote parameter into ode (X)
			{
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					variable = subConstants(variable, iCouldCareLess, false);
					string equation = rule.substr(index + 1);
					equation = subConstants(equation, iCouldCareLess);
					/*result << "   '" << variable.substr(0, variable.length()-1) << "';" << endl;*/
					result << "   '" << rule.substr(0, index-1) << "';" << endl;
				}
			}
		}
		result << "   };" << endl;

		return result.str();
	}

	void stringReplace(string & str, string & oldStr, string & newStr)
	{
		size_t pos = 0;
		while((pos = str.find(oldStr, pos)) != string::npos)
		{
            // Do not replace if match is actually a substring of a different
    		// parameter, e.g. rInfo.g_p1 in rInfo.g_p10
			if (!isdigit((int) str.at(pos + oldStr.length())))
			{
				str.replace(pos, oldStr.length(), newStr);
				pos += newStr.length();
			}
			else
			{
				pos += oldStr.length();
			}
		}
	}


	// prints out the list of assignment rules
	string PrintOutRules()
	{
		stringstream result;

		if (_currentModel->numRules > 0)
		{
			result << endl << "    % listOfRules" << endl;

			for (int i = 0; i < _currentModel->numRules; i++)
			{
				string rule = _currentModel->rules[i];
				int ruleType = _currentModel->ruleTypes[i];
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					variable = subConstants(variable, iCouldCareLess, false);
					string equation = rule.substr(index + 1);
					equation = subConstants(equation, iCouldCareLess);
					//if (ruleType == SBML_ALGEBRAIC_RULE) // if an algebraic rule
					//{
					//	result << "   % algebraicRule " << endl;
					//	result << "   x = fsolve(@(x)(" << variable.substr(0, variable.length()-1) << "),x);" << endl;
					//	result << "   ALGEBRAIC RULE ERROR, NOT IN TRANSLATED FUNCTION " << endl;
					//}
					//else if ((ruleType != SBML_RATE_RULE) && (ruleType != SBML_ASSIGNMENT_RULE))
					//{
					result << "   " <<  variable.substr(0, variable.length()-1) << " = " << equation << endl;
					//}
				}
			}
		}

		return result.str();
	}

	// prints out user defined functions
	string PrintOutUserDefinedFunctions()
	{
		stringstream result;

		if (_currentModel->numUserDefinedFunctions > 0) 
		{
			result << endl << "% listOfUserDefinedFunctions" << endl;

			for (int i = 0; i < _currentModel->numUserDefinedFunctions; i++)
			{
				result << "function z = "<< _currentModel->userDefinedFunctions[i]->fnId << "(";
				for (int j=0; j < _currentModel->userDefinedFunctions[i]->numArgs; j++)
				{
					result << _currentModel->userDefinedFunctions[i]->argList[j];
					if (j < _currentModel->userDefinedFunctions[i]->numArgs-1)
					{
						result << ",";
					}
				}
				result << ")";
				result << endl;
				result << "    z = " << _currentModel->userDefinedFunctions[i]->body << ";" <<endl;
				result << endl;
			}
		}
		return result.str();
	}

	// prints out the events (not yet implemented)
	string PrintOutEvents()
	{
		string tstr;		

		// #####################
		//int numEvents = SBMLSupport::getNumEvents();

		//if (numEvents > 0)
		//{
		//	//tstr = tstr + "    % test event condition" + NL;
		//}

		return tstr;
	}

	// prints the calculation of the rates of change
	string PrintRatesOfChange()
	{
		stringstream result;

		result << endl <<  "    % calculate rates of change" << endl;

		for(int i = 0; i < _currentModel->numReactions; i++)
		{
			string kineticLaw = _currentModel->reactions[i].rateLaw;
			string reactionId = _currentModel->reactions[i].id;

			result << "   R" << i << " = " + (subConstants (kineticLaw, reactionId)) << endl;
		}

		return result.str();
	}


	// prints out the reaction scheme
	string PrintOutReactionScheme()
	{
		stringstream result;
		string strIndex;
		char buffer[100];
		result << endl << "   xdot = [" << endl;

		string floatingSpeciesName;
		int xdotIndex = 1;
		for (int i = 0; i < _currentModel->numFloatingSpecies; i++)
		{
			eqn = "     ";

			floatingSpeciesName.clear();
			floatingSpeciesName = _currentModel->sp_list[i].id;

			for (int j = 0; j < _currentModel->numReactions; j++)
			{
				int				numProducts = _currentModel->reactions[j].products.size();				

				string			productName;
				string			strStoichiometry;
				for(int k1 = 0; k1 < numProducts; k1++)
				{
					productName.clear();
					productName = _currentModel->reactions[j].products[k1].name;

					if (floatingSpeciesName == productName)
					{
						double			productStoichiometry;
						productStoichiometry = _currentModel->reactions[j].products[k1].value;

						if (productStoichiometry != 1)
						{
							sprintf(buffer, "%g", productStoichiometry);
							strStoichiometry = buffer;
							stoich			 = strStoichiometry + '*';
						}
						else
						{
							stoich   = "";
						}
						sprintf(buffer, "%d", j);
						strIndex = buffer;
						eqn      = eqn + " + " + stoich + "R" + strIndex;
					}
				}

				int				numReactants;
				numReactants = _currentModel->reactions[j].reactants.size();
				string			reactantName;
				for(int k1 = 0; k1 < numReactants; k1++)
				{
					reactantName.clear();
					reactantName = _currentModel->reactions[j].reactants[k1].name;
					if (floatingSpeciesName == reactantName)
					{
						double			reactantStoichiometry;
						reactantStoichiometry = _currentModel->reactions[j].reactants[k1].value;
						if (reactantStoichiometry != 1)
						{
							sprintf(buffer, "%g", reactantStoichiometry);
							strStoichiometry = buffer;
							stoich			 = strStoichiometry + "*";
						}
						else
						{
							stoich   = "";
						}
						sprintf(buffer, "%d", j);
						strIndex = buffer;
						eqn      = eqn + " - " + stoich + "R" + strIndex;
					}
				}
			}

			if (eqn == "     ") // add a rate rule reaction if defined for a floating species
			{
				for (int i = 0; i < _currentModel->numRules; i++)
				{

					string rule = _currentModel->rules[i];
					int ruleType = _currentModel->ruleTypes[i];
					if (ruleType == SBML_RATE_RULE)
					{
						// find equals sign ... split to get id translate id and combine ...
						size_t index = rule.find("=");
						string iCouldCareLess;
						if (index != string::npos)
						{
							string variable = rule.substr(0, index - 1);
							//variable = subConstants(variable, iCouldCareLess, false);
							string equation = rule.substr(index + 1);
							equation = subConstants(equation, iCouldCareLess);
							if (floatingSpeciesName == variable)
							{
								eqn = eqn + equation + "\t\t% From rate rule";
							}
						}
					}
				}
			}
			if (eqn == "     ") 
			{
				eqn =  eqn + "   0";
			}


			xdotIndex++;
			result << eqn << endl;
		}

		//// adding in reactions with parameters from rate rules
		//xdotIndex++;
		vector<string> paramsToConvert; // vector of rule parameters to convert to matlab x(n) variable and corresponding
		vector<string> convertParamsTo;
		vector<string> rulesToConvert;

		for (int i = 0; i < _currentModel->numRules; i++) //adding reaction for rate rule on a non-species
		{
			string rule = _currentModel->rules[i];
			int ruleType = _currentModel->ruleTypes[i];
			if (ruleType == SBML_RATE_RULE) // if rate rule, then promote parameter into ode (X)
			{
				// find equals sign ... split to get id translate id and combine ...
				size_t index = rule.find("=");
				string iCouldCareLess;
				if (index != string::npos)
				{
					string variable = rule.substr(0, index);
					string mVariable = subConstants(variable, iCouldCareLess, false);
					string equation = rule.substr(index + 1);
					equation = subConstants(equation, iCouldCareLess);


					//result << "   " << variable.substr(0, variable.length()-1) << " = " << equation << endl;
					stringstream ruleToConvertStream; // read in all the rules to convert to stringfo
					stringstream convertVarStream; // stringstream for assigning the appropriate matlab variable

					// create strings of rule and new parameter assignment
					string varToConvert = mVariable.substr(0, mVariable.length()-1);
					ruleToConvertStream << "   " << equation << endl;
					convertVarStream << "x(" << xdotIndex << ")";

					// making sure that the rule is not specifying a floating species, because adding a rate
					// rule for a floating species is done earlier
					if (!isFloatingSpecies(variable.substr(0, variable.length()-1)))
					{
						// adding rules and new parameter assignments to vectors
						paramsToConvert.push_back(varToConvert);
						rulesToConvert.push_back(ruleToConvertStream.str());
						convertParamsTo.push_back(convertVarStream.str());
						xdotIndex++;
					}
				}
			}
		}
		for (size_t i = 0; i < rulesToConvert.size(); i++)
		{
			for (size_t j = 0; j < paramsToConvert.size(); j++)
			{
				stringReplace(rulesToConvert[i], paramsToConvert[j], convertParamsTo[j]);
			}
			// check that rule is for non-species
			if (!isFloatingSpecies(paramsToConvert[i]))
			{
				result << rulesToConvert[i];
			}
		}



		//for (int i = 0; i < _currentModel->numRules; i++) //adding initial condition for rate rule on a non-species
		//{
		//	string rule = _currentModel->rules[i];
		//	int ruleType = _currentModel->ruleTypes[i];
		//	if (ruleType == SBML_RATE_RULE) // see if a rate rule specifies a non-species
		//	{
		//		// find equals sign ... split to get id translate id and combine ...
		//		size_t index = rule.find("=");
		//		string iCouldCareLess;
		//		if (index != string::npos)
		//		{
		//			string variable = rule.substr(0, index);
		//			variable.erase(variable.find_last_not_of(" \n\r\t")+1); //removing trailing spaces
		//			//variable = subConstants(variable, iCouldCareLess, false);
		//			bool ruleForFloatingSpecies = false;
		//			// check if variable is a floating species
		//			for (int j = 0; j< _currentModel->numFloatingSpecies; j++)
		//			{
		//				string jFloatingSpecies = _currentModel->sp_list[j].id;
		//				jFloatingSpecies.erase(jFloatingSpecies.find_last_not_of(" \n\r\t")+1);
		//				if (variable == jFloatingSpecies)
		//				{
		//					ruleForFloatingSpecies = true;
		//				}
		//			}
		//			if (!ruleForFloatingSpecies)
		//			{
		//				string equation = rule.substr(index + 1);
		//				equation = subConstants(equation, iCouldCareLess);


		//				result << "   " << equation << endl;
		//			}


		//		}
		//	}
		//}


		result <<  "   ];" << endl;
		result <<  "end;" << endl;
		result << endl << endl;
		return result.str();
	}

	bool isFloatingSpecies (string item)
	{
		for (int i = 0; i < _currentModel->numFloatingSpecies ; i++)
		{
			if (item == _currentModel->sp_list[i].id)
			{
				return true;
			}
		}
		return false;
	}


	// prints the list of supported functions
	string PrintSupportedFunctions()
	{
		stringstream result;

		result << PrintOutUserDefinedFunctions();

		result <<  "%listOfSupportedFunctions" << endl;

		result <<  "function z = pow (x,y) " << endl;
		result <<  "    z = x^y; " << endl;
		result <<  endl << endl;

		result <<  "function z = sqr (x) " << endl;
		result <<  "    z = x*x; " << endl;
		result <<  endl << endl;

		result <<  "function z = piecewise(varargin) " << endl;
		result <<  "		numArgs = nargin; " << endl;
		result <<  "		result = 0; " << endl;
		result <<  "		foundResult = 0; " << endl;
		result <<  "		for k=1:2: numArgs-1 " << endl;
		result <<  "			if varargin{k+1} == 1 " << endl;
		result <<  "				result = varargin{k}; " << endl;
		result <<  "				foundResult = 1; " << endl;
		result <<  "				break; " << endl;
		result <<  "			end " << endl;
		result <<  "		end " << endl;
		result <<  "		if foundResult == 0 " << endl;
		result <<  "			result = varargin{numArgs}; " << endl;
		result <<  "		end " << endl;
		result <<  "		z = result; " << endl;
		result <<  endl << endl;

		result <<  "function z = gt(a,b) " << endl;
		result <<  "   if a > b " << endl;
		result <<  "   	  z = 1; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 0; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = lt(a,b) " << endl;
		result <<  "   if a < b " << endl;
		result <<  "   	  z = 1; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 0; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = geq(a,b) " << endl;
		result <<  "   if a >= b " << endl;
		result <<  "   	  z = 1; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 0; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = leq(a,b) " << endl;
		result <<  "   if a <= b " << endl;
		result <<  "   	  z = 1; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 0; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = neq(a,b) " << endl;
		result <<  "   if a ~= b " << endl;
		result <<  "   	  z = 1; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 0; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = and(varargin) " << endl;
		result <<  "		result = 1;		 " << endl;
		result <<  "		for k=1:nargin " << endl;
		result <<  "		   if varargin{k} ~= 1 " << endl;
		result <<  "		      result = 0; " << endl;
		result <<  "		      break; " << endl;
		result <<  "		   end " << endl;
		result <<  "		end " << endl;
		result <<  "		z = result; " << endl;
		result <<  endl << endl;

		result <<  "function z = or(varargin) " << endl;
		result <<  "		result = 0;		 " << endl;
		result <<  "		for k=1:nargin " << endl;
		result <<  "		   if varargin{k} ~= 0 " << endl;
		result <<  "		      result = 1; " << endl;
		result <<  "		      break; " << endl;
		result <<  "		   end " << endl;
		result <<  "		end " << endl;
		result <<  "		z = result; " << endl;
		result <<  endl << endl;

		result <<  "function z = xor(varargin) " << endl;
		result <<  "		foundZero = 0; " << endl;
		result <<  "		foundOne = 0; " << endl;
		result <<  "		for k = 1:nargin " << endl;
		result <<  "			if varargin{k} == 0 " << endl;
		result <<  "			   foundZero = 1; " << endl;
		result <<  "			else " << endl;
		result <<  "			   foundOne = 1; " << endl;
		result <<  "			end " << endl;
		result <<  "		end " << endl;
		result <<  "		if foundZero && foundOne " << endl;
		result <<  "			z = 1; " << endl;
		result <<  "		else " << endl;
		result <<  "		  z = 0; " << endl;
		result <<  "		end " << endl;
		result <<  "		 " << endl;
		result <<  endl << endl;

		result <<  "function z = not(a) " << endl;
		result <<  "   if a == 1 " << endl;
		result <<  "   	  z = 0; " << endl;
		result <<  "   else " << endl;
		result <<  "      z = 1; " << endl;
		result <<  "   end " << endl;
		result <<  endl << endl;

		result <<  "function z = root(a,b) " << endl;
		result <<  "	z = a^(1/b); " << endl;
		result <<  " " << endl;


		return result.str();
	}

	string translate(const string &fileName)
	{
		string sbml, buffer;

		ifstream oFile (fileName.c_str());
		if (oFile.is_open())
		{
			while (!oFile.eof()) 
			{
				getline(oFile, buffer);
				sbml+=buffer;
			}
			oFile.close();
		} else {
			fprintf (stderr, "File could not be opened\n");
			exit (0);
		}
		return translateSBML(sbml);
	}

	// translates the given sbml string to a matlab string
	string translateSBML(const string &sbmlInput)
	{
		stringstream result;
		char * outSbml;
		string outSbml_str;
		if (validate((char *) sbmlInput.c_str())==-1)
		{
			fprintf (stderr, "Invalid SBML: %s\n", (char *) getError()); 
			exit (0);
		}


		getParamPromotedSBML((char *)sbmlInput.c_str(), &outSbml);
		reorderRules(&outSbml);
		outSbml_str = outSbml;
		_currentModel = new SBMLInfo(outSbml_str);

		result << PrintHeader();
		result << PrintWrapper();
		result << PrintSpeciesOverview();
		result << PrintOutCompartments();
		result << PrintOutGlobalParameters();
		result << PrintOutBoundarySpecies();
		//result << PrintLocalParameters(); a bug is caused in linux for BIOMD0000000006
		result << PrintInitialConditions();
		result << PrintOutRules();
		result << PrintOutEvents();
		result << PrintRatesOfChange();
		result << PrintOutReactionScheme();
		result << PrintSupportedFunctions();

		//delete _currentModel;

		return result.str();
	}



	// This method provides the implementation of getStoichiometryMatrix method
	/// Read input sbml string and construct a Stoichiometry Matrix.
	/*DataBlockWriter getStoichiometryMatrix(Module from, DataBlockReader reader)
	{
	bool			isFound = false;
	char			buffer[100];
	int				numReactions;
	int				numProducts;
	int				numReactants;
	double			productStoichiometry;
	double			reactantStoichiometry;
	int				numFloatingSpecies;
	string			sbmlInput;
	string			tstr;
	string			strIndex;
	string			eqn;
	string			reactionName;
	string			strStoichiometry;
	string			productName;
	string			reactantName;
	string			floatingSpeciesName;

	reader >> sbmlInput;
	SBMLSupport::loadSBML(sbmlInput);

	tstr = "function sm = getStoichiometryMatrix()" + NL;

	tstr		= tstr + "%  floating species List: (Rows)" + NL;

	numFloatingSpecies = SBMLSupport::getNumFloatingSpecies();


	for(int i = 0; i < numFloatingSpecies; i++)
	{
	floatingSpeciesName = SBMLSupport::getNthFloatingSpeciesId(i);
	sprintf( buffer, "%d", (i+1) );
	strIndex	= buffer;
	tstr		= tstr + "%  " + floatingSpeciesName + "      " + strIndex + NL;
	}

	tstr		= tstr + "%  reaction names List: (Columns)" + NL;
	numReactions = SBMLSupport::getNumReactions();

	for(int i = 0; i < numReactions; i++)
	{
	reactionName = SBMLSupport::getNthReactionId(i);

	sprintf( buffer, "%d", (i+1) );
	strIndex	= buffer;
	tstr		= tstr + "%  " + reactionName + "      " + strIndex + NL;
	}

	tstr = tstr + NL + "   sm	= [" + NL;

	for (int i = 0; i < numFloatingSpecies; i++)
	{
	eqn = "     ";
	floatingSpeciesName = SBMLSupport::getNthFloatingSpeciesId(i);

	for (int j = 0; j < numReactions; j++)
	{
	isFound = false;
	numProducts = SBMLSupport::getNumProducts(j);
	for(int k1 = 0; k1 < numProducts; k1++)
	{
	productName = SBMLSupport::getNthProductName(j, k1);					
	if (floatingSpeciesName == productName)
	{
	productStoichiometry = SBMLSupport::getNthProductStoichiometryDouble(j, k1);
	sprintf(buffer, "%g", productStoichiometry);
	strStoichiometry = buffer;
	eqn      = eqn + "  -" + strStoichiometry;
	isFound  = true;
	}
	}
	if (!isFound)
	{
	numReactants = SBMLSupport::getNumReactants(j);
	for(int k1 = 0; k1 < numReactants; k1++)
	{
	reactantName = SBMLSupport::getNthReactantName(j, k1);
	if (floatingSpeciesName == reactantName)
	{
	reactantStoichiometry = SBMLSupport::getNthReactantStoichiometryDouble(j, k1);
	sprintf(buffer, "%g", reactantStoichiometry);
	strStoichiometry = buffer;
	eqn      = eqn + "  " + strStoichiometry;
	isFound  = true;
	}
	}
	}
	if (!isFound)
	{
	eqn = eqn + "  0";
	}
	}

	if (i < numFloatingSpecies - 1)
	{
	tstr = tstr + eqn + ";" + NL;
	}
	else
	{
	tstr = tstr + eqn + NL;
	}
	}
	tstr = tstr + "   ];" + NL;

	return DataBlockWriter() << tstr;
	}*/


};
// #################
//#if defined(WIN32)
//const string MatlabTranslator::NL      = "\r\n";
//#else
//const string MatlabTranslator::NL      = "\n";
//#endif

DLL_EXPORT int sbml2matlab(char* sbmlInput, char** matlabOutput)
{
	try
	{
		MatlabTranslator translator(false);
		string translation = translator.translateSBML(sbmlInput);
		*matlabOutput = (char *) malloc((translation.length()+1)*sizeof(char));
		strcpy(*matlabOutput,(char *) translation.c_str());
	}
	catch (MatlabError *e)
	{
		fprintf(stderr, "MatlabTranslator exception: %s\n", e->getMessage().c_str());
		return -1;
	}
	return 0;
}

DLL_EXPORT void freeMatlabString(char* matlabInput)
{
	free(matlabInput);
}

DLL_EXPORT const char *getNomErrors()
{
	return getError();
}

DLL_EXPORT int getNumSbmlErrors()
{
	return getNumErrors();
}

DLL_EXPORT int getNthSbmlError (int index, int *line, int *column, int *errorId, char **errorType, char **errorMsg)
{
	return getNthError(index, line, column, errorId, errorType, errorMsg);
}

DLL_EXPORT int validateSBMLString (char *cSBML)
{
	int test = validateSBML(cSBML);
	return validateSBML(cSBML);
}

int main(int argc, char* argv[])
{
	// bool bInline = false;
	bool doTranslate = false;
	bool doWriteToFile = false;
	bool stdinInput = true;
	bool directSbml = false;
	char * matlabOutput;
	string infileName; 
	string outfileName;
	int success = 0;
	if (argc <= 1) { // called with no args, read sbml from stdin
		//printf ("sbml2matlab, -h for help, -v for version info\n");
		//exit (0);
		stringstream sbmlStream;
		string inputLine;
		while (cin)
		{
		getline(cin, inputLine);
		sbmlStream << inputLine;
		}
		success = sbml2matlab(strdup(sbmlStream.str().c_str()), &matlabOutput);
	} else { 
		try
		{
			setlocale(LC_ALL,"C");

			for (int i = 0; i < argc; i++)
			{
				string current(argv[i]);
				/*
				if (current ==  "--inline") 
				{
				bInline = true;
				break;
				}
				else if (current == "-input" && i + 1 < argc)
				*/
				if (current == "-input" && i + 1 < argc)
				{
					stdinInput = false;
					doTranslate = true;
					infileName = argv[i+1];
					i++;
				}
				else if (current == "-output" && i + 1 < argc)
				{
					outfileName = argv[i+1];
					doWriteToFile = true;
					i++;
				}
				else if (current == "-h") {
					fprintf (stdout, "To translate an sbml file use: -input sbml.xml [-output output.m]\n");
					stdinInput = false;
				}
				else if (current == "-v") {
					fprintf (stdout, "sbml2matlab version 1.1.1\n");
					stdinInput = false;
				}
				else if (i == 1) { // translate if sent as first param
					char * sbmlString = strdup(current.c_str());
					success = sbml2matlab(sbmlString, &matlabOutput);
					free(sbmlString);
				}
			}
		} 	
		catch (MatlabError *e)
		{
			fprintf(stderr, "MatlabTranslator exception: %s\n", e->getMessage().c_str());
			return -1;
		}

		// still should read input from command line
		if (stdinInput)
		{
			stringstream sbmlStream;
			string inputLine;
			//while (getline(cin, inputLine))
			while (cin)
			{
				getline(cin, inputLine);
				sbmlStream << inputLine;
			}
			success = sbml2matlab(strdup(sbmlStream.str().c_str()), &matlabOutput);
		}

		if (doTranslate) // translate from file and write to a file
		{
			MatlabTranslator translator(false);
			if (doWriteToFile) {
				ofstream out(outfileName.c_str());
				if (!out) { 
					cout << "Cannot open file. You may not have write-access to this location\n"; 
					return -1; 
				} 
				out << translator.translate(infileName) << endl;
				out.close();
			} 
			else {
				cout << translator.translate(infileName) << endl;
			}
		}
		else if (doWriteToFile) // already translated but write out to a file
		{
			ofstream out(outfileName.c_str());
			if (!out) { 
				cout << "Cannot open file. You may not have write-access to this location\n"; 
				return -1; 
			} 
			out << matlabOutput << endl;
			out.close();
		} 
		else // already translated string and send to stdout
		{
			cout << matlabOutput << endl;
		}
		//exit(0);
		return success;
	}
}