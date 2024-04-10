//
// Created by 99065 on 2024/3/25.
//
#include "laxerAST.h"

#include <string.h>

using std::string;

string filestring;
size_t flag = 0;    //flag为遍历filestring元素时的索引
string IdentifierStr1;  // Filled in if tok_identifier
double NumVal1;              // Filled in if tok_number
int CurTokForStr;                 // CurTok is the current token the parser is looking at.
// BinopPrecedence - This holds the precedence for each binary operator that is defined.
std::map<char, int> BinopPrecedenceForStr;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

char getCharFromStr()
{
    if(flag < filestring.size())
    {
        char c = filestring.at(flag++);
        return c;
    }
    //遍历完filestring，返回';'作为结束符
    return ';';
}
// gettokForStr - Return the next token from string.
int gettokForStr()
{
    static int LastChar = ' ';
    static bool minusFlag = false;

    // Skip any whitespace.
    while(isspace(LastChar))
        LastChar = getCharFromStr();

    if(LastChar == ';')    //当LastChar == ';'时，进入新一轮解析，此时s会被更换，LastChar也要更换
        LastChar = getCharFromStr();

    if((minusFlag == false) && isalpha(LastChar))
    {  // identifier: [a-zA-Z][a-zA-Z0-9]*
        IdentifierStr1 = LastChar;
        while(isalnum((LastChar = getCharFromStr())))
            IdentifierStr1 += LastChar;

        if(IdentifierStr1 == "def")
            return tok_def_forstr;
        if(IdentifierStr1 == "extern")
            return tok_extern_forstr;
        return tok_identifier_forstr;
    }

    if(LastChar == '-')
    {  // judge if there is a minus sign
        LastChar = getCharFromStr();
        if(isspace(LastChar))
        {  // if the character after '-' is a space, just return '-'
            LastChar = getCharFromStr();
            return '-';
        }
        minusFlag = true;
        NumVal1 = -1;
        return tok_number_forstr;
    }

    if(minusFlag)
    {
        minusFlag = false;
        return '`';
    }

    if(isdigit(LastChar) || LastChar == '.')
    {  // Number: [0-9.]+
        string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = getCharFromStr();
        } while(isdigit(LastChar) || LastChar == '.');
        if(LastChar == 'e' || LastChar == 'E')
        {
            NumStr += LastChar;
            LastChar = getCharFromStr();
            if(LastChar == '-')
            {
                NumStr += LastChar;
                LastChar = getCharFromStr();
            }
            if(isalpha(LastChar))
            {
                fprintf(stderr, "ERROR: gettokForStr: the %c is not '-' or number, which should not appear after 1.0e\n", LastChar);
                exit(EXIT_FAILURE);
            }
            do
            {
                NumStr += LastChar;
                LastChar = getCharFromStr();
            } while(isdigit(LastChar) || LastChar == '.');
        }
        NumVal1 = strtod(NumStr.c_str(), nullptr);
        return tok_number_forstr;
    }

    if(LastChar == '#')
    {
        // Comment until end of line.
        do
            LastChar = getCharFromStr();
        while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF)
            return gettokForStr();
    }

    // Check for end of file.  Don't eat the EOF.
    if(LastChar == EOF)
        return tok_eof_forstr;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getCharFromStr();
    return ThisChar;
}

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
int getNextTokenForStr() { return CurTokForStr = gettokForStr(); }

// Install standard binary operators.
void installOperatorsForStr()
{
    // 1 is lowest precedence.
    BinopPrecedenceForStr['<'] = 10;
    BinopPrecedenceForStr['+'] = 20;
    BinopPrecedenceForStr['-'] = 20;
    BinopPrecedenceForStr['*'] = 40;
    BinopPrecedenceForStr['/'] = 40;
    BinopPrecedenceForStr['`'] = 50;  // highest.
}

/// GetTokPrecedenceForStr - Get the precedence of the pending binary operator token.
int GetTokPrecedenceForStr()
{
    if(!isascii(CurTokForStr))
        return -1;

    // Make sure it's a declared binop.
    int TokPrec = BinopPrecedenceForStr[CurTokForStr];
    if(TokPrec <= 0)
        return -1;
    return TokPrec;
}

/// LogErrorForStr* - These are little helper functions for error handling.
ast_ptr LogErrorForStr(const char *Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorPForStr(const char *Str)
{
    LogErrorForStr(Str);
    return nullptr;
}