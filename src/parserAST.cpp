//
// Created by 99065 on 2024/3/25.
//
#include "basic.h"
#include "laxerAST.h"
#include "parserAST.h"
#include "string.h"
#include <unistd.h>

using std::string;
using std::vector;

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

ast_ptr ParseExpressionForStr();

/// numberexpr ::= number
ast_ptr ParseNumberExprForStr()
{
    auto Result = makePtr<NumberExprAST>(NumVal1);
    getNextTokenForStr();  // consume the number
    return Result;
}

/// parenexpr ::= '(' expression ')'
ast_ptr ParseParenExprForStr()
{
    getNextTokenForStr();  // eat (.
    auto V = ParseExpressionForStr();
    if(!V)
        return nullptr;

    if(CurTokForStr != ')')
        return LogErrorForStr("expected ')'");
    getNextTokenForStr();  // eat ).
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
ast_ptr ParseIdentifierExprForStr()
{
    string IdName = IdentifierStr1;

    getNextTokenForStr();  // eat identifier.

    if(CurTokForStr != '(')  // Simple variable ref.
        return makePtr<VariableExprAST>(IdName);

    // Call.
    getNextTokenForStr();  // eat (
    vector<ast_ptr> Args;
    if(CurTokForStr != ')')
    {
        while(true)
        {
            if(auto Arg = ParseExpressionForStr())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if(CurTokForStr == ')')
                break;

            if(CurTokForStr != ',')
                return LogErrorForStr("ParseIdentifierExprForStr: Expected ')' or ',' in argument list");
            getNextTokenForStr();
        }
    }

    // Eat the ')'.
    getNextTokenForStr();

    return makePtr<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
ast_ptr ParsePrimaryForStr()
{
    switch(CurTokForStr)
    {
        default:
            return LogErrorForStr("ParsePrimaryForStr: unknown token when expecting an expression");
        case tok_identifier_forstr:
            return ParseIdentifierExprForStr();
        case tok_number_forstr:
            return ParseNumberExprForStr();
        case '(':
            return ParseParenExprForStr();
    }
}

/// binoprhs
///   ::= ('+' primary)*
ast_ptr ParseBinOpRHSForStr(int ExprPrec, ast_ptr LHS)
{
    // If this is a binop, find its precedence.
    while(true)
    {
        int TokPrec = GetTokPrecedenceForStr();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if(TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        int BinOp = CurTokForStr;
        getNextTokenForStr();  // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimaryForStr();
        if(!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedenceForStr();
        if(TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHSForStr(TokPrec + 1, std::move(RHS));
            if(!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        if(BinOp == '`'){
            BinOp = '*';
        }
        LHS = makePtr<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
///   ::= primary binoprhs
///
ast_ptr ParseExpressionForStr()
{
    auto LHS = ParsePrimaryForStr();
    if(!LHS)
        return nullptr;

    return ParseBinOpRHSForStr(0, std::move(LHS));
}

//从文件读入到string里
string readFileIntoString(const char * filename)
{
    std::ifstream ifile(filename);
    //将文件读入到ostringstream对象buf中
    std::ostringstream buf;
    char ch;
    while(buf&&ifile.get(ch))
        buf.put(ch);
    //返回与流对象buf关联的字符串
    return buf.str();
}
ast_ptr ParseExpressionFromString()
{
    //清空filestring，flag归零
    if(CurTokForStr == ';')
    {
        filestring.clear();
        filestring.shrink_to_fit();
        flag = 0;
    }

    string filename;
    char buf[128] = {0};
    getcwd(buf, sizeof(buf));
    string bufStr = buf;
    filename = bufStr + "/src/pythonAfter.txt";
    const char* fn = filename.c_str();
    // std::cout << "------- " << fn << std::endl;
    filestring = readFileIntoString(fn);

    getNextTokenForStr();    //eat first element

    if (auto E = ParseExpressionForStr()){
        ast_ptr es = E->Clone();
        return es;
    }
    return nullptr;
}
ast_ptr ParseExpressionFromString(string str)
{
    //清空filestring，flag归零
    if(CurTokForStr == ';')
    {
        filestring.clear();
        filestring.shrink_to_fit();
        flag = 0;
    }

    filestring = str;

    getNextTokenForStr();    //eat first element

    if (auto E = ParseExpressionForStr()){
        ast_ptr es = E->Clone();
        return es;
    }
    return nullptr;
}