//
// Created by 99065 on 2024/3/25.
//

#ifndef FPEPD_PARSERAST_H
#define FPEPD_PARSERAST_H
#include "basic.h"

#include <fstream>
#include <sstream>
#include <iostream>

using std::string;

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

ast_ptr ParseExpressionForStr();

/// numberexpr ::= number
ast_ptr ParseNumberExprForStr();

/// parenexpr ::= '(' expression ')'
ast_ptr ParseParenExprForStr();

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
ast_ptr ParseIdentifierExprForStr();

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
ast_ptr ParsePrimaryForStr();

/// binoprhs
///   ::= ('+' primary)*
ast_ptr ParseBinOpRHSForStr(int ExprPrec, ast_ptr LHS);

/// expression
///   ::= primary binoprhs
///
ast_ptr ParseExpressionForStr();

string readFileIntoString(const char * filename);

ast_ptr ParseExpressionFromString();
ast_ptr ParseExpressionFromString(string str);
#endif //FPEPD_PARSERAST_H
