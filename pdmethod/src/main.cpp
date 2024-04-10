//
// Created by 99065 on 2024/3/25.
//
#include "function.h"
#include <iostream>
int main(int argc, char *argv[]) {
    if (argc == 3) {
        double x1_l = atof(argv[1]);
        double x1_r = atof(argv[2]);
        detect1(x1_l, x1_r);
    }
    if (argc == 5) {
        double x1_l = atof(argv[1]);
        double x1_r = atof(argv[2]);
        double x2_l = atof(argv[3]);
        double x2_r = atof(argv[4]);
        detect2(x1_l, x1_r, x2_l, x2_r);
    }
    if (argc == 7) {
        double x1_l = atof(argv[1]);
        double x1_r = atof(argv[2]);
        double x2_l = atof(argv[3]);
        double x2_r = atof(argv[4]);
        double x3_l = atof(argv[5]);
        double x3_r = atof(argv[6]);
        detect3(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r);
    }
    return true;
}