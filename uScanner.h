//  **************************************************************************
//  *                                                                        *
//  *     u S C A N N E R  General purpose C++ tokenizer	                 *
//  *                                                                        *
//  *  Usage:                                                                *
//  *    TScanner sc = new TScanner();                                       *
//  *    sc.stream = <A stream>; // eg sc.stream = File.OpenRead (fileName); *
//  *    sc.startScanner();                                                  *
//  *    sc.nextToken();                                                     *
//  *    if (sc.token == TScanner.TTokenCodes.tEndOFStreamToken)             *
//  *       exit;                                                            *
//  *                                                                        *
//  *  Copyright (c) 2004 by Herbert Sauro                                   *
//  *  Licenced under the Artistic open source licence                       *
//  *                                                                        *
//  **************************************************************************

//using System;
//using System.IO;
//using System.Collections;
#include <istream>
#include <streambuf>
#include <map>
#include <cstring>
#include <string>
#include <cmath>

namespace uScanner {

    // Declare a Scanner exception type
    class EScannerException {
	public:
		std::string eMessage;

        EScannerException (std::string message) {
			eMessage = message;
        }
    };

    // Different types of recognised tokens
    enum TTokenCode
    {
        tEmptyToken,           tEndOfStreamToken, tIntToken,             tDoubleToken,          tComplexToken,
        tStringToken,          tWordToken,        tEolToken,             tSemiColonToken,       tCommaToken,
        tEqualsToken,          tPlusToken,        tMinusToken,           tMultToken,            tDivToken,
        tLParenToken,          tRParenToken,      tLBracToken,           tRBracToken,           tLCBracToken,
        tRCBracToken,          tOrToken,          tAndToken,             tNotToken,             tXorToken,
        tPowerToken,           tLessThanToken,    tLessThanOrEqualToken, tMoreThanToken,        tMoreThanOrEqualToken,
        tNotEqualToken,        tReversibleArrow,  tIrreversibleArrow,    tStartComment,         tInternalToken,
        tExternalToken,        tParameterToken,   tIfToken,              tWhileToken,           tModelToken,
        tEndToken
    };


/*    // This class is Experimental....
    public class tokenClass
    {
        private string description;
        public TTokenCode tokenCode;

        public override string ToString () { return description; }

        public tokenClass (TTokenCode token, string value)
        {
            this.tokenCode = token;
            this.description = value;
        }
    }*/

    // This is currently used to store the previous token and support simple look ahead
	class TToken
    {
	public:
        enum TTokenCode tokenCode;
        std::string          tokenString;
        int             tokenInteger;
        double          tokenDouble;
        double          tokenValue;  // Used to retrieve int or double

		TToken()
		{
			tokenCode    = tEmptyToken;
			tokenString  = "";
			tokenInteger = 0;
			tokenDouble  = 0.0;
			tokenValue   = 0.0;
		}
    };


    // -------------------------------------------------------------------
    //    Start of TScanner Class
    // -------------------------------------------------------------------
	
    class TScanner
    {
	private:
        const static char EOFCHAR;
        const static char CR;
//SRI        const static char LF;

        enum TCharCode {
            cLETTER,
            cDIGIT,
            cPOINT,
            cDOUBLEQUOTE,
            cUNDERSCORE,
            cSPECIAL,
            cWHITESPACE,
            cETX
        };

        // private variables
        TCharCode		   FCharTable[255];
        char			   buffer[255];              // Input buffer
        int				   bufferPtr;                // Index of position in buffer containing current char
        int				   bufferLength;
        int				   yylineno;                 // Current line number
        std::istream*      myFStream;
        TTokenCode		   ftoken;
        std::map<std::string, int>   wordTable;

        void initScanner()
        {
		     int ch;
		     for (ch=0; ch < 255; ch++)
		         FCharTable[ch] = cSPECIAL;
		     for (ch=(int)'0'; ch <= (int)'9'; ch++)
		         FCharTable[ch] = cDIGIT;
		     for (ch=(int)'A'; ch <= (int)'Z'; ch++)
		         FCharTable[ch] = cLETTER;
		     for (ch=(int)'a'; ch <= (int)'z'; ch++)
		         FCharTable[ch] = cLETTER;

		     FCharTable[(int)'.']        = cPOINT;
		     FCharTable[(int)'"']        = cDOUBLEQUOTE;
		     FCharTable[(int)'_']        = cUNDERSCORE;
		     FCharTable[(int)'\t']       = cWHITESPACE;
		     FCharTable[(int)' ']        = cWHITESPACE;
		     FCharTable[(int)EOFCHAR]    = cETX;

		     //wordTable["and"]       = tAndToken;
		     //wordTable["or"]        = tOrToken;
		     //wordTable["not"]       = tNotToken;
		     //wordTable["xor"]       = tXorToken;

		     wordTable["model"]     = tModelToken;
		     wordTable["if"]        = tIfToken;
		     wordTable["while"]     = tWhileToken;
		     wordTable["end"]       = tEndToken;
		     wordTable["internal"]  = tInternalToken;
		     wordTable["external"]  = tExternalToken;
		     wordTable["parameter"] = tParameterToken;
        }

        char getCharFromBuffer()
		{
			char ch;
			// If the buffer is empty, read a new chuck of text from the
			// input stream, this might be a stream or console
			if (bufferPtr == 0)
		    {
		        // Read a chunck of data from the input stream
				myFStream->read(buffer, 255);
				bufferLength = (int) myFStream->gcount();

		        if (bufferLength == 0)
		           return EOFCHAR;
		    }

		    ch = (char) buffer[bufferPtr];
		    bufferPtr++;
		    if (bufferPtr >= bufferLength)
		       bufferPtr = 0;  // Indicates the buffer is empty

			return ch;
		}

	   // -------------------------------------------------------------------
       // Scan for a word, words start with letter or underscore then continue
       // with letters, digits or underscore
       // -------------------------------------------------------------------

        void getWord()
        {
			std::string tempfch;
            while ((FCharTable[(int)fch] == cLETTER)
                || (FCharTable[(int)fch] == cDIGIT)
                || (FCharTable[(int)fch] == cUNDERSCORE))
            {
                   tokenString = tokenString + fch;  // Inefficient but convenient
                   nextChar();
            }

			if ( wordTable.find (tokenString) != wordTable.end() )
               ftoken = (TTokenCode) wordTable[tokenString];
            else
               ftoken = tWordToken;
        }

       // -------------------------------------------------------------------
       // Scan for a number, integer, double or complex
       // -------------------------------------------------------------------

        void getNumber()
        {
            const int MAX_DIGIT_COUNT = 3;  // Max number of digits in exponent

            int single_digit;
            double scale;
            double evalue;
            int exponent_sign;
            int digit_count;

            tokenInteger  = 0;
            tokenDouble   = 0.0;
            tokenScalar   = 0.0;
            evalue        = 0.0;
            exponent_sign = 1;

            // Assume first it's an integer
            ftoken = tIntToken;

            // Pick up number before any decimal place
            if (fch != '.')
               try
               {
                   do
                   {
                       single_digit = fch - '0';
                       tokenInteger = 10*tokenInteger + single_digit;
                       tokenScalar  = tokenInteger;
                       nextChar();
                   } while (FCharTable[(int)fch] == cDIGIT);
               }
               catch (...)
               {
                  throw new EScannerException ("Integer Overflow - constant value too large to read");
               }

            scale = 1;
            if (fch == '.')
            {
               // Then it's a float. Start collecting fractional part
               ftoken      = tDoubleToken;
               tokenDouble = tokenInteger;
               nextChar();
               if (FCharTable[(int)fch] != cDIGIT)
                  throw new EScannerException ("Syntax error: expecting number after decimal point");

               try
               {
                  while (FCharTable[(int)fch] == cDIGIT)
                  {
                      scale        = scale * 0.1;
                      single_digit = fch - '0';
                      tokenDouble  = tokenDouble + (single_digit * scale);
                      tokenScalar  = tokenDouble;
                      nextChar();
                  }
               }
               catch (...)
               {
                  throw new EScannerException ("Floating point overflow - constant value too large to read in");
               }
            }

            // Next check for scientific notation
            if ((fch == 'e') || (fch == 'E'))
            {
               // Then it's a float. Start collecting exponent part
               if (ftoken == tIntToken)
               {
                  ftoken      = tDoubleToken;
                  tokenDouble = tokenInteger;
                  tokenScalar = tokenInteger;
               }
               nextChar();
               if ((fch == '-') || (fch == '+'))
               {
                  if (fch == '-') exponent_sign = -1;
                  nextChar();
               }
               // accumulate exponent, check that first ch is a digit
               if (FCharTable[(int)fch] != cDIGIT)
                  throw new EScannerException ("Syntax error: number expected in exponent");

               digit_count = 0;
               try {
                  do {
                     digit_count++;
                     single_digit = fch - '0';
                     evalue       = 10*evalue + single_digit;
                     nextChar();
                  } while ((FCharTable[(int)fch] == cDIGIT) && (digit_count <= MAX_DIGIT_COUNT));
               }
               catch (...)
               {
                   throw new EScannerException ("Floating point overflow - Constant value too large to read");
               }

               if (digit_count > MAX_DIGIT_COUNT)
                  throw new EScannerException ("Syntax error: too many digits in exponent");

               evalue = evalue * exponent_sign;
               if (evalue > 300)
                  throw new EScannerException ("Exponent overflow while parsing floating point number");
               evalue      = pow (10.0, evalue);
               tokenDouble = tokenDouble * evalue;
               tokenScalar = tokenDouble;
            }

            // Check for complex number
           if ((fch == 'i') || (fch == 'j'))
           {
              if (ftoken == tIntToken)
                  tokenDouble = tokenInteger;
              ftoken = tComplexToken;
              nextChar();
           }
        }


        bool IsDoubleQuote (char ch) {
            if (FCharTable[(int)ch] == cDOUBLEQUOTE)
                return true;
            else
                return false;
        }

        // -------------------------------------------------------------------
        // Scan for string, "abc"
        // -------------------------------------------------------------------

        void getString() {
            bool OldIgnoreNewLines;
            tokenString = "";
            nextChar();

            ftoken = tStringToken;
            while (fch != EOFCHAR)
            {
                // Check for escape characters
                if (fch == '\\')
                {
                    nextChar();
                    switch (fch)
                    {
                        case '\\' : tokenString = tokenString + '\\';
                                    break;
                        case 'n'  : tokenString = tokenString + CR; 
                                    break;
//SRI                        case 'n'  : tokenString = tokenString + CR + LF;
//SRI                                    break;
                        case 'r'  : tokenString = tokenString + CR;
                                    break;
//SRI                        case 'f'  : tokenString = tokenString + LF;
//SRI                                    break;
                        case 't'  : tokenString = tokenString + "      ";
                                    break;
                        default:
                            throw new EScannerException ("Syntax error: Unrecognised control code in string");
                    }
                    nextChar();
                }
                else
                {
                    OldIgnoreNewLines = IgnoreNewLines;
                    if (IsDoubleQuote (fch))
                    {
                        // Just in case the double quote is at the end of a line and another string
                        // start immediately in the next line, if we ignore newlines we'll
                        // pick up a double quote rather than the end of a string
                        IgnoreNewLines = false;
                        nextChar();
                        if (IsDoubleQuote (fch))
                        {
                            tokenString = tokenString + fch;
                            nextChar();
                        }
                        else
                        {
                            if (OldIgnoreNewLines)
                            {
                                while (fch == CR)
                                {
                                    nextChar();
//SRI                                    while (fch == LF)
//SRI                                        nextChar();
                                }
                            }
                            IgnoreNewLines = OldIgnoreNewLines;
                            return;
                        }
                    }
                    else
                    {
                        tokenString = tokenString + fch;
                        nextChar();
                    }
                    IgnoreNewLines = OldIgnoreNewLines;
                }
            }
            if (fch == EOFCHAR)
               throw new EScannerException ("Syntax error: String without terminating quotation mark");
        }


        // -------------------------------------------------------------------
        // Scan for special characters
        // -------------------------------------------------------------------

        void getSpecial() {
            char tch;

            switch (fch) {
                case 13:   ftoken = tEolToken;
                           nextChar();
                           break;

                case ';':  ftoken = tSemiColonToken;
                           nextChar();
                           break;

                case ',':  ftoken = tCommaToken;
                           nextChar();
                           break;

                case '=':  nextChar();
                           if (fch == '>')
                           {
                              ftoken = tReversibleArrow;
                              nextChar();
                           }
                           else
                              ftoken = tEqualsToken;
                           break;

                case '+':  ftoken = tPlusToken;
                           nextChar();
                           break;

                case '-':  nextChar();
                           if (fch == '>')
                           {
                              ftoken = tIrreversibleArrow;
                              nextChar();
                           }
                           else
                             ftoken = tMinusToken;
                           break;

                case '*':  nextChar();
                           ftoken = tMultToken;
                           break;

                case '/':  // look ahead at next ch
                           tch = nextChar();
                           if (tch == '/')
                           {
                              ftoken = tStartComment;
                              nextChar();
                           }
                           else
                             ftoken = tDivToken;
                           break;

                case '(':  nextChar();
                           ftoken = tLParenToken;
                           break;

                case ')':  nextChar();
                           ftoken = tRParenToken;
                           break;

                case '[':  nextChar();
                           ftoken = tLBracToken;
                           break;

                case ']':  nextChar();
                           ftoken = tRBracToken;
                           break;

                case '{':  nextChar();
                           ftoken = tLCBracToken;
                           break;

                case '}':  nextChar();
                           ftoken = tRCBracToken;
                           break;

                case '^':  nextChar();
                           ftoken = tPowerToken;
                           break;

                case '<':  nextChar();
                           if (fch == '=')
                           {
                               ftoken = tLessThanOrEqualToken;
                               nextChar();
                           }
                           else
                               ftoken = tLessThanToken;
                           break;

                case '>':  nextChar();
                           if (fch == '=')
                           {
                              ftoken = tMoreThanOrEqualToken;
                              nextChar();
                           }
                           else
                              ftoken = tMoreThanToken;
                           break;

                case '!':  nextChar();
                           if (fch == '=')
                           {
                              ftoken = tNotEqualToken;
                              nextChar();
                           }
                           break;

            default:
				std::string message;
				message = "Syntax error: Unknown special token [" + fch;
				message = message + "]";
                throw new EScannerException (message);
            }
        }


        void nextTokenInternal() {
            // check if a token has been pushed back into the token stream, if so use it first
            if (previousToken->tokenCode != tEmptyToken) {
               ftoken                  = previousToken->tokenCode;
               tokenString             = previousToken->tokenString;
               tokenDouble             = previousToken->tokenDouble;
               tokenInteger            = previousToken->tokenInteger;
               previousToken->tokenCode = tEmptyToken;
               return;
            }

            skipBlanks();
            tokenString = "";

            switch (FCharTable[(int)fch]) {
                case cLETTER      :
                case cUNDERSCORE  : getWord();
                                              break;
                case cDIGIT       : getNumber();
                                              break;
                case cDOUBLEQUOTE : getString();
                                              break;
                case cETX         : ftoken = tEndOfStreamToken;
                                              break;
                default           : getSpecial();
                                              break;
            }
        }


        // Public Variables

	public:
        TToken* currentToken;
        TToken* previousToken;
		
        bool   IgnoreNewLines;

        std::string tokenString;
        int    tokenInteger;
        double tokenDouble;
        double tokenScalar;  // Used to retrieve int or double
        char   fch;

		// -------------------------------------------------------------------
        //      Constructor
        // -------------------------------------------------------------------
        TScanner()
        {
            //wordTable     = new Hashtable();
            previousToken = new TToken();
            currentToken  = new TToken();
			IgnoreNewLines  = true;
            initScanner();
        }

		// -------------------------------------------------------------------
        //      Destructor
        // -------------------------------------------------------------------
	/*	~TScanner()
		{
			delete previousToken;
			delete currentToken;
		}*/

        // Create a readonly property for the current line number
        int getLineNumber() {
                return yylineno;
        }

        // writeonly stream property
        void setStream(std::istream* inputStream) {
             //istream inputStream(sb);
			 myFStream = inputStream;
        }

        // readonly current token property
        TTokenCode getToken()  {
                return ftoken;
        }

        // Must be called before using nextToken()
		void startScanner() {
            yylineno = 1;
            bufferPtr = 0;
            nextChar();
        }

		// -------------------------------------------------------------------
        // Fetches next character from input stream and filters NL if required
        // -------------------------------------------------------------------

        char nextChar() {
            fch = getCharFromBuffer();
            if (IgnoreNewLines)
            {
                // Turn any CFs or LFs into space characters
                if (fch == CR) {
                    yylineno++;
                    fch = ' ';
                    return fch;
                }

//SRI                if (fch == LF)
//SRI                    fch = ' ';
//SRI                    return fch;
            }
            else
            {
                 if (fch == CR)
                    yylineno++;
            }
            return fch;
        }


        // -------------------------------------------------------------------
        // Skips any blanks, ie TAB, ' '
        // -------------------------------------------------------------------

        void skipBlanks() {
            while (FCharTable[(int)fch] == cWHITESPACE) {
//SRI                if ((fch == LF) || (fch == CR))
                if (fch == CR)
                    return;
                nextChar();
            }
        }

        // -------------------------------------------------------------------
        // Retrieve next token in stream, returns tEndOfStreamToken
        // if it reaches the end of the stream
        // -------------------------------------------------------------------

        void nextToken()
        {
            nextTokenInternal();
            while (ftoken == tStartComment)
            {
                // Comment ends with an end of line char
                while ((fch != CR) && (fch != EOFCHAR))
                      fch = getCharFromBuffer();
//SRI                while ((fch == LF) && (fch !=  EOFCHAR))
//SRI                      fch = getCharFromBuffer();
                while (fch == CR)
                {
                    yylineno++;
//SRI                    while (fch == LF)
//SRI                          nextChar();  // Dump the linefeed
                    fch = nextChar();
                }
                nextTokenInternal();  // get the real next token
            }
        }


        // -------------------------------------------------------------------
        // Allows one token look ahead
        // Push token back into token stream
        // -------------------------------------------------------------------

        void UnGetToken()
        {
            previousToken->tokenCode    = ftoken;
            previousToken->tokenString  = tokenString;
            previousToken->tokenInteger = tokenInteger;
            previousToken->tokenDouble  = tokenDouble;
        }

        // -------------------------------------------------------------------
        // Given a token, this function returns the string eqauivalent
        // -------------------------------------------------------------------

        std::string tokenToString (TTokenCode code) {
			std::string strtokenInteger;
			std::string strtokenDouble;
			char   buffer[100]; 

			switch (code) {
                case tIntToken               :     sprintf(buffer, "%d", tokenInteger);
												   strtokenInteger = buffer;
												   return "<Integer: " + strtokenInteger + ">";
											       break;
                case tDoubleToken            :     sprintf(buffer, "%lf", tokenDouble);
												   strtokenDouble = buffer;
												   return "<Double: "  + strtokenDouble + ">";  
					                               break;
                case tComplexToken           :     return "<Complex: " + strtokenDouble + "i>"; 
					                               break;
                case tStringToken            :     return "<String: "  + tokenString + ">";     
					                               break;
                case tWordToken              :     //return "<Identifier: " + tokenString + ">";
												   return "(" + tokenString + ")";  	
					                               break;
                case tEndOfStreamToken       :     return "<end of stream>";                    
					                               break;
                case tEolToken              :      return "<EOLN>";  
					                               break;
                case tSemiColonToken        :      return "<;>";     
					                               break;
                case tCommaToken            :      return "<,>";     
					                               break;
                case tEqualsToken           :      return "<=>";     
					                               break;
                case tPlusToken             :      return "<+>";     
					                               break;
                case tMinusToken            :      return "<->";     
					                               break;
                case tMultToken             :      return "<*>";     
					                               break;
                case tDivToken              :      return "</>";     
					                               break;
                case tPowerToken            :      return "<^>";
					                               break;
                case tLParenToken           :      return "<(>";
					                               break;
                case tRParenToken           :      return "<)>";
					                               break;
                case tLBracToken            :      return "<[>";
					                               break;
                case tRBracToken            :      return "<]>";
					                               break;
                case tLCBracToken           :      return "<{>";
					                               break;
                case tRCBracToken           :      return "<}>";
					                               break;
                /*case tOrToken               :      return "<or>";
					                               break;
                case tAndToken              :      return "<and>";
					                               break;
                case tNotToken              :      return "<not>";
					                               break;
                case tXorToken              :      return "<xor>";
					                               break;*/
                case tLessThanToken         :      return "[<]";
					                               break;
                case tLessThanOrEqualToken  :      return "[<=]";
					                               break;
                case tMoreThanToken         :      return "[>]";
					                               break;
                case tMoreThanOrEqualToken  :      return "[>=]";
					                               break;
                case tNotEqualToken         :      return "!=";
					                               break;
                case tReversibleArrow       :      return "[=>]";
					                               break;
                case tIrreversibleArrow     :      return "[->]";
					                               break;
                case tIfToken               :      return "<if>";
					                               break;
                case tWhileToken            :      return "<while>";
					                               break;
                case tModelToken            :      return "<model>";
					                               break;
                case tEndToken              :      return "<end>";
					                               break;
                case tInternalToken         :      return "<Internal>";
					                               break;
                case tExternalToken         :      return "<External>";
					                               break;
                case tParameterToken        :      return "<Parameter>";
                                                   break;

             default:
                return "<unknown>";
            }
        }
     };  // end of TScanner class
	 const char TScanner::EOFCHAR = '\x7F';  // Deemed end of string marker, used internally
	 const char TScanner::CR      = (char)10;
//SRI	 const char TScanner::CR      = (char)13;
//SRI     const char TScanner::LF      = (char)10;

}  // end of uScanner namespace