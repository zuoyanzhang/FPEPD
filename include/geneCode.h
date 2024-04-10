//
// Created by 99065 on 2024/3/25.
//

#ifndef FPEPD_GENECODE_H
#define FPEPD_GENECODE_H
#include "basic.h"

#include<stdio.h>
#include <fstream>
#include<iostream>
#include<vector>
using namespace std;
using std::cout;
using std::cin;
using std::vector;
using std::endl;
using std::string;

bool getVariablesFromExpr(const ast_ptr &expr, vector<string> &vars);

int geneMpfrCode(const string exprStr, vector<string> vars);
#endif //FPEPD_GENECODE_H
