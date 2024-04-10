//
// Created by 99065 on 2024/3/25.
//

#ifndef FPEPD_COMMON_H
#define FPEPD_COMMON_H
#include <iostream>
#include <gmp.h>
#include <mpfr.h>
#include <iomanip>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdlib>
#include <random>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional>
#include <random>
#include <mutex>
#include <future>
#include <limits>
#include <atomic>
#include <tuple>


using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::thread;
using std::future;
using std::async;
using std::atomic;
using std::ref;
using std::setprecision;
using std::mutex;
using std::lock_guard;
using std::pair;
using std::tuple;
using std::make_pair;
using std::get;

int jiou(int x);
double computeULP(double y);
float computeULPf(float y);
double computeULPDiff(mpfr_t origin, mpfr_t oracle);
float computeULPFDiff(mpfr_t origin, mpfr_t oracle);
#endif //FPEPD_COMMON_H
