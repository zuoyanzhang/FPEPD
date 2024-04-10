//
// Created by 99065 on 2024/3/25.
//

#ifndef FPEPD_FUNCTION_H
#define FPEPD_FUNCTION_H

#include "common.h"
// onepara
struct IntervalResult {
    double avgUlp;
    double maxUlp;
    double maxUlpPoint;
    pair<double, double> interval;
};
// twopara
struct SubIntervalResult {
    double avgUlp;
    double maxUlp;
    pair<double, double> maxUlpPoint;
    pair<pair<double, double>, pair<double, double>> interval;
};
// threepara
struct SubIntervalResultT {
    double avgUlp;
    double maxUlp;
    tuple<double, double, double> maxUlpPoint;
    tuple<pair<double, double>, pair<double, double>, pair<double, double>> interval;
};
// onepara
void processInterval(double a, double b, int samples, vector<IntervalResult> &results);
void detectAndReOne(double x_l, double x_r, int samples, double threshold, bool isFinalIteration);
void detect1(double x1_l, double x1_r);

// twopara
void detectAndRefine(double x1_l, double x1_r, double x2_l, double x2_r, int samples, double threshold, bool isFinalIteration);
void processSubInterval(double a, double b, double c, double d, int samples, vector<SubIntervalResult> &results);
//void detectSubInterval(double a, double b, double c, double d, int samples);
void detect2(double x1_l, double x1_r, double x2_l, double x2_r);

// threepara
void processIntervalT(double a, double b, double c, double d, double e, double f, int samples, vector<SubIntervalResultT> &results);
void detectAndRefineT(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r, int samples, double threshold, bool isFinalIteration);
void detect3(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r);

#endif //FPEPD_FUNCTION_H
