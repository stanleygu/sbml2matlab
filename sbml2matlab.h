/**
* @file sbml2matlab.h
* @author Stanley Gu <stanleygu@gmail.com>
* @date Februrary 16, 2012
* @brief SBML-to-MATLAB translator
*
*/

/* 
Copyright (c) 2012, Stanley Gu
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
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

using namespace std;
#ifdef WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C"
{
	/** @brief translates SBML to the MATLAB function equivalent
	*
	* @param[in] sbmlInput The SBML string to be translated
	* @param[in] matlabOutput Pointer to the C string to assign the translated MATLAB function
	*
	* @return 0 if translation was successful, -1 if not
	*/
	DLL_EXPORT int sbml2matlab(char* sbmlInput, char** matlabOutput);
	
	/** @brief Returns the error message from NOM 
	*
	* @return char* to the error message
	*/
	DLL_EXPORT char *getNomErrors();

	/** @brief Returns number of errors in SBML model
	*
	* @return -1 if there has been an error, otherwise returns number of errors in SBML model
	*/
	DLL_EXPORT int getNumSbmlErrors();

	/** @brief Returns details on the index^th SBML error
	*
	* @param[in] index The index^th error in the list
	* @param[out] line The line number in the SBML file that corresponds to the error
	* @param[out] column The column number in the SBML file that corresponds to the error
	* @param[out] errorId The SBML errorId (see libSBML for details);
	* @param[out] errorType The error type includes "Advisory", "Warning", "Fatal", "Error", and "Warning"
	* @param[out] errorMsg The error message associated with the error
	* @return -1 if there has been an error
	*/
	DLL_EXPORT int getNthSbmlError (int index, int *line, int *column, int *errorId, char **errorType, char **errorMsg);

	/** @brief Validates the given SBML model
	*
	* @return -1 if the SBML model is invalid, else returns 0
	*/
	DLL_EXPORT int validateSBMLString (char *cSBML);

}