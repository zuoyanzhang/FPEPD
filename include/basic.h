//
// Created by 99065 on 2024/3/25.
//

#ifndef FPEPD_BASIC_H
#define FPEPD_BASIC_H

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>
#include <fstream>

using std::string;
using std::vector;
using std::ofstream;
#define makePtr std::make_unique
#ifndef DOUBLE_PRECISION
#define DOUBLE_PRECISION 17
#endif

extern int callLevel;
extern const char callLevelChar;
extern const char callCountChar;
extern const size_t promtTimes;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

// namespace
// {

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
    virtual ~ExprAST() = default;

    virtual void printExpr() { fprintf(stderr, "the base class for expression\n"); }

    virtual string type() { return "Base"; }

    virtual std::unique_ptr<ExprAST> Clone() { return makePtr<ExprAST>(); }
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}

    void printExpr() { fprintf(stderr, "num = %f\n", Val); }

    string type() { return "Number"; }

    double getNumber() { return Val; }

    std::unique_ptr<ExprAST> Clone() { return makePtr<NumberExprAST>(Val); }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
    string Name;

public:
    VariableExprAST(const string &Name) : Name(Name) {}

    void printExpr() { fprintf(stderr, "variable = %s\n", Name.c_str()); }

    string type() { return "Variable"; }

    string getVariable() { return Name; }

    std::unique_ptr<ExprAST> Clone() { return makePtr<VariableExprAST>(Name); }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS) : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    void printExpr()
    {
        fprintf(stderr, "op = %c\n", Op);
        LHS->printExpr();
        RHS->printExpr();
    }

    string type() { return "Binary"; }

    char getOp() { return Op; }
    void setOp(char opNew) { Op = opNew; }
    std::unique_ptr<ExprAST> &getLHS() { return LHS; }
    std::unique_ptr<ExprAST> &getRHS() { return RHS; }

    void setLHS(std::unique_ptr<ExprAST> &newLHS) { LHS = newLHS->Clone(); }
    void setRHS(std::unique_ptr<ExprAST> &newRHS) { RHS = newRHS->Clone(); }

    std::unique_ptr<ExprAST> Clone()
    {
        auto LHSNew = LHS->Clone();
        auto RHSNew = RHS->Clone();
        return makePtr<BinaryExprAST>(Op, std::move(LHSNew), std::move(RHSNew));
    }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
    string Callee;
    vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const string &Callee, vector<std::unique_ptr<ExprAST>> Args) : Callee(Callee), Args(std::move(Args)) {}

    CallExprAST(std::unique_ptr<CallExprAST> &func) : Callee(func->Callee), Args(std::move(func->Args)) {}

    void printExpr()
    {
        fprintf(stderr, "call function name = %s\n", Callee.c_str());
        fprintf(stderr, "call function args =\n");
        for (long unsigned int i = 0; i < Args.size(); ++i)
        {
            (Args[i])->printExpr();
        }
    }

    string type() { return "Call"; }

    string getCallee() { return Callee; }
    vector<std::unique_ptr<ExprAST>> &getArgs() { return Args; }

    std::unique_ptr<ExprAST> Clone()
    {
        vector<std::unique_ptr<ExprAST>> ArgsNew;
        for (long unsigned int i = 0; i < Args.size(); ++i)
        {
            auto arg = (Args.at(i))->Clone();
            ArgsNew.push_back(std::move(arg));
        }

        return makePtr<CallExprAST>(Callee, std::move(ArgsNew));
    }
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST
{
    string Name;
    vector<string> Args;

public:
    PrototypeAST(const string &Name, vector<string> Args) : Name(Name), Args(std::move(Args)) {}

    const string &getName() const { return Name; }
    const vector<string> &getArgs() const { return Args; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body) : Proto(std::move(Proto)), Body(std::move(Body)) {}

    const string &getFuncName() const { return Proto->getName(); }
    const vector<string> &getFuncArgs() const { return Proto->getArgs(); }
    std::unique_ptr<ExprAST> &getFuncBody() { return Body; }
};

using ast_ptr = std::unique_ptr<ExprAST>;  // typedef std::unique_ptr<ExprAST> ast_ptr;

//===----------------------------------------------------------------------===//
// basic operation
//===----------------------------------------------------------------------===//

bool isEqual(const std::unique_ptr<ExprAST> &expr1, const std::unique_ptr<ExprAST> &expr2);

bool isFraction(const std::unique_ptr<ExprAST> &expr);

bool isConstant(const vector<std::unique_ptr<ExprAST>> &exprs);

std::unique_ptr<ExprAST> getNumerator(const std::unique_ptr<ExprAST> &expr);

std::unique_ptr<ExprAST> getDenominator(const std::unique_ptr<ExprAST> &expr);

std::unique_ptr<ExprAST> createBinaryExpr(const std::unique_ptr<ExprAST> &expr1, const std::unique_ptr<ExprAST> &expr2, const char op);

std::unique_ptr<ExprAST> addExpr(const std::unique_ptr<ExprAST> &expr1, const std::unique_ptr<ExprAST> &expr2);

std::unique_ptr<ExprAST> mulExpr(const std::unique_ptr<ExprAST> &expr1, const std::unique_ptr<ExprAST> &expr2);

std::unique_ptr<ExprAST> divExpr(const std::unique_ptr<ExprAST> &expr1, const std::unique_ptr<ExprAST> &expr2);

void mineAppend(vector<ast_ptr> &dest, vector<ast_ptr> &origin);

//===----------------------------------------------------------------------===//
// print information
//===----------------------------------------------------------------------===//

string PrintExpression(const std::unique_ptr<ExprAST> &expr, const size_t PRECISION = DOUBLE_PRECISION);

void PrintFunction(std::unique_ptr<FunctionAST> &fun);

void printExpr(const ast_ptr &expr, string prefix = "", const size_t PRECISION = DOUBLE_PRECISION, int index = -1);

void printExprs(const vector<ast_ptr> &exprs, string prefix = "", bool showTree = false, const size_t PRECISION = DOUBLE_PRECISION);

void printAST(const ast_ptr &expr, const size_t PRECISION = DOUBLE_PRECISION);

void printAST(const ast_ptr &expr, string &result, const size_t PRECISION = DOUBLE_PRECISION);

string getMpfrParameterNumber(const ast_ptr &expr, size_t &mpfr_variabless);

string mpfrCodeGenerator(const ast_ptr &expr, size_t &mpfr_variables, const std::map<string, string> &map, ofstream &ofs, string &variable_tmp);

// } // end anonymous namespace

#endif //FPEPD_BASIC_H
